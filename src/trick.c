#include "trick.h"
#include "utils/stb_sb.h"
#include "utils/log.h"
#include "sock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct DcTrick {
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

    // time between reconnects
    time_t reconnect_start;

    // stretchy buffers
    char *rx_cmds;
    int  *rx_cmd_offsets;
    char *tx_cmds;
    int  *tx_cmd_offsets;
    char *rx_oad_vars;
    int  *rx_oad_var_offsets;
    char *tx_buffer;
    char *rx_buffer;
    char *rx_var_values;
    int  *rx_var_offsets;
    char *rx_oad_var_values;
    int  *rx_oad_var_value_offsets;

    // general use buffer
    char *temp_buffer;
};

// connect to trick variable server
static void _dc_trick_connect(DcTrick *trick);

// disconnect from trick variable server
static void _dc_trick_close(DcTrick *trick);

// append data to tx buffer
static void _dc_trick_append_to_tx_buffer(DcTrick *trick, const char *in, size_t in_size);

// send chunk of tx buffer, update internal tx buffer offset
static DcTrickResult _dc_trick_send(DcTrick *trick);

// receive chunk of rx buffer, process data if full packet, update internal tx buffer offset
static DcTrickResult _dc_trick_receive(DcTrick *trick);

#define DC_TRICK_TEMP_BUFFER_SIZE 16384

void dc_trick_init(void) {
    // Retained for API compatibility; each connection now initializes itself.
}

DcTrick *dc_trick_create(const char *host, int port, float data_rate, int timeout_s) {

    DcTrick *trick = (DcTrick *)calloc(1, sizeof(DcTrick));
    if (!trick) {
        DC_LOG_ERROR("Trick", "Failed to allocate Trick");
        return NULL;
    }

    if (dc_sock_host_to_ip(host, trick->ip) != DC_SOCK_RESULT_SUCCESS) {
        free(trick);
        return NULL;
    }
    trick->port                     = port;
    trick->data_rate                = data_rate;
    trick->timeout_s                = timeout_s;
    trick->is_connected             = false;
    trick->has_new_data             = false;
    trick->sock                     = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));
    if (!trick->sock) {
        free(trick);
        return NULL;
    }
    trick->state                    = DC_SOCK_STATE_DISCONNECTED;
    trick->reconnect_start          = 0;
    trick->rx_cmds                  = NULL;
    trick->rx_cmd_offsets           = NULL;
    trick->tx_cmds                  = NULL;
    trick->tx_cmd_offsets           = NULL;
    trick->rx_oad_vars              = NULL;
    trick->rx_oad_var_offsets       = NULL;
    trick->tx_buffer                = NULL;
    trick->rx_buffer                = NULL;
    trick->rx_var_values            = NULL;
    trick->rx_var_offsets           = NULL;
    trick->rx_oad_var_values        = NULL;
    trick->rx_oad_var_value_offsets = NULL;
    trick->temp_buffer              = (char *)malloc(DC_TRICK_TEMP_BUFFER_SIZE);
    if (!trick->temp_buffer) {
        DC_LOG_ERROR("Trick", "Failed to allocate temp buffer");
        dc_sock_close(trick->sock);
        free(trick);
        return NULL;
    }

    return trick;
}

void dc_trick_cleanup(DcTrick *trick) {

    if (!trick) return;

    _dc_trick_close(trick);
    sbfree(trick->rx_cmds);
    sbfree(trick->rx_cmd_offsets);
    sbfree(trick->rx_oad_vars);
    sbfree(trick->rx_oad_var_offsets);
    sbfree(trick->rx_oad_var_values);
    sbfree(trick->rx_oad_var_value_offsets);
    sbfree(trick->tx_cmds);
    sbfree(trick->tx_cmd_offsets);
    sbfree(trick->tx_buffer);
    sbfree(trick->rx_buffer);
    sbfree(trick->rx_var_values);
    sbfree(trick->rx_var_offsets);
    free(trick->temp_buffer);
    free(trick);
}

