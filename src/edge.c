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

struct DcEdge {
    char  ip[46]; // INET6_ADDRSTRLEN
    int   port;
    float data_rate;
    int   timeout_s;

    // state
    bool is_connected;
    bool has_new_data;

    // socket
    DcSock      *sock;
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
};

#define DC_EDGE_TEMP_BUFFER_SIZE 16384

// internal helpers
static DcEdgeResult _dc_edge_connect(DcEdge *edge);
static void         _dc_edge_close(DcEdge *edge);
static DcEdgeResult _dc_edge_send_command(DcEdge *edge, const char *cmd, char **response);
static DcEdgeResult _dc_edge_read_message(DcEdge *edge, char **response);
static DcEdgeResult _dc_edge_setup_command_group(DcEdge *edge);

void dc_edge_init(void) {
    // Retained for API compatibility; each connection now initializes itself.
}

DcEdge *dc_edge_create(const char *host, int port, float data_rate, int timeout_s) {

    DcEdge *edge = (DcEdge *)calloc(1, sizeof(DcEdge));
    if (!edge) {
        DC_LOG_ERROR("Edge", "Failed to allocate Edge");
        return NULL;
    }

    if (dc_sock_host_to_ip(host, edge->ip) != DC_SOCK_RESULT_SUCCESS) {
        free(edge);
        return NULL;
    }
    edge->port            = port > 0 ? port : DC_EDGE_DEFAULT_PORT;
    edge->data_rate       = data_rate;
    edge->timeout_s       = timeout_s;
    edge->is_connected    = false;
    edge->has_new_data    = false;
    edge->sock            = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));
    if (!edge->sock) {
        free(edge);
        return NULL;
    }
    edge->state           = DC_SOCK_STATE_DISCONNECTED;
    edge->reconnect_start = 0;
    edge->last_update     = 0;
    edge->cmd_group_id    = NULL;
    edge->rx_cmds         = NULL;
    edge->rx_cmd_offsets  = NULL;
    edge->tx_cmds         = NULL;
    edge->tx_cmd_offsets  = NULL;
    edge->rx_var_values   = NULL;
    edge->rx_var_offsets  = NULL;
    edge->tx_buffer       = NULL;
    edge->temp_buffer     = (char *)malloc(DC_EDGE_TEMP_BUFFER_SIZE);
    if (!edge->temp_buffer) {
        DC_LOG_ERROR("Edge", "Failed to allocate temp buffer");
        dc_sock_close(edge->sock);
        free(edge);
        return NULL;
    }

    return edge;
}

void dc_edge_cleanup(DcEdge *edge) {

    if (!edge) return;

    _dc_edge_close(edge);
    sbfree(edge->rx_cmds);
    sbfree(edge->rx_cmd_offsets);
    sbfree(edge->tx_cmds);
    sbfree(edge->tx_cmd_offsets);
    sbfree(edge->rx_var_values);
    sbfree(edge->rx_var_offsets);
    sbfree(edge->tx_buffer);
    free(edge->cmd_group_id);
    free(edge->temp_buffer);
    free(edge);
}

