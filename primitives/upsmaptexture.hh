#ifndef _UPSMAPTEXTURE_HH_
#define _UPSMAPTEXTURE_HH_

#include <vector>
#include <string>
#include "map.hh"
#include "maptexture.hh"

class dcUpsMapTexture : public dcMapTexture
{
    public:
        dcUpsMapTexture(dcMap*);
        virtual ~dcUpsMapTexture();

        void setParams(const std::string &, const std::string &, const std::string &, 
            const std::string &, const std::string &);

    private:
        double topLeftUnitX;
        double topLeftUnitY;
        double bottomRightUnitX;
        double bottomRightUnitY;
        int thetaFactor;

        void computePosRatios(void);
        void addGhostTrail(std::vector<std::pair<double, double>>, double, Kolor);
        void computeZoneRatios(void);
        void computePointRatios(void);
};

#endif
