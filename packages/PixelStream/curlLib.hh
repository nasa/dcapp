#ifndef _CURLLIB_HH_
#define _CURLLIB_HH_

extern void curlLibInit(void);
extern int curlLibAddHandle(void *);
extern void curlLibRemoveHandle(void *);
extern void curlLibRun(void);
extern size_t curlLibProcessBuffer(char *, size_t, size_t, void *);
extern void curlLibTerm(void);

#endif
