#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include "kolor.hh"
#include "loadUtils.hh"
#include "varlist.hh"

float StrToFloat(const char *instr, float default_value)
{
    if (!instr) return default_value;
    else return (strtof(instr, 0x0));
}

int StrToInt(const char *instr, int default_value)
{
    if (!instr) return default_value;
    else return (strtol(instr, 0x0, 10));
}

bool StrToBool(const char *instr, bool default_value)
{
    if (!instr) return default_value;
    else if (!strcasecmp(instr, "true") || !strcasecmp(instr, "yes") || !strcasecmp(instr, "on")) return true;
    else if (!strcasecmp(instr, "false") || !strcasecmp(instr, "no") || !strcasecmp(instr, "off")) return false;
    else if (StrToInt(instr, 0) || StrToFloat(instr, 0)) return true;
    else return default_value;
}

static float *color_element(int index, const char *strval, float defval)
{
    if (index < 0) return dcLoadConstant(defval);
    else if (check_dynamic_element(strval)) return (float *)get_pointer(strval);
    else return dcLoadConstant(strtof(strval, 0x0));
}

Kolor StrToColor(const char *instr, float r, float g, float b, float a)
{
    Kolor retval;

    if (!instr)
    {
        retval.R = dcLoadConstant(r);
        retval.G = dcLoadConstant(g);
        retval.B = dcLoadConstant(b);
        retval.A = dcLoadConstant(a);
    }
    else
    {
        size_t itemlen = strlen(instr) + 1;
        char *rstr, *gstr, *bstr, *astr;
        rstr = (char *)malloc(itemlen);
        gstr = (char *)malloc(itemlen);
        bstr = (char *)malloc(itemlen);
        astr = (char *)malloc(itemlen);
        int count = sscanf(instr, "%s %s %s %s", rstr, gstr, bstr, astr);
        retval.R = color_element(count-1, rstr, r);
        retval.G = color_element(count-2, gstr, g);
        retval.B = color_element(count-3, bstr, b);
        retval.A = color_element(count-4, astr, a);
        free(rstr);
        free(gstr);
        free(bstr);
        free(astr);
    }

    return retval;
}
