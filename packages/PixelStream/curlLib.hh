#ifndef _CURLLIB_HH_
#define _CURLLIB_HH_

#include "PixelStreamMjpeg.hh"

extern void curlLibInit(void);
extern void *curlLibCreateHandle(const char *, int, const char *, const char *, const char *, PixelStreamMjpeg *);
extern void curlLibDestroyHandle(void *);
extern int curlLibAddHandle(void *);
extern void curlLibRemoveHandle(void *);
extern void curlLibRun(void);
extern void curlLibTerm(void);

#endif
