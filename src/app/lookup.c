#include "lookup.h"

#include "value.h"
#include "../utils/string.h"
#include "../utils/stb_sb.h"
#include "../utils/log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct _DcAppLookupVar {
    DcAppValIndex value_index;
    DcValue      *sb_value_stack; // per-variable stack for push/pop
    uint64_t      write_sequence;
} _DcAppLookupVar;

struct DcAppLookup {
    // warning suppression
    bool suppress_missing_variable;
    bool sealed;

    // vars
    char            *sb_var_names;
    int             *sb_var_name_offsets;
    _DcAppLookupVar *sb_vars;

    // values
    DcValue *sb_vals;
};

static _DcAppLookupVar *_get_var(DcAppLookup *lookup, DcAppVarIndex index) {
    if (index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        DC_LOG_ERROR("Lookup", "dc_app_lookup_get_var_value_index(): attempting to fetch invalid index %d", index);
        return NULL;
    }
    return &lookup->sb_vars[index];
}

// create an app lookup
DcAppLookup *dc_app_lookup_create(void) {
    DcAppLookup *lookup = (DcAppLookup *)malloc(sizeof(DcAppLookup));
    *lookup             = (DcAppLookup){0};

    // reserve index 0 as undefined for values and variables
    sbresize(lookup->sb_vals, 1);
    sbresize(lookup->sb_vars, 1);
    sbresize(lookup->sb_var_name_offsets, 1);
    sbresize(lookup->sb_var_names, 1);
    return lookup;
}

void dc_app_lookup_destroy(DcAppLookup *lookup) {
    sbfree(lookup->sb_var_names);
    sbfree(lookup->sb_var_name_offsets);

    // free per-variable stacks
    for (int ii = DC_APP_LOOKUP_FIRST_INDEX; ii < sbcount(lookup->sb_vars); ii++) {
        sbfree(lookup->sb_vars[ii].sb_value_stack);
    }
    sbfree(lookup->sb_vars);

    sbfree(lookup->sb_vals);
    free(lookup);
}

DcValue *dc_app_lookup_get_value(DcAppLookup *lookup, DcAppValIndex index) {
    if (index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        // DC_LOG_ERROR("Lookup", "dc_app_lookup_get_value(): attempting to fetch invalid index %d", index);
        return NULL;
    }
    return &(lookup->sb_vals[index]);
}

DcAppValIndex dc_app_lookup_register_value(DcAppLookup *lookup, DcValue *value) {
    if (!lookup || lookup->sealed) {
        DC_LOG_ERROR("Lookup", "cannot register a value after the lookup is sealed");
        return DC_APP_VAL_INDEX_UNDEFINED;
    }
    sbpush(lookup->sb_vals, *value);
    return sbcount(lookup->sb_vals) - 1;
}

DcAppValIndex dc_app_lookup_register_value_from_string(DcAppLookup *lookup, DcValueType type, const char *text) {
    char text_cleaned[DC_VALUE_STRING_BUFFER_SIZE];
    dc_utils_trim_whitespace_copy(text, text_cleaned, sizeof(text_cleaned));

    // check for var
    if (strlen(text_cleaned) > 1 && text_cleaned[0] == '@') {
        DcAppValIndex value_index = dc_app_lookup_get_var_value_index_by_name(lookup, &(text_cleaned[1]));
        if (value_index != DC_APP_VAL_INDEX_UNDEFINED) {
            return value_index;
        }
        return DC_APP_VAL_INDEX_UNDEFINED;
    }

    // otherwise create new DcValue and return its index
    DcValue val = dc_value_create_typed_value_from_string(type, text);
    return dc_app_lookup_register_value(lookup, &val);
}

int dc_app_lookup_get_var_count(DcAppLookup *lookup) {
    return sbcount(lookup->sb_vars);
}

DcAppVarIndex dc_app_lookup_get_var_index(DcAppLookup *lookup, const char *name) {
    for (int ii = DC_APP_LOOKUP_FIRST_INDEX; ii < sbcount(lookup->sb_var_name_offsets); ii++) {
        const char *lookup_name = &(lookup->sb_var_names[lookup->sb_var_name_offsets[ii]]);
        if (strcmp(name, lookup_name) == 0) {
            return ii;
        }
    }
    return DC_APP_LOOKUP_INDEX_UNDEFINED;
}

DcAppValIndex dc_app_lookup_get_var_value_index(DcAppLookup *lookup, DcAppVarIndex index) {
    _DcAppLookupVar *var = _get_var(lookup, index);
    return var ? var->value_index : DC_APP_VAL_INDEX_UNDEFINED;
}

