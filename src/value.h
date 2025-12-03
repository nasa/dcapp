#ifndef _DC_VALUE_
#define _DC_VALUE_

#include <stdbool.h>

#define DC_VALUE_STRING_BUFFER_SIZE 256

typedef enum _DcValueType {
    DC_VALUE_TYPE_UNDEFINED,
    DC_VALUE_TYPE_STRING,
    DC_VALUE_TYPE_INTEGER,
    DC_VALUE_TYPE_DOUBLE,
    DC_VALUE_TYPE_BOOLEAN,
} DcValueType;

typedef struct _DcValue {
    DcValueType type;

    char  *value_string;
    int    value_integer;
    double value_double;
    bool   value_boolean;
} DcValue;

typedef struct _DcValue2 {
    union {
        DcValue x, r, lat;
    };
    union {
        DcValue y, g, lon;
    };
} DcValue2;

typedef struct _DcValue3 {
    union {
        DcValue x, r, lat;
    };
    union {
        DcValue y, g, lon;
    };
    union {
        DcValue z, b, ele;
    };
} DcValue3;

typedef struct _DcValue4 {
    union {
        DcValue x, r;
    };
    union {
        DcValue y, g;
    };
    union {
        DcValue z, b;
    };
    union {
        DcValue w, a;
    };
} DcValue4;

#ifdef __cplusplus
extern "C" {
#endif

void    dc_value_copy(DcValue *dst, DcValue *src);
DcValue dc_value_create_typed_value_from_string(DcValueType type, const char *value_str);
DcValue dc_value_create_value_string(const char *value);
DcValue dc_value_create_value_integer(int value);
DcValue dc_value_create_value_double(double value);
DcValue dc_value_create_value_boolean(bool value);
void    dc_value_refresh(DcValue *value);
void    dc_value_set_from_string(DcValue *value, const char *string_value);

bool dc_value_is_equal(DcValue *value1, DcValue *value2);
bool dc_value_is_not_equal(DcValue *value1, DcValue *value2);
bool dc_value_is_greater(DcValue *value1, DcValue *value2);
bool dc_value_is_greater_or_equal(DcValue *value1, DcValue *value2);
bool dc_value_is_less(DcValue *value1, DcValue *value2);
bool dc_value_is_less_or_equal(DcValue *value1, DcValue *value2);

#ifdef __cplusplus
}
#endif

#endif
