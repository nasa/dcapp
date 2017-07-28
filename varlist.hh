#ifndef _VARLIST_HH_
#define _VARLIST_HH_

#include "varlist_constants.hh"

extern void varlist_append(const char *, const char *, const char *);
extern void varlist_term(void);
extern char *create_virtual_variable(const char *, const char *);
extern void *get_pointer(const char *);
extern bool check_dynamic_element(const char *);
extern float *getFloatPointer(const char *);
extern int *getIntegerPointer(const char *);
extern char *getStringPointer(const char *);
extern void *getVariablePointer(int, const char *);
extern int get_datatype(const char *);
extern int get_data_type(const char *);

#endif
