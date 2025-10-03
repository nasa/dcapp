#ifndef _DC_UTILS_STRING_
#define _DC_UTILS_STRING_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int dc_utils_str_find(const char *s, const char *pattern);
int dc_utils_str_find_first(const char *s, char ch);
int dc_utils_str_find_first_of(const char *s, const char *chars);
int dc_utils_str_find_first_not_of(const char *s, const char *chars);

void dc_utils_trim_whitespace_inplace(char *text);
void dc_utils_trim_whitespace_copy(const char *input, char *out, size_t out_size);

double dc_utils_string_to_double(const char *text);
int    dc_utils_string_to_integer(const char *text);
int    dc_utils_string_to_boolean(const char *text);

void dc_utils_string_to_hash(const char *text, char *out, size_t out_size);

void dc_utils_split_string_inplace(char *text, const char *delimiters, size_t *out_indices, size_t out_indices_size, size_t *out_indices_count);
void dc_utils_split_string_copy(const char *text, const char *delimiters, char *out, size_t out_size, size_t *out_indices, size_t out_indices_size, size_t *out_indices_count);

bool dc_utils_is_format_specifier_bool(const char *value);
bool dc_utils_is_format_specifier_int(const char *value);
bool dc_utils_is_format_specifier_double(const char *value);
bool dc_utils_is_format_specifier_string(const char *value);

static const char* dc_utils_whitespace = " \t\n\v\f\r";

#ifdef __cplusplus
}
#endif

#endif
