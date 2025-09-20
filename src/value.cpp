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
    DcValue new_value = (DcValue){
        .type = type,
        .is_dynamic = false,
    };

    switch (type)
    {
    case DC_APP_VALUE_TYPE_BOOLEAN:
        new_value.value_boolean = dc_utils_string_to_boolean(value);
        break;
    case DC_APP_VALUE_TYPE_FLOAT:
        new_value.value_float = dc_utils_string_to_float(value);
        break;
    case DC_APP_VALUE_TYPE_INTEGER:
        new_value.value_integer = dc_utils_string_to_integer(value);
        break;
    case DC_APP_VALUE_TYPE_STRING:
        new_value.value_string = value;
        break;
    default:
        throw std::runtime_error("Invalid DcValue type");
        break;
    }
    dc_value_refresh_value(&new_value);
    return new_value;
}

DcValue dc_value_create_value_string(const std::string &value)
{
    DcValue new_value = (DcValue){
        .type = DC_APP_VALUE_TYPE_STRING,
        .value_string = value,
        .is_dynamic = false,
    };
    dc_value_refresh_value(&new_value);
    return new_value;
}

DcValue dc_value_create_value_integer(int value)
{
    DcValue new_value = (DcValue){
        .type = DC_APP_VALUE_TYPE_INTEGER,
        .value_integer = value,
        .is_dynamic = false,
    };
    dc_value_refresh_value(&new_value);
    return new_value;
}

DcValue dc_value_create_value_float(float value)
{
    DcValue new_value = (DcValue){
        .type = DC_APP_VALUE_TYPE_FLOAT,
        .value_float = value,
        .is_dynamic = false,
    };
    dc_value_refresh_value(&new_value);
    return new_value;
}

DcValue dc_value_create_value_boolean(bool value)
{
    DcValue new_value = (DcValue){
        .type = DC_APP_VALUE_TYPE_BOOLEAN,
        .value_boolean = value,
        .is_dynamic = false,
    };
    dc_value_refresh_value(&new_value);
    return new_value;
}

void dc_value_refresh_value(DcValue *value)
{
    switch (value->type)
    {
    case DC_APP_VALUE_TYPE_BOOLEAN:
        value->value_integer = (int)value->value_boolean;
        value->value_boolean = (double)value->value_boolean;
        value->value_string = value->value_boolean ? "true" : "false";
        break;
    case DC_APP_VALUE_TYPE_INTEGER:
        value->value_boolean = (bool)value->value_integer;
        value->value_float = (double)value->value_integer;
        value->value_string = std::to_string(value->value_integer);
        break;
    case DC_APP_VALUE_TYPE_FLOAT:
        value->value_boolean = (bool)value->value_float;
        value->value_integer = (int)value->value_float;
        value->value_string = std::to_string(value->value_float);
        break;
    case DC_APP_VALUE_TYPE_STRING:
        value->value_boolean = dc_utils_string_to_boolean(value->value_string);
        value->value_float = dc_utils_string_to_float(value->value_string);
        value->value_integer = dc_utils_string_to_integer(value->value_string);
        break;
    default:
        throw std::runtime_error("Invalid value tyep");
    }
}
