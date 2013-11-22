#include "simio_constants.h"
#include "edgeio_constants.h"

extern void edgeio_initialize_parameter_list(int);
extern int edgeio_add_parameter(int, const char *, const char *);
extern int edgeio_finish_initialization(char *, char *, float);
extern int edgeio_activatecomm(void);
extern int edgeio_readsimdata(void);
extern int edgeio_writesimdata(void);
extern void edgeio_forcewrite(void *);
extern void edgeio_term(void);
