#ifndef _MOUSEMOTION_HH_
#define _MOUSEMOTION_HH_

#include "object.hh"
#include "parent.hh"

class dcMouseMotion : public dcObject
{
    public:
        dcMouseMotion(dcParent *, const char *, const char *);
        virtual ~dcMouseMotion();

        void handleMouseMotion(double, double);

    private:
        double *pointerX;
        double *pointerY;
        double noval;
};

#endif
