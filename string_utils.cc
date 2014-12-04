#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include "nodes.hh"
#include "Objects.hh"
#include "msg.hh"

extern void *LoadConstant(int, void *);
extern int check_dynamic_element(const char *);
extern void *get_pointer(const char *);

float StrToFloat(const char *instr, float default_value)
{
    if (instr == NULL) return default_value;
    else return (strtof(instr, NULL));
}

int StrToInt(const char *instr, int default_value)
{
    if (instr == NULL) return default_value;
    else return (strtol(instr, NULL, 10));
}

int BoolStrToInt(const char *instr, int default_value)
{
    if (instr == NULL) return default_value;
    else if (!strcasecmp(instr, "true") || !strcasecmp(instr, "yes") || !strcasecmp(instr, "on")) return 1;
    else if (!strcasecmp(instr, "false") || !strcasecmp(instr, "no") || !strcasecmp(instr, "off")) return 0;
    else if (StrToInt(instr, default_value) || StrToFloat(instr, default_value)) return 1;
    else return default_value;
}

static float *color_element(int index, char *strval, float defval)
{
    float *retptr = NULL;
    float fval;

    if (index < 0) retptr = (float *)LoadConstant(FLOAT, &defval);
    else
    {
        if (check_dynamic_element(strval)) retptr = (float *)get_pointer(&strval[1]);
        else
        {
            fval = strtof(strval, NULL);
            retptr = (float *)LoadConstant(FLOAT, &fval);
        }
    }

    return retptr;
}

struct kolor StrToColor(const char *instr, float r, float g, float b, float a)
{
    struct kolor retval;

    if (instr == NULL)
    {
        retval.R = (float *)LoadConstant(FLOAT, &r);
        retval.G = (float *)LoadConstant(FLOAT, &g);
        retval.B = (float *)LoadConstant(FLOAT, &b);
        retval.A = (float *)LoadConstant(FLOAT, &a);
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

char *StrToStr(const char *instr, char *default_value)
{
    if (instr == NULL) return default_value;
    else return (char *)instr;
}

HAlignment StrToHAlign(const char *instr, HAlignment default_value)
{
    if (instr == NULL) return default_value;
    else if (!strcasecmp(instr, "Left")) return AlignLeft;
    else if (!strcasecmp(instr, "Center")) return AlignCenter;
    else if (!strcasecmp(instr, "Right")) return AlignRight;
    else return default_value;
}

VAlignment StrToVAlign(const char *instr, VAlignment default_value)
{
    if (instr == NULL) return default_value;
    else if (!strcasecmp(instr, "Bottom")) return AlignBottom;
    else if (!strcasecmp(instr, "Middle")) return AlignMiddle;
    else if (!strcasecmp(instr, "Top")) return AlignTop;
    else return default_value;
}
