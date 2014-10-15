#ifndef _VARLIST_HH_
#define _VARLIST_HH_

#include "varlist_constants.hh"

extern void varlist_init(void);
extern int varlist_append(const char *, const char *, const char *);
extern void *get_pointer(const char *);
extern int get_datatype(const char *);
extern void varlist_term(void);

#endif
