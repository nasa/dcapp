#ifndef _UTMMAP_HH_
#define _UTMMAP_HH_

#include "map.hh"

class dcUtmMap : public dcMap
{
    public:
        dcUtmMap(dcParent *);
        virtual ~dcUtmMap();

        void setLonLatParams(const std::string &, const std::string &, const std::string &, const std::string &, const std::string &);

    private:
        typedef struct {
            double lonMin;
            double lonMax;
            double latMin;
            double latMax;
        } utmLayerInfo;

        std::map<int,utmLayerInfo> utmLayerInfos;
        utmLayerInfo* uliCurrent;

        void fetchLonLat(void);
        void fetchChildParams(void);
        void computePosRatios(void);
        std::vector<std::pair<float,float>> computeGhostTrailRatios(int, std::vector<std::pair<double, double>>);
        void computeZoneRatios(void);
        void computePointRatios(void);
};

#endif
