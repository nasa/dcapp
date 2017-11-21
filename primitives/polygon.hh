#ifndef _POLYGON_HH_
#define _POLYGON_HH_

#include "kolor.hh"
#include "parent.hh"

class dcPolygon : public dcParent
{
    public:
        dcPolygon(dcParent *);
        void setFillColor(const char *);
        void setLineColor(const char *);
        void setLineWidth(const char *);
        void draw(void);
    
    private:
        double linewidth;
        bool fill;
        bool outline;
        Kolor FillColor;
        Kolor LineColor;
};

#endif
