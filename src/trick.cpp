#include "trick.hpp"
#include "sb.hpp"
#include "sock.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct __DcTrickContext {
    char  ip[20];
    int   port;
    float data_rate;
    int   timeout_s;

    // socket
    DcSock      sock;
    DcSockState state;

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
    char *rx_oad_var_value_offsets;

    // general use buffer
    char *temp_buffer;
} _DcTrickContext;

static _DcTrickContext *_contexts = NULL; // stretchy buffer

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

DcTrick dc_trick_create(char *host, int port, float data_rate, int timeout_s) {

    _DcTrickContext context;
    dc_sock_host_to_ip(host, context.ip);
    context.port                     = port;
    context.data_rate                = data_rate;
    context.timeout_s                = timeout_s;
    context.sock                     = dc_sock_create((DcSockFlags)(DC_SOCK_FLAGS_NON_BLOCKING | DC_SOCK_FLAGS_NON_NAGLE));
    context.state                    = DC_SOCK_STATE_DISCONNECTED;
    context.reconnect_start          = 0;
    context.rx_cmds                  = NULL;
    context.rx_cmd_offsets           = NULL;
    context.tx_cmds                  = NULL;
    context.tx_cmd_offsets           = NULL;
    context.rx_oad_vars              = NULL;
    context.rx_oad_var_offsets       = NULL;
    context.tx_buffer                = NULL;
    context.rx_buffer                = NULL;
    context.rx_var_values            = NULL;
    context.rx_var_offsets           = NULL;
    context.rx_oad_var_values        = NULL;
    context.rx_oad_var_value_offsets = NULL;
    context.temp_buffer              = (char *)malloc(DC_TRICK_TEMP_BUFFER_SIZE);
    sbpush(_contexts, context);

    DcTrick trick;
    trick._index       = sbcount(_contexts) - 1;
    trick.has_new_data = false;
    return trick;
}

void dc_trick_cleanup(DcTrick *trick) {

    _DcTrickContext *context = &(_contexts[trick->_index]);

    _dc_trick_close(trick);
    sbfree(context->rx_cmds);
    sbfree(context->rx_cmd_offsets);
    sbfree(context->rx_oad_vars);
    sbfree(context->rx_oad_var_offsets);
    sbfree(context->tx_cmds);
    sbfree(context->tx_cmd_offsets);
    sbfree(context->tx_buffer);
    sbfree(context->rx_buffer);
    sbfree(context->rx_var_values);
    sbfree(context->rx_var_offsets);
    free(context->temp_buffer);
}

