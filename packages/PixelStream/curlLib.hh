#ifndef _CURLLIB_HH_
#define _CURLLIB_HH_

#include <string>
#include "PixelStreamMjpeg.hh"

extern void curlLibInit(void);
extern void *curlLibCreateHandle(const std::string &, int, const std::string &, const std::string &, const std::string &, PixelStreamMjpeg *);
extern void curlLibDestroyHandle(void *);
extern int curlLibAddHandle(void *);
extern void curlLibRemoveHandle(void *);
extern void curlLibRun(void);
extern void curlLibTerm(void);

#endif
