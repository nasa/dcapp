#ifndef _POLYGON_HH_
#define _POLYGON_HH_

#include <string>
#include "kolor.hh"
#include "parent.hh"

class dcPolygon : public dcParent
{
    public:
        dcPolygon(dcParent *);
        virtual ~dcPolygon();

        void setFillColor(const std::string &);
        void setLineColor(const std::string &);
        void setLineWidth(const std::string &);
        void setLinePattern(const std::string &);
        void setLineFactor(const std::string &);
        void handleMousePress(double, double);
        void handleMouseRelease(void);
        void processPreCalculations();
        void processPostCalculations();
        void draw(void);
    
        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        double linewidth;
        bool fill;
        bool outline;
        Kolor FillColor;
        Kolor LineColor;
        std::vector<float> vertices;
        bool selected;
        uint16_t linePattern;
        int lineFactor;
};

#endif
