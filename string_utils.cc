#include <cstdlib>
#include <strings.h>

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
