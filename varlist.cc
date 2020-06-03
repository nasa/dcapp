#include <cstdio> // for asprintf
#include <cstring> // for strlen
#include <map>
#include <string>
#include "basicutils/msg.hh"
#include "valuedata.hh"
#include "constants.hh"

static std::map<std::string, Variable> variables;

void registerVariable(const char *paramname, const char *typestr, const char *initval)
{
    if (!paramname)
    {
        error_msg("Attempting to create a variable without a name");
        return;
    }

    if (!typestr)
    {
        error_msg("Attempting to create the variable \"" << paramname << "\" without a specified type");
        return;
    }

    const char *mylabel;

    if (paramname[0] == '@') mylabel = &paramname[1];
    else mylabel = paramname;

    Variable *vinfo = new Variable;
    vinfo->setType(typestr);
    vinfo->setToCharstr(initval);

    variables[std::string(mylabel)] = *vinfo;
}

Variable *getVariable(const char *label)
{
    if (!label) return 0x0;

    const char *mylabel;

    if (label[0] == '@') mylabel = &label[1];
    else mylabel = label;

    if (variables.find(mylabel) != variables.end()) return &(variables[mylabel]);
    else
    {
        warning_msg("Invalid variable label: " << label);
        return 0x0;
    }
}

// get_pointer should only be used in the auto-generated dcapp header files used by the logic plug-ins.  Since they're
// auto-generated, this routine should never return 0x0.  But if it does return 0x0, a crash could occur.
void *get_pointer(const char *label)
{
    Variable *myvar = getVariable(label);
    if (myvar) return myvar->getPointer();
    else return 0x0;
}

char *create_virtual_variable(const char *typestr, const char *initval)
{
    static unsigned id_count = 0;
    char *vname;
    asprintf(&vname, "@dcappVirtualVariable%u", id_count);
    registerVariable(vname, typestr, initval);
    id_count++;
    return vname;
}

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
