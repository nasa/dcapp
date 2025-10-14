#include "lookup.h"

#include "enums.h"
#include "../utils/env.h"
#include "../utils/string.h"
#include "../utils/stb_sb.h"

#include <string.h>
#include <stdio.h>

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

    // Reds & Pinks
    _lookup_add_const(&context, "_color_red_", "1.0 0.0 0.0");
    _lookup_add_const(&context, "_color_crimson_", "0.86 0.08 0.24");
    _lookup_add_const(&context, "_color_maroon_", "0.5 0.0 0.0");
    _lookup_add_const(&context, "_color_burgundy_", "0.6 0.0 0.13");
    _lookup_add_const(&context, "_color_ruby_", "0.88 0.07 0.37");
    _lookup_add_const(&context, "_color_cherry_", "0.87 0.19 0.39");
    _lookup_add_const(&context, "_color_rose_", "1.0 0.0 0.5");
    _lookup_add_const(&context, "_color_pink_", "1.0 0.75 0.8");
    _lookup_add_const(&context, "_color_salmon_", "0.98 0.5 0.45");
    _lookup_add_const(&context, "_color_coral_", "1.0 0.5 0.31");
    _lookup_add_const(&context, "_color_peach_", "1.0 0.85 0.73");
    _lookup_add_const(&context, "_color_fuchsia_", "1.0 0.0 1.0");
    _lookup_add_const(&context, "_color_hot_pink_", "1.0 0.41 0.71");
    _lookup_add_const(&context, "_color_light_pink_", "1.0 0.71 0.76");
    _lookup_add_const(&context, "_color_mulberry_", "0.77 0.29 0.55");

    // Oranges
    _lookup_add_const(&context, "_color_orange_", "1.0 0.5 0.0");
    _lookup_add_const(&context, "_color_tangerine_", "1.0 0.6 0.0");
    _lookup_add_const(&context, "_color_pumpkin_", "1.0 0.46 0.1");
    _lookup_add_const(&context, "_color_apricot_", "0.98 0.81 0.69");
    _lookup_add_const(&context, "_color_cantaloupe_", "1.0 0.71 0.55");
    _lookup_add_const(&context, "_color_amber_", "1.0 0.75 0.0");
    _lookup_add_const(&context, "_color_burnt_orange_", "0.8 0.33 0.0");
    _lookup_add_const(&context, "_color_rust_", "0.72 0.25 0.05");
    _lookup_add_const(&context, "_color_terracotta_", "0.89 0.45 0.36");

    // Yellows
    _lookup_add_const(&context, "_color_yellow_", "1.0 1.0 0.0");
    _lookup_add_const(&context, "_color_lemon_", "1.0 1.0 0.31");
    _lookup_add_const(&context, "_color_mustard_", "1.0 0.86 0.35");
    _lookup_add_const(&context, "_color_gold_", "1.0 0.84 0.0");
    _lookup_add_const(&context, "_color_butter_", "1.0 0.94 0.75");
    _lookup_add_const(&context, "_color_champagne_", "0.97 0.91 0.81");
    _lookup_add_const(&context, "_color_sunflower_", "1.0 0.8 0.0");
    _lookup_add_const(&context, "_color_flax_", "0.93 0.87 0.51");

    // Greens
    _lookup_add_const(&context, "_color_green_", "0.0 1.0 0.0");
    _lookup_add_const(&context, "_color_lime_", "0.75 1.0 0.0");
    _lookup_add_const(&context, "_color_olive_", "0.5 0.5 0.0");
    _lookup_add_const(&context, "_color_moss_", "0.53 0.6 0.42");
    _lookup_add_const(&context, "_color_forest_green_", "0.13 0.55 0.13");
    _lookup_add_const(&context, "_color_emerald_", "0.31 0.78 0.47");
    _lookup_add_const(&context, "_color_jade_", "0.0 0.66 0.42");
    _lookup_add_const(&context, "_color_mint_", "0.74 0.99 0.79");
    _lookup_add_const(&context, "_color_pistachio_", "0.58 0.77 0.45");
    _lookup_add_const(&context, "_color_seafoam_", "0.62 0.89 0.76");
    _lookup_add_const(&context, "_color_chartreuse_", "0.5 1.0 0.0");

    // Blues
    _lookup_add_const(&context, "_color_blue_", "0.0 0.0 1.0");
    _lookup_add_const(&context, "_color_navy_", "0.0 0.0 0.5");
    _lookup_add_const(&context, "_color_sky_blue_", "0.53 0.81 0.92");
    _lookup_add_const(&context, "_color_baby_blue_", "0.87 0.92 1.0");
    _lookup_add_const(&context, "_color_azure_", "0.0 0.5 1.0");
    _lookup_add_const(&context, "_color_denim_", "0.08 0.38 0.65");
    _lookup_add_const(&context, "_color_sapphire_", "0.08 0.15 0.39");
    _lookup_add_const(&context, "_color_steel_blue_", "0.27 0.51 0.71");
    _lookup_add_const(&context, "_color_powder_blue_", "0.69 0.88 0.9");
    _lookup_add_const(&context, "_color_cerulean_", "0.0 0.48 0.65");
    _lookup_add_const(&context, "_color_teal_", "0.0 0.5 0.5");

    // Purples & Violets
    _lookup_add_const(&context, "_color_purple_", "0.5 0.0 0.5");
    _lookup_add_const(&context, "_color_indigo_", "0.29 0.0 0.51");
    _lookup_add_const(&context, "_color_lavender_", "0.9 0.9 0.98");
    _lookup_add_const(&context, "_color_plum_", "0.56 0.27 0.52");
    _lookup_add_const(&context, "_color_violet_", "0.93 0.51 0.93");
    _lookup_add_const(&context, "_color_amethyst_", "0.6 0.4 0.8");
    _lookup_add_const(&context, "_color_orchid_", "0.85 0.44 0.84");
    _lookup_add_const(&context, "_color_thistle_", "0.85 0.75 0.85");
    _lookup_add_const(&context, "_color_eggplant_", "0.38 0.25 0.32");

    // Browns
    _lookup_add_const(&context, "_color_brown_", "0.6 0.4 0.2");
    _lookup_add_const(&context, "_color_chocolate_", "0.82 0.41 0.12");
    _lookup_add_const(&context, "_color_saddle_brown_", "0.55 0.27 0.07");
    _lookup_add_const(&context, "_color_umber_", "0.39 0.32 0.28");
    _lookup_add_const(&context, "_color_mahogany_", "0.65 0.19 0.19");
    _lookup_add_const(&context, "_color_copper_", "0.72 0.45 0.2");
    _lookup_add_const(&context, "_color_tan_", "0.82 0.71 0.55");
    _lookup_add_const(&context, "_color_walnut_", "0.39 0.26 0.13");
    _lookup_add_const(&context, "_color_espresso_", "0.36 0.25 0.2");
    _lookup_add_const(&context, "_color_caramel_", "0.87 0.58 0.36");
    _lookup_add_const(&context, "_color_mocha_", "0.44 0.31 0.22");
    _lookup_add_const(&context, "_color_pecan_", "0.78 0.52 0.25");
    _lookup_add_const(&context, "_color_wood_", "0.76 0.6 0.42");
    _lookup_add_const(&context, "_color_bronze_", "0.8 0.5 0.2");
    _lookup_add_const(&context, "_color_russet_", "0.5 0.27 0.23");

    // Neutrals & Grays
    _lookup_add_const(&context, "_color_white_", "1.0 1.0 1.0");
    _lookup_add_const(&context, "_color_black_", "0.0 0.0 0.0");
    _lookup_add_const(&context, "_color_gray_", "0.5 0.5 0.5");
    _lookup_add_const(&context, "_color_light_gray_", "0.83 0.83 0.83");
    _lookup_add_const(&context, "_color_dark_gray_", "0.33 0.33 0.33");
    _lookup_add_const(&context, "_color_charcoal_", "0.21 0.27 0.31");
    _lookup_add_const(&context, "_color_silver_", "0.75 0.75 0.75");
    _lookup_add_const(&context, "_color_ash_", "0.7 0.75 0.71");
    _lookup_add_const(&context, "_color_slate_", "0.44 0.5 0.56");
    _lookup_add_const(&context, "_color_eggshell_", "0.94 0.92 0.84");
    _lookup_add_const(&context, "_color_alabaster_", "0.98 0.98 0.95");
    _lookup_add_const(&context, "_color_beige_", "0.96 0.96 0.86");
    _lookup_add_const(&context, "_color_khaki_", "0.76 0.69 0.57");
    _lookup_add_const(&context, "_color_sand_", "0.94 0.87 0.73");
    _lookup_add_const(&context, "_color_taupe_", "0.56 0.52 0.51");

    sbpush(_sb_contexts, context);

    DcAppLookup *lookup = (DcAppLookup *)malloc(sizeof(DcAppLookup));
    lookup->index       = sbcount(_sb_contexts) - 1;
    return lookup;
}

