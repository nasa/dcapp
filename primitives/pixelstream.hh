#ifndef _PIXELSTREAM_PRIMITIVE_HH_
#define _PIXELSTREAM_PRIMITIVE_HH_

#include "PixelStream/PixelStreamData.hh"
#include "nodes.hh"
#include "object.hh"

class dcPixelStream : public dcObject
{
    public:
        dcPixelStream(float *, float *, float *, float *, float *, float *, unsigned, unsigned, float *, dcTexture, PixelStreamItem *);
        ~dcPixelStream();
        void updateStreams(unsigned);
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
        PixelStreamItem *psi;
        void *pixels;
        size_t memallocation;
};

#endif
