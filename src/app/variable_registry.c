#include "variable_registry.h"

#include "value.h"
#include "../utils/string.h"
#include "../utils/stb_sb.h"
#include "../utils/log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//~ internal types

typedef struct _DcAppVariableRegistryVariable {
    DcAppVariableRegistryValueIndex value_index;
    DcAppValue *sb_value_stack; // per-variable stack for push/pop
    uint64_t write_sequence;
} _DcAppVariableRegistryVariable;

struct DcAppVariableRegistryContext {
    //- registry state
    bool suppress_missing_variable;
    bool sealed;

    //- variables
    char *sb_var_names;
    int *sb_var_name_offsets;
    _DcAppVariableRegistryVariable *sb_vars;

    //- values
    DcAppValue *sb_vals;
};

//~ forward declarations

static _DcAppVariableRegistryVariable *_get_var(DcAppVariableRegistryContext *lookup, DcAppVariableRegistryVariableIndex index);

//~ registry lifecycle

DcAppVariableRegistryContext *dc_app_variable_registry_context_create(void) {
    DcAppVariableRegistryContext *lookup = (DcAppVariableRegistryContext *)malloc(sizeof(DcAppVariableRegistryContext));
    *lookup = (DcAppVariableRegistryContext){0};

    // reserve index zero for undefined values and variables
    sbresize(lookup->sb_vals, 1);
    sbresize(lookup->sb_vars, 1);
    sbresize(lookup->sb_var_name_offsets, 1);
    sbresize(lookup->sb_var_names, 1);
    return lookup;
}

void dc_app_variable_registry_context_destroy(DcAppVariableRegistryContext *lookup) {
    sbfree(lookup->sb_var_names);
    sbfree(lookup->sb_var_name_offsets);

    // release stacks owned by each variable
    for (int ii = DC_APP_VARIABLE_REGISTRY_FIRST_INDEX; ii < sbcount(lookup->sb_vars); ii++) {
        sbfree(lookup->sb_vars[ii].sb_value_stack);
    }
    sbfree(lookup->sb_vars);

    sbfree(lookup->sb_vals);
    free(lookup);
}

//~ value registry

DcAppValue *dc_app_variable_registry_get_value(DcAppVariableRegistryContext *lookup, DcAppVariableRegistryValueIndex index) {
    if (index == DC_APP_VARIABLE_REGISTRY_INDEX_UNDEFINED) {
        return NULL;
    }
    return &(lookup->sb_vals[index]);
}

DcAppVariableRegistryValueIndex dc_app_variable_registry_register_value(DcAppVariableRegistryContext *lookup, DcAppValue *value) {
    if (!lookup || lookup->sealed) {
        DC_LOG_ERROR("Lookup", "cannot register a value after the lookup is sealed");
        return DC_APP_VARIABLE_REGISTRY_VALUE_INDEX_UNDEFINED;
    }
    sbpush(lookup->sb_vals, *value);
    return sbcount(lookup->sb_vals) - 1;
}

DcAppVariableRegistryValueIndex dc_app_variable_registry_register_value_from_string(DcAppVariableRegistryContext *lookup, DcAppValueType type, const char *text) {
    char text_cleaned[DC_APP_VALUE_STRING_BUFFER_SIZE];
    dc_utils_trim_whitespace_copy(text, text_cleaned, sizeof(text_cleaned));

    // resolve variable references before creating literals
    if (strlen(text_cleaned) > 1 && text_cleaned[0] == '@') {
        DcAppVariableRegistryValueIndex value_index = dc_app_variable_registry_get_variable_value_index_by_name(lookup, &(text_cleaned[1]));
        if (value_index != DC_APP_VARIABLE_REGISTRY_VALUE_INDEX_UNDEFINED) {
            return value_index;
        }
        return DC_APP_VARIABLE_REGISTRY_VALUE_INDEX_UNDEFINED;
    }

    // register literal values directly
    DcAppValue val = dc_app_value_create_typed_value_from_string(type, text);
    return dc_app_variable_registry_register_value(lookup, &val);
}

//~ variable registry

int dc_app_variable_registry_get_variable_count(DcAppVariableRegistryContext *lookup) {
    return sbcount(lookup->sb_vars);
}

DcAppVariableRegistryVariableIndex dc_app_variable_registry_get_variable_index(DcAppVariableRegistryContext *lookup, const char *name) {
    for (int ii = DC_APP_VARIABLE_REGISTRY_FIRST_INDEX; ii < sbcount(lookup->sb_var_name_offsets); ii++) {
        const char *lookup_name = &(lookup->sb_var_names[lookup->sb_var_name_offsets[ii]]);
        if (strcmp(name, lookup_name) == 0) {
            return ii;
        }
    }
    return DC_APP_VARIABLE_REGISTRY_INDEX_UNDEFINED;
}

DcAppVariableRegistryValueIndex dc_app_variable_registry_get_variable_value_index(DcAppVariableRegistryContext *lookup, DcAppVariableRegistryVariableIndex index) {
    _DcAppVariableRegistryVariable *var = _get_var(lookup, index);
    return var ? var->value_index : DC_APP_VARIABLE_REGISTRY_VALUE_INDEX_UNDEFINED;
}

DcAppVariableRegistryValueIndex dc_app_variable_registry_get_variable_value_index_by_name(DcAppVariableRegistryContext *lookup, const char *name) {
    DcAppVariableRegistryVariableIndex index = dc_app_variable_registry_get_variable_index(lookup, name);
    if (index == DC_APP_VARIABLE_REGISTRY_INDEX_UNDEFINED) {
        if (!lookup->suppress_missing_variable) {
            DC_LOG_WARN("Lookup", "dc_app_variable_registry_get_variable_value_index_by_name(): attempting to fetch non-existant variable '%s'", name);
        }
        return DC_APP_VARIABLE_REGISTRY_VALUE_INDEX_UNDEFINED;
    }
    return dc_app_variable_registry_get_variable_value_index(lookup, index);
}

DcAppVariableRegistryVariableIndex dc_app_variable_registry_register_variable(DcAppVariableRegistryContext *lookup, const char *name, DcAppVariableRegistryValueIndex value_index) {
    if (!lookup || lookup->sealed) {
        DC_LOG_ERROR("Lookup", "cannot register a variable after the lookup is sealed");
        return DC_APP_VARIABLE_REGISTRY_VARIABLE_INDEX_UNDEFINED;
    }
    DcAppVariableRegistryVariableIndex index = dc_app_variable_registry_get_variable_index(lookup, name);
    if (index != DC_APP_VARIABLE_REGISTRY_INDEX_UNDEFINED) {
        DC_LOG_WARN("Lookup", "dc_app_variable_registry_register_variable(): variable already exists '%s'", name);
        return index;
    }

    sbpush(lookup->sb_var_name_offsets, sbcount(lookup->sb_var_names));
    sbpushn(lookup->sb_var_names, name, (int)strlen(name));
    sbpush(lookup->sb_var_names, '\0');
    _DcAppVariableRegistryVariable var = {
        .value_index = value_index,
    };
    sbpush(lookup->sb_vars, var);
    return sbcount(lookup->sb_vars) - 1;
}

const char *dc_app_variable_registry_get_variable_name(DcAppVariableRegistryContext *lookup, DcAppVariableRegistryVariableIndex index) {
    if (index == DC_APP_VARIABLE_REGISTRY_INDEX_UNDEFINED) {
        DC_LOG_ERROR("Lookup", "dc_app_variable_registry_get_variable_name(): attempting to fetch invalid index %d", index);
        return NULL;
    }
    return &(lookup->sb_var_names[lookup->sb_var_name_offsets[index]]);
}

uint64_t dc_app_variable_registry_get_variable_write_sequence(DcAppVariableRegistryContext *lookup, DcAppVariableRegistryVariableIndex index) {
    if (!lookup || index == DC_APP_VARIABLE_REGISTRY_INDEX_UNDEFINED || index >= (DcAppVariableRegistryVariableIndex)sbcount(lookup->sb_vars)) {
        return 0;
    }
    return lookup->sb_vars[index].write_sequence;
}

void dc_app_variable_registry_mark_variable_written(DcAppVariableRegistryContext *lookup, DcAppVariableRegistryVariableIndex index) {
    if (!lookup || index == DC_APP_VARIABLE_REGISTRY_INDEX_UNDEFINED || index >= (DcAppVariableRegistryVariableIndex)sbcount(lookup->sb_vars)) {
        return;
    }
    lookup->sb_vars[index].write_sequence++;
}

void dc_app_variable_registry_set_variable_to_string(DcAppVariableRegistryContext *lookup, DcAppVariableRegistryVariableIndex var_index, const char *new_string) {
    DcAppVariableRegistryValueIndex value_index = dc_app_variable_registry_get_variable_value_index(lookup, var_index);
    DcAppValue *val = dc_app_variable_registry_get_value(lookup, value_index);
    dc_app_value_set_from_string(val, new_string);
}

//~ variable stacks

void dc_app_variable_registry_variable_push(DcAppVariableRegistryContext *lookup, DcAppVariableRegistryVariableIndex var_index) {
    _DcAppVariableRegistryVariable *var = _get_var(lookup, var_index);
    if (!var) return;
    DcAppValue *value = dc_app_variable_registry_get_value(lookup, var->value_index);
    sbpush(var->sb_value_stack, *value);
}

void dc_app_variable_registry_variable_pop(DcAppVariableRegistryContext *lookup, DcAppVariableRegistryVariableIndex var_index) {
    _DcAppVariableRegistryVariable *var = _get_var(lookup, var_index);
    if (!var) return;
    if (sbcount(var->sb_value_stack) > 0) {
        DcAppValue *value = dc_app_variable_registry_get_value(lookup, var->value_index);
        *value = sbpop(var->sb_value_stack);
    } else {
        DC_LOG_ERROR("Lookup", "dc_app_variable_registry_variable_pop(): stack underflow for variable index %d", var_index);
    }
}

void dc_app_variable_registry_reset_variable_stacks(DcAppVariableRegistryContext *lookup) {
    for (int ii = DC_APP_VARIABLE_REGISTRY_FIRST_INDEX; ii < sbcount(lookup->sb_vars); ii++) {
        _DcAppVariableRegistryVariable *var = &lookup->sb_vars[ii];
        if (sbcount(var->sb_value_stack) > 0) {
            // restore the original value before clearing the stack
            DcAppValue *value = &lookup->sb_vals[var->value_index];
            *value = var->sb_value_stack[0];
            sbclear(var->sb_value_stack);
        }
    }
}

//~ registry controls

void dc_app_variable_registry_set_suppress_missing_variable(DcAppVariableRegistryContext *lookup, bool suppress) {
    lookup->suppress_missing_variable = suppress;
}

void dc_app_variable_registry_seal(DcAppVariableRegistryContext *lookup) {
    if (lookup) lookup->sealed = true;
}

//~ internal helpers

static _DcAppVariableRegistryVariable *_get_var(DcAppVariableRegistryContext *lookup, DcAppVariableRegistryVariableIndex index) {
    if (index == DC_APP_VARIABLE_REGISTRY_INDEX_UNDEFINED) {
        DC_LOG_ERROR("Lookup", "dc_app_variable_registry_get_variable_value_index(): attempting to fetch invalid index %d", index);
        return NULL;
    }
    return &lookup->sb_vars[index];
}
