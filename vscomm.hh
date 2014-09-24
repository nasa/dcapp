#ifndef _VSCOMM_HH_
#define _VSCOMM_HH_

#include "vscomm_constants.hh"

extern void *vscomm_add_var(char *, char *, int, int);
extern int vscomm_remove_var(char *);
extern int vscomm_activate(char *, int, char *, char *);
extern int vscomm_get(void);
extern int vscomm_put(char *, int, void *, char *);
extern void vscomm_terminate(void);

#endif
