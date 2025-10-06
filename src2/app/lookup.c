#include "lookup.h"

#include "../utils/string.h"
#include "../utils/stb_sb.h"
#include "enums.h"

#include <stdio.h>
#include <string.h>

typedef struct __LookupContext {

    // constants
    char  *sb_const_names;
    int   *sb_const_name_offsets;
    char **sb_const_vals;

    // vars
    char           *sb_var_names;
    int            *sb_var_name_offsets;
    DcAppLookupVar *sb_vars;

    // values
    DcValue *sb_vals;

} _LookupContext;

static _LookupContext *_sb_contexts;

static DcAppLookupIndex _lookup_get_const_index(_LookupContext *context, const char *name);
static void             _lookup_set_const(_LookupContext *context, DcAppLookupIndex index, const char *new_value);
static void             _lookup_add_const(_LookupContext *context, const char *name, const char *value);
static void             _lookup_add_const_int(_LookupContext *context, const char *name, int);

// create an app lookup
DcAppLookup *dc_app_lookup_create() {
    _LookupContext context = {0};

    // set initial values
    _lookup_add_const_int(&context, "_align_left_", DC_APP_ALIGN_TYPE_LEFT);
    _lookup_add_const_int(&context, "_align_center_", DC_APP_ALIGN_TYPE_CENTER);
    _lookup_add_const_int(&context, "_align_right_", DC_APP_ALIGN_TYPE_RIGHT);
    _lookup_add_const_int(&context, "_align_bottom_", DC_APP_ALIGN_TYPE_BOTTOM);
    _lookup_add_const_int(&context, "_align_middle_", DC_APP_ALIGN_TYPE_MIDDLE);
    _lookup_add_const_int(&context, "_align_top_", DC_APP_ALIGN_TYPE_TOP);

    _lookup_add_const_int(&context, "_conditional_true_", DC_APP_CONDITIONAL_TYPE_TRUE);
    _lookup_add_const_int(&context, "_conditional_false_", DC_APP_CONDITIONAL_TYPE_FALSE);
    _lookup_add_const_int(&context, "_conditional_eq_", DC_APP_CONDITIONAL_TYPE_EQ);
    _lookup_add_const_int(&context, "_conditional_ne_", DC_APP_CONDITIONAL_TYPE_NE);
    _lookup_add_const_int(&context, "_conditional_lt_", DC_APP_CONDITIONAL_TYPE_LT);
    _lookup_add_const_int(&context, "_conditional_gt_", DC_APP_CONDITIONAL_TYPE_GT);
    _lookup_add_const_int(&context, "_conditional_lte_", DC_APP_CONDITIONAL_TYPE_LTE);
    _lookup_add_const_int(&context, "_conditional_gte_", DC_APP_CONDITIONAL_TYPE_GTE);

    _lookup_add_const_int(&context, "_set_equal_", DC_APP_SET_TYPE_EQUAL);
    _lookup_add_const_int(&context, "_set_add_", DC_APP_SET_TYPE_ADD);
    _lookup_add_const_int(&context, "_set_subtract_", DC_APP_SET_TYPE_SUBTRACT);
    _lookup_add_const_int(&context, "_set_multiply_", DC_APP_SET_TYPE_MULTIPLY);
    _lookup_add_const_int(&context, "_set_divide_", DC_APP_SET_TYPE_DIVIDE);

    _lookup_add_const_int(&context, "_color_black_", "0 0 0");
    _lookup_add_const_int(&context, "_color_blue_", "0 0 1");
    _lookup_add_const_int(&context, "_color_green_", "0 1 0");
    _lookup_add_const_int(&context, "_color_cyan_", "0 1 1");
    _lookup_add_const_int(&context, "_color_red_", "1 0 0");
    _lookup_add_const_int(&context, "_color_magenta_", "1 0 1");
    _lookup_add_const_int(&context, "_color_yellow_", "1 1 0");
    _lookup_add_const_int(&context, "_color_white_", "1 1 1");

    sbpush(_sb_contexts, context);

    DcAppLookup *lookup = (DcAppLookup *)malloc(sizeof(DcAppLookup));
    lookup->index       = sbcount(_sb_contexts) - 1;
    return lookup;
}

