#ifndef _DC_TRICK_
#define _DC_TRICK_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    DC_TRICK_RESULT_SUCCESS = 0,
    DC_TRICK_RESULT_FAILß
} DcTrickResult;

typedef uint8_t DcTrickIndex;

typedef struct _DcTrick {
    DcTrickIndex _index;
    bool         has_new_data;
} DcTrick;

#ifdef __cplusplus
extern "C" {
#endif

// main "draw" functions
DcTrick dc_trick_create(char *host, int port, float data_rate, int timeout_ms);
void    dc_trick_cleanup(DcTrick *trick);
void    dc_trick_update(DcTrick *trick);

// for setting up initial context
DcTrickIndex dc_trick_add_tx_var(DcTrick *trick, const char *path, const char *units, bool is_string);
DcTrickIndex dc_trick_add_rx_var(DcTrick *trick, const char *path, const char *units);
DcTrickIndex dc_trick_add_rx_oad_var(DcTrick *trick, const char *path, const char *units);

// variable set/get functions
DcTrickResult dc_trick_set_tx_var(DcTrick *trick, DcTrickIndex var, const char *value);
DcTrickResult dc_trick_get_rx_var_value(DcTrick *trick, DcTrickIndex var_index, char *out);
DcTrickResult dc_trick_get_rx_oad_value(DcTrick *trick, DcTrickIndex oad_index, char *out);

#ifdef __cplusplus
}
#endif

#endif
