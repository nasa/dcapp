#include <cstring>
#include "types.hh"
#include "string_utils.hh"
#include "loadUtils.hh"

extern void varlist_append(const char *, const char *, const char *);
extern void *get_pointer(const char *);
extern int get_datatype(const char *);

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

float *getFloatPointer(const char *valstr)
{
    if (check_dynamic_element(valstr)) return (float *)get_pointer(valstr);
    else return dcLoadConstant(StrToFloat(valstr, 0));
}

int *getIntegerPointer(const char *valstr)
{
    if (check_dynamic_element(valstr)) return (int *)get_pointer(valstr);
    else return dcLoadConstant(StrToInt(valstr, 0));
}

char *getStringPointer(const char *valstr)
{
    if (check_dynamic_element(valstr)) return (char *)get_pointer(valstr);
    else return dcLoadConstant(valstr);
}

void *getVariablePointer(int datatype, const char *valstr)
{
    switch (datatype)
    {
        case FLOAT_TYPE:
            return getFloatPointer(valstr);
            break;
        case INTEGER_TYPE:
            return getIntegerPointer(valstr);
            break;
        case STRING_TYPE:
            return getStringPointer(valstr);
            break;
    }
    return 0x0;
}

int get_data_type(const char *valstr)
{
    if (check_dynamic_element(valstr)) return get_datatype(valstr);
    return UNDEFINED_TYPE;
}

float getFloatValue(int type, const void *val)
{
    switch (type)
    {
        case FLOAT_TYPE:
            return *(float *)val;
        case INTEGER_TYPE:
            return (float)(*(int *)val);
        case STRING_TYPE:
            return strtof((char *)val, 0x0);
    }
    return 0;
}

int getIntegerValue(int type, const void *val)
{
    switch (type)
    {
        case FLOAT_TYPE:
            return (int)(*(float *)val);
        case INTEGER_TYPE:
            return *(int *)val;
        case STRING_TYPE:
            return strtol((char *)val, 0x0, 10);
    }
    return 0;
}
