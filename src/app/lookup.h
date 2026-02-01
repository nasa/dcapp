#ifndef _DC_APP_LOOKUP_
#define _DC_APP_LOOKUP_

#include "../value.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// lookup types
typedef int DcAppLookupIndex;
#define DC_APP_LOOKUP_INDEX_UNDEFINED (-1)

typedef DcAppLookupIndex   DcAppVarIndex;
typedef DcAppLookupIndex   DcAppValIndex;
typedef DcAppLookupIndex   DcAppStyleIndex;
static const DcAppVarIndex DC_APP_VAR_INDEX_UNDEFINED   = DC_APP_LOOKUP_INDEX_UNDEFINED;
static const DcAppValIndex DC_APP_VAL_INDEX_UNDEFINED   = DC_APP_LOOKUP_INDEX_UNDEFINED;
static const DcAppValIndex DC_APP_STYLE_INDEX_UNDEFINED = DC_APP_LOOKUP_INDEX_UNDEFINED;

typedef struct _DcAppLookupVar {
    DcAppValIndex  value_index;
    DcValue       *sb_value_stack; // per-variable stack for push/pop
} DcAppLookupVar;

typedef struct _DcAppLookup {
    DcAppLookupIndex index;
} DcAppLookup;

// Warning suppression flags (used by SuppressWarnings attribute on DCAPP element)
typedef enum _DcAppSuppressWarning {
    DC_APP_SUPPRESS_NONE             = 0,
    DC_APP_SUPPRESS_MISSING_CONSTANT = 1 << 0,
    DC_APP_SUPPRESS_MISSING_VARIABLE = 1 << 1,
    DC_APP_SUPPRESS_MISSING_STYLE    = 1 << 2,
    DC_APP_SUPPRESS_STYLE_OVERRIDE   = 1 << 3,
} DcAppSuppressWarning;

#ifdef __cplusplus
extern "C" {
#endif

// lookup functions
DcAppLookup *dc_app_lookup_create(void);
void         dc_app_lookup_cleanup(DcAppLookup *lookup);

DcValue      *dc_app_lookup_get_value(DcAppLookup *lookup, DcAppValIndex index); // don't store
DcAppValIndex dc_app_lookup_register_value(DcAppLookup *lookup, DcValue *value);
DcAppValIndex dc_app_create_and_register_typed_value_from_string(DcAppLookup *lookup, DcValueType type, const char *text);

int             dc_app_lookup_get_var_count(DcAppLookup *lookup);
DcAppVarIndex   dc_app_lookup_get_var_index(DcAppLookup *lookup, const char *name);
DcAppLookupVar *dc_app_lookup_get_var(DcAppLookup *lookup, DcAppVarIndex index);      // don't store
DcAppLookupVar *dc_app_lookup_get_var_by_name(DcAppLookup *lookup, const char *name); // don't store
DcAppVarIndex   dc_app_lookup_register_var(DcAppLookup *lookup, const char *name, DcAppLookupVar *var);
const char     *dc_app_lookup_get_var_name(DcAppLookup *lookup, DcAppVarIndex index);
DcAppVarIndex   dc_app_lookup_register_anon_var(DcAppLookup *lookup, DcValueType type, const char *initial_value_str);

void dc_app_lookup_set_var_to_string(DcAppLookup *lookup, DcAppVarIndex var_index, const char *value);

// push/pop operations for per-variable stacks
void dc_app_lookup_var_push(DcAppLookup *lookup, DcAppVarIndex var_index);
void dc_app_lookup_var_pop(DcAppLookup *lookup, DcAppVarIndex var_index);
void dc_app_lookup_reset_var_stacks(DcAppLookup *lookup);

void dc_app_lookup_set_suppress_warnings(DcAppLookup *lookup, unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif
