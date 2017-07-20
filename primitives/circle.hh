#ifndef _CIRCLE_HH_
#define _CIRCLE_HH_

#include "kolor.hh"
#include "object.hh"

class dcCircle : public dcObject
{
    public:
        dcCircle(float *, float *, float *, float *, unsigned, unsigned, float *, float, bool, bool, Kolor *, Kolor *, unsigned);

        void draw(void);

    private:
        float *x;
        float *y;
        float *containerw;
        float *containerh;
        unsigned halign;
        unsigned valign;
        float *radius;
        float linewidth;
        bool fill;
        bool outline;
        Kolor FillColor;
        Kolor LineColor;
        unsigned segments;
};

#endif
