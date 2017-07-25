#ifndef _CIRCLE_HH_
#define _CIRCLE_HH_

#include "kolor.hh"
#include "geometric.hh"
#include "parent.hh"

class dcCircle : public dcGeometric
{
    public:
        dcCircle(dcParent *);
        void setFillColor(const char *);
        void setLineColor(const char *);
        void setLineWidth(const char *);
        void setRadius(const char *);
        void setSegments(const char *);
        void draw(void);

    private:
        float *radius;
        float linewidth;
        bool fill;
        bool outline;
        Kolor FillColor;
        Kolor LineColor;
        unsigned segments;
};

#endif