// main update, called each frame
void dc_trick_update(DcTrick *trick) {

    if (!trick) return;

    DcSockState last_state = trick->state;
    switch (last_state) {
        case DC_SOCK_STATE_DISCONNECTED:
        case DC_SOCK_STATE_CONNECTING: {
            DcSockState curr_state = dc_sock_connection_status(trick->sock);
            switch (curr_state) {

                // if it's still disconnected, check the timeout and reconnect if needed
                case DC_SOCK_STATE_DISCONNECTED:
                case DC_SOCK_STATE_CONNECTING:

                    if (difftime(time(NULL), trick->reconnect_start) > trick->timeout_s) {
                        if (curr_state == DC_SOCK_STATE_CONNECTING) {
                            _dc_trick_close(trick);
                        }
                        DC_LOG_ERROR("Trick", "[%s:%d] Attempting reconnect..", trick->ip, trick->port);
                        _dc_trick_connect(trick);
                    }
                    break;

                // if it is now connected, send the initial conditions
                case DC_SOCK_STATE_CONNECTED: {

                    // clear buffers
                    sbclear(trick->tx_buffer);
                    sbclear(trick->rx_buffer);

                    // send initial conditions
                    // 1) pause variable server
                    strcpy(trick->temp_buffer, "trick.var_pause()\n");
                    _dc_trick_append_to_tx_buffer(trick, trick->temp_buffer, strlen(trick->temp_buffer));

                    // 2) set sample rate
                    snprintf(trick->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, "trick.var_cycle(%f)\n", trick->data_rate);
                    _dc_trick_append_to_tx_buffer(trick, trick->temp_buffer, strlen(trick->temp_buffer));

                    // 3) write list of variables to listen to
                    for (int ii = 0; ii < sbcount(trick->rx_cmd_offsets); ii++) {
                        char *varAddCmd = &(trick->rx_cmds[trick->rx_cmd_offsets[ii]]);
                        _dc_trick_append_to_tx_buffer(trick, varAddCmd, strlen(varAddCmd));
                    }

                    // 4) unpause
                    strcpy(trick->temp_buffer, "trick.var_unpause()\n");
                    _dc_trick_append_to_tx_buffer(trick, trick->temp_buffer, strlen(trick->temp_buffer));

                    // 5) send
                    DcTrickResult result = _dc_trick_send(trick);
                    if (result == DC_TRICK_RESULT_FAIL) {
                        curr_state = DC_SOCK_STATE_DISCONNECTED;
                    }

                    // update connection state
                    trick->state        = curr_state;
                    trick->is_connected = curr_state == DC_SOCK_STATE_CONNECTED;

                    break;
                }

                default:
                    break;
            }
            break;
        }
        case DC_SOCK_STATE_CONNECTED: {
            // send updated variable values here
            DcTrickResult result = _dc_trick_send(trick);
            if (result == DC_TRICK_RESULT_SUCCESS) {

                // receive updated values
                result = _dc_trick_receive(trick);
            }

            // if now disconnected, cleanup the socket
            if (result == DC_TRICK_RESULT_FAIL) {
                _dc_trick_close(trick);
            }

            break;
        }
        default:
            DC_LOG_ERROR("Trick", "[%s:%d] Unknown sock state: %d", trick->ip, trick->port, last_state);
            break;
    }
}

bool dc_trick_is_connected(DcTrick *trick) {
    return trick && trick->is_connected;
}

bool dc_trick_has_new_data(DcTrick *trick) {
    return trick && trick->has_new_data;
}

DcTrickVarIndex dc_trick_add_tx_var(DcTrick *trick, const char *path, const char *units, bool is_string) {


    // create cmd
    if (is_string) {
        snprintf(trick->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, "trick.var_set(\"%s\", \"%%s\"", path);
    } else {
        snprintf(trick->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, "trick.var_set(\"%s\", %%s", path);
    }
    if (units) {
        strcat(trick->temp_buffer, ", \"");
        strcat(trick->temp_buffer, units);
        strcat(trick->temp_buffer, "\")\n");
    } else {
        strcat(trick->temp_buffer, ")\n");
    }

    // copy
    int start = sbcount(trick->tx_cmds);
    sbpush(trick->tx_cmd_offsets, start);
    sbpushn(trick->tx_cmds, trick->temp_buffer, (int)strlen(trick->temp_buffer));
    sbpush(trick->tx_cmds, '\0');

    // return index
    return sbcount(trick->tx_cmd_offsets) - 1;
}

