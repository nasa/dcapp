#include <string>
#include <cstdlib>
#include <strings.h>

double StringToDecimal(const char *instr, double default_value = 0.0)
{
    if (!instr) return default_value;
    else return (strtod(instr, 0x0));
}

double StringToDecimal(const std::string &instr, double default_value = 0.0)
{
    try { return std::stod(instr); } catch(...) { return default_value; }
}

int StringToInteger(const char *instr, int default_value = 0)
{
    if (!instr) return default_value;
    else return (strtol(instr, 0x0, 10));
}

int StringToInteger(const std::string &instr, int default_value = 0)
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

bool StringToBoolean(const std::string &instr, bool default_value = false)
{
    if (instr.empty()) return default_value;

    std::string lcstr;

    for (unsigned i = 0; i < instr.length(); i++) lcstr += tolower(instr[i]);

    if (lcstr == "true" || lcstr == "yes" || lcstr == "on") return true;
    if (lcstr == "false" || lcstr == "no" || lcstr == "off") return false;
    if (StringToInteger(instr) || StringToDecimal(instr)) return true;

    return default_value;
}

bool CaseInsensitiveCompare(const std::string &a, const std::string &b)
{
    unsigned int size = a.size();

    if (b.size() != size) return false;

    for (unsigned int i = 0; i < size; i++)
    {
        if (tolower(a[i]) != tolower(b[i])) return false;
    }

    return true;
}

int HexStringToInteger(const char *instr, int default_value = 0)
{
    if (!instr) return default_value;
    else return (strtol(instr, 0x0, 16));
}

int HexStringToInteger(const std::string &instr, int default_value = 0)
{
    try { return std::stoi(instr, 0x0, 16); } catch(...) { return default_value; }
}
