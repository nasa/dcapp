#include "value.h"
#include "utils/string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dc_value_copy(DcValue *dst, DcValue *src) {
    dst->value_boolean = src->value_boolean;
    dst->value_double  = src->value_double;
    dst->value_integer = src->value_integer;
    if (!dst->value_string) {
        dst->value_string = (char *)calloc(sizeof(char), DC_VALUE_STRING_BUFFER_SIZE);
    }
    strncpy(dst->value_string, src->value_string, DC_VALUE_STRING_BUFFER_SIZE - 1);
    dst->value_string[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
}

DcValue dc_value_create_typed_value_from_string(DcValueType type, const char *value_str) {
    DcValue new_value;
    new_value.type         = type;
    new_value.value_string = (char *)calloc(sizeof(char), DC_VALUE_STRING_BUFFER_SIZE);
    dc_value_set_from_string(&new_value, value_str);
    return new_value;
}

DcValue dc_value_create_value_string(const char *value) {
    DcValue new_value;
    new_value.type         = DC_VALUE_TYPE_STRING;
    new_value.value_string = (char *)calloc(sizeof(char), DC_VALUE_STRING_BUFFER_SIZE);
    strncpy(new_value.value_string, value, DC_VALUE_STRING_BUFFER_SIZE - 1);
    new_value.value_string[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
    dc_value_refresh(&new_value);
    return new_value;
}

DcValue dc_value_create_value_integer(int value) {
    DcValue new_value;
    new_value.type          = DC_VALUE_TYPE_INTEGER;
    new_value.value_integer = value;
    new_value.value_string  = (char *)calloc(sizeof(char), DC_VALUE_STRING_BUFFER_SIZE);
    dc_value_refresh(&new_value);
    return new_value;
}

DcValue dc_value_create_value_double(double value) {
    DcValue new_value      = {0};
    new_value.type         = DC_VALUE_TYPE_DOUBLE;
    new_value.value_double = value;
    new_value.value_string = (char *)calloc(sizeof(char), DC_VALUE_STRING_BUFFER_SIZE);
    dc_value_refresh(&new_value);
    return new_value;
}

DcValue dc_value_create_value_boolean(bool value) {
    DcValue new_value;
    new_value.type          = DC_VALUE_TYPE_BOOLEAN;
    new_value.value_boolean = value;
    new_value.value_string  = (char *)calloc(sizeof(char), DC_VALUE_STRING_BUFFER_SIZE);
    dc_value_refresh(&new_value);
    return new_value;
}

void dc_value_refresh(DcValue *value) {
    switch (value->type) {
        case DC_VALUE_TYPE_BOOLEAN:
            value->value_integer = (int)value->value_boolean;
            value->value_boolean = (double)value->value_boolean;
            if (value->value_boolean) {
                strncpy(value->value_string, "True", DC_VALUE_STRING_BUFFER_SIZE - 1);
            } else {
                strncpy(value->value_string, "False", DC_VALUE_STRING_BUFFER_SIZE - 1);
            }
            value->value_string[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
            break;
        case DC_VALUE_TYPE_INTEGER:
            value->value_boolean = (bool)value->value_integer;
            value->value_double  = (double)value->value_integer;
            snprintf(value->value_string, DC_VALUE_STRING_BUFFER_SIZE, "%d", value->value_integer);
            break;
        case DC_VALUE_TYPE_DOUBLE:
            value->value_boolean = (bool)value->value_double;
            value->value_integer = (int)value->value_double;
            snprintf(value->value_string, DC_VALUE_STRING_BUFFER_SIZE, "%f", value->value_double);
            break;
        case DC_VALUE_TYPE_STRING:
            value->value_boolean = dc_utils_string_to_boolean(value->value_string);
            value->value_double  = dc_utils_string_to_double(value->value_string);
            value->value_integer = dc_utils_string_to_integer(value->value_string);
            break;
        default:
            fprintf(stderr, "DCAPP dc_value_refresh(): invalid value type %d\n", value->type);
            break;
    }
}

void dc_value_set_from_string(DcValue *value, const char *value_str) {
    switch (value->type) {
        case DC_VALUE_TYPE_BOOLEAN:
            value->value_boolean = dc_utils_string_to_boolean(value_str);
            break;
        case DC_VALUE_TYPE_DOUBLE:
            value->value_double = dc_utils_string_to_double(value_str);
            break;
        case DC_VALUE_TYPE_INTEGER:
            value->value_integer = dc_utils_string_to_integer(value_str);
            break;
        case DC_VALUE_TYPE_STRING:
            strncpy(value->value_string, value_str, DC_VALUE_STRING_BUFFER_SIZE - 1);
            value->value_string[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
            break;
        default:
            fprintf(stderr, "DCAPP dc_value_set_from_string(): invalid value type %d\n", value->type);
            break;
    }
    dc_value_refresh(value);
}

void *dc_value_get_addr(DcValue *value) {
    switch (value->type) {
        case DC_VALUE_TYPE_BOOLEAN:
            return &(value->value_boolean);
        case DC_VALUE_TYPE_DOUBLE:
            return &(value->value_double);
        case DC_VALUE_TYPE_INTEGER:
            return &(value->value_integer);
        case DC_VALUE_TYPE_STRING:
            return &(value->value_string);
        default:
            fprintf(stderr, "DCAPP dc_value_set_from_string(): invalid value type %d\n", value->type);
            break;
    }
    return NULL;
}

bool dc_value_is_equal(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_BOOLEAN:
            return value1->value_boolean == value2->value_boolean;
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer == value2->value_integer;
        case DC_VALUE_TYPE_DOUBLE:
            return value1->value_double == value2->value_double;
        case DC_VALUE_TYPE_STRING:
            return strcmp(value1->value_string, value2->value_string) == 0;
        default:
            fprintf(stderr, "DCAPP dc_value_set_from_string(): invalid value type %d\n", value1->type);
            break;
    }
    return false;
}

bool dc_value_is_not_equal(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_BOOLEAN:
            return value1->value_boolean != value2->value_boolean;
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer != value2->value_integer;
        case DC_VALUE_TYPE_DOUBLE:
            return value1->value_double != value2->value_double;
        case DC_VALUE_TYPE_STRING:
            return strcmp(value1->value_string, value2->value_string) != 0;
        default:
            fprintf(stderr, "DCAPP dc_value_is_not_equal(): invalid value type %d\n", value1->type);
            break;
    }
    return false;
}

bool dc_value_is_greater(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer > value2->value_integer;
        case DC_VALUE_TYPE_DOUBLE:
            return value1->value_double > value2->value_double;
        default:
            fprintf(stderr, "DCAPP dc_value_is_greater(): invalid value type %d\n", value1->type);
            break;
    }
    return false;
}

bool dc_value_is_greater_or_equal(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer >= value2->value_integer;
        case DC_VALUE_TYPE_DOUBLE:
            return value1->value_double >= value2->value_double;
        default:
            fprintf(stderr, "DCAPP dc_value_is_greater_or_equal(): invalid value type %d\n", value1->type);
            break;
    }
    return false;
}

bool dc_value_is_less(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer < value2->value_integer;
        case DC_VALUE_TYPE_DOUBLE:
            return value1->value_double < value2->value_double;
        default:
            fprintf(stderr, "DCAPP dc_value_is_less(): invalid value type %d\n", value1->type);
            break;
    }
    return false;
}

bool dc_value_is_less_or_equal(DcValue *value1, DcValue *value2) {
    switch (value1->type) {
        case DC_VALUE_TYPE_INTEGER:
            return value1->value_integer <= value2->value_integer;
        case DC_VALUE_TYPE_DOUBLE:
            return value1->value_double <= value2->value_double;
        default:
            fprintf(stderr, "DCAPP dc_value_is_less_or_equal(): invalid value type %d\n", value1->type);
            break;
    }
    return false;
}
