#include <string>
#include <cstdlib>
#include <cstring>
#include "basicutils/msg.hh"
#include "basicutils/stringutils.hh"
#include "valuedata.hh"



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
    this->strval = DecimalToString(val);
    if (val) this->boolval = true;
    else this->boolval = false;
}

void Constant::setToInteger(int val)
{
    this->decval = (double)val;
    this->intval = val;
    this->strval = IntegerToString(val);
    if (val) this->boolval = true;
    else this->boolval = false;
}

void Constant::setToCharstr(const char *val)
{
    if (val)
    {
        this->decval = StringToDecimal(val);
        this->intval = StringToInteger(val);
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

unsigned Constant::compareToValue(Value &that)
{
    // evaluate as String by default
    if (this->strval > that.getString()) return isGreaterThan;
    else if (this->strval < that.getString()) return isLessThan;
    else return isEqual;
}

double Constant::getDecimal(void) { return this->decval; }
int Constant::getInteger(void) { return this->intval; }
std::string Constant::getString(std::string) { return this->strval; }
bool Constant::getBoolean(void) { return this->boolval; }

bool Constant::isConstant(void) { return true; };



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

void Variable::setToCharstr(const char *input)
{
    if (input)
    {
        switch (this->type)
        {
            case DECIMAL_TYPE: this->decval = StringToDecimal(input); break;
            case INTEGER_TYPE: this->intval = StringToInteger(input); break;
            case STRING_TYPE:  this->strval = input;                  break;
        }
    }
}

void Variable::setToString(std::string &input)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: this->decval = StringToDecimal(input); break;
        case INTEGER_TYPE: this->intval = StringToInteger(input); break;
        case STRING_TYPE:  this->strval = input;                  break;
    }
}

void Variable::setToDecimal(double val) { this->decval = val; }
void Variable::setToInteger(int val) { this->intval = val; }

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

void Variable::setToValue(Value &that)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: this->decval = that.getDecimal(); break;
        case INTEGER_TYPE: this->intval = that.getInteger(); break;
        case STRING_TYPE:  this->strval = that.getString();  break;
    }
}

void Variable::incrementByValue(Value &that)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: this->decval += that.getDecimal(); break;
        case INTEGER_TYPE: this->intval += that.getInteger(); break;
        case STRING_TYPE:  this->strval += that.getString();  break;
    }
}

void Variable::decrementByValue(Value &that)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: this->decval -= that.getDecimal(); break;
        case INTEGER_TYPE: this->intval -= that.getInteger(); break;
        // There is no good way to decrement a String variable
    }
}

void Variable::applyMinimumByValue(Value &that)
{
    switch (this->type)
    {
        case DECIMAL_TYPE:
            if (this->decval < that.getDecimal()) this->decval = that.getDecimal();
            break;
        case INTEGER_TYPE:
            if (this->intval < that.getInteger()) this->intval = that.getInteger();
            break;
        // There is no good way to bound a String variable
    }
}

void Variable::applyMaximumByValue(Value &that)
{
    switch (this->type)
    {
        case DECIMAL_TYPE:
            if (this->decval > that.getDecimal()) this->decval = that.getDecimal();
            break;
        case INTEGER_TYPE:
            if (this->intval > that.getInteger()) this->intval = that.getInteger();
            break;
        // There is no good way to bound a String variable
    }
}

unsigned Variable::compareToValue(Value &that)
{
    switch (this->type)
    {
        case DECIMAL_TYPE:
            if (this->decval > that.getDecimal()) return isGreaterThan;
            else if (this->decval < that.getDecimal()) return isLessThan;
            else return isEqual;
        case INTEGER_TYPE:
            if (this->intval > that.getInteger()) return isGreaterThan;
            else if (this->intval < that.getInteger()) return isLessThan;
            else return isEqual;
        case STRING_TYPE:
            if (this->strval > that.getString()) return isGreaterThan;
            else if (this->strval < that.getString()) return isLessThan;
            else return isEqual;
    }
    return isEqual; // undefined behavior if type isn't defined
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

std::string Variable::getString(std::string format)
{
    if (format.empty())
    {
        switch (this->type)
        {
            case DECIMAL_TYPE:
                return DecimalToString(this->decval);
            case INTEGER_TYPE:
                return IntegerToString(this->intval);
            case STRING_TYPE:
                return this->strval;
            default:
                return "";
        }
    }
    else
    {
        char *tmp_str = 0x0;

        switch (this->type)
        {
            case DECIMAL_TYPE:
                asprintf(&tmp_str, format.c_str(), this->decval);
                break;
            case INTEGER_TYPE:
                asprintf(&tmp_str, format.c_str(), this->intval);
                break;
            case STRING_TYPE:
                asprintf(&tmp_str, format.c_str(), this->strval.c_str());
                break;
            default:
                return "";
        }

        if (tmp_str)
        {
            std::string ret_str = tmp_str;
            free(tmp_str);
            return ret_str;
        }
        else return "";
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

void Variable::setType(int type_spec) { this->type = type_spec; }

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
        this->type = STRING_TYPE;
        warning_msg("Attempting to create variable with unknown type (" << type_spec << ") - assuming \"String\"");
    }
}

void Variable::setAttributes(Variable &that) { this->type = that.type; }

void * Variable::getPointer(void)
{
    switch (this->type)
    {
        case DECIMAL_TYPE: return (void *)(&(this->decval));
        case INTEGER_TYPE: return (void *)(&(this->intval));
        case STRING_TYPE:  return (void *)(&(this->strval));
        default:           return 0x0;
    }
}

bool Variable::isDecimal(void) { if (this->type == DECIMAL_TYPE) return true; else return false; }
bool Variable::isInteger(void) { if (this->type == INTEGER_TYPE) return true; else return false; }
bool Variable::isString(void)  { if (this->type == STRING_TYPE)  return true; else return false; }

bool Variable::isVariable(void) { return true; };
