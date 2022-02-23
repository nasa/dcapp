#ifndef _UPSMAP_HH_
#define _UPSMAP_HH_

#include "map.hh"

class dcUpsMap : public dcMap
{
    public:
        dcUpsMap(dcParent *);
        virtual ~dcUpsMap();

        void setLonLatParams(const std::string &, const std::string &, const std::string &, const std::string &);
        void setEnableInverseTheta(const std::string &);

    private:
        typedef struct {
            double polarAxisOffset;
            double latOrigin;
            double latOuter;
            double baseRadius;
        } upsLayerInfo;

        std::map<int,upsLayerInfo> upsLayerInfos;
        upsLayerInfo* uliCurrent;
        int enableInverseThetaMultiplier;

        void fetchLonLat(void);
        void fetchChildParams(void);
        void computePosRatios(void);
        void computeGhostTrailRatios(std::vector<std::pair<double, double>>);
        void computeZoneRatios(void);
        void computePointRatios(void);
};

#endif
