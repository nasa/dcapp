#ifndef _SIMIO_HH_
#define _SIMIO_HH_

#include "simio_constants.hh"

extern void simio_initialize_parameter_list(void);
extern int simio_add_parameter(const char *, const char *, const char *);
extern void *get_pointer(const char *);
extern int get_datatype(const char *);
extern void simio_term(void);

#endif
