#include "edge.h"
#include "utils/stb_sb.h"
#include "utils/log.h"
#include "sock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define DC_EDGE_END_OF_MSG 0x04
#define DC_EDGE_DEFAULT_PORT 5451

typedef struct __DcEdgeContext {
    char  ip[46]; // INET6_ADDRSTRLEN
    int   port;
    float data_rate;
    int   timeout_s;

    // state
    bool is_connected;
    bool has_new_data;

    // socket
    DcSockHandle sock;
    DcSockState  state;

    // time tracking
    time_t reconnect_start;
    time_t last_update;

    // command group ID (returned by EDGE RCS)
    char *cmd_group_id;

    // stretchy buffers for rx commands (for command group)
    char *rx_cmds;
    int  *rx_cmd_offsets;

    // stretchy buffers for tx commands
    char *tx_cmds;
    int  *tx_cmd_offsets;

    // stretchy buffers for received values
    char *rx_var_values;
    int  *rx_var_offsets;

    // tx buffer for pending writes
    char *tx_buffer;

    // general use buffer
    char *temp_buffer;
} _DcEdgeContext;

static _DcEdgeContext *_contexts = NULL; // stretchy buffer

#define DC_EDGE_TEMP_BUFFER_SIZE 16384

// internal helpers
static DcEdgeResult _dc_edge_connect(DcEdgeHandle edge);
static void         _dc_edge_close(DcEdgeHandle edge);
static DcEdgeResult _dc_edge_send_command(DcEdgeHandle edge, const char *cmd, char **response);
static DcEdgeResult _dc_edge_read_message(DcEdgeHandle edge, char **response);
static DcEdgeResult _dc_edge_setup_command_group(DcEdgeHandle edge);

void dc_edge_init(void) {
    sbresize(_contexts, 1);
}

DcEdgeHandle dc_edge_create(const char *host, int port, float data_rate, int timeout_s) {

    _DcEdgeContext context;
    dc_sock_host_to_ip(host, context.ip);
    context.port            = port > 0 ? port : DC_EDGE_DEFAULT_PORT;
    context.data_rate       = data_rate;
    context.timeout_s       = timeout_s;
    context.is_connected    = false;
    context.has_new_data    = false;
    context.sock            = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));
    context.state           = DC_SOCK_STATE_DISCONNECTED;
    context.reconnect_start = 0;
    context.last_update     = 0;
    context.cmd_group_id    = NULL;
    context.rx_cmds         = NULL;
    context.rx_cmd_offsets  = NULL;
    context.tx_cmds         = NULL;
    context.tx_cmd_offsets  = NULL;
    context.rx_var_values   = NULL;
    context.rx_var_offsets  = NULL;
    context.tx_buffer       = NULL;
    context.temp_buffer     = (char *)malloc(DC_EDGE_TEMP_BUFFER_SIZE);
    sbpush(_contexts, context);

    DcEdgeHandle edge;
    edge.index = (uint8_t)(sbcount(_contexts) - 1);
    return edge;
}

void dc_edge_cleanup(DcEdgeHandle edge) {

    _DcEdgeContext *context = &(_contexts[edge.index]);

    _dc_edge_close(edge);
    sbfree(context->rx_cmds);
    sbfree(context->rx_cmd_offsets);
    sbfree(context->tx_cmds);
    sbfree(context->tx_cmd_offsets);
    sbfree(context->rx_var_values);
    sbfree(context->rx_var_offsets);
    sbfree(context->tx_buffer);
    free(context->cmd_group_id);
    free(context->temp_buffer);
}

