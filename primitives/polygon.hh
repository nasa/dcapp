#ifndef _POLYGON_HH_
#define _POLYGON_HH_

#include "kolor.hh"
#include "opengl_draw.hh"
#include "parent.hh"

class dcPolygon : public dcParent
{
    public:
        dcPolygon(float, bool, bool, Kolor *, Kolor *);
        void completeInitialization(void);
        void draw(void);
    
    private:
        float linewidth;
        bool fill;
        bool outline;
        Kolor FillColor;
        Kolor LineColor;
};

#endif