void dc_edge_update(DcEdge *edge) {

    if (!edge) return;

    edge->has_new_data = false;

    // check if we need to reconnect
    if (!edge->is_connected) {
        if (difftime(time(NULL), edge->reconnect_start) > edge->timeout_s) {
            DC_LOG_INFO("EDGE", "[%s:%d] Attempting reconnect..", edge->ip, edge->port);
            DcEdgeResult result = _dc_edge_connect(edge);
            if (result == DC_EDGE_RESULT_SUCCESS) {
                DC_LOG_INFO("EDGE", "[%s:%d] Connected", edge->ip, edge->port);
                edge->is_connected = true;

                // setup command group for rx variables
                if (sbcount(edge->rx_cmd_offsets) > 0) {
                    result = _dc_edge_setup_command_group(edge);
                    if (result != DC_EDGE_RESULT_SUCCESS) {
                        DC_LOG_ERROR("EDGE", "[%s:%d] Failed to setup command group", edge->ip, edge->port);
                        _dc_edge_close(edge);
                    }
                }
            } else {
                edge->reconnect_start = time(NULL);
            }
        }
        return;
    }

    // check if enough time has passed for an update
    if (difftime(time(NULL), edge->last_update) < edge->data_rate) {
        return;
    }
    edge->last_update = time(NULL);

    // send any pending tx commands
    for (int ii = 0; ii < sbcount(edge->tx_buffer);) {
        // find end of command (null terminated)
        char *cmd     = &edge->tx_buffer[ii];
        int   cmd_len = (int)strlen(cmd);

        char        *response = NULL;
        DcEdgeResult result   = _dc_edge_send_command(edge, cmd, &response);
        free(response);

        if (result != DC_EDGE_RESULT_SUCCESS) {
            DC_LOG_WARN("EDGE", "[%s:%d] TX command failed, disconnecting", edge->ip, edge->port);
            _dc_edge_close(edge);
            return;
        }

        ii += cmd_len + 1;
    }
    sbclear(edge->tx_buffer);

    // execute command group to read rx variables
    if (edge->cmd_group_id && sbcount(edge->rx_cmd_offsets) > 0) {
        snprintf(edge->temp_buffer, DC_EDGE_TEMP_BUFFER_SIZE, "execute_command_group %s", edge->cmd_group_id);

        char        *response = NULL;
        DcEdgeResult result   = _dc_edge_send_command(edge, edge->temp_buffer, &response);

        if (result != DC_EDGE_RESULT_SUCCESS || !response) {
            DC_LOG_WARN("EDGE", "[%s:%d] execute_command_group failed, disconnecting", edge->ip, edge->port);
            free(response);
            _dc_edge_close(edge);
            return;
        }

        // parse response - space-separated values
        sbclear(edge->rx_var_values);
        sbclear(edge->rx_var_offsets);

        char *ptr = response;
        while (*ptr) {
            // skip leading whitespace
            while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')
                ptr++;
            if (!*ptr) break;

            // record start offset
            int start = sbcount(edge->rx_var_values);
            sbpush(edge->rx_var_offsets, start);

            // copy until whitespace or end
            while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '\r') {
                sbpush(edge->rx_var_values, *ptr);
                ptr++;
            }
            sbpush(edge->rx_var_values, '\0');
        }

        free(response);

        // verify we got the expected number of values
        if (sbcount(edge->rx_var_offsets) == sbcount(edge->rx_cmd_offsets)) {
            edge->has_new_data = true;
        } else {
            DC_LOG_WARN("EDGE", "[%s:%d] Value count mismatch (got %d, expected %d)",
                        edge->ip, edge->port, sbcount(edge->rx_var_offsets), sbcount(edge->rx_cmd_offsets));
        }
    }
}

bool dc_edge_is_connected(DcEdge *edge) {
    return edge && edge->is_connected;
}

bool dc_edge_has_new_data(DcEdge *edge) {
    return edge && edge->has_new_data;
}

DcEdgeVarIndex dc_edge_add_tx_var(DcEdge *edge, const char *command) {

    // store command template (will append value when setting)
    int start = sbcount(edge->tx_cmds);
    sbpush(edge->tx_cmd_offsets, start);
    sbpushn(edge->tx_cmds, command, (int)strlen(command));
    sbpush(edge->tx_cmds, '\0');

    return sbcount(edge->tx_cmd_offsets) - 1;
}

DcEdgeVarIndex dc_edge_add_rx_var(DcEdge *edge, const char *command) {

    // store command for command group
    int start = sbcount(edge->rx_cmds);
    sbpush(edge->rx_cmd_offsets, start);
    sbpushn(edge->rx_cmds, command, (int)strlen(command));
    sbpush(edge->rx_cmds, '\0');

    return sbcount(edge->rx_cmd_offsets) - 1;
}

void dc_edge_set_tx_var(DcEdge *edge, DcEdgeVarIndex var, const char *value) {

    // build command: "<command> <value>"
    char *cmd_template = &(edge->tx_cmds[edge->tx_cmd_offsets[var]]);
    snprintf(edge->temp_buffer, DC_EDGE_TEMP_BUFFER_SIZE, "%s %s", cmd_template, value);

    // append to tx buffer (null-terminated)
    int len = (int)strlen(edge->temp_buffer);
    sbpushn(edge->tx_buffer, edge->temp_buffer, len + 1);
}

