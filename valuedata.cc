#include <string>
#include <cstdlib>
#include "basicutils/msg.hh"
#include "types.hh"
#include "valuedata.hh"
#include "string_utils.hh"

Value::Value() : decval(0), intval(0) { }
Value::~Value() { }



Constant::Constant() : boolval(false) { }
Constant::~Constant() { }

bool Constant::operator == (const Constant &that)
{
    if (this->decval == that.decval && this->intval == that.intval && this->strval == that.strval && this->boolval == that.boolval) return true;
    else return false;
}

bool Constant::operator != (const Constant &that)
{
    return !(*this == that);
}

void Constant::setToDecimal(double val)
{
    this->decval = val;
    this->intval = (int)val;
    this->strval = std::to_string(val);
    if (val) this->boolval = true;
    else this->boolval = false;
}

void Constant::setToInteger(int val)
{
    this->decval = (double)val;
    this->intval = val;
    this->strval = std::to_string(val);
    if (val) this->boolval = true;
    else this->boolval = false;
}

void Constant::setToCharstr(const char *val)
{
    if (val)
    {
        this->decval = strtod(val, 0x0);
        this->intval = (int)strtol(val, 0x0, 10);
        this->strval = val;
        this->boolval = StringToBoolean(val);
    }
}

void Constant::setToBoolean(bool val)
{
    if (val)
    {
        this->decval = 1.0;
        this->intval = 1;
        this->strval = "true";
    }
    else
    {
        this->decval = 0.0;
        this->intval = 0;
        this->strval = "false";
    }

    this->boolval = val;
}

double Constant::getDecimal(void) { return this->decval; }
int Constant::getInteger(void) { return this->intval; }
std::string Constant::getString(void) { return this->strval; }
bool Constant::getBoolean(void) { return this->boolval; }



Variable::Variable() : type(UNDEFINED_TYPE) { }
Variable::~Variable() { }

bool Variable::operator == (const Variable &that)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: if (this->decval == that.decval) return true; break;
        case INTEGER_TYPE: if (this->intval == that.intval) return true; break;
        case STRING_TYPE:  if (this->strval == that.strval) return true; break;
    }
    return false;
}

bool Variable::operator != (const Variable &that)
{
    return !(*this == that);
}

void Variable::setToCharstr(const char *input, unsigned length)
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
            if (length)
            {
                if (length >= this->strval.max_size()) length = this->strval.max_size() - 1;
                this->strval.clear();
                for (unsigned i=0; i<length; i++) this->strval += input[i];
            }
            else this->strval = input;
            break;
        }
    }
}
void Variable::setToValue(Value &that)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: this->decval = that.getDecimal(); break;
        case INTEGER_TYPE: this->intval = that.getInteger(); break;
        case STRING_TYPE:  this->strval = that.getString(); break;
    }
}
void Variable::setToDecimal(double val) { this->decval = val; }
//void Variable::setToInteger(int val) { this->intval = val; }
//void Variable::setToString(std::string val) { this->strval = val; }
void Variable::setToBoolean(bool input)
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

double Variable::getDecimal(void)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: return this->decval;
        case INTEGER_TYPE: return (double)(this->intval);
        case STRING_TYPE:  return StringToDecimal(this->strval);
        default:           return 0.0;
    }
}

int Variable::getInteger(void)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: return (int)(this->decval);
        case INTEGER_TYPE: return this->intval;
        case STRING_TYPE:  return StringToInteger(this->strval);
        default:           return 0;
    }
}

std::string Variable::getString(void)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: return std::to_string(this->decval);
        case INTEGER_TYPE: return std::to_string(this->intval);
        case STRING_TYPE:  return this->strval;
        default:           return "";
    }
}

bool Variable::getBoolean(void)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: if (this->decval) return true; break;
        case INTEGER_TYPE: if (this->intval) return true; break;
        case STRING_TYPE:  if (StringToBoolean(this->strval)) return true; break;
    }
    return false;
}

// Probably don't need two of these routines...
void Variable::setType(int type_spec) { this->type = type_spec; } // Should probably verify a valid input type here
void Variable::setType(const char *type_spec)
{
    if (!strcmp(type_spec, "Decimal") || !strcmp(type_spec, "Float"))
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

int Variable::getType(void) { return this->type; }

void * Variable::getPointer(void)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: return (void *)(&(this->decval)); break;
        case INTEGER_TYPE: return (void *)(&(this->intval)); break;
        case STRING_TYPE:  return (void *)(&(this->strval)); break;
default:  return (void *)(&(this->strval)); break;
//        default:           return 0x0;
// this seems weird.  probably shouldn't be a default getPointer, but it's needed in pixelstream.cc
    }
}

bool Variable::isDecimal(void) { if (this->type == DECIMAL_TYPE) return true; else return false; }
bool Variable::isInteger(void) { if (this->type == INTEGER_TYPE) return true; else return false; }
bool Variable::isString(void)  { if (this->type == STRING_TYPE)  return true; else return false; }