// main update, called each frame
void dc_trick_update(DcTrick *trick) {

    _DcTrickContext *context = &(_contexts[trick->_index]);

    DcSockState last_state = context->state;
    switch (last_state) {
        case DC_SOCK_STATE_DISCONNECTED:
        case DC_SOCK_STATE_CONNECTING: {
            DcSockState curr_state = dc_sock_connection_status(&(context->sock));
            switch (curr_state) {

                // if it's still disconnected, check the timeout and reconnect if needed
                case DC_SOCK_STATE_DISCONNECTED:
                case DC_SOCK_STATE_CONNECTING:

                    if (difftime(time(NULL), context->reconnect_start) > context->timeout_s) {
                        if (curr_state == DC_SOCK_STATE_CONNECTING) {
                            _dc_trick_close(trick);
                        }
                        printf("attempting reconnect..\n");
                        _dc_trick_connect(trick);
                    }
                    break;

                // if it is now connected, send the initial conditions
                case DC_SOCK_STATE_CONNECTED: {

                    // clear buffers
                    sbclear(context->tx_buffer);
                    sbclear(context->rx_buffer);

                    // send initial conditions
                    // 1) pause variable server
                    strcpy(context->temp_buffer, "trick.var_pause()\n");
                    _dc_trick_append_to_tx_buffer(trick, context->temp_buffer, strlen(context->temp_buffer));

                    // 2) set sample rate
                    snprintf(context->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, "trick.var_cycle(%f)\n", context->data_rate);
                    _dc_trick_append_to_tx_buffer(trick, context->temp_buffer, strlen(context->temp_buffer));

                    // 3) write list of variables to listen to
                    for (int ii = 0; ii < sbcount(context->rx_cmd_offsets); ii++) {
                        char *varAddCmd = &(context->rx_cmds[context->rx_cmd_offsets[ii]]);
                        _dc_trick_append_to_tx_buffer(trick, varAddCmd, strlen(varAddCmd));
                    }

                    // 4) unpause
                    strcpy(context->temp_buffer, "trick.var_unpause()\n");
                    _dc_trick_append_to_tx_buffer(trick, context->temp_buffer, strlen(context->temp_buffer));

                    // 5) send
                    DcTrickResult result = _dc_trick_send(trick);
                    if (result == DC_TRICK_RESULT_FAIL) {
                        curr_state = DC_SOCK_STATE_DISCONNECTED;
                    }

                    // update connection state
                    context->state = curr_state;

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
            fprintf(stderr, "Unknown sock state: %d\n", last_state);
            break;
    }
}

DcTrickIndex dc_trick_add_tx_var(DcTrick *trick, const char *path, const char *units, bool is_string) {

    _DcTrickContext *context = &(_contexts[trick->_index]);

    // create cmd
    if (is_string) {
        snprintf(context->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, "trick.var_set(\"%s\", \"%%s\"", path);
    } else {
        snprintf(context->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, "trick.var_set(\"%s\", %%s", path);
    }
    if (units) {
        strcat(context->temp_buffer, ", \"");
        strcat(context->temp_buffer, units);
        strcat(context->temp_buffer, "\")\n");
    } else {
        strcat(context->temp_buffer, ")\n");
    }

    // copy
    int start = sbcount(context->tx_cmds);
    sbpush(context->tx_cmd_offsets, start);
    sbpushn(context->tx_cmds, context->temp_buffer, strlen(context->temp_buffer));
    sbpush(context->tx_cmds, '\0');

    // return index
    return sbcount(context->tx_cmd_offsets) - 1;
}

DcTrickIndex dc_trick_add_rx_var(DcTrick *trick, const char *path, const char *units) {
    _DcTrickContext *context = &(_contexts[trick->_index]);

    // create cmd
    snprintf(context->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, "trick.var_add(\"%s\"", path);
    if (units) {
        strcat(context->temp_buffer, ", \"");
        strcat(context->temp_buffer, units);
        strcat(context->temp_buffer, "\")\n");
    } else {
        strcat(context->temp_buffer, ")\n");
    }

    // copy
    int start = sbcount(context->rx_cmds);
    sbpush(context->rx_cmd_offsets, start);
    sbpushn(context->rx_cmds, context->temp_buffer, strlen(context->temp_buffer));
    sbpush(context->rx_cmds, '\0');

    // return index
    return sbcount(context->rx_cmd_offsets) - 1;
}

DcTrickIndex dc_trick_add_rx_oad_var(DcTrick *trick, const char *path, const char *units) {
    _DcTrickContext *context = &(_contexts[trick->_index]);

    // copy
    int start = sbcount(context->rx_oad_vars);
    sbpush(context->rx_oad_var_offsets, start);
    sbpushn(context->rx_oad_vars, path, strlen(path));
    sbpush(context->rx_oad_vars, '\0');

    // return index
    return sbcount(context->rx_cmd_offsets) - 1;
}

void dc_trick_set_tx_var(DcTrick *trick, DcTrickIndex var, const char *value) {
    _DcTrickContext *context = &(_contexts[trick->_index]);
    snprintf(context->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, &(context->tx_cmds[context->tx_cmd_offsets[var]]), value);
    _dc_trick_append_to_tx_buffer(trick, context->temp_buffer, strlen(context->temp_buffer));
}

void dc_trick_get_rx_var_value(DcTrick *trick, DcTrickIndex var_index, char *out) {
    _DcTrickContext *context = &(_contexts[trick->_index]);
    strcpy(out, &(context->rx_var_values[context->rx_var_offsets[var_index]]));
}

void dc_trick_get_rx_oad_value(DcTrick *trick, DcTrickIndex oad_index, char *out) {
    _DcTrickContext *context = &(_contexts[trick->_index]);
    strcpy(out, &(context->rx_oad_var_values[context->rx_oad_var_value_offsets[oad_index]]));
}

// static helpers

void _dc_trick_append_to_tx_buffer(DcTrick *trick, const char *in, size_t in_size) {
    _DcTrickContext *context = &(_contexts[trick->_index]);
    sbpushn(context->tx_buffer, in, in_size);
}

// send chunk of tx buffer, update internal tx buffer offset
DcTrickResult _dc_trick_send(DcTrick *trick) {
    _DcTrickContext *context = &(_contexts[trick->_index]);

    // only send if there is data to send
    if (sbcount(context->tx_buffer)) {

        int          sent_count;
        DcSockResult result = dc_sock_send(&(context->sock), context->tx_buffer, sbcount(context->tx_buffer), &sent_count);
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
                sbshiftn(context->tx_buffer, sent_count);
                return DC_TRICK_RESULT_SUCCESS;
                break;

            default:
                fprintf(stderr, "dc_trick_send(): unknown result from dc_sock_send() %d", result);
                return DC_TRICK_RESULT_FAIL;
                break;
        }
    } else {
        return DC_TRICK_RESULT_SUCCESS;
    }
}

DcTrickResult _dc_trick_receive(DcTrick *trick) {
    _DcTrickContext *context = &(_contexts[trick->_index]);

    int          recv_count;
    DcSockResult result = dc_sock_receive(&(context->sock), context->temp_buffer, DC_TRICK_TEMP_BUFFER_SIZE, &recv_count);
    switch (result) {
        case DC_SOCK_RESULT_FAIL:
        case DC_SOCK_RESULT_CONN_CLOSED:
            trick->has_new_data = false;
            return DC_TRICK_RESULT_FAIL;
            break;

        case DC_SOCK_RESULT_CONN_WOULD_BLOCK:
        case DC_SOCK_RESULT_CONN_INTERRUPTED:
            trick->has_new_data = false;
            return DC_TRICK_RESULT_SUCCESS;
            break;

        case DC_SOCK_RESULT_SUCCESS:

            // add it to the existing buffer
            sbpushn(context->rx_buffer, context->temp_buffer, recv_count);

            // find start and end indices (filtered by newlines)
            int end_index;
            for (end_index = sbcount(context->rx_buffer) - 1; end_index >= 0; end_index--) {
                if (context->rx_buffer[end_index] == '\n') {
                    break;
                }
            }

            int start_index;
            if (end_index > 0) {
                for (start_index = end_index - 1; start_index >= 0; start_index--) {
                    if (context->rx_buffer[start_index] == '\n') {
                        break;
                    }
                }
            }

            // if there is a valid result
            if (start_index >= 0 && end_index >= 0 && start_index != end_index) {
                int key_index = start_index + 1;

                // result from var updates
                if (context->rx_buffer[key_index] == '0' && context->rx_buffer[key_index + 1] == '\t') {

                    // copy to value buffer
                    sbclear(context->rx_var_values);
                    int first_value_index = key_index + 2;
                    sbpushn(context->rx_var_values, &(context->rx_buffer[first_value_index]), end_index - first_value_index);

                    // for loop to replace tabs with nulls, set indices
                    sbclear(context->rx_var_offsets);
                    sbpush(context->rx_var_offsets, 0);
                    for (int ii = 0; ii < sbcount(context->rx_var_values); ii++) {
                        if (context->rx_var_values[ii] == '\t') {
                            if (ii + 1 < sbcount(context->rx_var_values) - 1) {
                                sbpush(context->rx_var_offsets, ii + 1);
                            }
                            context->rx_var_values[ii] = '\0';
                        }
                    }

                    // set last character to null
                    sbpush(context->rx_var_values, '\0');

                    // remove everything up to end_index in buffer
                    sbshiftn(context->rx_buffer, end_index);

                    // raise fag that there are new values
                    trick->has_new_data = true;
                }
            } else {
                trick->has_new_data = false;
            }

            return DC_TRICK_RESULT_SUCCESS;
            break;

        default:
            fprintf(stderr, "dc_trick_send(): unknown result from dc_sock_send() %d", result);
            break;
    }
}

void _dc_trick_connect(DcTrick *trick) {
    _DcTrickContext *context = &(_contexts[trick->_index]);
    dc_sock_connect(&(context->sock), context->ip, context->port);
    context->reconnect_start = time(NULL);
}

void _dc_trick_close(DcTrick *trick) {
    _DcTrickContext *context = &(_contexts[trick->_index]);
    dc_sock_close(&(context->sock));
    context->state = DC_SOCK_STATE_DISCONNECTED;
}
