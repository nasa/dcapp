#ifndef _VERTEX_HH_
#define _VERTEX_HH_

#include "values.hh"
#include "object.hh"
#include "parent.hh"

class dcVertex : public dcObject
{
    public:
        dcVertex(dcParent *);
        void setPosition(const char *, const char *);
        void setOrigin(const char *, const char *);
        void draw(void);

    private:
        Value *x;
        Value *y;
        Value *containerw;
        Value *containerh;
        unsigned originx;
        unsigned originy;
};

#endif
