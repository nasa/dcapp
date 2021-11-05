#ifndef _UTMMAP_HH_
#define _UTMMAP_HH_

#include "map.hh"

class dcUtmMap : public dcMap
{
    public:
        dcUtmMap(dcParent *);
        virtual ~dcUtmMap();

        void setLonLatParams(const std::string &, const std::string &, const std::string &, const std::string &);

    private:
        void computeLonLat(void);
        void computePosRatios(void);
        void computeZoneRatios(void);
        void computePointRatios(void);
        
        double lonMin;
        double lonMax;
        double latMin;
        double latMax;
};

#endif
