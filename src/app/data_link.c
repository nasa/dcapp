#include "data_link.h"

#include "app/variable_registry.h"
#include "value.h"

#include "pl.h"

#include "edge.h"
#include "trick.h"
#include "utils/stb_sb.h"

#include <string.h>

typedef struct _DcAppTrickTxBinding {
    DcAppVariableRegistryVariableIndex dcapp_var_index;
    DcTrickVarIndex trick_var_index;
    DcAppValue prev_value;
    uint64_t last_write_sequence;
    bool force_send;
} DcAppTrickTxBinding;

typedef struct _DcAppTrickRxBinding {
    DcAppVariableRegistryVariableIndex dcapp_var_index;
    DcTrickVarIndex trick_var_index;
} DcAppTrickRxBinding;

typedef struct _DcAppTrickConnection {
    DcTrick *trick;
    DcAppTrickTxBinding *sb_tx_var_contexts;
    DcAppTrickRxBinding *sb_rx_var_contexts;
    DcAppVariableRegistryVariableIndex connected_var_index; // variable updated with connection status
    bool was_connected;                                     // previous connection state for init-on-connect
} DcAppTrickConnection;

typedef struct _DcAppEdgeTxBinding {
    DcAppVariableRegistryVariableIndex dcapp_var_index;
    DcEdgeVarIndex edge_var_index;
    DcAppValue prev_value;
    uint64_t last_write_sequence;
    bool force_send;
} DcAppEdgeTxBinding;

typedef struct _DcAppEdgeRxBinding {
    DcAppVariableRegistryVariableIndex dcapp_var_index;
    DcEdgeVarIndex edge_var_index;
} DcAppEdgeRxBinding;

typedef struct _DcAppEdgeConnection {
    DcEdge *edge;
    DcAppEdgeTxBinding *sb_tx_var_contexts;
    DcAppEdgeRxBinding *sb_rx_var_contexts;
    DcAppVariableRegistryVariableIndex connected_var_index; // variable updated with connection status
    bool was_connected;                                     // previous connection state for disconnect detection
} DcAppEdgeConnection;

struct DcAppDataLinkContext {
    DcAppTrickConnection *sb_tricks;
    DcAppEdgeConnection *sb_edges;
};

static const plMemoryI *_ext_memory = NULL;

#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_FREE(x) _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

void dc_app_data_link_init(plApiRegistryI *api_registry) {
    _ext_memory = pl_get_api_latest(api_registry, plMemoryI);
}

DcAppDataLinkContext *dc_app_data_link_context_create(void) {
    DcAppDataLinkContext *data_link = PL_ALLOC(sizeof(*data_link));
    if (!data_link) return NULL;
    memset(data_link, 0, sizeof(*data_link));
    dc_trick_init();
    dc_edge_init();
    return data_link;
}

void dc_app_data_link_context_destroy(DcAppDataLinkContext *data_link) {
    if (!data_link) return;

    // cleanup trick contexts
    for (int i = 0; i < sbcount(data_link->sb_tricks); i++) {
        DcAppTrickConnection *draw_ctx = &data_link->sb_tricks[i];
        sbfree(draw_ctx->sb_tx_var_contexts);
        sbfree(draw_ctx->sb_rx_var_contexts);
        dc_trick_cleanup(draw_ctx->trick);
    }

    // cleanup edge contexts
    for (int i = 0; i < sbcount(data_link->sb_edges); i++) {
        DcAppEdgeConnection *draw_ctx = &data_link->sb_edges[i];
        sbfree(draw_ctx->sb_tx_var_contexts);
        sbfree(draw_ctx->sb_rx_var_contexts);
        dc_edge_cleanup(draw_ctx->edge);
    }
    sbfree(data_link->sb_tricks);
    sbfree(data_link->sb_edges);
    PL_FREE(data_link);
}