DcTrickVarIndex dc_trick_add_rx_var(DcTrick *trick, const char *path, const char *units) {

    // create cmd
    snprintf(trick->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, "trick.var_add(\"%s\"", path);
    if (units) {
        strcat(trick->temp_buffer, ", \"");
        strcat(trick->temp_buffer, units);
        strcat(trick->temp_buffer, "\")\n");
    } else {
        strcat(trick->temp_buffer, ")\n");
    }

    // copy
    int start = sbcount(trick->rx_cmds);
    sbpush(trick->rx_cmd_offsets, start);
    sbpushn(trick->rx_cmds, trick->temp_buffer, (int)strlen(trick->temp_buffer));
    sbpush(trick->rx_cmds, '\0');

    // return index
    return sbcount(trick->rx_cmd_offsets) - 1;
}

DcTrickVarIndex dc_trick_add_rx_oad_var(DcTrick *trick, const char *path, const char *units) {

    // copy
    int start = sbcount(trick->rx_oad_vars);
    sbpush(trick->rx_oad_var_offsets, start);
    sbpushn(trick->rx_oad_vars, path, (int)strlen(path));
    sbpush(trick->rx_oad_vars, '\0');

    // return index
    return sbcount(trick->rx_oad_var_offsets) - 1;
}

void dc_trick_set_tx_var(DcTrick *trick, DcTrickVarIndex var, const char *value) {
    snprintf(trick->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, &(trick->tx_cmds[trick->tx_cmd_offsets[var]]), value);
    _dc_trick_append_to_tx_buffer(trick, trick->temp_buffer, strlen(trick->temp_buffer));
}

void dc_trick_get_rx_var_value(DcTrick *trick, DcTrickVarIndex var_index, char *out) {
    strcpy(out, &(trick->rx_var_values[trick->rx_var_offsets[var_index]]));
}

void dc_trick_get_rx_oad_value(DcTrick *trick, DcTrickVarIndex oad_index, char *out) {
    strcpy(out, &(trick->rx_oad_var_values[trick->rx_oad_var_value_offsets[oad_index]]));
}

// static helpers

static void _dc_trick_append_to_tx_buffer(DcTrick *trick, const char *in, size_t in_size) {
    sbpushn(trick->tx_buffer, in, (int)in_size);
}

// send chunk of tx buffer, update internal tx buffer offset
static DcTrickResult _dc_trick_send(DcTrick *trick) {

    // only send if there is data to send
    if (sbcount(trick->tx_buffer)) {

        int          sent_count;
        DcSockResult result = dc_sock_send(trick->sock, trick->tx_buffer, sbcount(trick->tx_buffer), &sent_count);
        switch (result) {
            case DC_SOCK_RESULT_FAIL:
            case DC_SOCK_RESULT_CONN_CLOSED:
                return DC_TRICK_RESULT_FAIL;
                break;

            case DC_SOCK_RESULT_CONN_WOULD_BLOCK:
            case DC_SOCK_RESULT_CONN_INTERRUPTED:
                return DC_TRICK_RESULT_SUCCESS;
                break;

            case DC_SOCK_RESULT_SUCCESS:
                // remove the sent elements from the buffer
                sbshiftn(trick->tx_buffer, sent_count);
                return DC_TRICK_RESULT_SUCCESS;
                break;

            default:
                DC_LOG_ERROR("Trick", "[%s:%d] Unknown result from dc_sock_send(): %d", trick->ip, trick->port, result);
                return DC_TRICK_RESULT_FAIL;
                break;
        }
    } else {
        return DC_TRICK_RESULT_SUCCESS;
    }
}