DcAppValIndex dc_app_lookup_get_var_value_index_by_name(DcAppLookup *lookup, const char *name) {
    DcAppVarIndex index = dc_app_lookup_get_var_index(lookup, name);
    if (index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        if (!lookup->suppress_missing_variable) {
            DC_LOG_WARN("Lookup", "dc_app_lookup_get_var_value_index_by_name(): attempting to fetch non-existant variable '%s'", name);
        }
        return DC_APP_VAL_INDEX_UNDEFINED;
    }
    return dc_app_lookup_get_var_value_index(lookup, index);
}

DcAppVarIndex dc_app_lookup_register_var(DcAppLookup *lookup, const char *name, DcAppValIndex value_index) {
    if (!lookup || lookup->sealed) {
        DC_LOG_ERROR("Lookup", "cannot register a variable after the lookup is sealed");
        return DC_APP_VAR_INDEX_UNDEFINED;
    }
    DcAppVarIndex index = dc_app_lookup_get_var_index(lookup, name);
    if (index != DC_APP_LOOKUP_INDEX_UNDEFINED) {
        DC_LOG_WARN("Lookup", "dc_app_lookup_register_var(): variable already exists '%s'", name);
        return index;
    }

    sbpush(lookup->sb_var_name_offsets, sbcount(lookup->sb_var_names));
    sbpushn(lookup->sb_var_names, name, (int)strlen(name));
    sbpush(lookup->sb_var_names, '\0');
    _DcAppLookupVar var = {
        .value_index = value_index,
    };
    sbpush(lookup->sb_vars, var);
    return sbcount(lookup->sb_vars) - 1;
}

const char *dc_app_lookup_get_var_name(DcAppLookup *lookup, DcAppVarIndex index) {
    if (index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        DC_LOG_ERROR("Lookup", "dc_app_lookup_get_var_name(): attempting to fetch invalid index %d", index);
        return NULL;
    }
    return &(lookup->sb_var_names[lookup->sb_var_name_offsets[index]]);
}

uint64_t dc_app_lookup_get_var_write_sequence(DcAppLookup *lookup, DcAppVarIndex index) {
    if (!lookup || index == DC_APP_LOOKUP_INDEX_UNDEFINED || index >= (DcAppVarIndex)sbcount(lookup->sb_vars)) {
        return 0;
    }
    return lookup->sb_vars[index].write_sequence;
}

void dc_app_lookup_mark_var_written(DcAppLookup *lookup, DcAppVarIndex index) {
    if (!lookup || index == DC_APP_LOOKUP_INDEX_UNDEFINED || index >= (DcAppVarIndex)sbcount(lookup->sb_vars)) {
        return;
    }
    lookup->sb_vars[index].write_sequence++;
}

void dc_app_lookup_set_var_to_string(DcAppLookup *lookup, DcAppVarIndex var_index, const char *new_string) {
    DcAppValIndex value_index = dc_app_lookup_get_var_value_index(lookup, var_index);
    DcValue      *val         = dc_app_lookup_get_value(lookup, value_index);
    dc_value_set_from_string(val, new_string);
}

void dc_app_lookup_var_push(DcAppLookup *lookup, DcAppVarIndex var_index) {
    _DcAppLookupVar *var = _get_var(lookup, var_index);
    if (!var) return;
    DcValue *value = dc_app_lookup_get_value(lookup, var->value_index);
    sbpush(var->sb_value_stack, *value);
}

void dc_app_lookup_var_pop(DcAppLookup *lookup, DcAppVarIndex var_index) {
    _DcAppLookupVar *var = _get_var(lookup, var_index);
    if (!var) return;
    if (sbcount(var->sb_value_stack) > 0) {
        DcValue *value = dc_app_lookup_get_value(lookup, var->value_index);
        *value         = sbpop(var->sb_value_stack);
    } else {
        DC_LOG_ERROR("Lookup", "dc_app_lookup_var_pop(): stack underflow for variable index %d", var_index);
    }
}

void dc_app_lookup_reset_var_stacks(DcAppLookup *lookup) {
    for (int ii = DC_APP_LOOKUP_FIRST_INDEX; ii < sbcount(lookup->sb_vars); ii++) {
        _DcAppLookupVar *var = &lookup->sb_vars[ii];
        if (sbcount(var->sb_value_stack) > 0) {
            // restore original value (bottom of stack) and clear
            DcValue *value = &lookup->sb_vals[var->value_index];
            *value         = var->sb_value_stack[0];
            sbclear(var->sb_value_stack);
        }
    }
}

void dc_app_lookup_set_suppress_missing_variable(DcAppLookup *lookup, bool suppress) {
    lookup->suppress_missing_variable = suppress;
}

void dc_app_lookup_seal(DcAppLookup *lookup) {
    if (lookup) lookup->sealed = true;
}
