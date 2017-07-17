#ifndef _ADI_HH_
#define _ADI_HH_

#include "dc.hh"
#include "object.hh"

class dcADI : public dcObject
{
    public:
        dcADI(float *, float *, float *, float *, float *, float *, unsigned, unsigned, dcTexture, dcTexture, float *, float *, float *, float *, float *, float *, float *, float *, float *, float *);

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
        dcTexture bkgdID;
        dcTexture ballID;
        float *outerradius;
        float *ballradius;
        float *chevronW;
        float *chevronH;
        float *roll;
        float *pitch;
        float *yaw;
        float *rollError;
        float *pitchError;
        float *yawError;

        void draw_roll_bug(float, float, float, float);
        void draw_cross_hairs(float);
        void draw_needles(float, float, float, float);
        float get_error_info(float, float);
};

#endif
