#ifndef _PIXELSTREAM_PRIMITIVE_HH_
#define _PIXELSTREAM_PRIMITIVE_HH_

#include "PixelStream/PixelStreamData.hh"
#include "nodes.hh"
#include "geometric.hh"
#include "parent.hh"

class dcPixelStream : public dcGeometric
{
    public:
        dcPixelStream(dcParent *);
        ~dcPixelStream();
        void setProtocol(const char *, const char *, const char *, const char *, const char *);
        void updateStreams(unsigned);
        void draw(void);

    private:
        dcTexture textureID;
        PixelStreamItem *psi;
        void *pixels;
        size_t memallocation;
};

#endif
