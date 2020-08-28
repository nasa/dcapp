#ifndef _MOUSEMOTION_HH_
#define _MOUSEMOTION_HH_

#include <string>
#include "object.hh"
#include "parent.hh"

class dcMouseMotion : public dcObject
{
    public:
        dcMouseMotion(dcParent *, const std::string &, const std::string &);
        virtual ~dcMouseMotion();

        void handleMouseMotion(double, double);

    private:
        Variable *pointerX;
        Variable *pointerY;
        Variable noval;
};

#endif
