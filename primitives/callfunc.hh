#ifndef _CALLFUNC_HH_
#define _CALLFUNC_HH_

#include "xml_data.hh"
#include "variables.hh"
#include "values.hh"
#include "animation.hh"
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
