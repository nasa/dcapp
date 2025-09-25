// dcapp includes
#include <utils/string.hpp>
#include <value.hpp>

// library includes

// c++ standard includes
#include <stdexcept>

// Q: why do we have this function?
// A: in case we want to move to C eventually..or more simply,
//    to remove the std::string dependency
void dc_value_copy(DcValue *dst, DcValue *src) {
    *dst = *src;
}

DcValueType dc_value_type_from_string(const std::string &type) {
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

DcValue dc_value_create_typed_value_from_string(DcValueType type, const std::string &value) {
    DcValue new_value = (DcValue){
        .type       = type,
        .is_dynamic = false,
    };

    dc_value_set_from_string(&new_value, value);
    return new_value;
}

DcValue dc_value_create_value_string(const std::string &value) {
    DcValue new_value = (DcValue){
        .type         = DC_VALUE_TYPE_STRING,
        .value_string = value,
        .is_dynamic   = false,
    };
    dc_value_refresh(&new_value);
    return new_value;
}

DcValue dc_value_create_value_integer(int value) {
    DcValue new_value = (DcValue){
        .type          = DC_VALUE_TYPE_INTEGER,
        .value_integer = value,
        .is_dynamic    = false,
    };
    dc_value_refresh(&new_value);
    return new_value;
}

DcValue dc_value_create_value_float(float value) {
    DcValue new_value = (DcValue){
        .type        = DC_VALUE_TYPE_FLOAT,
        .value_float = value,
        .is_dynamic  = false,
    };
    dc_value_refresh(&new_value);
    return new_value;
}

DcValue dc_value_create_value_boolean(bool value) {
    DcValue new_value = (DcValue){
        .type          = DC_VALUE_TYPE_BOOLEAN,
        .value_boolean = value,
        .is_dynamic    = false,
    };
    dc_value_refresh(&new_value);
    return new_value;
}

void dc_value_refresh(DcValue *value) {
    switch (value->type) {
        case DC_VALUE_TYPE_BOOLEAN:
            value->value_integer = (int)value->value_boolean;
            value->value_boolean = (double)value->value_boolean;
            value->value_string  = value->value_boolean ? "True" : "False";
            break;
        case DC_VALUE_TYPE_INTEGER:
            value->value_boolean = (bool)value->value_integer;
            value->value_float   = (double)value->value_integer;
            value->value_string  = std::to_string(value->value_integer);
            break;
        case DC_VALUE_TYPE_FLOAT:
            value->value_boolean = (bool)value->value_float;
            value->value_integer = (int)value->value_float;
            value->value_string  = std::to_string(value->value_float);
            break;
        case DC_VALUE_TYPE_STRING:
            value->value_boolean = dc_utils_string_to_boolean(value->value_string);
            value->value_float   = dc_utils_string_to_float(value->value_string);
            value->value_integer = dc_utils_string_to_integer(value->value_string);
            break;
        default:
            throw std::runtime_error("Invalid value tyep");
    }
}

void dc_value_set_from_string(DcValue *value, const std::string &string_value) {
    switch (value->type) {
        case DC_VALUE_TYPE_BOOLEAN:
            value->value_boolean = dc_utils_string_to_boolean(string_value);
            break;
        case DC_VALUE_TYPE_FLOAT:
            value->value_float = dc_utils_string_to_float(string_value);
            break;
        case DC_VALUE_TYPE_INTEGER:
            value->value_integer = dc_utils_string_to_integer(string_value);
            break;
        case DC_VALUE_TYPE_STRING:
            value->value_string = string_value;
            break;
        default:
            throw std::runtime_error("Invalid DcValue type");
            break;
    }
    dc_value_refresh(value);
}

bool dc_value_is_equal(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_BOOLEAN:
            return value1->value_boolean == value2->value_boolean;
            break;
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer == value2->value_integer;
            break;
        case DC_VALUE_TYPE_FLOAT:
            return value1->value_float == value2->value_float;
            break;
        case DC_VALUE_TYPE_STRING:
            return value1->value_string == value2->value_string;
            break;
        default:
            throw std::runtime_error("Invalid value tyep");
    }
}

bool dc_value_is_not_equal(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_BOOLEAN:
            return value1->value_boolean != value2->value_boolean;
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer != value2->value_integer;
        case DC_VALUE_TYPE_FLOAT:
            return value1->value_float != value2->value_float;
        case DC_VALUE_TYPE_STRING:
            return value1->value_string != value2->value_string;
        default:
            throw std::runtime_error("Invalid value type");
    }
}

bool dc_value_is_greater(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer > value2->value_integer;
        case DC_VALUE_TYPE_FLOAT:
            return value1->value_float > value2->value_float;
        default:
            throw std::runtime_error("Invalid type for greater-than comparison: " + std::to_string(value1->type));
    }
}

bool dc_value_is_greater_or_equal(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer >= value2->value_integer;
        case DC_VALUE_TYPE_FLOAT:
            return value1->value_float >= value2->value_float;
        default:
            throw std::runtime_error("Invalid type for greater-than comparison: " + std::to_string(value1->type));
    }
}

bool dc_value_is_less(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer < value2->value_integer;
        case DC_VALUE_TYPE_FLOAT:
            return value1->value_float < value2->value_float;
        default:
            throw std::runtime_error("Invalid type for greater-than comparison: " + std::to_string(value1->type));
    }
}

bool dc_value_is_less_or_equal(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer <= value2->value_integer;
        case DC_VALUE_TYPE_FLOAT:
            return value1->value_float <= value2->value_float;
        default:
            throw std::runtime_error("Invalid type for greater-than comparison: " + std::to_string(value1->type));
    }
}
