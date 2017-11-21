#include <cstdlib>
#include <strings.h>

double StringToDecimal(const char *instr, double default_value)
{
    if (!instr) return default_value;
    else return (strtod(instr, 0x0));
}

int StringToInteger(const char *instr, int default_value)
{
    if (!instr) return default_value;
    else return (strtol(instr, 0x0, 10));
}

bool StringToBoolean(const char *instr, bool default_value)
{
    if (!instr) return default_value;
    else if (!strcasecmp(instr, "true") || !strcasecmp(instr, "yes") || !strcasecmp(instr, "on")) return true;
    else if (!strcasecmp(instr, "false") || !strcasecmp(instr, "no") || !strcasecmp(instr, "off")) return false;
    else if (StringToInteger(instr, 0) || StringToDecimal(instr, 0)) return true;
    else return default_value;
}
