#ifndef _DC_TRICK_
#define _DC_TRICK_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    DC_TRICK_RESULT_SUCCESS = 0,
    DC_TRICK_RESULT_FAIL
} DcTrickResult;

typedef struct _DcTrickHandle {
    uint8_t index;
} DcTrickHandle;

typedef uint32_t DcTrickVarIndex;

#ifdef __cplusplus
extern "C" {
#endif

// initialization
void dc_trick_init(void);

// lifecycle
DcTrickHandle dc_trick_create(const char *host, int port, float data_rate, int timeout_s);
void          dc_trick_cleanup(DcTrickHandle trick);
void          dc_trick_update(DcTrickHandle trick);

// state checks
bool dc_trick_is_connected(DcTrickHandle trick);
bool dc_trick_has_new_data(DcTrickHandle trick);

// for setting up initial context
DcTrickVarIndex dc_trick_add_tx_var(DcTrickHandle trick, const char *path, const char *units, bool is_string);
DcTrickVarIndex dc_trick_add_rx_var(DcTrickHandle trick, const char *path, const char *units);
DcTrickVarIndex dc_trick_add_rx_oad_var(DcTrickHandle trick, const char *path, const char *units);

// variable set/get functions
void dc_trick_set_tx_var(DcTrickHandle trick, DcTrickVarIndex var, const char *value);
void dc_trick_get_rx_var_value(DcTrickHandle trick, DcTrickVarIndex var_index, char *out);
void dc_trick_get_rx_oad_value(DcTrickHandle trick, DcTrickVarIndex oad_index, char *out);

#ifdef __cplusplus
}
#endif

#endif