void dc_edge_update(DcEdgeHandle edge) {

    _DcEdgeContext *context = &(_contexts[edge.index]);

    context->has_new_data = false;

    // check if we need to reconnect
    if (!context->is_connected) {
        if (difftime(time(NULL), context->reconnect_start) > context->timeout_s) {
            DC_LOG_INFO("EDGE", "[%s:%d] Attempting reconnect..", context->ip, context->port);
            DcEdgeResult result = _dc_edge_connect(edge);
            if (result == DC_EDGE_RESULT_SUCCESS) {
                DC_LOG_INFO("EDGE", "[%s:%d] Connected", context->ip, context->port);
                context->is_connected = true;

                // setup command group for rx variables
                if (sbcount(context->rx_cmd_offsets) > 0) {
                    result = _dc_edge_setup_command_group(edge);
                    if (result != DC_EDGE_RESULT_SUCCESS) {
                        DC_LOG_ERROR("EDGE", "[%s:%d] Failed to setup command group", context->ip, context->port);
                        _dc_edge_close(edge);
                    }
                }
            } else {
                context->reconnect_start = time(NULL);
            }
        }
        return;
    }

    // check if enough time has passed for an update
    if (difftime(time(NULL), context->last_update) < context->data_rate) {
        return;
    }
    context->last_update = time(NULL);

    // send any pending tx commands
    for (int ii = 0; ii < sbcount(context->tx_buffer);) {
        // find end of command (null terminated)
        char *cmd     = &context->tx_buffer[ii];
        int   cmd_len = (int)strlen(cmd);

        char        *response = NULL;
        DcEdgeResult result   = _dc_edge_send_command(edge, cmd, &response);
        free(response);

        if (result != DC_EDGE_RESULT_SUCCESS) {
            DC_LOG_WARN("EDGE", "[%s:%d] TX command failed, disconnecting", context->ip, context->port);
            _dc_edge_close(edge);
            return;
        }

        ii += cmd_len + 1;
    }
    sbclear(context->tx_buffer);

    // execute command group to read rx variables
    if (context->cmd_group_id && sbcount(context->rx_cmd_offsets) > 0) {
        snprintf(context->temp_buffer, DC_EDGE_TEMP_BUFFER_SIZE, "execute_command_group %s", context->cmd_group_id);

        char        *response = NULL;
        DcEdgeResult result   = _dc_edge_send_command(edge, context->temp_buffer, &response);

        if (result != DC_EDGE_RESULT_SUCCESS || !response) {
            DC_LOG_WARN("EDGE", "[%s:%d] execute_command_group failed, disconnecting", context->ip, context->port);
            free(response);
            _dc_edge_close(edge);
            return;
        }

        // parse response - space-separated values
        sbclear(context->rx_var_values);
        sbclear(context->rx_var_offsets);

        char *ptr = response;
        while (*ptr) {
            // skip leading whitespace
            while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')
                ptr++;
            if (!*ptr) break;

            // record start offset
            int start = sbcount(context->rx_var_values);
            sbpush(context->rx_var_offsets, start);

            // copy until whitespace or end
            while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '\r') {
                sbpush(context->rx_var_values, *ptr);
                ptr++;
            }
            sbpush(context->rx_var_values, '\0');
        }

        free(response);

        // verify we got the expected number of values
        if (sbcount(context->rx_var_offsets) == sbcount(context->rx_cmd_offsets)) {
            context->has_new_data = true;
        } else {
            DC_LOG_WARN("EDGE", "[%s:%d] Value count mismatch (got %d, expected %d)",
                        context->ip, context->port, sbcount(context->rx_var_offsets), sbcount(context->rx_cmd_offsets));
        }
    }
}

bool dc_edge_is_connected(DcEdgeHandle edge) {
    _DcEdgeContext *context = &(_contexts[edge.index]);
    return context->is_connected;
}

bool dc_edge_has_new_data(DcEdgeHandle edge) {
    _DcEdgeContext *context = &(_contexts[edge.index]);
    return context->has_new_data;
}

DcEdgeVarIndex dc_edge_add_tx_var(DcEdgeHandle edge, const char *command) {
    _DcEdgeContext *context = &(_contexts[edge.index]);

    // store command template (will append value when setting)
    int start = sbcount(context->tx_cmds);
    sbpush(context->tx_cmd_offsets, start);
    sbpushn(context->tx_cmds, command, (int)strlen(command));
    sbpush(context->tx_cmds, '\0');

    return sbcount(context->tx_cmd_offsets) - 1;
}

DcEdgeVarIndex dc_edge_add_rx_var(DcEdgeHandle edge, const char *command) {
    _DcEdgeContext *context = &(_contexts[edge.index]);

    // store command for command group
    int start = sbcount(context->rx_cmds);
    sbpush(context->rx_cmd_offsets, start);
    sbpushn(context->rx_cmds, command, (int)strlen(command));
    sbpush(context->rx_cmds, '\0');

    return sbcount(context->rx_cmd_offsets) - 1;
}

void dc_edge_set_tx_var(DcEdgeHandle edge, DcEdgeVarIndex var, const char *value) {
    _DcEdgeContext *context = &(_contexts[edge.index]);

    // build command: "<command> <value>"
    char *cmd_template = &(context->tx_cmds[context->tx_cmd_offsets[var]]);
    snprintf(context->temp_buffer, DC_EDGE_TEMP_BUFFER_SIZE, "%s %s", cmd_template, value);

    // append to tx buffer (null-terminated)
    int len = (int)strlen(context->temp_buffer);
    sbpushn(context->tx_buffer, context->temp_buffer, len + 1);
}

