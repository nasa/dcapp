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

//~ types

struct DcEdge {
    char ip[46]; // ipv6 address capacity
    int port;
    float data_rate;
    int timeout_s;

    // connection state
    bool is_connected;
    bool has_new_data;

    // active socket
    DcSock *sock;
    DcSockState state;

    // update timing
    time_t reconnect_start;
    time_t last_update;

    // edge command group id
    char *cmd_group_id;

    // grouped receive commands
    char *rx_cmds;
    int *rx_cmd_offsets;

    // transmit command templates
    char *tx_cmds;
    int *tx_cmd_offsets;

    // received values
    char *rx_var_values;
    int *rx_var_offsets;

    // pending writes
    char *tx_buffer;

    // scratch space
    char *temp_buffer;
};

#define DC_EDGE_TEMP_BUFFER_SIZE 16384

//~ internal declarations

static DcEdgeResult _dc_edge_connect(DcEdge *edge);
static void _dc_edge_close(DcEdge *edge);
static DcEdgeResult _dc_edge_send_command(DcEdge *edge, const char *cmd, char **response);
static DcEdgeResult _dc_edge_read_message(DcEdge *edge, char **response);
static DcEdgeResult _dc_edge_setup_command_group(DcEdge *edge);

//~ public api

void dc_edge_init(void) {
    // retained for api compatibility
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
    edge->port = port > 0 ? port : DC_EDGE_DEFAULT_PORT;
    edge->data_rate = data_rate;
    edge->timeout_s = timeout_s;
    edge->is_connected = false;
    edge->has_new_data = false;
    edge->sock = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));
    if (!edge->sock) {
        free(edge);
        return NULL;
    }
    edge->state = DC_SOCK_STATE_DISCONNECTED;
    edge->reconnect_start = 0;
    edge->last_update = 0;
    edge->cmd_group_id = NULL;
    edge->rx_cmds = NULL;
    edge->rx_cmd_offsets = NULL;
    edge->tx_cmds = NULL;
    edge->tx_cmd_offsets = NULL;
    edge->rx_var_values = NULL;
    edge->rx_var_offsets = NULL;
    edge->tx_buffer = NULL;
    edge->temp_buffer = (char *)malloc(DC_EDGE_TEMP_BUFFER_SIZE);
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

    //- reconnect when due
    if (!edge->is_connected) {
        if (difftime(time(NULL), edge->reconnect_start) > edge->timeout_s) {
            DC_LOG_INFO("EDGE", "[%s:%d] Attempting reconnect..", edge->ip, edge->port);
            DcEdgeResult result = _dc_edge_connect(edge);
            if (result == DC_EDGE_RESULT_SUCCESS) {
                DC_LOG_INFO("EDGE", "[%s:%d] Connected", edge->ip, edge->port);
                edge->is_connected = true;

                // rebuild the receive command group
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

    //- honor the sample rate
    if (difftime(time(NULL), edge->last_update) < edge->data_rate) {
        return;
    }
    edge->last_update = time(NULL);

    //- flush pending writes
    for (int ii = 0; ii < sbcount(edge->tx_buffer);) {
        // find the next null-terminated command
        char *cmd = &edge->tx_buffer[ii];
        int cmd_len = (int)strlen(cmd);

        char *response = NULL;
        DcEdgeResult result = _dc_edge_send_command(edge, cmd, &response);
        free(response);

        if (result != DC_EDGE_RESULT_SUCCESS) {
            DC_LOG_WARN("EDGE", "[%s:%d] TX command failed, disconnecting", edge->ip, edge->port);
            _dc_edge_close(edge);
            return;
        }

        ii += cmd_len + 1;
    }
    sbclear(edge->tx_buffer);

    //- refresh grouped values
    if (edge->cmd_group_id && sbcount(edge->rx_cmd_offsets) > 0) {
        snprintf(edge->temp_buffer, DC_EDGE_TEMP_BUFFER_SIZE, "execute_command_group %s", edge->cmd_group_id);

        char *response = NULL;
        DcEdgeResult result = _dc_edge_send_command(edge, edge->temp_buffer, &response);

        if (result != DC_EDGE_RESULT_SUCCESS || !response) {
            DC_LOG_WARN("EDGE", "[%s:%d] execute_command_group failed, disconnecting", edge->ip, edge->port);
            free(response);
            _dc_edge_close(edge);
            return;
        }

        // split the space-separated response
        sbclear(edge->rx_var_values);
        sbclear(edge->rx_var_offsets);

        char *ptr = response;
        while (*ptr) {
            // skip leading whitespace
            while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')
                ptr++;
            if (!*ptr) break;

            // record the value start
            int start = sbcount(edge->rx_var_values);
            sbpush(edge->rx_var_offsets, start);

            // copy through the value boundary
            while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '\r') {
                sbpush(edge->rx_var_values, *ptr);
                ptr++;
            }
            sbpush(edge->rx_var_values, '\0');
        }

        free(response);

        // accept only complete value sets
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

    // store the command template for later values
    int start = sbcount(edge->tx_cmds);
    sbpush(edge->tx_cmd_offsets, start);
    sbpushn(edge->tx_cmds, command, (int)strlen(command));
    sbpush(edge->tx_cmds, '\0');

    return sbcount(edge->tx_cmd_offsets) - 1;
}

DcEdgeVarIndex dc_edge_add_rx_var(DcEdge *edge, const char *command) {

    // store the command for group setup
    int start = sbcount(edge->rx_cmds);
    sbpush(edge->rx_cmd_offsets, start);
    sbpushn(edge->rx_cmds, command, (int)strlen(command));
    sbpush(edge->rx_cmds, '\0');

    return sbcount(edge->rx_cmd_offsets) - 1;
}

void dc_edge_set_tx_var(DcEdge *edge, DcEdgeVarIndex var, const char *value) {

    // build the command and value pair
    char *cmd_template = &(edge->tx_cmds[edge->tx_cmd_offsets[var]]);
    snprintf(edge->temp_buffer, DC_EDGE_TEMP_BUFFER_SIZE, "%s %s", cmd_template, value);

    // queue the null-terminated command
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

//~ internal helpers

static DcEdgeResult _dc_edge_connect(DcEdge *edge) {

    // reset any prior connection
    _dc_edge_close(edge);

    // create a fresh socket
    edge->sock = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));
    if (!edge->sock)
        return DC_EDGE_RESULT_FAIL;

    // begin connecting
    if (dc_sock_connect(edge->sock, edge->ip, edge->port) != DC_SOCK_RESULT_SUCCESS) {
        _dc_edge_close(edge);
        return DC_EDGE_RESULT_FAIL;
    }

    // wait for the connection deadline
    time_t start = time(NULL);
    while (difftime(time(NULL), start) < 2.0) {
        DcSockState state = dc_sock_connection_status(edge->sock);
        if (state == DC_SOCK_STATE_CONNECTED) {
            // switch to bounded blocking io
            dc_sock_set_blocking(edge->sock);
            dc_sock_set_recv_timeout(edge->sock, 2000);

            // consume the server greeting
            char *version = NULL;
            DcEdgeResult result = _dc_edge_read_message(edge, &version);
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
        // avoid a busy loop
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
    edge->sock = NULL;
    edge->state = DC_SOCK_STATE_DISCONNECTED;
    edge->is_connected = false;
    edge->has_new_data = false;

    // force command group recreation after reconnect
    free(edge->cmd_group_id);
    edge->cmd_group_id = NULL;
}

static DcEdgeResult _dc_edge_send_command(DcEdge *edge, const char *cmd, char **response) {

    // edge rcs requires a fresh connection per command
    DcSock *cmd_sock = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));
    if (!cmd_sock)
        return DC_EDGE_RESULT_FAIL;
    if (dc_sock_connect(cmd_sock, edge->ip, edge->port) != DC_SOCK_RESULT_SUCCESS) {
        dc_sock_close(cmd_sock);
        return DC_EDGE_RESULT_FAIL;
    }

    // wait for the connection deadline
    time_t start = time(NULL);
    bool connected = false;
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

    // bound blocking io to avoid stalling rendering
    dc_sock_set_blocking(cmd_sock);
    dc_sock_set_recv_timeout(cmd_sock, 500);

    // discard the server greeting
    int buf_size = 256;
    int nread = 0;
    char *buf = (char *)malloc(buf_size);
    if (!buf) {
        dc_sock_close(cmd_sock);
        return DC_EDGE_RESULT_FAIL;
    }

    for (;;) {
        char c;
        int recv_count;
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

    // send the command
    int cmd_len = (int)strlen(cmd);
    int sent_count;
    DcSockResult result = dc_sock_send(cmd_sock, cmd, cmd_len, &sent_count);
    if (result != DC_SOCK_RESULT_SUCCESS || sent_count != cmd_len) {
        dc_sock_close(cmd_sock);
        return DC_EDGE_RESULT_FAIL;
    }

    // signal the end of the command
    dc_sock_shutdown_write(cmd_sock);

    // collect the response
    buf_size = 256;
    nread = 0;
    buf = (char *)malloc(buf_size);
    if (!buf) {
        dc_sock_close(cmd_sock);
        return DC_EDGE_RESULT_FAIL;
    }

    for (;;) {
        char c;
        int recv_count;
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

    int buf_size = 256;
    int nread = 0;
    char *buf = (char *)malloc(buf_size);
    if (!buf) return DC_EDGE_RESULT_FAIL;

    // read through the protocol terminator with the configured timeout
    for (;;) {
        char c;
        int recv_count;
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

    // create the remote command group
    char *group_id = NULL;
    DcEdgeResult result = _dc_edge_send_command(edge, "create_command_group", &group_id);
    if (result != DC_EDGE_RESULT_SUCCESS || !group_id || !group_id[0]) {
        free(group_id);
        return DC_EDGE_RESULT_FAIL;
    }

    // retain the returned group id
    edge->cmd_group_id = group_id;

    // add each receive command over its own connection
    for (int ii = 0; ii < sbcount(edge->rx_cmd_offsets); ii++) {
        char *rx_cmd = &(edge->rx_cmds[edge->rx_cmd_offsets[ii]]);
        snprintf(edge->temp_buffer, DC_EDGE_TEMP_BUFFER_SIZE,
                 "add_command_to_group %s \"%s\"", edge->cmd_group_id, rx_cmd);

        char *response = NULL;
        result = _dc_edge_send_command(edge, edge->temp_buffer, &response);
        free(response);

        if (result != DC_EDGE_RESULT_SUCCESS) {
            return DC_EDGE_RESULT_FAIL;
        }
    }

    return DC_EDGE_RESULT_SUCCESS;
}
