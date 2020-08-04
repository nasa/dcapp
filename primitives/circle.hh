#ifndef _CIRCLE_HH_
#define _CIRCLE_HH_

#include "values.hh"
#include "kolor.hh"
#include "geometric.hh"
#include "parent.hh"

class dcCircle : public dcGeometric
{
    public:
        dcCircle(dcParent *);
        virtual ~dcCircle();

        void setFillColor(const char *);
        void setLineColor(const char *);
        void setLineWidth(const char *);
        void setRadius(const char *);
        void setSegments(const char *);
        void handleMousePress(double, double);
        void handleMouseRelease(void);
        void draw(void);

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        Value *radius;
        double linewidth;
        bool fill;
        bool outline;
        Kolor FillColor;
        Kolor LineColor;
        unsigned segments;
        bool selected;
};

#endif
