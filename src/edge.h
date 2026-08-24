#ifndef _DC_EDGE_
#define _DC_EDGE_

#include <stdint.h>
#include <stdbool.h>

//~ types

typedef enum {
    DC_EDGE_RESULT_SUCCESS = 0,
    DC_EDGE_RESULT_FAIL
} DcEdgeResult;

typedef struct DcEdge DcEdge;

typedef uint32_t DcEdgeVarIndex;

#ifdef __cplusplus
extern "C" {
#endif

//~ api

//- initialization
void dc_edge_init(void);

//- lifecycle
DcEdge *dc_edge_create(const char *host, int port, float data_rate, int timeout_s);
void dc_edge_cleanup(DcEdge *edge);
void dc_edge_update(DcEdge *edge);

//- state
bool dc_edge_is_connected(DcEdge *edge);
bool dc_edge_has_new_data(DcEdge *edge);

//- variable registration
DcEdgeVarIndex dc_edge_add_tx_var(DcEdge *edge, const char *command);
DcEdgeVarIndex dc_edge_add_rx_var(DcEdge *edge, const char *command);

//- variable access
void dc_edge_set_tx_var(DcEdge *edge, DcEdgeVarIndex var, const char *value);
void dc_edge_get_rx_var_value(DcEdge *edge, DcEdgeVarIndex var_index, char *out);

#ifdef __cplusplus
}
#endif

#endif
