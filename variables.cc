#include <map>
#include <string>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "basicutils/msg.hh"
#include "basicutils/stringutils.hh"
#include "values.hh"
#include "variables.hh"

static std::map<std::string, Variable> variables;

void registerVariable(const std::string &paramname, const std::string &typestr, const std::string &initval)
{
    if (paramname.empty())
    {
        error_msg("Attempting to create a variable without a name");
        return;
    }

    if (typestr.empty())
    {
        error_msg("Attempting to create the variable \"" << paramname << "\" without a specified type");
        return;
    }

    std::string mylabel;

    if (paramname[0] == '@') mylabel = paramname.substr(1);
    else mylabel = paramname;

    Variable *vinfo = new Variable;
    vinfo->setType(typestr);
    vinfo->setToString(initval);

    variables[mylabel] = *vinfo;
}

Variable *getVariable(const std::string &label)
{
    if (label.empty()) return 0x0;

    if (label[0] == '@')
    {
        std::string mylabel = label.substr(1);
        if (variables.find(mylabel) != variables.end()) return &(variables[mylabel]);
    }
    else
    {
        if (variables.find(label) != variables.end()) return &(variables[label]);
    }

    warning_msg("Invalid variable label: " << label);
    return 0x0;
}

// get_pointer should only be used in the auto-generated dcapp header files used by the logic plug-ins.  Since they're
// auto-generated, this routine should never return 0x0.  But if it does return 0x0, a crash could occur.
void *get_pointer(const char *label)
{
    Variable *myvar = getVariable(label);
    if (myvar) return myvar->getPointer();
    else return 0x0;
}

std::string create_virtual_variable(const char *typestr, const char *initval)
{
    static unsigned id_count = 0;
    std::string myvar = "@dcappVirtualVariable" + std::to_string(id_count);
    registerVariable(myvar, typestr, initval);
    id_count++;
    return myvar;
}


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

void Variable::setToString(const std::string &input)
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

std::string Variable::getString(std::string format, double zeroTrim)
{
    double dval;

    if (zeroTrim && (fabs(this->decval) < fabs(zeroTrim))) dval = 0;
    else dval = this->decval;

    if (format.empty())
    {
        switch (this->type)
        {
            case DECIMAL_TYPE:
                return ConvertToString(dval);
            case INTEGER_TYPE:
                return ConvertToString(this->intval);
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
                asprintf(&tmp_str, format.c_str(), dval);
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

void Variable::setType(const std::string &type_spec)
{
    if (type_spec == "Decimal" || type_spec == "Float")
    {
        this->type = DECIMAL_TYPE;
    }
    else if (type_spec == "Integer")
    {
        this->type = INTEGER_TYPE;
    }
    else if (type_spec == "String")
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
