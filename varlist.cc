#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include "types.hh"
#include "basicutils/msg.hh"

typedef struct
{
    int type;
    int intval;
    double decval;
    std::string strval;
} varInfo;

static std::map<std::string, varInfo> varMap;

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

    varInfo vinfo;

    if (!strcmp(typestr, "Float"))
    {
        vinfo.type = DECIMAL_TYPE;
        if (initval) vinfo.decval = strtof(initval, 0x0);
        else vinfo.decval = 0;
    }
    else if (!strcmp(typestr, "Integer"))
    {
        vinfo.type = INTEGER_TYPE;
        if (initval) vinfo.intval = strtol(initval, 0x0, 10);
        else vinfo.intval = 0;
    }
    else if (!strcmp(typestr, "String"))
    {
        vinfo.type = STRING_TYPE;
        if (initval) vinfo.strval = initval;
        else vinfo.strval = "";
    }
    else
    {
        vinfo.type = UNDEFINED_TYPE;
        error_msg("Attempting to create the variable \"" << paramname << "\" with an unknown type: " << typestr);
    }

    varMap[std::string(mylabel)] = vinfo;
}

void *get_pointer(const char *label)
{
    if (!label) return 0x0;

    const char *mylabel;

    if (label[0] == '@') mylabel = &label[1];
    else mylabel = label;

    if (varMap.find(mylabel) != varMap.end())
    {
        switch (varMap[mylabel].type)
        {
            case DECIMAL_TYPE:
                return (void *)(&(varMap[mylabel].decval));
            case INTEGER_TYPE:
                return (void *)(&(varMap[mylabel].intval));
            case STRING_TYPE:
                return (void *)(&(varMap[mylabel].strval));
            default:
                return 0x0;
        }
    }
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

    if (varMap.find(mylabel) != varMap.end()) return varMap[mylabel].type;
    else
    {
        error_msg("Invalid parameter label: " << label);
        return UNDEFINED_TYPE;
    }
}
