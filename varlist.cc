#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include "msg.hh"
#include "varlist_constants.hh"

typedef struct
{
    int type;
    void *value;
} varInfo;

std::map<std::string, varInfo> varMap;


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

    varInfo vinfo;

    if (!strcmp(typestr, "Float"))
    {
        vinfo.type = VARLIST_FLOAT;
        vinfo.value = calloc(1, sizeof(float));
        if (initval) *(float *)vinfo.value = strtof(initval, 0x0);
    }
    else if (!strcmp(typestr, "Integer"))
    {
        vinfo.type = VARLIST_INTEGER;
        vinfo.value = calloc(1, sizeof(int));
        if (initval) *(int *)vinfo.value = strtol(initval, 0x0, 10);
    }
    else if (!strcmp(typestr, "String"))
    {
        vinfo.type = VARLIST_STRING;
        vinfo.value = calloc(STRING_DEFAULT_LENGTH, sizeof(char));
        if (initval) strcpy((char *)vinfo.value, initval);
    }
    else
    {
        vinfo.type = VARLIST_UNKNOWN_TYPE;
        vinfo.value = 0x0;
        error_msg("Attempting to create the variable \"" << paramname << "\" with an unknown type: " << typestr);
    }

    varMap[std::string(paramname)] = vinfo;
}

void *get_pointer(const char *label)
{
    if (varMap.find(label) != varMap.end()) return varMap[label].value;
    else
    {
        error_msg("Invalid parameter label: " << label);
        return 0x0;
    }
}

int get_datatype(const char *label)
{
    if (varMap.find(label) != varMap.end()) return varMap[label].type;
    else
    {
        error_msg("Invalid parameter label: " << label);
        return VARLIST_UNKNOWN_TYPE;
    }
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
