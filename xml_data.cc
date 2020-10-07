#include <string>
#include "xml_data.hh"

xmldata::xmldata() : std::string(), isDefined(false) {};

xmldata::xmldata(const std::string &instr) : std::string(), isDefined(false)
{
    isDefined = true;
    this->assign(instr);
};

xmldata::xmldata(const char *instr) : std::string(), isDefined(false)
{
    if (instr)
    {
        isDefined = true;
        this->assign(instr);
    }
};

void xmldata::reset(void)
{
    isDefined = false;
    this->clear();
}
