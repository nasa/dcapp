#ifndef _STRING_UTILS_HH_
#define _STRING_UTILS_HH_

#include "alignment.hh"
#include "kolor.hh"

extern float StrToFloat(const char *, float);
extern int StrToInt(const char *, int);
extern bool StrToBool(const char *, bool);
extern Kolor StrToColor(const char *, float, float, float, float);
extern HAlignment StrToHAlign(const char *, HAlignment);
extern VAlignment StrToVAlign(const char *, VAlignment);

#endif
