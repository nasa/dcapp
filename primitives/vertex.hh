#ifndef _VERTEX_HH_
#define _VERTEX_HH_

#include "object.hh"
#include "parent.hh"

class dcVertex : public dcObject
{
    public:
        dcVertex(dcParent *);
        void setPosition(const char *, const char *);
        void draw(void);

    private:
        float *x;
        float *y;
        float *containerw;
        float *containerh;
};

#endif