#ifndef _VARLIST_HH_
#define _VARLIST_HH_

#include "valuedata.hh"

extern void registerVariable(const char *, const char *, const char *);
extern Variable *getVariable(const char *);
extern void *get_pointer(const char *);
extern char *create_virtual_variable(const char *, const char *);
extern bool check_dynamic_element(const char *);
extern Value *getValue(const char *);

#endif
