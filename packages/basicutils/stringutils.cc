#include <string>
#include <sstream>
#include <cstdlib>
#include <strings.h>

double StringToDecimal(const char *instr, double default_value = 0.0)
{
    if (!instr) return default_value;
    else return (strtod(instr, 0x0));
}

double StringToDecimal(std::string &instr, double default_value = 0.0)
{
    try { return std::stod(instr); } catch(...) { return default_value; }
}

int StringToInteger(const char *instr, int default_value = 0)
{
    if (!instr) return default_value;
    else return (strtol(instr, 0x0, 10));
}

int StringToInteger(std::string &instr, int default_value = 0)
{
    try { return std::stoi(instr); } catch(...) { return default_value; }
}

bool StringToBoolean(const char *instr, bool default_value = false)
{
    if (!instr) return default_value;

    if (!strcasecmp(instr, "true") || !strcasecmp(instr, "yes") || !strcasecmp(instr, "on")) return true;
    if (!strcasecmp(instr, "false") || !strcasecmp(instr, "no") || !strcasecmp(instr, "off")) return false;
    if (StringToInteger(instr) || StringToDecimal(instr)) return true;

    return default_value;
}

bool StringToBoolean(std::string &instr, bool default_value = false)
{
    if (instr.empty()) return default_value;

    std::string lcstr;

    for (std::string::iterator it = instr.begin(); it != instr.end(); it++) lcstr += tolower(*it);

    if (lcstr == "true" || lcstr == "yes" || lcstr == "on") return true;
    if (lcstr == "false" || lcstr == "no" || lcstr == "off") return false;
    if (StringToInteger(instr) || StringToDecimal(instr)) return true;

    return default_value;
}

std::string DecimalToString(double val)
{
    std::ostringstream mystream;
    mystream << val;
    return mystream.str();
}

std::string IntegerToString(int val)
{
    std::ostringstream mystream;
    mystream << val;
    return mystream.str();
}
