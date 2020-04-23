#ifndef _CONSTANTS_HH_
#define _CONSTANTS_HH_

#include <string>
#include "valuedata.hh"

extern ValueData *getConstantValue(int);
extern ValueData *getConstantValue(double);
extern ValueData *getConstantValue(const char *);

#endif