static DcTrickResult _dc_trick_receive(DcTrick *trick) {

    // read all available data from socket, not just one chunk
    bool received_any = false;
    for (;;) {
        int          recv_count;
        DcSockResult result = dc_sock_receive(trick->sock, trick->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, &recv_count);
        if (result == DC_SOCK_RESULT_FAIL || result == DC_SOCK_RESULT_CONN_CLOSED) {
            trick->has_new_data = false;
            return DC_TRICK_RESULT_FAIL;
        }
        if (result == DC_SOCK_RESULT_CONN_WOULD_BLOCK || result == DC_SOCK_RESULT_CONN_INTERRUPTED) {
            break;
        }
        if (result != DC_SOCK_RESULT_SUCCESS) {
            DC_LOG_ERROR("Trick", "[%s:%d] unknown result from dc_sock_receive(): %d", trick->ip, trick->port, result);
            return DC_TRICK_RESULT_FAIL;
        }
        sbpushn(trick->rx_buffer, trick->temp_buffer, recv_count);
        received_any = true;
    }

    if (!received_any) {
        trick->has_new_data = false;
        return DC_TRICK_RESULT_SUCCESS;
    }

    // find last newline (end of last complete line)
    int end_index;
    for (end_index = sbcount(trick->rx_buffer) - 1; end_index >= 0; end_index--) {
        if (trick->rx_buffer[end_index] == '\n') {
            break;
        }
    }

    // find second-to-last newline (end of previous line)
    int start_index = -1;
    if (end_index > 0) {
        for (start_index = end_index - 1; start_index >= 0; start_index--) {
            if (trick->rx_buffer[start_index] == '\n') {
                break;
            }
        }
    }

    trick->has_new_data = false;

    // if there is a complete line
    if (end_index > 0) {
        // handle first message with no preceding newline
        int key_index = (start_index >= 0) ? start_index + 1 : 0;

        // result from var updates (message type 0)
        if (trick->rx_buffer[key_index] == '0' && trick->rx_buffer[key_index + 1] == '\t') {

            // copy to value buffer
            sbclear(trick->rx_var_values);
            int first_value_index = key_index + 2;
            sbpushn(trick->rx_var_values, &(trick->rx_buffer[first_value_index]), end_index - first_value_index);

            // for loop to replace tabs with nulls, set indices
            sbclear(trick->rx_var_offsets);
            sbpush(trick->rx_var_offsets, 0);
            for (int ii = 0; ii < sbcount(trick->rx_var_values); ii++) {
                if (trick->rx_var_values[ii] == '\t') {
                    if (ii + 1 < sbcount(trick->rx_var_values)) {
                        sbpush(trick->rx_var_offsets, ii + 1);
                    }
                    trick->rx_var_values[ii] = '\0';

                } else if (trick->rx_var_values[ii] == ' ') {
                    if (ii + 1 < sbcount(trick->rx_var_values) && trick->rx_var_values[ii + 1] == '{') {
                        // also put a null before the units, if it exists
                        trick->rx_var_values[ii] = '\0';
                    }
                }
            }

            // set last character to null
            sbpush(trick->rx_var_values, '\0');

            // raise flag that there are new values
            trick->has_new_data = true;
            if (sbcount(trick->rx_var_offsets) != sbcount(trick->rx_cmd_offsets)) {
                DC_LOG_ERROR("Trick", "[%s:%d] Size mismatch between expected and received variable count", trick->ip, trick->port);
                trick->has_new_data = false;
            }
        }

        // always remove processed data, regardless of message type
        sbshiftn(trick->rx_buffer, end_index);
    }

    return DC_TRICK_RESULT_SUCCESS;
}

static void _dc_trick_connect(DcTrick *trick) {
    // Closing a socket destroys the socket object, so reconnects need a new one.
    if (!trick->sock)
        trick->sock = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));
    if (trick->sock)
        dc_sock_connect(trick->sock, trick->ip, trick->port);
    trick->reconnect_start = time(NULL);
}

static void _dc_trick_close(DcTrick *trick) {

    // send cleanup commands if connected
    if (trick->is_connected) {
        int sent;
        dc_sock_send(trick->sock, "trick.var_clear()\n", 18, &sent);
        dc_sock_send(trick->sock, "trick.var_exit()\n", 17, &sent);
    }

    dc_sock_close(trick->sock);
    trick->sock         = NULL;
    trick->state        = DC_SOCK_STATE_DISCONNECTED;
    trick->is_connected = false;
    trick->has_new_data = false;
}
