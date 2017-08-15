#ifndef _ADI_HH_
#define _ADI_HH_

#include "dc.hh"
#include "geometric.hh"
#include "parent.hh"

class dcADI : public dcGeometric
{
    public:
        dcADI(dcParent *);
        void setBackgrountTexture(const char *);
        void setBallTexture(const char *);
        void setRPY(const char *, const char *, const char *);
        void setRPYerrors(const char *, const char *, const char *);
        void setRadius(const char *, const char *);
        void setChevron(const char *, const char *);
        void draw(void);

    private:
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
