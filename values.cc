#include <cstring>
#include "constants.hh"
#include "variables.hh"
#include "values.hh"

bool check_dynamic_element(const char *spec)
{
    if (spec)
    {
        if (strlen(spec) > 1)
        {
            if (spec[0] == '@') return true;
        }
    }
    return false;
}

Value *getValue(const char *valstr)
{
    Value *retval = 0x0;

    if (check_dynamic_element(valstr)) retval = getVariable(valstr);

    if (retval) return retval;
    else return getConstantFromCharstr(valstr);
}


Value::Value() : decval(0), intval(0) { }
Value::~Value() { }
