#ifndef _TRICKIO_HH_
#define _TRICKIO_HH_

#include "trickio_constants.hh"

extern void trickio_initialize_parameter_list(int);
extern int trickio_add_parameter(int, const char *, const char *, const char *, const char *);
extern int trickio_finish_initialization(void);
extern int trickio_activatecomm(char *, int, char *);
extern int trickio_readsimdata(void);
extern void trickio_writesimdata(void);
extern void trickio_forcewrite(void *);
extern void trickio_term(void);

#endif
