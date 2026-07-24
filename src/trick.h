#ifndef _DC_TRICK_
#define _DC_TRICK_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DC_TRICK_RESULT_SUCCESS = 0,
    DC_TRICK_RESULT_FAIL
} DcTrickResult;

typedef struct DcTrick DcTrick;

typedef uint32_t DcTrickVarIndex;

#ifdef __cplusplus
extern "C" {
#endif

// initialization
void dc_trick_init(void);

// lifecycle
DcTrick *dc_trick_create(const char *host, int port, float data_rate, int timeout_s);
void     dc_trick_cleanup(DcTrick *trick);
void     dc_trick_update(DcTrick *trick);

// state checks
bool dc_trick_is_connected(DcTrick *trick);
bool dc_trick_has_new_data(DcTrick *trick);

// variable setup
DcTrickVarIndex dc_trick_add_tx_var(DcTrick *trick, const char *path, const char *units, bool is_string);
DcTrickVarIndex dc_trick_add_rx_var(DcTrick *trick, const char *path, const char *units);
DcTrickVarIndex dc_trick_add_rx_oad_var(DcTrick *trick, const char *path, const char *units);

// variable set/get functions
void dc_trick_set_tx_var(DcTrick *trick, DcTrickVarIndex var, const char *value);
void dc_trick_get_rx_var_value(DcTrick *trick, DcTrickVarIndex var_index, char *out);
void dc_trick_get_rx_oad_value(DcTrick *trick, DcTrickVarIndex oad_index, char *out);

#ifdef __cplusplus
}
#endif

#endif
