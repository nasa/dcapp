#ifndef DC_APP_VALUE_H
#define DC_APP_VALUE_H

#include "value_types.h"

#include <stdbool.h>

#define DC_APP_VALUE_STRING_BUFFER_SIZE 256

// Each value caches every representation while type marks the authoritative field.
typedef struct DcAppValue {
    DcAppValueType type;

    char value_string[DC_APP_VALUE_STRING_BUFFER_SIZE];
    int value_integer;
    double value_double;
    bool value_boolean;
} DcAppValue;

typedef struct DcAppValue2 {
    union {
        DcAppValue x, r, lat;
    };
    union {
        DcAppValue y, g, lon;
    };
} DcAppValue2;

typedef struct DcAppValue3 {
    union {
        DcAppValue x, r, lat;
    };
    union {
        DcAppValue y, g, lon;
    };
    union {
        DcAppValue z, b, ele;
    };
} DcAppValue3;

typedef struct DcAppValue4 {
    union {
        DcAppValue x, r;
    };
    union {
        DcAppValue y, g;
    };
    union {
        DcAppValue z, b;
    };
    union {
        DcAppValue w, a;
    };
} DcAppValue4;

#ifdef __cplusplus
extern "C" {
#endif

DcAppValue dc_app_value_create_typed_value_from_string(DcAppValueType type, const char *value_str);
DcAppValue dc_app_value_create_value_string(const char *value);
DcAppValue dc_app_value_create_value_integer(int value);
DcAppValue dc_app_value_create_value_double(double value);
DcAppValue dc_app_value_create_value_boolean(bool value);
void dc_app_value_refresh(DcAppValue *value);
void dc_app_value_refresh_from_type(DcAppValue *value, DcAppValueType type);
void dc_app_value_set_from_string(DcAppValue *value, const char *string_value);
void *dc_app_value_get_addr(DcAppValue *value);

bool dc_app_value_is_equal(DcAppValue *value1, DcAppValue *value2);
bool dc_app_value_is_not_equal(DcAppValue *value1, DcAppValue *value2);
bool dc_app_value_is_greater(DcAppValue *value1, DcAppValue *value2);
bool dc_app_value_is_greater_or_equal(DcAppValue *value1, DcAppValue *value2);
bool dc_app_value_is_less(DcAppValue *value1, DcAppValue *value2);
bool dc_app_value_is_less_or_equal(DcAppValue *value1, DcAppValue *value2);

#ifdef __cplusplus
}
#endif

#endif
