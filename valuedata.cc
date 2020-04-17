#include <string>
#include <cstdlib>
#include "basicutils/msg.hh"
#include "types.hh"
#include "valuedata.hh"
#include "string_utils.hh"

ValueData::ValueData() : decval(0), intval(0) { }

ValueData::~ValueData() { }

bool ValueData::operator == (const ValueData &that)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: if (this->decval == that.decval) return true; break;
        case INTEGER_TYPE: if (this->intval == that.intval) return true; break;
        case STRING_TYPE:  if (this->strval == that.strval) return true; break;
    }
    return false;
}

bool ValueData::operator != (const ValueData &that)
{
    return !(*this == that);
}

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
void ValueData::setValue(const ValueData &that)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: this->decval = that.decval; break;
        case INTEGER_TYPE: this->intval = that.intval; break;
        case STRING_TYPE:  this->strval = that.strval; break;
    }
}
void ValueData::setValue(double val) { this->decval = val; }
void ValueData::setValue(int val) { this->intval = val; }

void ValueData::setBoolean(bool input)
{
    switch (this->type)
    {
        case DECIMAL_TYPE:
            if (input) this->decval = 1.0;
            else       this->decval = 0.0;
            break;
        case INTEGER_TYPE:
            if (input) this->intval = 1;
            else       this->intval = 0;
            break;
        case STRING_TYPE:
            if (input) this->strval = "true";
            else       this->strval = "false";
            break;
    }
}

int ValueData::getType(void) { return this->type; }

bool ValueData::getBoolean(void)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: if (this->decval) return true; break;
        case INTEGER_TYPE: if (this->intval) return true; break;
        case STRING_TYPE:  if (StringToBoolean(this->strval)) return true; break;
    }
    return false;
}

double ValueData::getDecimal(void)
{
    switch (this->type)
    {
        case INTEGER_TYPE: return (double)(this->intval);
        case STRING_TYPE:  return StringToDecimal(this->strval);
        default:           return this->decval;
    }
}

int ValueData::getInteger(void)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: return (int)(this->decval);
        case STRING_TYPE:  return StringToInteger(this->strval);
        default:           return this->intval;
    }
}

std::string ValueData::getString(void)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: return std::to_string(this->decval);
        case INTEGER_TYPE: return std::to_string(this->intval);
        default:           return this->strval;
    }
}

void * ValueData::getPointer(void)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: return (void *)(&(this->decval)); break;
        case INTEGER_TYPE: return (void *)(&(this->intval)); break;
        case STRING_TYPE:  return (void *)(&(this->strval)); break;
        default:           return 0x0;
    }
}

bool ValueData::isDecimal(void) { if (this->type == DECIMAL_TYPE) return true; else return false; }
bool ValueData::isInteger(void) { if (this->type == INTEGER_TYPE) return true; else return false; }
bool ValueData::isString(void)  { if (this->type == STRING_TYPE)  return true; else return false; }
