#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include "types.hh"
#include "basicutils/msg.hh"
#include "varlist_constants.hh"

typedef struct
{
    int type;
    void *value;
} varInfo;

std::map<std::string, varInfo> varMap;

void *varlist_append(const char *paramname, const char *typestr, const char *initval)
{
    const char *mylabel;

    if (paramname[0] == '@') mylabel = &paramname[1];
    else mylabel = paramname;

    if (!paramname)
    {
        error_msg("Attempting to create a variable without a name");
        return nullptr;
    }

    if (!typestr)
    {
        error_msg("Attempting to create the variable \"" << paramname << "\" without a specified type");
        return nullptr;
    }

    varInfo vinfo;

    if (!strcmp(typestr, "Float"))
    {
        vinfo.type = DECIMAL_TYPE;
        vinfo.value = calloc(1, sizeof(double));
        if (initval) *(double *)vinfo.value = strtof(initval, 0x0);
    }
    else if (!strcmp(typestr, "Integer"))
    {
        vinfo.type = INTEGER_TYPE;
        vinfo.value = calloc(1, sizeof(int));
        if (initval) *(int *)vinfo.value = strtol(initval, 0x0, 10);
    }
    else if (!strcmp(typestr, "String"))
    {
        vinfo.type = STRING_TYPE;
        vinfo.value = calloc(STRING_DEFAULT_LENGTH, sizeof(char));
        if (initval) strcpy((char *)vinfo.value, initval);
    }
    else
    {
        vinfo.type = UNDEFINED_TYPE;
        vinfo.value = 0x0;
        error_msg("Attempting to create the variable \"" << paramname << "\" with an unknown type: " << typestr);
    }

    varMap[std::string(mylabel)] = vinfo;
    return vinfo.value;
}

void varlist_term(void)
{
    static std::map<std::string, varInfo>::iterator myitem;
    for (myitem = varMap.begin(); myitem != varMap.end(); myitem++)
    {
        if (myitem->second.type) free(myitem->second.value);
    }
    varMap.clear();
}

void *get_pointer(const char *label)
{
    if (!label) return 0x0;

    const char *mylabel;

    if (label[0] == '@') mylabel = &label[1];
    else mylabel = label;

    if (varMap.find(mylabel) != varMap.end()) return varMap[mylabel].value;
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
