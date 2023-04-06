#ifndef _FUNCTION_HH_
#define _FUNCTION_HH_

#include "object.hh"
#include "parent.hh"

typedef void (*voidfunc)();

class dcFunction : public dcObject
{
    public:
        dcFunction(dcParent *, void(*)());
        void draw(void);

    private:
        void (*func)();
};

extern voidfunc registerFunction(const std::string &, void(*)());
extern voidfunc getFunction(const std::string &);

#endif
