#ifndef _UTMMAPTEXTURE_HH_
#define _UTMMAPTEXTURE_HH_

#include "map.hh"
#include "maptexture.hh"

class dcMap;

class dcUtmMapTexture : public dcMapTexture
{
    public:
        dcUtmMapTexture(dcMap*);
        virtual ~dcUtmMapTexture();

        void setParams(const std::string &, const std::string &, const std::string &, 
            const std::string &);

    private:
        double lonMin;
        double lonMax;
        double latMin;
        double latMax;

        void computePosRatios(void);
        void computeYaw(void);
        void addGhostTrail(std::vector<std::pair<double, double>>, double, Kolor);
        void computeZoneRatios(void);
        void computePointRatios(void);

        double lonToHRatio(double lon);
};

#endif