DcAppLookupIndex _lookup_get_const_index(_LookupContext *context, const char *name) {

    for (int ii = 0; ii < sbcount(context->sb_const_name_offsets); ii++) {
        const char *lookup_name = &(context->sb_const_names[context->sb_const_name_offsets[ii]]);
        if (strcmp(name, lookup_name) == 0) {
            return ii;
        }
    }
    return DC_APP_LOOKUP_INDEX_UNDEFINED;
}

// sets an existing constant
void _lookup_set_const(_LookupContext *context, DcAppLookupIndex index, const char *new_value) {

    // set const value at index
    char **addr = &(context->sb_const_vals[index]);
    sbclear(*addr);
    sbpushn(*addr, new_value, strlen(new_value));
    sbpush(*addr, '\0');
}

// adds a new constant
void _lookup_add_const(_LookupContext *context, const char *name, const char *value) {

    // add const name to buffer
    sbpush(context->sb_const_name_offsets, sbcount(context->sb_const_names));
    sbpushn(context->sb_const_names, name, strlen(name));
    sbpush(context->sb_const_names, '\0');

    // set const to value
    char *buffer;
    sbpushn(buffer, value, strlen(value));
    sbpush(buffer, '\0');
    sbpush(context->sb_const_vals, buffer);
}

static void _lookup_add_const_int(_LookupContext *context, const char *name, int value) {
    char value_str[20];
    snprintf(value, 20, "%d", value);
    _lookup_add_const(context, name, value_str);
}

// set a consts value
void dc_app_lookup_set_const_by_name(DcAppLookup *lookup, const char *name, const char *new_value) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);

    DcAppLookupIndex const_index = _lookup_get_const_index(context, name);
    if (const_index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        _lookup_add_const(context, name, new_value);
    } else {
        _lookup_set_const(context, const_index, new_value);
    }
}

// get a consts value
const char *dc_app_lookup_get_const_by_name(DcAppLookup *lookup, const char *name) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);

    DcAppLookupIndex const_index = _lookup_get_const_index(context, name);
    if (const_index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        fprintf(stderr, "DCAPP dc_app_lookup_get_const_by_name(): constant '%s' does not exist\n", name);
        return NULL;
    } else {
        return context->sb_const_vals[const_index];
    }
}

