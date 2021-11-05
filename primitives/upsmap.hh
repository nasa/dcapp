#ifndef _UPSMAP_HH_
#define _UPSMAP_HH_

#include "map.hh"

class dcUpsMap : public dcMap
{
    public:
        dcUpsMap(dcParent *);
        virtual ~dcUpsMap();

        void setLonLatParams(const std::string &, const std::string &, const std::string &);
        void setEnableInverseTheta(const std::string &);

    private:
        void computeLonLat(void);
        void computePosRatios(void);
        void computeZoneRatios(void);
        void computePointRatios(void);
        
        double polarAxisOffset;
        double latOrigin;
        double latOuter;
        int enableInverseThetaMultiplier;
};

#endif
