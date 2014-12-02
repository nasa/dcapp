#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include "Objects.hh"
#include "msg.hh"

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

struct kolor StrToColor(const char *instr, float r, float g, float b, float a)
{
    struct kolor retval;
    int count;

    if (instr == NULL)
    {
        retval.R = r;
        retval.G = g;
        retval.B = b;
        retval.A = a;
    }
    else
    {
        count = sscanf(instr, "%f %f %f %f", &(retval.R), &(retval.G), &(retval.B), &(retval.A));
        if (count < 4) retval.A = a;
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
