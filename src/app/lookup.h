#ifndef DC_APP_LOOKUP_H
#define DC_APP_LOOKUP_H

#include "lookup_types.h"
#include "value_types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct DcAppLookup DcAppLookup;
struct DcValue;

#define DC_APP_LOOKUP_INDEX_UNDEFINED (0)
#define DC_APP_LOOKUP_FIRST_INDEX (1)

static const DcAppVarIndex DC_APP_VAR_INDEX_UNDEFINED   = DC_APP_LOOKUP_INDEX_UNDEFINED;
static const DcAppValIndex DC_APP_VAL_INDEX_UNDEFINED   = DC_APP_LOOKUP_INDEX_UNDEFINED;

#ifdef __cplusplus
extern "C" {
#endif

// lookup functions
DcAppLookup *dc_app_lookup_create(void);
void         dc_app_lookup_destroy(DcAppLookup *lookup);
// Prevents further registration so published value pointers remain stable.
void         dc_app_lookup_seal(DcAppLookup *lookup);

// The pointer may move during registration but remains stable after sealing.
struct DcValue *dc_app_lookup_get_value(DcAppLookup *lookup, DcAppValIndex index);
DcAppValIndex dc_app_lookup_register_value(DcAppLookup *lookup, struct DcValue *value);
DcAppValIndex dc_app_lookup_register_value_from_string(DcAppLookup *lookup, DcValueType type, const char *text);

int           dc_app_lookup_get_var_count(DcAppLookup *lookup);
DcAppVarIndex dc_app_lookup_get_var_index(DcAppLookup *lookup, const char *name);
DcAppValIndex dc_app_lookup_get_var_value_index(DcAppLookup *lookup, DcAppVarIndex index);
DcAppValIndex dc_app_lookup_get_var_value_index_by_name(DcAppLookup *lookup, const char *name);
DcAppVarIndex dc_app_lookup_register_var(DcAppLookup *lookup, const char *name, DcAppValIndex value_index);
const char   *dc_app_lookup_get_var_name(DcAppLookup *lookup, DcAppVarIndex index);
uint64_t      dc_app_lookup_get_var_write_sequence(DcAppLookup *lookup, DcAppVarIndex index);
void          dc_app_lookup_mark_var_written(DcAppLookup *lookup, DcAppVarIndex index);

void dc_app_lookup_set_var_to_string(DcAppLookup *lookup, DcAppVarIndex var_index, const char *value);

// push/pop operations for per-variable stacks
void dc_app_lookup_var_push(DcAppLookup *lookup, DcAppVarIndex var_index);
void dc_app_lookup_var_pop(DcAppLookup *lookup, DcAppVarIndex var_index);
void dc_app_lookup_reset_var_stacks(DcAppLookup *lookup);

void dc_app_lookup_set_suppress_missing_variable(DcAppLookup *lookup, bool suppress);

#ifdef __cplusplus
}
#endif

#endif
