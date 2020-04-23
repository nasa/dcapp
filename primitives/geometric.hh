#ifndef _GEOMETRIC_HH_
#define _GEOMETRIC_HH_

#include "valuedata.hh"
#include "parent.hh"
#include "object.hh"

class dcGeometric : public dcObject
{
    public:
        dcGeometric(dcParent *);
        void setPosition(const char *, const char *);
        void setSize(const char *, const char *);
        void setRotation(const char *);
        void setAlignment(const char *, const char *);
        void computeGeometry(void);

    protected:
        ValueData *x;
        ValueData *y;
        ValueData *w;
        ValueData *h;
        ValueData *containerw;
        ValueData *containerh;
        unsigned halign;
        unsigned valign;
        ValueData *rotate;

        double refx;
        double refy;
        double delx;
        double dely;
        double width;
        double height;
        double left;
        double right;
        double bottom;
        double top;
        double center;
        double middle;

    private:
        double GeomX(ValueData *, double, double, int);
        double GeomY(ValueData *, double, double, int);
};

#endif