void dc_app_lookup_cleanup(DcAppLookup *lookup) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    sbfree(context->sb_const_names);
    sbfree(context->sb_const_name_offsets);
    sbfree(context->sb_var_names);
    sbfree(context->sb_var_name_offsets);
    sbfree(context->sb_vars);

    for (int ii = 0; ii < sbcount(context->sb_const_vals); ii++) {
        free(context->sb_const_vals[ii]);
    }
    sbfree(context->sb_const_vals);

    for (int ii = 0; ii < sbcount(context->sb_vals); ii++) {
        free(context->sb_vals[ii].value_string);
    }
    sbfree(context->sb_vals);
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
    char *buffer = NULL;
    sbpushn(buffer, value, strlen(value));
    sbpush(buffer, '\0');
    sbpush(context->sb_const_vals, buffer);
}

static void _lookup_add_const_int(_LookupContext *context, const char *name, int value) {
    char value_str[20];
    snprintf(value_str, 20, "%d", value);
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
void dc_app_lookup_dereference_constants(DcAppLookup *lookup, const char *in, char *out, size_t out_size) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);

    // return string if no '$'/'#'
    if (dc_utils_str_find_first_of(in, "#$") == -1) {
        strncpy(out, in, out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    // iterate through each character
    size_t in_length = strlen(in);
    int out_index = 0;
    for (int in_index = 0; in_index < in_length && out_index < out_size - 1; in_index++) {

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
            dc_app_lookup_dereference_constants(lookup, subtext, subtext_cleaned, sizeof(subtext_cleaned));

            // if constant, pull value from list of constants. otherwise use the environment
            if (in[in_index] == '#') {
                const char *const_value = dc_app_lookup_get_const_by_name(lookup, subtext);
                if (const_value) {
                    for (int ii = 0; ii < strlen(const_value) && out_index < out_size - 1;) {
                        out[out_index++] = const_value[ii++];
                    }
                }
            } else if (in[in_index] == '$') {
                const char *env_value = dc_utils_get_env(subtext);
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

    if (out_index < out_size) {
        out[out_index] = '\0';
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
    return sbcount(context->sb_vals) - 1;
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

const char *dc_app_lookup_get_var_name(DcAppLookup *lookup, DcAppVarIndex index) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);
    return &(context->sb_var_names[context->sb_var_name_offsets[index]]);
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
