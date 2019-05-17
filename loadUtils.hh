#ifndef _LOADUTILS_HH_
#define _LOADUTILS_HH_

#include "dc.hh"

extern dcTexture dcLoadTexture(const char *filename);
extern dcFont dcLoadFont(const char *filename, const char *face=0x0, unsigned int basesize=20);

#endif
