#ifndef _CALLFUNC_HH_
#define _CALLFUNC_HH_

#include "values.hh"
#include "object.hh"
#include "parent.hh"

class dcCallFunc : public dcObject
{
    public:
        dcCallFunc(dcParent *,const std::string &);
        void draw(void);

    private:
        Value* funcname;
};

#endif
