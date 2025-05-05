// dcapp includes
#include <value.hpp>
#include <utils/string-utils.hpp>

// library includes

// c++ standard includes

DcValueType valueTypeFromString(const std::string &type)
{
    if (type == "Decimal" || type == "Float" || type == "Double")
        return DC_VALUE_TYPE_FLOAT;
    else if (type == "Integer")
        return DC_VALUE_TYPE_INTEGER;
    else if (type == "String")
        return DC_VALUE_TYPE_STRING;
    else if (type == "Boolean")
        return DC_VALUE_TYPE_BOOLEAN;
    return DC_VALUE_TYPE_UNDEFINED;
}

DcValue createValueFromString(DcValueType type, const std::string &value)
{
    DcValue newValue = (DcValue){
        .type = type,
        .isDynamic = false,
    };

    switch (type)
    {
    case DC_VALUE_TYPE_BOOLEAN:
        newValue.valueBoolean = stringToBoolean(value);
        break;
    case DC_VALUE_TYPE_FLOAT:
        newValue.valueFloat = stringToFloat(value);
        break;
    case DC_VALUE_TYPE_INTEGER:
        newValue.valueInteger = stringToInteger(value);
        break;
    case DC_VALUE_TYPE_STRING:
        newValue.valueString = value;
        break;
    }
    refreshValue(&newValue);
    return newValue;
}

DcValue createValueString(const std::string &value)
{
    DcValue newValue = (DcValue){
        .type = DC_VALUE_TYPE_STRING,
        .valueString = value,
        .isDynamic = false,
    };
    refreshValue(&newValue);
    return newValue;
}

DcValue createValueInteger(int value)
{
    DcValue newValue = (DcValue){
        .type = DC_VALUE_TYPE_INTEGER,
        .valueInteger = value,
        .isDynamic = false,
    };
    refreshValue(&newValue);
    return newValue;
}

DcValue createValueFloat(float value)
{
    DcValue newValue = (DcValue){
        .type = DC_VALUE_TYPE_FLOAT,
        .valueFloat = value,
        .isDynamic = false,
    };
    refreshValue(&newValue);
    return newValue;
}

DcValue createValueBoolean(bool value)
{
    DcValue newValue = (DcValue){
        .type = DC_VALUE_TYPE_BOOLEAN,
        .valueBoolean = value,
        .isDynamic = false,
    };
    refreshValue(&newValue);
    return newValue;
}

void refreshValue(DcValue *value)
{
    switch (value->type)
    {
    case DC_VALUE_TYPE_BOOLEAN:
        value->valueInteger = (int)value->valueBoolean;
        value->valueBoolean = (double)value->valueBoolean;
        value->valueString = value->valueBoolean ? "true" : "false";
        break;
    case DC_VALUE_TYPE_INTEGER:
        value->valueBoolean = (bool)value->valueInteger;
        value->valueFloat = (double)value->valueInteger;
        value->valueString = std::to_string(value->valueInteger);
        break;
    case DC_VALUE_TYPE_FLOAT:
        value->valueBoolean = (bool)value->valueFloat;
        value->valueInteger = (int)value->valueFloat;
        value->valueString = std::to_string(value->valueFloat);
        break;
    case DC_VALUE_TYPE_STRING:
        value->valueBoolean = stringToBoolean(value->valueString);
        value->valueFloat = stringToFloat(value->valueString);
        value->valueInteger = stringToInteger(value->valueString);
        break;
    }
}
