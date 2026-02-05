#include "lookup.h"

#include "../utils/string.h"
#include "../utils/stb_sb.h"
#include "../utils/log.h"

#include <string.h>
#include <stdio.h>

typedef struct __LookupContext {

    // warning suppression
    unsigned int suppress_warnings;

    // vars
    char           *sb_var_names;
    int            *sb_var_name_offsets;
    DcAppLookupVar *sb_vars;

    // values
    DcValue *sb_vals;

} _LookupContext;

static _LookupContext *_sb_contexts;

// create an app lookup
DcAppLookup *dc_app_lookup_create() {
    _LookupContext context = {0};
    sbpush(_sb_contexts, context);

    DcAppLookup *lookup = (DcAppLookup *)malloc(sizeof(DcAppLookup));
    lookup->index       = sbcount(_sb_contexts) - 1;
    return lookup;
}

void dc_app_lookup_cleanup(DcAppLookup *lookup) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    sbfree(context->sb_var_names);
    sbfree(context->sb_var_name_offsets);

    // free per-variable stacks
    for (int ii = 0; ii < sbcount(context->sb_vars); ii++) {
        sbfree(context->sb_vars[ii].sb_value_stack);
    }
    sbfree(context->sb_vars);

    sbfree(context->sb_vals);
    free(lookup);
}

DcValue *dc_app_lookup_get_value(DcAppLookup *lookup, DcAppValIndex index) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    if (index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        DC_LOG_ERROR("Lookup", "dc_app_lookup_get_value(): attempting to fetch invalid index %d", index);
        return NULL;
    }
    return &(context->sb_vals[index]);
}

DcAppValIndex dc_app_lookup_register_value(DcAppLookup *lookup, DcValue *value) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    sbpush(context->sb_vals, *value);
    return sbcount(context->sb_vals) - 1;
}

DcAppValIndex dc_app_create_and_register_typed_value_from_string(DcAppLookup *lookup, DcValueType type, const char *text) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);

    char text_cleaned[DC_VALUE_STRING_BUFFER_SIZE];
    dc_utils_trim_whitespace_copy(text, text_cleaned, sizeof(text_cleaned));

    // check for var
    if (strlen(text_cleaned) > 1 && text_cleaned[0] == '@') {
        DcAppLookupVar *var = dc_app_lookup_get_var_by_name(lookup, &(text_cleaned[1]));
        if (var) {
            return var->value_index;
        }
        return DC_APP_VAL_INDEX_UNDEFINED;
    }

    // otherwise create new DcValue and return its index
    DcValue val = dc_value_create_typed_value_from_string(type, text);
    return dc_app_lookup_register_value(lookup, &val);
}

int dc_app_lookup_get_var_count(DcAppLookup *lookup) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    return sbcount(context->sb_vars);
}

DcAppVarIndex dc_app_lookup_get_var_index(DcAppLookup *lookup, const char *name) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    for (int ii = 0; ii < sbcount(context->sb_var_name_offsets); ii++) {
        const char *lookup_name = &(context->sb_var_names[context->sb_var_name_offsets[ii]]);
        if (strcmp(name, lookup_name) == 0) {
            return ii;
        }
    }
    return DC_APP_LOOKUP_INDEX_UNDEFINED;
}

DcAppLookupVar *dc_app_lookup_get_var(DcAppLookup *lookup, DcAppVarIndex index) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    if (index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        DC_LOG_ERROR("Lookup", "dc_app_lookup_get_var(): attempting to fetch invalid index %d", index);
        return NULL;
    }
    return &(context->sb_vars[index]);
}

DcAppLookupVar *dc_app_lookup_get_var_by_name(DcAppLookup *lookup, const char *name) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    DcAppVarIndex   index   = dc_app_lookup_get_var_index(lookup, name);
    if (index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        if (!(context->suppress_warnings & DC_APP_SUPPRESS_MISSING_VARIABLE)) {
            DC_LOG_WARN("Lookup", "dc_app_lookup_get_var_by_name(): attempting to fetch non-existant variable '%s'", name);
        }
        return NULL;
    }
    return dc_app_lookup_get_var(lookup, index);
}

DcAppVarIndex dc_app_lookup_register_var(DcAppLookup *lookup, const char *name, DcAppLookupVar *var) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);

    DcAppVarIndex index = dc_app_lookup_get_var_index(lookup, name);
    if (index != DC_APP_LOOKUP_INDEX_UNDEFINED) {
        DC_LOG_WARN("Lookup", "dc_app_lookup_register_var(): variable already exists '%s'", name);
        return index;
    }

    sbpush(context->sb_var_name_offsets, sbcount(context->sb_var_names));
    sbpushn(context->sb_var_names, name, (int)strlen(name));
    sbpush(context->sb_var_names, '\0');
    sbpush(context->sb_vars, *var);
    return sbcount(context->sb_vars) - 1;
}

const char *dc_app_lookup_get_var_name(DcAppLookup *lookup, DcAppVarIndex index) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    return &(context->sb_var_names[context->sb_var_name_offsets[index]]);
}

void dc_app_lookup_set_var_to_string(DcAppLookup *lookup, DcAppVarIndex var_index, const char *new_string) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    DcAppLookupVar *var     = dc_app_lookup_get_var(lookup, var_index);
    DcValue        *val     = dc_app_lookup_get_value(lookup, var->value_index);
    dc_value_set_from_string(val, new_string);
}

void dc_app_lookup_var_push(DcAppLookup *lookup, DcAppVarIndex var_index) {
    DcAppLookupVar *var = dc_app_lookup_get_var(lookup, var_index);
    if (!var) return;
    DcValue *value = dc_app_lookup_get_value(lookup, var->value_index);
    sbpush(var->sb_value_stack, *value);
}

void dc_app_lookup_var_pop(DcAppLookup *lookup, DcAppVarIndex var_index) {
    DcAppLookupVar *var = dc_app_lookup_get_var(lookup, var_index);
    if (!var) return;
    if (sbcount(var->sb_value_stack) > 0) {
        DcValue *value = dc_app_lookup_get_value(lookup, var->value_index);
        *value = sbpop(var->sb_value_stack);
    } else {
        DC_LOG_ERROR("Lookup", "dc_app_lookup_var_pop(): stack underflow for variable index %d", var_index);
    }
}

void dc_app_lookup_reset_var_stacks(DcAppLookup *lookup) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    for (int ii = 0; ii < sbcount(context->sb_vars); ii++) {
        DcAppLookupVar *var = &context->sb_vars[ii];
        if (sbcount(var->sb_value_stack) > 0) {
            // restore original value (bottom of stack) and clear
            DcValue *value = &context->sb_vals[var->value_index];
            *value = var->sb_value_stack[0];
            sbclear(var->sb_value_stack);
        }
    }
}

void dc_app_lookup_set_suppress_warnings(DcAppLookup *lookup, unsigned int flags) {
    _LookupContext *context    = &(_sb_contexts[lookup->index]);
    context->suppress_warnings = flags;
}
