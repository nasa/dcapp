#include <list>
#include <string>
#include "basicutils/stringutils.hh"
#include "values.hh"
#include "constants.hh"

static std::list<Constant> constants;

static Constant *findOrRegisterConstant(Constant &vinfo)
{
    std::list<Constant>::iterator it;
    for (it = constants.begin(); it != constants.end(); it++)
    {
        if (*it == vinfo) return &(*it);
    }
    constants.push_back(vinfo);
    return &(constants.back());
}

Constant *getConstantFromDecimal(double inval)
{
    Constant vinfo;
    vinfo.setToDecimal(inval);
    return findOrRegisterConstant(vinfo);
}

Constant *getConstantFromInteger(int inval)
{
    Constant vinfo;
    vinfo.setToInteger(inval);
    return findOrRegisterConstant(vinfo);
}

Constant *getConstantFromCharstr(const char *inval)
{
    Constant vinfo;
    vinfo.setToCharstr(inval);
    return findOrRegisterConstant(vinfo);
}

Constant *getConstantFromBoolean(bool inval)
{
    Constant vinfo;
    vinfo.setToBoolean(inval);
    return findOrRegisterConstant(vinfo);
}


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
