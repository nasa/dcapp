#pragma once

#include <string>

enum DcValueType {
    DC_VALUE_TYPE_UNDEFINED,
    DC_VALUE_TYPE_STRING,
    DC_VALUE_TYPE_INTEGER,
    DC_VALUE_TYPE_FLOAT,
    DC_VALUE_TYPE_BOOLEAN,
};

typedef struct _DcValue {
    DcValueType type;

    std::string value_string;
    int         value_integer;
    float       value_float;
    bool        value_boolean;

    bool is_dynamic;
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

void        dc_value_copy(DcValue *dst, DcValue *src);
DcValueType dc_value_type_from_string(const std::string &type);
DcValue     dc_value_create_typed_value_from_string(DcValueType type, const std::string &value);
DcValue     dc_value_create_value_string(const std::string &value);
DcValue     dc_value_create_value_integer(int value);
DcValue     dc_value_create_value_float(float value);
DcValue     dc_value_create_value_boolean(bool value);
void        dc_value_refresh(DcValue *value);
void        dc_value_set_from_string(DcValue *value, const std::string &string_value);
bool        dc_value_is_equal(DcValue *value1, DcValue *value2);
