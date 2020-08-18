#ifndef _RECTANGLE_HH_
#define _RECTANGLE_HH_

#include <string>
#include "kolor.hh"
#include "geometric.hh"
#include "parent.hh"

class dcRectangle : public dcGeometric
{
    public:
        dcRectangle(dcParent *);
        virtual ~dcRectangle();

        void setFillColor(const std::string &);
        void setLineColor(const std::string &);
        void setLineWidth(const std::string &);
        void handleMousePress(double, double);
        void handleMouseRelease(void);
        void draw(void);

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        double linewidth;
        bool fill;
        bool outline;
        Kolor FillColor;
        Kolor LineColor;
        bool selected;
};

#endif
