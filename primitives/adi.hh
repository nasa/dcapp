#ifndef _ADI_HH_
#define _ADI_HH_

#include <vector>
#include "dc.hh"
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
	
		std::vector< float >	sphereTriangles;
	
        void draw_roll_bug(float, float, float, float);
        void draw_cross_hairs(float);
        void draw_needles(float, float, float, float);
		float get_error_info(float, float);
	
		void				BuildTriangles	( std::vector<float> &listA, const std::vector<LocalPoint> &list1A, const std::vector<LocalPoint> &list2A  );
		std::vector<float>	BuildSphere		( void );
};

#endif