void dc_edge_get_rx_var_value(DcEdge *edge, DcEdgeVarIndex var_index, char *out) {

    if (var_index < (DcEdgeVarIndex)sbcount(edge->rx_var_offsets)) {
        strcpy(out, &(edge->rx_var_values[edge->rx_var_offsets[var_index]]));
    } else {
        out[0] = '\0';
    }
}

// ============================================================================
// Internal helpers
// ============================================================================

static DcEdgeResult _dc_edge_connect(DcEdge *edge) {

    // close any existing connection
    _dc_edge_close(edge);

    // create new socket
    edge->sock = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));
    if (!edge->sock)
        return DC_EDGE_RESULT_FAIL;

    // attempt connection
    if (dc_sock_connect(edge->sock, edge->ip, edge->port) != DC_SOCK_RESULT_SUCCESS) {
        _dc_edge_close(edge);
        return DC_EDGE_RESULT_FAIL;
    }

    // wait for connection (with timeout)
    time_t start = time(NULL);
    while (difftime(time(NULL), start) < 2.0) {
        DcSockState state = dc_sock_connection_status(edge->sock);
        if (state == DC_SOCK_STATE_CONNECTED) {
            // switch to blocking for data I/O
            dc_sock_set_blocking(edge->sock);
            dc_sock_set_recv_timeout(edge->sock, 2000);

            // read server version string
            char        *version = NULL;
            DcEdgeResult result  = _dc_edge_read_message(edge, &version);
            if (result == DC_EDGE_RESULT_SUCCESS && version) {
                DC_LOG_INFO("EDGE", "[%s:%d] Server version: %s", edge->ip, edge->port, version);
                free(version);
                edge->state = DC_SOCK_STATE_CONNECTED;
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

static void _dc_edge_close(DcEdge *edge) {

    dc_sock_close(edge->sock);
    edge->sock         = NULL;
    edge->state        = DC_SOCK_STATE_DISCONNECTED;
    edge->is_connected = false;
    edge->has_new_data = false;

    // clear command group ID (will need to recreate on reconnect)
    free(edge->cmd_group_id);
    edge->cmd_group_id = NULL;
}

static DcEdgeResult _dc_edge_send_command(DcEdge *edge, const char *cmd, char **response) {

    // EDGE RCS protocol requires a NEW connection for each command
    DcSock *cmd_sock = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));
    if (!cmd_sock)
        return DC_EDGE_RESULT_FAIL;
    if (dc_sock_connect(cmd_sock, edge->ip, edge->port) != DC_SOCK_RESULT_SUCCESS) {
        dc_sock_close(cmd_sock);
        return DC_EDGE_RESULT_FAIL;
    }

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

static DcEdgeResult _dc_edge_read_message(DcEdge *edge, char **response) {

    int   buf_size = 256;
    int   nread    = 0;
    char *buf      = (char *)malloc(buf_size);
    if (!buf) return DC_EDGE_RESULT_FAIL;

    // blocking read until END_OF_MSG character (0x04)
    // (socket is already set to blocking with recv timeout)
    for (;;) {
        char         c;
        int          recv_count;
        DcSockResult result = dc_sock_receive(edge->sock, &c, 1, &recv_count);

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

static DcEdgeResult _dc_edge_setup_command_group(DcEdge *edge) {

    // create command group
    char        *group_id = NULL;
    DcEdgeResult result   = _dc_edge_send_command(edge, "create_command_group", &group_id);
    if (result != DC_EDGE_RESULT_SUCCESS || !group_id || !group_id[0]) {
        free(group_id);
        return DC_EDGE_RESULT_FAIL;
    }

    // store group ID
    edge->cmd_group_id = group_id;

    // add each rx command to the group
    // (_dc_edge_send_command creates a new connection for each command)
    for (int ii = 0; ii < sbcount(edge->rx_cmd_offsets); ii++) {
        char *rx_cmd = &(edge->rx_cmds[edge->rx_cmd_offsets[ii]]);
        snprintf(edge->temp_buffer, DC_EDGE_TEMP_BUFFER_SIZE,
                 "add_command_to_group %s \"%s\"", edge->cmd_group_id, rx_cmd);

        char *response = NULL;
        result         = _dc_edge_send_command(edge, edge->temp_buffer, &response);
        free(response);

        if (result != DC_EDGE_RESULT_SUCCESS) {
            return DC_EDGE_RESULT_FAIL;
        }
    }

    return DC_EDGE_RESULT_SUCCESS;
}