// expand a string using consts
void dc_app_lookup_dereference_consts(DcAppLookup *lookup, const char *in, char *out, size_t out_size) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);

    // return string if no '$'/'#'
    if (dc_utils_str_find_first_of(in, "#$") == -1) {
        strncpy(out, in, out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    // iterate through each character
    size_t in_length = strlen(in);
    for (int in_index = 0, out_index = 0; in_index < in_length && out_index < out_size - 1; in_index++) {

        // skip backslash-escaped # and $
        if (in[in_index] == '\\' && in_index + 1 < in_length && (in[in_index + 1] == '#' || in[in_index + 1] == '$')) {
            out[out_index++] = in[++in_index];
            continue;
        }

        // if a character is a constant '#' or env variable '$', recursively
        // expand the values within.
        if (in[in_index] == '#' || in[in_index] == '$') {
            // error if ending on a #/$
            if (in_index + 1 >= in_length) {
                fprintf(stderr, "DCAPP dc_app_lookup_dereference_constants(): Cannot have string ending on an unescaped #/$: '%s'\n", in);
                return;
            }

            // find ending index for constant/env variable reference (account for both non-squigglied and squigglied references)
            size_t subtext_start_index;
            size_t subtext_length;
            size_t subtext_length_with_symbols;
            if (in[in_index + 1] == '{') {
                int num_open_brackets = 1;
                int subtext_end_index;
                for (subtext_end_index = in_index + 2; subtext_end_index < in_length; subtext_end_index++) {
                    if (in[subtext_end_index] == '{') {
                        num_open_brackets++;
                    } else if (in[subtext_end_index] == '}') {
                        num_open_brackets--;
                    }

                    if (num_open_brackets == 0) {
                        break;
                    }
                }

                if (num_open_brackets > 0) {
                    fprintf(stderr, "DCAPP dc_app_lookup_dereference_constants(): mismatch with squiggly braces: '%s'\n", in);
                    return;
                }

                subtext_start_index         = in_index + 2;
                subtext_length              = subtext_end_index - subtext_start_index;
                subtext_length_with_symbols = subtext_length + 2;
            } else {
                static const char *valid_chars       = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#$";
                size_t             subtext_end_index = dc_utils_str_find_first_not_of(&(in[in_index + 1]), valid_chars);
                if (subtext_end_index == -1) {
                    subtext_end_index = in_length;
                }

                subtext_start_index         = in_index + 1;
                subtext_length              = subtext_end_index - subtext_start_index;
                subtext_length_with_symbols = subtext_length;
            }

            // get substring, ensure no strange values within
            char subtext[DC_VALUE_STRING_BUFFER_SIZE];
            strncpy(subtext, &in[subtext_start_index], subtext_length);
            subtext[subtext_length] = '\0';
            if (dc_utils_str_find_first(subtext, '@') != -1) {
                fprintf(stderr, "DCAPP dc_app_lookup_dereference_constants(): cannot have variable nested inside variable/constant expansion: '%s'\n", in);
                return;
            }

            // recursion to clean the inner text
            char subtext_cleaned[DC_VALUE_STRING_BUFFER_SIZE];
            dc_app_dereference_constants(lookup, subtext, subtext_cleaned, sizeof(subtext_cleaned));

            // if constant, pull value from list of constants. otherwise use the environment
            if (in[in_index] == '#') {
                const char *const_value = dc_app_lookup_get_const_by_name(lookup, subtext);
                if (const_value) {
                    for (int ii = 0; ii < strlen(const_value) && out_index < out_size - 1;) {
                        out[out_index++] = const_value[ii++];
                    }
                }
            } else if (in[in_index] == '$') {
                const char *env_value = getenv(subtext);
                if (env_value) {
                    for (int ii = 0; ii < strlen(env_value) && out_index < out_size - 1;) {
                        out[out_index++] = env_value[ii++];
                    }
                }
            }

            // increment in_index by the cleaned amount
            in_index += subtext_length_with_symbols;
        } else {
            out[out_index++] = in[in_index];
        }
    }
}

// for XML parsing
DcValueType dc_app_value_type_from_string(const char *type_str) {

    if (type_str == NULL) {
        return DC_VALUE_TYPE_UNDEFINED;
    } else if (strcmp(type_str, "Decimal") == 0 || strcmp(type_str, "Float") == 0 || strcmp(type_str, "Double") == 0) {
        return DC_VALUE_TYPE_DOUBLE;
    } else if (strcmp(type_str, "Integer") == 0) {
        return DC_VALUE_TYPE_INTEGER;
    } else if (strcmp(type_str, "String") == 0) {
        return DC_VALUE_TYPE_STRING;
    } else if (strcmp(type_str, "Boolean") == 0) {
        return DC_VALUE_TYPE_BOOLEAN;
    }
    fprintf(stderr, "DCAPP dc_app_value_type_from_string(): Unknown value type of type '%s'\n", type_str);
    return DC_VALUE_TYPE_UNDEFINED;
}

DcValue *dc_app_lookup_get_value(DcAppLookup *lookup, DcAppValIndex index) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    if (index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        fprintf(stderr, "dc_app_lookup_get_value(): attempting to fetch invalid index %d\n", index);
        return NULL;
    }
    return &(context->sb_vals[index]);
}

DcAppValIndex dc_app_lookup_register_value(DcAppLookup *lookup, DcValue *value) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    sbpush(context->sb_vals, *value);
    value->value_string = NULL; // steal it!
}

DcAppValIndex dc_app_create_and_register_typed_value_from_string(DcAppLookup *lookup, DcValueType type, const char *text) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);

    char text_cleaned[DC_VALUE_STRING_BUFFER_SIZE];
    dc_utils_trim_whitespace_copy(text, text_cleaned, sizeof(text_cleaned));

    // check for var
    if (strlen(text_cleaned) > 1 && text_cleaned[0] == '@') {
        DcAppLookupVar *var = dc_app_lookup_get_var_by_name(lookup, &(text_cleaned[1]));
        return var->value_index;
    }

    // otherwise create new DcValue and return its index
    DcValue val = dc_value_create_typed_value_from_string(type, text);
    return dc_app_lookup_register_value(lookup, &val);
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
        fprintf(stderr, "dc_app_lookup_get_var(): attempting to fetch invalid index %d\n", index);
        return NULL;
    }
    return &(context->sb_vars[index]);
}

