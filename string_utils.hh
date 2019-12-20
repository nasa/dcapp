#ifndef _STRING_UTILS_HH_
#define _STRING_UTILS_HH_

#include <string>

extern double StringToDecimal(const char *instr, double default_value = 0.0);
extern double StringToDecimal(std::string &instr, double default_value = 0.0);
extern int StringToInteger(const char *instr, int default_value = 0);
extern int StringToInteger(std::string &instr, int default_value = 0);
extern bool StringToBoolean(const char *instr, bool default_value = false);
extern bool StringToBoolean(std::string &instr, bool default_value = false);

#endif
