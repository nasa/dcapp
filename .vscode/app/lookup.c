#include "lookup.h"

#include "../utils/stb_sb.h"
#include "../value.h"

#include <stdio.h>
#include <string.h>

typedef struct __LookupVar {
    void    *extern_data;
    uint32_t value_index;
} _LookupVar;

typedef struct __LookupContext {

    // constants
    char  *sb_const_names;
    int   *sb_const_name_offsets;
    char **sb_const_vals;

    // vars
    char       *sb_var_names;
    int        *sb_var_name_offsets;
    _LookupVar *sb_vars;

    // values
    DcValue *sb_vals;

} _LookupContext;

static _LookupContext *_sb_contexts;

static int  _lookup_get_const_index(_LookupContext *context, const char *name);
static void _lookup_set_const(_LookupContext *context, int index, const char *new_value);
static void _lookup_add_const(_LookupContext *context, const char *name, const char *value);

// create an app lookup
DcAppLookup *dc_app_lookup_create() {
    _LookupContext context = {0};
    sbpush(_sb_contexts, context);

    DcAppLookup *lookup = (DcAppLookup *)malloc(sizeof(DcAppLookup));
    lookup->index       = sbcount(_sb_contexts) - 1;
    return lookup;
}

// set a consts value
void dc_app_lookup_set_const_by_name(DcAppLookup *lookup, const char *name, const char *new_value) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);

    int const_index = _lookup_get_const_index(context, name);
    if (const_index == -1) {
        _lookup_add_const(context, name, new_value);
    } else {
        _lookup_set_const(context, const_index, new_value);
    }
}

// get a consts value
const char *dc_app_lookup_get_const_by_name(DcAppLookup *lookup, const char *name) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);

    int const_index = _lookup_get_const_index(context, name);
    if (const_index == -1) {
        fprintf(stderr, "DCAPP dc_app_lookup_get_const_by_name(): constant '%s' does not exist\n", name);
    } else {
        return context->sb_const_vals[const_index];
    }
}

// expand a string using consts
void *dc_app_lookup_dereference_consts(DcAppLookup *lookup, const char *in, char *out, size_t out_size) {
    _LookupContext *context = &(_sb_contexts[lookup->index]);

    // return string if no '$'/'#'
    if (dc_utils_str_find_first_of(in, "#$"))
    if (text.find_first_of("#$") == std::string::npos) {
        return text;
    }

    // iterate through each character
    std::string output = "";
    for (int ii = 0; ii < text.length(); ii++) {
        // skip backslash-escaped # and $
        if (text[ii] == '\\' && ii + 1 < text.length() && (text[ii + 1] == '#' || text[ii + 1] == '$')) {
            output += text[++ii];
            continue;
        }

        // if a character is a constant '#' or env variable '$', recursively
        // expand the values within.
        if (text[ii] == '#' || text[ii] == '$') {
            // error if ending on a #/$
            if (ii + 1 >= text.length()) {
                throw std::runtime_error("Invalid string format: cannot have variable ending on a #/$. " + text);
            }

            // find ending index for constant/env variable reference (account for both
            // non-squigglied and squigglied references)
            std::string subtext;
            size_t      subtext_start_index;
            size_t      subtext_length;
            size_t      subtext_length_with_symbols;
            if (text[ii + 1] == '{') {
                int num_open_brackets = 1;
                int jj;
                for (jj = ii + 2; jj < text.length(); jj++) {
                    if (text[jj] == '{') {
                        num_open_brackets++;
                    } else if (text[jj] == '}') {
                        num_open_brackets--;
                    }

                    if (num_open_brackets == 0) {
                        break;
                    }
                }

                if (num_open_brackets > 0) {
                    throw std::runtime_error("Invalid string format: mismatch with squiggly braces. " + text);
                }

                subtext_start_index         = ii + 2;
                subtext_length              = jj - subtext_start_index;
                subtext_length_with_symbols = subtext_length + 2;
            } else {
                static const std::string validChars        = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#$";
                size_t                   subtext_end_index = text.find_first_not_of(validChars, ii + 1);
                if (subtext_end_index == std::string::npos) {
                    subtext_end_index = text.length();
                }

                subtext_start_index         = ii + 1;
                subtext_length              = subtext_end_index - subtext_start_index;
                subtext_length_with_symbols = subtext_length;
            }

            // get substring, ensure no strange values within
            subtext = text.substr(subtext_start_index, subtext_length);
            if (subtext.find('@') != std::string::npos) {
                throw std::runtime_error("Invalid string format: cannot have variable nested inside variable/constant expansion. " + text);
            }

            // recursion to clean the inner text
            subtext = dc_app_dereference_constants(subtext);

            // if constant, pull value from list of constants
            if (text[ii] == '#') {
                output += dc_app_get_constant(subtext);
            }
            // otherwise use the environment
            else if (text[ii] == '$') {
                char *env_value = getenv(subtext.c_str());
                if (env_value) {
                    output += std::string(env_value);
                }
            }

            // increment ii by the cleaned amount
            ii += subtext_length_with_symbols;
        } else {
            output += text[ii];
        }
    }
    return output;
}

int _lookup_get_const_index(_LookupContext *context, const char *name) {

    for (int ii = 0; ii < sbcount(context->sb_const_name_offsets); ii++) {
        const char *lookup_name = &(context->sb_const_names[context->sb_const_name_offsets[ii]]);
        if (strcmp(name, lookup_name) == 0) {
            return ii;
        }
    }
    return -1;
}

// sets an existing constant
void _lookup_set_const(_LookupContext *context, int index, const char *new_value) {

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
