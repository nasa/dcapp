#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include "basicutils/msg.hh"
#include "types.hh"
#include "valuedata.hh"
#include "constants.hh"

static std::map<std::string, ValueData> varMap;

void varlist_append(const char *paramname, const char *typestr, const char *initval)
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

    ValueData *vinfo = new ValueData;
    vinfo->setType(typestr);
    vinfo->setValue(initval);

//include some kind of error here if there's a problem...
// error_msg("Attempting to create the variable \"" << paramname << "\" with an unknown type: " << typestr);

    varMap[std::string(mylabel)] = *vinfo;
}

ValueData *getVariableValue(const char *label)
{
    if (!label) return 0x0;

    const char *mylabel;

    if (label[0] == '@') mylabel = &label[1];
    else mylabel = label;

    if (varMap.find(mylabel) != varMap.end()) return &(varMap[mylabel]);
    else
    {
        error_msg("Invalid parameter label: " << label);
        return 0x0;
    }
}

void *get_pointer(const char *label)
{
    ValueData *myvar = getVariableValue(label);
    if (myvar) return myvar->getPointer();
    else return 0x0;
}

char *create_virtual_variable(const char *typestr, const char *initval)
{
    static unsigned id_count = 0;
    char *vname;
    asprintf(&vname, "@dcappVirtualVariable%u", id_count);
    varlist_append(vname, typestr, initval);
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

ValueData *getValueData(const char *valstr)
{
    if (check_dynamic_element(valstr)) return getVariableValue(valstr);
    else return getConstantValue(valstr);
}