void dc_edge_get_rx_var_value(DcEdgeHandle edge, DcEdgeVarIndex var_index, char *out) {
    _DcEdgeContext *context = &(_contexts[edge.index]);

    if (var_index < (DcEdgeVarIndex)sbcount(context->rx_var_offsets)) {
        strcpy(out, &(context->rx_var_values[context->rx_var_offsets[var_index]]));
    } else {
        out[0] = '\0';
    }
}

// ============================================================================
// Internal helpers
// ============================================================================

static DcEdgeResult _dc_edge_connect(DcEdgeHandle edge) {
    _DcEdgeContext *context = &(_contexts[edge.index]);

    // close any existing connection
    _dc_edge_close(edge);

    // create new socket
    context->sock = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));

    // attempt connection
    dc_sock_connect(context->sock, context->ip, context->port);

    // wait for connection (with timeout)
    time_t start = time(NULL);
    while (difftime(time(NULL), start) < 2.0) {
        DcSockState state = dc_sock_connection_status(context->sock);
        if (state == DC_SOCK_STATE_CONNECTED) {
            // switch to blocking for data I/O
            dc_sock_set_blocking(context->sock);
            dc_sock_set_recv_timeout(context->sock, 2000);

            // read server version string
            char        *version = NULL;
            DcEdgeResult result  = _dc_edge_read_message(edge, &version);
            if (result == DC_EDGE_RESULT_SUCCESS && version) {
                DC_LOG_INFO("EDGE", "[%s:%d] Server version: %s", context->ip, context->port, version);
                free(version);
                context->state = DC_SOCK_STATE_CONNECTED;
                return DC_EDGE_RESULT_SUCCESS;
            }
            free(version);
            break;
        } else if (state == DC_SOCK_STATE_DISCONNECTED) {
            break;
        }
// small sleep to avoid busy loop
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
    }

    _dc_edge_close(edge);
    return DC_EDGE_RESULT_FAIL;
}

static void _dc_edge_close(DcEdgeHandle edge) {
    _DcEdgeContext *context = &(_contexts[edge.index]);

    dc_sock_close(context->sock);
    context->state        = DC_SOCK_STATE_DISCONNECTED;
    context->is_connected = false;
    context->has_new_data = false;

    // clear command group ID (will need to recreate on reconnect)
    free(context->cmd_group_id);
    context->cmd_group_id = NULL;
}

static DcEdgeResult _dc_edge_send_command(DcEdgeHandle edge, const char *cmd, char **response) {
    _DcEdgeContext *context = &(_contexts[edge.index]);

    // EDGE RCS protocol requires a NEW connection for each command
    DcSockHandle cmd_sock = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));
    dc_sock_connect(cmd_sock, context->ip, context->port);

    // wait for connection (with timeout)
    time_t start     = time(NULL);
    bool   connected = false;
    while (difftime(time(NULL), start) < 2.0) {
        DcSockState state = dc_sock_connection_status(cmd_sock);
        if (state == DC_SOCK_STATE_CONNECTED) {
            connected = true;
            break;
        } else if (state == DC_SOCK_STATE_DISCONNECTED) {
            break;
        }
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
    }

    if (!connected) {
        dc_sock_close(cmd_sock);
        return DC_EDGE_RESULT_FAIL;
    }

    // switch to blocking for data I/O (short timeout to avoid stalling render)
    dc_sock_set_blocking(cmd_sock);
    dc_sock_set_recv_timeout(cmd_sock, 500);

    // read and discard server version
    int   buf_size = 256;
    int   nread    = 0;
    char *buf      = (char *)malloc(buf_size);
    if (!buf) {
        dc_sock_close(cmd_sock);
        return DC_EDGE_RESULT_FAIL;
    }

    for (;;) {
        char         c;
        int          recv_count;
        DcSockResult res = dc_sock_receive(cmd_sock, &c, 1, &recv_count);

        if (res == DC_SOCK_RESULT_CONN_INTERRUPTED) continue;
        if (res != DC_SOCK_RESULT_SUCCESS || recv_count != 1) {
            free(buf);
            dc_sock_close(cmd_sock);
            return DC_EDGE_RESULT_FAIL;
        }

        if (c == DC_EDGE_END_OF_MSG) break;

        if (nread + 2 > buf_size) {
            buf_size *= 2;
            char *new_buf = (char *)realloc(buf, buf_size);
            if (!new_buf) {
                free(buf);
                dc_sock_close(cmd_sock);
                return DC_EDGE_RESULT_FAIL;
            }
            buf = new_buf;
        }
        buf[nread++] = c;
    }
    free(buf);

    // send command
    int          cmd_len = (int)strlen(cmd);
    int          sent_count;
    DcSockResult result = dc_sock_send(cmd_sock, cmd, cmd_len, &sent_count);
    if (result != DC_SOCK_RESULT_SUCCESS || sent_count != cmd_len) {
        dc_sock_close(cmd_sock);
        return DC_EDGE_RESULT_FAIL;
    }

    // shutdown write side to signal end of command
    dc_sock_shutdown_write(cmd_sock);

    // read response
    buf_size = 256;
    nread    = 0;
    buf      = (char *)malloc(buf_size);
    if (!buf) {
        dc_sock_close(cmd_sock);
        return DC_EDGE_RESULT_FAIL;
    }

    for (;;) {
        char         c;
        int          recv_count;
        DcSockResult res = dc_sock_receive(cmd_sock, &c, 1, &recv_count);

        if (res == DC_SOCK_RESULT_CONN_INTERRUPTED) continue;
        if (res != DC_SOCK_RESULT_SUCCESS || recv_count != 1) {
            free(buf);
            dc_sock_close(cmd_sock);
            return DC_EDGE_RESULT_FAIL;
        }

        if (c == DC_EDGE_END_OF_MSG) {
            buf[nread] = '\0';
            if (response) {
                *response = buf;
            } else {
                free(buf);
            }
            dc_sock_close(cmd_sock);
            return DC_EDGE_RESULT_SUCCESS;
        }

        if (nread + 2 > buf_size) {
            buf_size *= 2;
            char *new_buf = (char *)realloc(buf, buf_size);
            if (!new_buf) {
                free(buf);
                dc_sock_close(cmd_sock);
                return DC_EDGE_RESULT_FAIL;
            }
            buf = new_buf;
        }
        buf[nread++] = c;
    }
}

