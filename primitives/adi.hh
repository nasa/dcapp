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
        double *outerradius;
        double *ballradius;
        double *chevronW;
        double *chevronH;
        double *roll;
        double *pitch;
        double *yaw;
        double *rollError;
        double *pitchError;
        double *yawError;

        void draw_roll_bug(double, double, double, double);
        void draw_cross_hairs(double);
        void draw_needles(double, double, double, double);
        double get_error_info(double, double);
};

#endif
