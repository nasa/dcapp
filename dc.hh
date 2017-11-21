#ifndef _DC_HH_
#define _DC_HH_

#include "fontlib/fontlib.hh"

typedef flFont *dcFont;
typedef unsigned int dcTexture;
class dcPosition
{
    public:
        dcPosition(double X, double Y) { x = X; y = Y; };
        double x;
        double y;
};

#endif
