// dcapp includes
#include <utils/string.hpp>
#include <value.hpp>

// library includes

// c++ standard includes
#include <stdexcept>

DcValueType dc_value_type_from_string(const std::string &type)
{
    if (type == "Decimal" || type == "Float" || type == "Double")
        return DC_APP_VALUE_TYPE_FLOAT;
    else if (type == "Integer")
        return DC_APP_VALUE_TYPE_INTEGER;
    else if (type == "String")
        return DC_APP_VALUE_TYPE_STRING;
    else if (type == "Boolean")
        return DC_APP_VALUE_TYPE_BOOLEAN;
    return DC_APP_VALUE_TYPE_UNDEFINED;
}

DcValue dc_value_create_typed_value_from_string(DcValueType type, const std::string &value)
{
    DcValue newValue = (DcValue){
        .type = type,
        .isDynamic = false,
    };

    switch (type)
    {
    case DC_APP_VALUE_TYPE_BOOLEAN:
        newValue.valueBoolean = dc_utils_string_to_boolean(value);
        break;
    case DC_APP_VALUE_TYPE_FLOAT:
        newValue.valueFloat = dc_utils_string_to_float(value);
        break;
    case DC_APP_VALUE_TYPE_INTEGER:
        newValue.valueInteger = dc_utils_string_to_integer(value);
        break;
    case DC_APP_VALUE_TYPE_STRING:
        newValue.valueString = value;
        break;
    default:
        throw std::runtime_error("Invalid DcValue type");
        break;
    }
    dc_value_refresh_value(&newValue);
    return newValue;
}

DcValue dc_value_create_value_string(const std::string &value)
{
    DcValue newValue = (DcValue){
        .type = DC_APP_VALUE_TYPE_STRING,
        .valueString = value,
        .isDynamic = false,
    };
    dc_value_refresh_value(&newValue);
    return newValue;
}

DcValue dc_value_create_value_integer(int value)
{
    DcValue newValue = (DcValue){
        .type = DC_APP_VALUE_TYPE_INTEGER,
        .valueInteger = value,
        .isDynamic = false,
    };
    dc_value_refresh_value(&newValue);
    return newValue;
}

DcValue dc_value_create_value_float(float value)
{
    DcValue newValue = (DcValue){
        .type = DC_APP_VALUE_TYPE_FLOAT,
        .valueFloat = value,
        .isDynamic = false,
    };
    dc_value_refresh_value(&newValue);
    return newValue;
}

DcValue dc_value_create_value_boolean(bool value)
{
    DcValue newValue = (DcValue){
        .type = DC_APP_VALUE_TYPE_BOOLEAN,
        .valueBoolean = value,
        .isDynamic = false,
    };
    dc_value_refresh_value(&newValue);
    return newValue;
}

void dc_value_refresh_value(DcValue *value)
{
    switch (value->type)
    {
    case DC_APP_VALUE_TYPE_BOOLEAN:
        value->valueInteger = (int)value->valueBoolean;
        value->valueBoolean = (double)value->valueBoolean;
        value->valueString = value->valueBoolean ? "true" : "false";
        break;
    case DC_APP_VALUE_TYPE_INTEGER:
        value->valueBoolean = (bool)value->valueInteger;
        value->valueFloat = (double)value->valueInteger;
        value->valueString = std::to_string(value->valueInteger);
        break;
    case DC_APP_VALUE_TYPE_FLOAT:
        value->valueBoolean = (bool)value->valueFloat;
        value->valueInteger = (int)value->valueFloat;
        value->valueString = std::to_string(value->valueFloat);
        break;
    case DC_APP_VALUE_TYPE_STRING:
        value->valueBoolean = dc_utils_string_to_boolean(value->valueString);
        value->valueFloat = dc_utils_string_to_float(value->valueString);
        value->valueInteger = dc_utils_string_to_integer(value->valueString);
        break;
    default:
        throw std::runtime_error("Invalid value tyep");
    }
}