static DcEdgeResult _dc_edge_read_message(DcEdgeHandle edge, char **response) {
    _DcEdgeContext *context = &(_contexts[edge.index]);

    int   buf_size = 256;
    int   nread    = 0;
    char *buf      = (char *)malloc(buf_size);
    if (!buf) return DC_EDGE_RESULT_FAIL;

    // blocking read until END_OF_MSG character (0x04)
    // (socket is already set to blocking with recv timeout)
    for (;;) {
        char         c;
        int          recv_count;
        DcSockResult result = dc_sock_receive(context->sock, &c, 1, &recv_count);

        if (result == DC_SOCK_RESULT_CONN_INTERRUPTED) continue;
        if (result != DC_SOCK_RESULT_SUCCESS || recv_count != 1) {
            free(buf);
            return DC_EDGE_RESULT_FAIL;
        }

        if (c == DC_EDGE_END_OF_MSG) {
            buf[nread] = '\0';
            if (response) {
                *response = buf;
            } else {
                free(buf);
            }
            return DC_EDGE_RESULT_SUCCESS;
        }

        if (nread + 2 > buf_size) {
            buf_size *= 2;
            char *new_buf = (char *)realloc(buf, buf_size);
            if (!new_buf) {
                free(buf);
                return DC_EDGE_RESULT_FAIL;
            }
            buf = new_buf;
        }

        buf[nread++] = c;
    }
}

static DcEdgeResult _dc_edge_setup_command_group(DcEdgeHandle edge) {
    _DcEdgeContext *context = &(_contexts[edge.index]);

    // create command group
    char        *group_id = NULL;
    DcEdgeResult result   = _dc_edge_send_command(edge, "create_command_group", &group_id);
    if (result != DC_EDGE_RESULT_SUCCESS || !group_id || !group_id[0]) {
        free(group_id);
        return DC_EDGE_RESULT_FAIL;
    }

    // store group ID
    context->cmd_group_id = group_id;

    // add each rx command to the group
    // (_dc_edge_send_command creates a new connection for each command)
    for (int ii = 0; ii < sbcount(context->rx_cmd_offsets); ii++) {
        char *rx_cmd = &(context->rx_cmds[context->rx_cmd_offsets[ii]]);
        snprintf(context->temp_buffer, DC_EDGE_TEMP_BUFFER_SIZE,
                 "add_command_to_group %s \"%s\"", context->cmd_group_id, rx_cmd);

        char *response = NULL;
        result         = _dc_edge_send_command(edge, context->temp_buffer, &response);
        free(response);

        if (result != DC_EDGE_RESULT_SUCCESS) {
            return DC_EDGE_RESULT_FAIL;
        }
    }

    return DC_EDGE_RESULT_SUCCESS;
}
