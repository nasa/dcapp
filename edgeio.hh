#ifndef _EDGEIO_HH_
#define _EDGEIO_HH_

#include "simio_constants.hh"
#include "edgeio_constants.hh"

extern void edgeio_initialize_parameter_list(int);
extern int edgeio_add_parameter(int, const char *, const char *);
extern int edgeio_finish_initialization(char *, char *, float);
extern int edgeio_activatecomm(void);
extern int edgeio_readsimdata(void);
extern int edgeio_writesimdata(void);
extern void edgeio_forcewrite(void *);
extern void edgeio_term(void);

#endif
