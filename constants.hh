#ifndef _CONSTANTS_HH_
#define _CONSTANTS_HH_

#include <string>
#include "valuedata.hh"

extern double *dcLoadConstant(double fval);
extern int *dcLoadConstant(int ival);
extern std::string *dcLoadConstant(const char *sval);

extern ValueData *getConstantValue(int);

#endif