DcAppLookupVar *dc_app_lookup_get_var_by_name(DcAppLookup *lookup, const char *name) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    DcAppVarIndex   index   = dc_app_lookup_get_var_index(lookup, name);
    if (index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        fprintf(stderr, "dc_app_lookup_get_var_by_name(): attempting to fetch non-existant variable '%s'\n", name);
        return NULL;
    }
    return dc_app_lookup_get_var(lookup, index);
}

DcAppVarIndex dc_app_lookup_register_var(DcAppLookup *lookup, const char *name, DcAppLookupVar *var) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);

    DcAppVarIndex index = dc_app_lookup_get_var_index(lookup, name);
    if (index != DC_APP_LOOKUP_INDEX_UNDEFINED) {
        fprintf(stderr, "dc_app_lookup_register_var(): variable already exists '%s'\n", name);
        return index;
    }

    sbpush(context->sb_var_name_offsets, sbcount(context->sb_var_names));
    sbpushn(context->sb_var_names, name, strlen(name));
    sbpush(context->sb_var_names, '\0');
    sbpush(context->sb_vars, *var);
    var->extern_data = NULL;
    return sbcount(context->sb_vars) - 1;
}

void dc_app_lookup_set_var_to_string(DcAppLookup *lookup, DcAppVarIndex var_index, const char *new_string) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    DcAppLookupVar *var     = dc_app_lookup_get_var(lookup, var_index);
    DcValue        *val     = dc_app_lookup_get_value(lookup, var->value_index);
    dc_value_set_from_string(val, new_string);
    dc_app_lookup_refresh_var_from_value(lookup, var_index);
}

void dc_app_lookup_refresh_var_from_extern(DcAppLookup *lookup, DcAppVarIndex var_index) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    DcAppLookupVar *var     = dc_app_lookup_get_var(lookup, var_index);
    DcValue        *value   = dc_app_lookup_get_value(lookup, var->value_index);

    void *extern_data = var->extern_data;
    switch (value->type) {
        case DC_VALUE_TYPE_DOUBLE: {
            value->value_double = *((double *)(extern_data));
            break;
        }
        case DC_VALUE_TYPE_INTEGER: {
            value->value_integer = *((int *)(extern_data));
            break;
        }
        case DC_VALUE_TYPE_STRING: {
            value->value_string = (char *)extern_data;
            break;
        }
        case DC_VALUE_TYPE_BOOLEAN: {
            value->value_boolean = *((bool *)(extern_data));
            break;
        }
        default:
            fprintf(stderr, "dc_app_lookup_refresh_var_from_extern(): variable of invalid type '%d'\n", value->type);
            break;
    }
    dc_value_refresh(value);
}

void dc_app_lookup_refresh_var_from_value(DcAppLookup *lookup, DcAppVarIndex var_index) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    DcAppLookupVar *var     = dc_app_lookup_get_var(lookup, var_index);
    DcValue        *value   = dc_app_lookup_get_value(lookup, var->value_index);

    void *extern_data = var->extern_data;
    switch (value->type) {
        case DC_VALUE_TYPE_DOUBLE: {
            *((double *)(extern_data)) = value->value_double;
            break;
        }
        case DC_VALUE_TYPE_INTEGER: {
            *((int *)(extern_data)) = value->value_integer;
            break;
        }
        case DC_VALUE_TYPE_STRING: {
            extern_data = (void *)(value->value_string);
            break;
        }
        case DC_VALUE_TYPE_BOOLEAN: {
            *((bool *)(extern_data)) = value->value_boolean;
            break;
        }
        default:
            fprintf(stderr, "dc_app_lookup_refresh_var_from_value(): variable of invalid type '%d'\n", value->type);
            break;
    }
}
