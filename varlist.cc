#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include "types.hh"
#include "valuedata.hh"
#include "basicutils/msg.hh"

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

ValueData * getValue(const char *label)
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
    if (!label) return 0x0;

    const char *mylabel;

    if (label[0] == '@') mylabel = &label[1];
    else mylabel = label;

    if (varMap.find(mylabel) != varMap.end()) return varMap[mylabel].getPointer();
    else
    {
        error_msg("Invalid parameter label: " << label);
        return 0x0;
    }
}
