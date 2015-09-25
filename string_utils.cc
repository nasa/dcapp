#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "Objects.hh"
#include "loadUtils.hh"

extern bool check_dynamic_element(const char *);
extern void *get_pointer(const char *);

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

static float *color_element(int index, char *strval, float defval)
{
    float *retptr = 0x0;
    float fval;

    if (index < 0) retptr = dcLoadConstant(defval);
    else
    {
        if (check_dynamic_element(strval)) retptr = (float *)get_pointer(&strval[1]);
        else
        {
            fval = strtof(strval, 0x0);
            retptr = dcLoadConstant(fval);
        }
    }

    return retptr;
}

struct kolor StrToColor(const char *instr, float r, float g, float b, float a)
{
    struct kolor retval;

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

HAlignment StrToHAlign(const char *instr, HAlignment default_value)
{
    if (!instr) return default_value;
    else if (!strcasecmp(instr, "Left")) return AlignLeft;
    else if (!strcasecmp(instr, "Center")) return AlignCenter;
    else if (!strcasecmp(instr, "Right")) return AlignRight;
    else return default_value;
}

VAlignment StrToVAlign(const char *instr, VAlignment default_value)
{
    if (!instr) return default_value;
    else if (!strcasecmp(instr, "Bottom")) return AlignBottom;
    else if (!strcasecmp(instr, "Middle")) return AlignMiddle;
    else if (!strcasecmp(instr, "Top")) return AlignTop;
    else return default_value;
}