void dc_app_data_link_add_edge(DcAppDataLinkContext *data_link, const char *host, int port, float data_rate, DcAppVariableRegistryVariableIndex connected_var_index) {
    if (!data_link) return;
    DcAppEdgeConnection edge_context = {};
    edge_context.edge = dc_edge_create(host, port, data_rate, 2);
    edge_context.connected_var_index = connected_var_index;
    sbpush(data_link->sb_edges, edge_context);
}

void dc_app_data_link_add_edge_rx(DcAppDataLinkContext *data_link, const char *command, DcAppVariableRegistryVariableIndex dcapp_var_index) {
    if (!data_link || sbcount(data_link->sb_edges) == 0) return;
    DcAppEdgeConnection *connection = &data_link->sb_edges[sbcount(data_link->sb_edges) - 1];
    if (!connection->edge) return;
    DcAppEdgeRxBinding var = {};
    var.edge_var_index = dc_edge_add_rx_var(connection->edge, command);
    var.dcapp_var_index = dcapp_var_index;
    sbpush(connection->sb_rx_var_contexts, var);
}

void dc_app_data_link_add_edge_tx(DcAppDataLinkContext *data_link, const char *command, DcAppVariableRegistryVariableIndex dcapp_var_index, const DcAppValue *initial_value) {
    if (!data_link || sbcount(data_link->sb_edges) == 0) return;
    DcAppEdgeConnection *connection = &data_link->sb_edges[sbcount(data_link->sb_edges) - 1];
    if (!connection->edge) return;
    DcAppEdgeTxBinding var = {};
    var.dcapp_var_index = dcapp_var_index;
    var.edge_var_index = dc_edge_add_tx_var(connection->edge, command);
    var.force_send = false;
    if (initial_value) var.prev_value = *initial_value;
    sbpush(connection->sb_tx_var_contexts, var);
}

void dc_app_data_link_add_trick(DcAppDataLinkContext *data_link, const char *host, int port, float data_rate, DcAppVariableRegistryVariableIndex connected_var_index) {
    if (!data_link) return;
    DcAppTrickConnection trick_context = {};
    trick_context.trick = dc_trick_create(host, port, data_rate, 1);
    trick_context.connected_var_index = connected_var_index;
    sbpush(data_link->sb_tricks, trick_context);
}

void dc_app_data_link_add_trick_rx(DcAppDataLinkContext *data_link, const char *path, const char *units, DcAppVariableRegistryVariableIndex dcapp_var_index) {
    if (!data_link || sbcount(data_link->sb_tricks) == 0) return;
    DcAppTrickConnection *connection = &data_link->sb_tricks[sbcount(data_link->sb_tricks) - 1];
    if (!connection->trick) return;
    DcAppTrickRxBinding var = {};
    var.trick_var_index = dc_trick_add_rx_var(connection->trick, path, units);
    var.dcapp_var_index = dcapp_var_index;
    sbpush(connection->sb_rx_var_contexts, var);
}

void dc_app_data_link_add_trick_tx(DcAppDataLinkContext *data_link, const char *path, const char *units, DcAppVariableRegistryVariableIndex dcapp_var_index, const DcAppValue *initial_value) {
    if (!data_link || sbcount(data_link->sb_tricks) == 0) return;
    DcAppTrickConnection *connection = &data_link->sb_tricks[sbcount(data_link->sb_tricks) - 1];
    if (!connection->trick) return;
    DcAppTrickTxBinding var = {};
    var.dcapp_var_index = dcapp_var_index;
    var.trick_var_index = dc_trick_add_tx_var(
        connection->trick,
        path,
        units,
        initial_value && initial_value->type == DC_APP_VALUE_TYPE_STRING);
    var.force_send = false;
    if (initial_value) var.prev_value = *initial_value;
    sbpush(connection->sb_tx_var_contexts, var);
}

