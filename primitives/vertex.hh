#ifndef _VERTEX_HH_
#define _VERTEX_HH_

#include "object.hh"

class dcVertex : public dcObject
{
    public:
        dcVertex(float *, float *, float *, float *);

        void draw(void);

    private:
        float *x;
        float *y;
        float *containerw;
        float *containerh;
};

#endif
