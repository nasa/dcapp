#include "string.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

int dc_utils_str_find(const char *s, const char *pattern) {
    if (!s || !pattern)
        return -1;
    const char *p = strstr(s, pattern);
    return p ? (int)(p - s) : -1;
}

int dc_utils_str_find_first(const char *s, char ch) {
    if (!s)
        return -1;
    const char *p = strchr(s, ch);
    return p ? (int)(p - s) : -1;
}

int dc_utils_str_find_first_of(const char *s, const char *chars) {
    if (!s || !chars)
        return -1;
    const char *p = strpbrk(s, chars);
    return p ? (int)(p - s) : -1;
}

int dc_utils_str_find_first_not_of(const char *s, const char *chars) {
    if (!s || !chars)
        return -1;
    for (int i = 0; s[i] != '\0'; i++) {
        if (!strchr(chars, s[i])) {
            return i;
        }
    }
    return -1;
}

static bool _string_is_double(const char *text) {
    char *end = NULL;
    errno     = 0;
    strtof(text, &end);
    return (end != text && *end == '\0' && errno == 0);
}

static bool _string_is_int(const char *text) {
    char *end = NULL;
    errno     = 0;
    strtol(text, &end, 10);
    return (end != text && *end == '\0' && errno == 0);
}

void dc_utils_trim_whitespace_inplace(char *text) {
    if (text == NULL) {
        return;
    }

    size_t len = strlen(text);
    size_t start = 0;
    size_t end = len;

    while (start < len && isspace((unsigned char)text[start])) {
        start++;
    }

    while (end > start && isspace((unsigned char)text[end - 1])) {
        end--;
    }

    size_t trimmed_len = end - start;

    if (start > 0) {
        memmove(text, text + start, trimmed_len);
    }

    text[trimmed_len] = '\0';
}

void dc_utils_trim_whitespace_copy(const char *input, char *out, size_t out_size) {
    if (input == NULL || out == NULL || out_size == 0) {
        return;
    }

    size_t len   = strlen(input);
    size_t start = 0;
    size_t end   = len;

    while (start < len && isspace((unsigned char)input[start])) {
        start++;
    }

    while (end > start && isspace((unsigned char)input[end - 1])) {
        end--;
    }

    size_t trimmed_len = end - start;

    if (trimmed_len >= out_size) {
        trimmed_len = out_size - 1;
    }

    memcpy(out, input + start, trimmed_len);
    out[trimmed_len] = '\0';
}

double dc_utils_string_to_double(const char *text) {
    if (_string_is_double(text)) {
        return strtod(text, NULL);
    } else {
        return (double)(dc_utils_string_to_boolean(text));
    }
}

int dc_utils_string_to_integer(const char *text) {
    if (_string_is_int(text)) {
        return (int)strtol(text, NULL, 10);
    } else if (_string_is_double(text)) {
        return (int)strtod(text, NULL);
    } else {
        return (int)dc_utils_string_to_boolean(text);
    }
}

int dc_utils_string_to_boolean(const char *text) {

    if (!text) {
        return 0;
    }

    char result[strlen(text) + 1];
    dc_utils_trim_whitespace_copy(text, result, sizeof(result));

    if (result[0] == '\0') {
        return 0;
    }
    for (char *c = result; *c; c++) {
        if (*c >= 'A' && *c <= 'Z') {
            *c = (char)(*c + 32);
        }
    }

    if (strcmp(result, "false") == 0 || strcmp(result, "no") == 0 || strcmp(result, "off") == 0) {
        return 0;
    }
    return 1;
}

void dc_utils_string_to_hash(const char *text, char *out, size_t out_size) {
    if (!out || out_size == 0) {
        return;
    }
    if (out_size < 21) {
        out[0] = '\0';
        return;
    }

    uint64_t hash = 0;
    for (const char *c = text; c && *c; c++) {
        hash = *c + (hash << 6) + (hash << 16) - hash; // sdbm hash
    }

    (void)snprintf(out, out_size, "%020llu", (unsigned long long)hash);
}

void dc_utils_split_string_inplace(char *text, const char *delimiters, size_t *out_indices, size_t out_indices_size, size_t *out_indices_count) {
    *out_indices_count = 0;

    if (text == NULL || delimiters == NULL || out_indices == NULL || out_indices_size == 0) {
        return;
    }

    size_t len = strlen(text);
    size_t count = 0;
    size_t ii = 0;

    while (ii < len) {
        while (ii < len && strchr(delimiters, text[ii]) != NULL) {
            text[ii] = '\0';
            ii++;
        }

        if (ii >= len) {
            break;
        }

        if (count >= out_indices_size) {
            break;
        }

        out_indices[count] = ii;
        count++;

        while (ii < len && strchr(delimiters, text[ii]) == NULL) {
            ii++;
        }

        if (ii < len && strchr(delimiters, text[ii]) != NULL) {
            text[ii] = '\0';
            ii++;
        }
    }

    *out_indices_count = count;
}


void dc_utils_split_string_copy(const char *text, const char *delimiters, char *out, size_t out_size, size_t *out_indices, size_t out_indices_size, size_t *out_indices_count) {
    *out_indices_count = 0;

    if (text == NULL || delimiters == NULL || out == NULL || out_indices == NULL || out_indices_size == 0 || out_size == 0) {
        return;
    }

    strncpy(out, text, out_size - 1);
    out[out_size - 1] = '\0';

    size_t len   = strlen(out);
    size_t count = 0;
    size_t ii    = 0;

    while (ii < len) {
        while (ii < len && strchr(delimiters, out[ii]) != NULL) {
            out[ii] = '\0';
            ii++;
        }

        if (ii >= len) {
            break;
        }

        if (count >= out_indices_size) {
            break;
        }

        out_indices[count] = ii;
        count++;

        while (ii < len && strchr(delimiters, out[ii]) == NULL) {
            ii++;
        }

        if (ii < len && strchr(delimiters, out[ii]) != NULL) {
            out[ii] = '\0';
            ii++;
        }
    }

    *out_indices_count = count;
}

static bool _is_format_specifier(const char *value, const char *valid_specifiers) {
    if (!value || value[0] != '%')
        return false;
    size_t ii = 1;
    while (value[ii] && strchr("-+0 #", value[ii])) {
        ii++;
    }
    while (value[ii] && isdigit((unsigned char)value[ii])) {
        ii++;
    }
    if (value[ii] == '.') {
        ii++;
        while (value[ii] && isdigit((unsigned char)value[ii])) {
            ii++;
        }
    }
    if (value[ii] && strchr(valid_specifiers, value[ii])) {
        return true;
    }
    return false;
}

bool dc_utils_is_format_specifier_bool(const char *value) {
    static const char *chars = "dis";
    return _is_format_specifier(value, chars);
}

bool dc_utils_is_format_specifier_int(const char *value) {
    static const char *chars = "diuxXo";
    return _is_format_specifier(value, chars);
}

bool dc_utils_is_format_specifier_double(const char *value) {
    static const char *chars = "fFeEgG";
    return _is_format_specifier(value, chars);
}

bool dc_utils_is_format_specifier_string(const char *value) {
    static const char *chars = "s";
    return _is_format_specifier(value, chars);
}