void dc_app_data_link_update(DcAppDataLinkContext *data_link, DcAppVariableRegistryContext *lookup) {
    if (!data_link || !lookup) return;

    // send trick data
    for (int ii = 0; ii < sbcount(data_link->sb_tricks); ii++) {
        DcAppTrickConnection *trick_context = &data_link->sb_tricks[ii];
        DcTrick *trick = trick_context->trick;

        // On (re)connect, force every tx var to initialize the sim, including zero values.
        bool is_trick_connected = dc_trick_is_connected(trick);
        if (is_trick_connected && !trick_context->was_connected) {
            for (int jj = 0; jj < sbcount(trick_context->sb_tx_var_contexts); jj++) {
                trick_context->sb_tx_var_contexts[jj].force_send = false;
            }
        }
        trick_context->was_connected = is_trick_connected;

        // add tx commands to buffer
        if (is_trick_connected) {
            for (int jj = 0; jj < sbcount(trick_context->sb_tx_var_contexts); jj++) {
                DcAppTrickTxBinding *tx_var_context = &trick_context->sb_tx_var_contexts[jj];
                if (tx_var_context->dcapp_var_index == DC_APP_VARIABLE_REGISTRY_VARIABLE_INDEX_UNDEFINED) continue;
                DcAppVariableRegistryValueIndex value_index = dc_app_variable_registry_get_variable_value_index(lookup, tx_var_context->dcapp_var_index);
                DcAppValue *curr_value = dc_app_variable_registry_get_value(lookup, value_index);
                DcAppValue *prev_value = &tx_var_context->prev_value;
                uint64_t write_sequence = dc_app_variable_registry_get_variable_write_sequence(lookup, tx_var_context->dcapp_var_index);

                // Explicit Sets are commands: preserve legacy force-write behavior even
                // when this client's cached value is already the requested value.
                if (tx_var_context->force_send ||
                    write_sequence != tx_var_context->last_write_sequence ||
                    !dc_app_value_is_equal(curr_value, prev_value)) {
                    dc_trick_set_tx_var(trick, tx_var_context->trick_var_index, curr_value->value_string);
                    *prev_value = *curr_value;
                    tx_var_context->last_write_sequence = write_sequence;
                    tx_var_context->force_send = false;
                }
            }
        }

        // send the updated buffer, receive the new data, update the connection status
        dc_trick_update(trick);

        // update connected variable if defined
        if (trick_context->connected_var_index != DC_APP_VARIABLE_REGISTRY_VARIABLE_INDEX_UNDEFINED) {
            DcAppVariableRegistryValueIndex value_index = dc_app_variable_registry_get_variable_value_index(lookup, trick_context->connected_var_index);
            DcAppValue *value = dc_app_variable_registry_get_value(lookup, value_index);
            if (value) {
                bool is_connected = dc_trick_is_connected(trick);
                switch (value->type) {
                    case DC_APP_VALUE_TYPE_BOOLEAN:
                        value->value_boolean = is_connected;
                        break;
                    case DC_APP_VALUE_TYPE_INTEGER:
                        value->value_integer = is_connected ? 1 : 0;
                        break;
                    case DC_APP_VALUE_TYPE_STRING:
                        strncpy(value->value_string, is_connected ? "true" : "false", DC_APP_VALUE_STRING_BUFFER_SIZE - 1);
                        break;
                    default:
                        break;
                }
                dc_app_value_refresh(value);
            }
        }

        // receive the new data
        if (dc_trick_has_new_data(trick) && dc_trick_is_connected(trick)) {
            char rx_buffer[256];
            for (int jj = 0; jj < sbcount(trick_context->sb_rx_var_contexts); jj++) {
                DcAppTrickRxBinding *rx_var_context = &trick_context->sb_rx_var_contexts[jj];
                if (rx_var_context->dcapp_var_index == DC_APP_VARIABLE_REGISTRY_VARIABLE_INDEX_UNDEFINED) continue;
                dc_trick_get_rx_var_value(trick, rx_var_context->trick_var_index, rx_buffer);
                dc_app_variable_registry_set_variable_to_string(lookup, rx_var_context->dcapp_var_index, rx_buffer);
            }
        }
    }

    // send edge data
    for (int ii = 0; ii < sbcount(data_link->sb_edges); ii++) {
        DcAppEdgeConnection *edge_context = &data_link->sb_edges[ii];
        DcEdge *edge = edge_context->edge;

        // On (re)connect, force every tx var to initialize the scene, including zero values.
        bool is_connected = dc_edge_is_connected(edge);
        if (is_connected && !edge_context->was_connected) {
            for (int jj = 0; jj < sbcount(edge_context->sb_tx_var_contexts); jj++) {
                edge_context->sb_tx_var_contexts[jj].force_send = false;
            }
        }
        edge_context->was_connected = is_connected;

        // add tx commands to buffer
        if (is_connected) {
            for (int jj = 0; jj < sbcount(edge_context->sb_tx_var_contexts); jj++) {
                DcAppEdgeTxBinding *tx_var_context = &edge_context->sb_tx_var_contexts[jj];
                if (tx_var_context->dcapp_var_index == DC_APP_VARIABLE_REGISTRY_VARIABLE_INDEX_UNDEFINED) continue;
                DcAppVariableRegistryValueIndex value_index = dc_app_variable_registry_get_variable_value_index(lookup, tx_var_context->dcapp_var_index);
                DcAppValue *curr_value = dc_app_variable_registry_get_value(lookup, value_index);
                DcAppValue *prev_value = &tx_var_context->prev_value;
                uint64_t write_sequence = dc_app_variable_registry_get_variable_write_sequence(lookup, tx_var_context->dcapp_var_index);

                if (tx_var_context->force_send ||
                    write_sequence != tx_var_context->last_write_sequence ||
                    !dc_app_value_is_equal(curr_value, prev_value)) {
                    dc_edge_set_tx_var(edge, tx_var_context->edge_var_index, curr_value->value_string);
                    *prev_value = *curr_value;
                    tx_var_context->last_write_sequence = write_sequence;
                    tx_var_context->force_send = false;
                }
            }
        }

        // send the updated buffer, receive the new data, update the connection status
        dc_edge_update(edge);

        // update connected variable if defined
        if (edge_context->connected_var_index != DC_APP_VARIABLE_REGISTRY_VARIABLE_INDEX_UNDEFINED) {
            DcAppVariableRegistryValueIndex value_index = dc_app_variable_registry_get_variable_value_index(lookup, edge_context->connected_var_index);
            DcAppValue *value = dc_app_variable_registry_get_value(lookup, value_index);
            if (value) {
                is_connected = dc_edge_is_connected(edge);
                switch (value->type) {
                    case DC_APP_VALUE_TYPE_BOOLEAN:
                        value->value_boolean = is_connected;
                        break;
                    case DC_APP_VALUE_TYPE_INTEGER:
                        value->value_integer = is_connected ? 1 : 0;
                        break;
                    case DC_APP_VALUE_TYPE_STRING:
                        strncpy(value->value_string, is_connected ? "true" : "false", DC_APP_VALUE_STRING_BUFFER_SIZE - 1);
                        break;
                    default:
                        break;
                }
                dc_app_value_refresh(value);
            }
        }

        // receive the new data
        if (dc_edge_has_new_data(edge) && dc_edge_is_connected(edge)) {
            char rx_buffer[256];
            for (int jj = 0; jj < sbcount(edge_context->sb_rx_var_contexts); jj++) {
                DcAppEdgeRxBinding *rx_var_context = &edge_context->sb_rx_var_contexts[jj];
                if (rx_var_context->dcapp_var_index == DC_APP_VARIABLE_REGISTRY_VARIABLE_INDEX_UNDEFINED) continue;
                dc_edge_get_rx_var_value(edge, rx_var_context->edge_var_index, rx_buffer);
                dc_app_variable_registry_set_variable_to_string(lookup, rx_var_context->dcapp_var_index, rx_buffer);
            }
        }
    }
}
