#ifndef _GEOMETRIC_HH_
#define _GEOMETRIC_HH_

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
        float *x;
        float *y;
        float *w;
        float *h;
        float *containerw;
        float *containerh;
        unsigned halign;
        unsigned valign;
        float *rotate;

        float refx;
        float refy;
        float delx;
        float dely;
        float width;
        float height;
        float left;
        float right;
        float bottom;
        float top;
        float center;
        float middle;

    private:
        float GeomX(float *, float, float, int);
        float GeomY(float *, float, float, int);
};

#endif
