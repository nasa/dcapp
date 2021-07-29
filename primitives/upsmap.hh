#ifndef _UPSMAP_HH_
#define _UPSMAP_HH_

#include "map.hh"

class dcUpsMap : public dcMap
{
    public:
        dcUpsMap(dcParent *);
        virtual ~dcUpsMap();

        void setLonLatParams(const std::string &, const std::string &, const std::string &);

    private:
        void computeLonLat(void);
        void computePosRatios(void);
        
        double lonPolarAxis;
        double latOrigin;
        double latOuter;
        double polarAxisPosition;       // position of polar axis in radians (TODO)
};

#endif
