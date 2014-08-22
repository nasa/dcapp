#ifndef _VSCOMM_H
#define _VSCOMM_H

#include "vscomm_constants.h"

#ifdef __cplusplus
extern "C" {
#endif
extern void *vscomm_add_var(char *, char *, int, int);
extern int vscomm_remove_var(char *);
extern int vscomm_activate(char *, int, char *, char *);
extern int vscomm_get(void);
extern int vscomm_put(char *, int, void *, char *);
extern void vscomm_terminate(void);
#ifdef __cplusplus
}
#endif

#endif
