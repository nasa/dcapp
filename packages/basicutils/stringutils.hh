#ifndef _STRING_UTILS_HH_
#define _STRING_UTILS_HH_

#include <string>
#include <sstream>

extern double StringToDecimal(const char *instr, double default_value = 0.0);
extern double StringToDecimal(const std::string &instr, double default_value = 0.0);
extern int StringToInteger(const char *instr, int default_value = 0);
extern int StringToInteger(const std::string &instr, int default_value = 0);
extern bool StringToBoolean(const char *instr, bool default_value = false);
extern bool StringToBoolean(const std::string &instr, bool default_value = false);
extern bool CaseInsensitiveCompare(const std::string &, const std::string &);
extern int HexStringToInteger(const char *instr, int default_value = 0);
extern int HexStringToInteger(const std::string &instr, int default_value = 0);

// This is almost identical to std::to_string, but it uses %g instead of %f for decimal conversion, which is preferred
template <typename T> inline std::string ConvertToString(T val)
{
    std::ostringstream mystream;
    mystream << val;
    return mystream.str();
}

#endif
