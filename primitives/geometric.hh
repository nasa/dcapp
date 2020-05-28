#ifndef _GEOMETRIC_HH_
#define _GEOMETRIC_HH_

#include "valuedata.hh"
#include "parent.hh"
#include "object.hh"

enum { dcLeft, dcCenter, dcRight };
enum { dcBottom, dcMiddle, dcTop };

class dcGeometric : public dcObject
{
    public:
        dcGeometric(dcParent *);
        void setPosition(const char *, const char *);
        void setSize(const char *, const char *);
        void setRotation(const char *);
        void setAlignment(const char *, const char *);
        void setOrigin(const char *, const char *);
        void computeGeometry(void);

    protected:
        Value *x;
        Value *y;
        Value *w;
        Value *h;
        Value *containerw;
        Value *containerh;
        unsigned halign;
        unsigned valign;
        unsigned originx;
        unsigned originy;
        Value *rotate;

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
        double GeomX(Value *, double, double, int);
        double GeomY(Value *, double, double, int);
};

#endif
