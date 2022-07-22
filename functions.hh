#ifndef _FUNCTIONS_HH_
#define _FUNCTIONS_HH_

#include <string>
#include "functions.hh"

class Function
{
    public:
        Function();
        virtual ~Function();

        void setFunction(void(*)());
        void setName(const std::string &);
        void runFunction(void);
    private:
        void (*func)();
        std::string name;
};

extern void registerFunction(const std::string &, void(*)());
extern Function* getFunction(const std::string &);

#endif
