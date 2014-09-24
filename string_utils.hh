#ifndef _STRING_UTILS_HH_
#define _STRING_UTILS_HH_

#include "Objects.hh"

float StrToFloat(const char *, float);
int StrToInt(const char *, int);
int BoolStrToInt(const char *, int);
struct kolor StrToColor(const char *, float, float, float);
char *StrToStr(const char *, char *);
HAlignment StrToHAlign(const char *, HAlignment);
VAlignment StrToVAlign(const char *, VAlignment);

#endif
