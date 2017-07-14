#ifndef _RECTANGLE_HH_
#define _RECTANGLE_HH_

#include "kolor.hh"
#include "object.hh"

class dcRectangle : public dcObject
{
    public:
        dcRectangle(float *, float *, float *, float *, float *, float *, unsigned, unsigned, float *, float, bool, bool, Kolor *, Kolor *);

        void draw(void);

    private:
        float *x;
        float *y;
        float *w;
        float *h;
        float *containerw;
        float *containerh;
        unsigned halign;
        unsigned valign;
        float *rotate;
        float linewidth;
        bool fill;
        bool outline;
        Kolor FillColor;
        Kolor LineColor;
};

#endif
