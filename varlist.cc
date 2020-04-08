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
    const char *mylabel;

    if (paramname[0] == '@') mylabel = &paramname[1];
    else mylabel = paramname;

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

    ValueData *vinfo = new ValueData;
    vinfo->setType(typestr);
    vinfo->setValue(initval);

//include some kind of error here if there's a problem...
// error_msg("Attempting to create the variable \"" << paramname << "\" with an unknown type: " << typestr);

    varMap[std::string(mylabel)] = *vinfo;
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

int get_datatype(const char *label)
{
    if (!label) return UNDEFINED_TYPE;

    const char *mylabel;

    if (label[0] == '@') mylabel = &label[1];
    else mylabel = label;

    if (varMap.find(mylabel) != varMap.end()) return varMap[mylabel].getType();
    else
    {
        error_msg("Invalid parameter label: " << label);
        return UNDEFINED_TYPE;
    }
}
