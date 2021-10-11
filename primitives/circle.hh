#ifndef _CIRCLE_HH_
#define _CIRCLE_HH_

#include <string>
#include "values.hh"
#include "kolor.hh"
#include "geometric.hh"
#include "parent.hh"

class dcCircle : public dcGeometric
{
    public:
        dcCircle(dcParent *);
        virtual ~dcCircle();

        void setFillColor(const std::string &);
        void setLineColor(const std::string &);
        void setLineWidth(const std::string &);
        void setLinePattern(const std::string &);
        void setLineFactor(const std::string &);
        void setRadius(const std::string &);
        void setSegments(const std::string &);
        void handleMousePress(double, double);
        void handleMouseRelease(void);
        void processPreCalculations();
        void processPostCalculations();
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
        uint16_t linePattern;
        int lineFactor;
};

#endif
