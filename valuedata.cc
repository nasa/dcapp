#include <string>
#include <cstdlib>
#include "basicutils/msg.hh"
#include "types.hh"
#include "valuedata.hh"

ValueData::ValueData() : decval(0), intval(0) { }

ValueData::~ValueData() { }

// Probably don't need two of these routines...
void ValueData::setType(int type_spec) { this->type = type_spec; } // Should probably verify a valid input type here
void ValueData::setType(const char *type_spec)
{
    if (!strcmp(type_spec, "Float"))
    {
        this->type = DECIMAL_TYPE;
    }
    else if (!strcmp(type_spec, "Integer"))
    {
        this->type = INTEGER_TYPE;
    }
    else if (!strcmp(type_spec, "String"))
    {
        this->type = STRING_TYPE;
    }
    else
    {
        this->type = UNDEFINED_TYPE;
        error_msg("Attempting to create variable with an unknown type: " << type_spec);
// Should probably pass back an error here
    }
}

// Probably don't need two of these routines...
void ValueData::setValue(const char *input)
{
    if (input)
    {
        switch (this->type)
        {
        case DECIMAL_TYPE:
            this->decval = strtod(input, 0x0);
            break;
        case INTEGER_TYPE:
            this->intval = (int)strtol(input, 0x0, 10);
            break;
        case STRING_TYPE:
            this->strval = input;
            break;
        }
    }
}
void ValueData::setValue(const char *input, unsigned length)
{
    switch (this->type)
    {
    case DECIMAL_TYPE:
        this->decval = strtod(input, 0x0);
        break;
    case INTEGER_TYPE:
        this->intval = (int)strtol(input, 0x0, 10);
        break;
    case STRING_TYPE:
        if (length >= this->strval.max_size()) length = this->strval.max_size() - 1;
        this->strval.clear();
        for (unsigned i=0; i<length; i++) this->strval += input[i];
        break;
    }
}

int ValueData::getType(void) { return this->type; }

void * ValueData::getPointer(void)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: return (void *)(&(this->decval));
        case INTEGER_TYPE: return (void *)(&(this->intval));
        case STRING_TYPE:  return (void *)(&(this->strval));
        default:           return 0x0;
    }
}
