#ifndef _GEOMETRIC_HH_
#define _GEOMETRIC_HH_

#include <string>
#include "values.hh"
#include "parent.hh"
#include "object.hh"

enum { dcLeft, dcCenter, dcRight };
enum { dcBottom, dcMiddle, dcTop };

class dcGeometric : public dcObject
{
    public:
        dcGeometric(dcParent *);
        void setPosition(const std::string &, const std::string &);
        void setSize(const std::string &, const std::string &);
        void setRotation(const std::string &);
        void setAlignment(const std::string &, const std::string &);
        void setOrigin(const std::string &, const std::string &);
        virtual void computeGeometry(void);

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

        double GeomX(Value *, double, double, int);
        double GeomY(Value *, double, double, int);
};

#endif
