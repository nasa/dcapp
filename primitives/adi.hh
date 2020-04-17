#ifndef _ADI_HH_
#define _ADI_HH_

#include <vector>
#include "RenderLib/RenderLib.hh"
#include "valuedata.hh"
#include "geometric.hh"
#include "parent.hh"

typedef struct { float m_X, m_Y, m_Z, m_U, m_V; } LocalPoint;

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
        tdTexture *bkgdID;
        tdTexture *ballID;
        double *outerradius;
        double *ballradius;
        double *chevronW;
        double *chevronH;
        ValueData *roll;
        ValueData *pitch;
        ValueData *yaw;
        ValueData *rollError;
        ValueData *pitchError;
        ValueData *yawError;
        std::vector<float> sphereTriangles;

        void draw_roll_bug(double, double, double, double);
        void draw_cross_hairs(double);
        void draw_needles(double, double, double, double);
        double get_error_info(double, double);

        void BuildTriangles(std::vector<float> &, const std::vector<LocalPoint> &, const std::vector<LocalPoint> &);
        std::vector<float> BuildSphere(void);
};

#endif
