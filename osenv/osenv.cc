#include <vector>
#include <string>

int checkArgs(std::vector<std::string> &arglist)
{
    if (arglist.empty()) return (-1);
    else return 1;
}

std::vector<std::string> getArgs(void)
{
    std::vector<std::string> retvec;
    return retvec;
}

void storeArgs(const std::string &, const std::string &) { }
