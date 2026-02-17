#ifndef _DC_EDGE_
#define _DC_EDGE_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    DC_EDGE_RESULT_SUCCESS = 0,
    DC_EDGE_RESULT_FAIL
} DcEdgeResult;

typedef struct _DcEdgeHandle {
    uint8_t index;
} DcEdgeHandle;

typedef uint32_t DcEdgeVarIndex;

#ifdef __cplusplus
extern "C" {
#endif

// initialization
void dc_edge_init(void);

// lifecycle
DcEdgeHandle dc_edge_create(const char *host, int port, float data_rate, int timeout_s);
void         dc_edge_cleanup(DcEdgeHandle edge);
void         dc_edge_update(DcEdgeHandle edge);

// state checks
bool dc_edge_is_connected(DcEdgeHandle edge);
bool dc_edge_has_new_data(DcEdgeHandle edge);

// for setting up initial context
DcEdgeVarIndex dc_edge_add_tx_var(DcEdgeHandle edge, const char *command);
DcEdgeVarIndex dc_edge_add_rx_var(DcEdgeHandle edge, const char *command);

// variable set/get functions
void dc_edge_set_tx_var(DcEdgeHandle edge, DcEdgeVarIndex var, const char *value);
void dc_edge_get_rx_var_value(DcEdgeHandle edge, DcEdgeVarIndex var_index, char *out);

#ifdef __cplusplus
}
#endif

#endif
