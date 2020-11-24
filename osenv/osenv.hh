#ifndef _OSENV_HH_
#define _OSENV_HH_

#include <vector>
#include <string>

extern int checkArgs(std::vector<std::string> &);
extern std::vector<std::string> getArgs(void);
extern void storeArgs(const std::string &, const std::string &);

#endif
