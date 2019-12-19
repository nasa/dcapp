#ifndef _VARLIST_HH_
#define _VARLIST_HH_

#include <string>

extern void varlist_append(const char *, const char *, const char *);
extern void *get_pointer(const char *);
extern int get_datatype(const char *);

extern char *create_virtual_variable(const char *, const char *);
extern bool check_dynamic_element(const char *);
extern double *getDecimalPointer(const char *);
extern int *getIntegerPointer(const char *);
extern std::string *getStringPointer(const char *);
extern void *getVariablePointer(int, const char *);
extern int get_data_type(const char *);
extern double getDecimalValue(int, const void *);
extern int getIntegerValue(int, const void *);
extern std::string getStringValue(int, const void *);

#endif
