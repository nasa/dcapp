#ifndef _IMAGE_HH_
#define _IMAGE_HH_

#include "dc.hh"
#include "object.hh"

class dcImage : public dcObject
{
    public:
        dcImage(float *, float *, float *, float *, float *, float *, unsigned, unsigned, float *, dcTexture);

        void draw(void);

    private:
        float *x;
        float *y;
        float *w;
        float *h;
        float *containerw;
        float *containerh;
        unsigned halign;
        unsigned valign;
        float *rotate;
        dcTexture textureID;
};

#endif
