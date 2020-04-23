#ifndef _VARLIST_HH_
#define _VARLIST_HH_

#include <string>
#include "valuedata.hh"

extern void varlist_append(const char *, const char *, const char *);
extern ValueData *getVariableValue(const char *);
extern void *get_pointer(const char *);
extern char *create_virtual_variable(const char *, const char *);
extern bool check_dynamic_element(const char *);
extern ValueData *getValueData(const char *);

#endif
