#ifndef _DC_APP_LOOKUP_
#define _DC_APP_LOOKUP_

#include "../value.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// lookup types
typedef int                   DcAppLookupIndex;
static const DcAppLookupIndex DC_APP_LOOKUP_INDEX_UNDEFINED = -1;

typedef DcAppLookupIndex   DcAppVarIndex;
typedef DcAppLookupIndex   DcAppValIndex;
typedef DcAppLookupIndex   DcAppStyleIndex;
static const DcAppVarIndex DC_APP_VAR_INDEX_UNDEFINED   = DC_APP_LOOKUP_INDEX_UNDEFINED;
static const DcAppValIndex DC_APP_VAL_INDEX_UNDEFINED   = DC_APP_LOOKUP_INDEX_UNDEFINED;
static const DcAppValIndex DC_APP_STYLE_INDEX_UNDEFINED = DC_APP_LOOKUP_INDEX_UNDEFINED;

typedef struct _DcAppLookupVar {
    void         *extern_data;
    DcAppValIndex value_index;
} DcAppLookupVar;

typedef struct _DcAppLookup {
    DcAppLookupIndex index;
} DcAppLookup;

#ifdef __cplusplus
extern "C" {
#endif

// lookup functions
DcAppLookup *dc_app_lookup_create(void);
void         dc_app_lookup_cleanup(DcAppLookup *lookup);

DcValueType dc_app_value_type_from_string(const char *type_str);

DcValue      *dc_app_lookup_get_value(DcAppLookup *lookup, DcAppValIndex index); // don't store
DcAppValIndex dc_app_lookup_register_value(DcAppLookup *lookup, DcValue *value);
DcAppValIndex dc_app_create_and_register_typed_value_from_string(DcAppLookup *lookup, DcValueType type, const char *text);

int             dc_app_lookup_get_var_count(DcAppLookup *lookup);
DcAppVarIndex   dc_app_lookup_get_var_index(DcAppLookup *lookup, const char *name);
DcAppLookupVar *dc_app_lookup_get_var(DcAppLookup *lookup, DcAppVarIndex index);      // don't store
DcAppLookupVar *dc_app_lookup_get_var_by_name(DcAppLookup *lookup, const char *name); // don't store
DcAppVarIndex   dc_app_lookup_register_var(DcAppLookup *lookup, const char *name, DcAppLookupVar *var);
const char     *dc_app_lookup_get_var_name(DcAppLookup *lookup, DcAppVarIndex index);

void dc_app_lookup_set_var_to_string(DcAppLookup *lookup, DcAppVarIndex var_index, const char *value);
void dc_app_lookup_refresh_var_from_extern(DcAppLookup *lookup, DcAppVarIndex var_index);
void dc_app_lookup_refresh_var_from_value(DcAppLookup *lookup, DcAppVarIndex var_index);

#ifdef __cplusplus
}
#endif

#endif
