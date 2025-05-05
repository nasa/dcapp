#pragma once

#include <string>

enum DcValueType
{
    DC_VALUE_TYPE_UNDEFINED,
    DC_VALUE_TYPE_STRING,
    DC_VALUE_TYPE_INTEGER,
    DC_VALUE_TYPE_FLOAT,
    DC_VALUE_TYPE_BOOLEAN,
};

typedef struct _DcValue
{
    DcValueType type;

    std::string valueString;
    int valueInteger;
    float valueFloat;
    bool valueBoolean;

    bool isDynamic;
} DcValue;

typedef struct _DcValue2
{
    union
    {
        DcValue x, r, lat;
    };
    union
    {
        DcValue y, g, lon;
    };
} DcValue2;

typedef struct _DcValue3
{
    union
    {
        DcValue x, r, lat;
    };
    union
    {
        DcValue y, g, lon;
    };
    union
    {
        DcValue z, b, ele;
    };
} DcValue3;

typedef struct _DcValue4
{
    union
    {
        DcValue x, r;
    };
    union
    {
        DcValue y, g;
    };
    union
    {
        DcValue z, b;
    };
    union
    {
        DcValue w, a;
    };
} DcValue4;

DcValueType valueTypeFromString(const std::string& type);
DcValue createTypedValueFromString(DcValueType type, const std::string &value);
DcValue createValueString(const std::string &value);
DcValue createValueInteger(int value);
DcValue createValueFloat(float value);
DcValue createValueBoolean(bool value);
void refreshValue(DcValue *value);
