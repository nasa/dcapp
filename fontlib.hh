#ifndef _FONTLIB_HH_
#define _FONTLIB_HH_

#include "fontlibdefs.hh"

extern flFont *flInitFont(const char *, const char *, unsigned int);
extern float flGetFontAdvance(flFont *, flMonoOption, char *);
extern float flGetFontDescender(flFont *);
extern void flRenderFont(flFont *, flMonoOption, char *);

#endif
