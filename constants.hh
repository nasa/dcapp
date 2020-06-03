#ifndef _CONSTANTS_HH_
#define _CONSTANTS_HH_

#include "valuedata.hh"

extern Constant *getConstantFromDecimal(double);
extern Constant *getConstantFromInteger(int);
extern Constant *getConstantFromCharstr(const char *);
extern Constant *getConstantFromBoolean(bool);

#endif
