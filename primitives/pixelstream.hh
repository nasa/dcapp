#ifndef _PIXELSTREAM_PRIMITIVE_HH_
#define _PIXELSTREAM_PRIMITIVE_HH_

#include <string>
#include "psi.hh"
#include "geometric.hh"
#include "parent.hh"

class dcPixelStream : public dcGeometric
{
    public:
        dcPixelStream(dcParent *);
        ~dcPixelStream();
        void setProtocol(const std::string &, const char *, const std::string &, const char *, const char *, const char *, const std::string &, const char *, const std::string &);
        void setTestPattern(const std::string &);
        void updateStreams(unsigned);
        void draw(void);

    private:
        tdTexture *textureID;
        tdTexture *testpatternID;
        PixelStreamItem *psi;
        void *pixels;
        size_t memallocation;
};

#endif
