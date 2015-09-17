#ifndef _STRING_UTILS_HH_
#define _STRING_UTILS_HH_

#include "Objects.hh"

extern float StrToFloat(const char *, float);
extern int StrToInt(const char *, int);
extern bool StrToBool(const char *, bool);
extern struct kolor StrToColor(const char *, float, float, float, float);
extern HAlignment StrToHAlign(const char *, HAlignment);
extern VAlignment StrToVAlign(const char *, VAlignment);

#endif
