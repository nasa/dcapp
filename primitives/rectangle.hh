#ifndef _RECTANGLE_HH_
#define _RECTANGLE_HH_

#include "kolor.hh"
#include "geometric.hh"
#include "parent.hh"

class dcRectangle : public dcGeometric
{
    public:
        dcRectangle(dcParent *);
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
