#ifndef FONTLIB_H
#define FONTLIB_H

#include "fontlibdefs.h"

typedef void *flFont;

extern flFont *flInitFont(char *, char *, unsigned int);
extern float flGetFontAdvance(flFont *, flMonoOption, char *);
extern float flGetFontDescender(flFont *);
extern void flRenderFont(flFont *, flMonoOption, char *);

#endif
