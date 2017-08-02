#ifndef _DC_HH_
#define _DC_HH_

#include "fontlib/fontlib.hh"

typedef flFont *dcFont;
typedef unsigned int dcTexture;
class dcPosition
{
    public:
        dcPosition(float X, float Y) { x = X; y = Y; };
        float x;
        float y;
};

#endif
