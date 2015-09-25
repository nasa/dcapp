#ifndef _LOADUTILS_HH_
#define _LOADUTILS_HH_

#include "dc.hh"

extern dcTexture dcLoadTexture(const char *);
extern dcFont dcLoadFont(const char *);
extern dcFont dcLoadFont(const char *, const char *);
extern dcFont dcLoadFont(const char *, const char *, unsigned int);
extern float *dcLoadConstant(float);
extern int *dcLoadConstant(int);
extern char *dcLoadConstant(const char *);

#endif
