#ifndef _ADI_HH_
#define _ADI_HH_

#include <string>
#include <vector>
#include "RenderLib/RenderLib.hh"
#include "values.hh"
#include "geometric.hh"
#include "parent.hh"

typedef struct { float m_X, m_Y, m_Z, m_U, m_V; } LocalPoint;

class dcADI : public dcGeometric
{
    public:
        dcADI(dcParent *);
        void setBackgroundTexture(const std::string &);
        void setBallTexture(const std::string &);
        void setRPY(const std::string &, const std::string &, const std::string &);
        void setRPYerrors(const std::string &, const std::string &, const std::string &);
        void setRadius(const std::string &, const std::string &);
        void setChevron(const std::string &, const std::string &);
        void draw(void);

    private:
        tdTexture *bkgdID;
        tdTexture *ballID;
        Value *outerradius;
        Value *ballradius;
        Value *chevronW;
        Value *chevronH;
        Value *roll;
        Value *pitch;
        Value *yaw;
        Value *rollError;
        Value *pitchError;
        Value *yawError;
        std::vector<float> sphereTriangles;

        void draw_roll_bug(double, double, double, double);
        void draw_cross_hairs(double);
        void draw_needles(double, double, double, double);
        double get_error_info(double, double);

        void BuildTriangles(std::vector<float> &, const std::vector<LocalPoint> &, const std::vector<LocalPoint> &);
        std::vector<float> BuildSphere(void);
};

#endif
