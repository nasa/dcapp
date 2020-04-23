#ifndef _VERTEX_HH_
#define _VERTEX_HH_

#include "valuedata.hh"
#include "object.hh"
#include "parent.hh"

class dcVertex : public dcObject
{
    public:
        dcVertex(dcParent *);
        void setPosition(const char *, const char *);
        void draw(void);

    private:
        ValueData *x;
        ValueData *y;
        ValueData *containerw;
        ValueData *containerh;
};

#endif
