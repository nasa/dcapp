#ifndef _MAPTEXTURE_HH_
#define _MAPTEXTURE_HH_

#include <string>
#include <vector>
#include <map>
#include "RenderLib/RenderLib.hh"
#include "geometric.hh"
#include "parent.hh"
#include "kolor.hh"
#include "map.hh"

typedef struct {
            tdTexture* textureID;
            Value* vLongitude;
            Value* vLatitude;
            Value* vEnabled;
            double width;
            double height;
            double hRatio;
            double vRatio;
            bool enableScaling;
        } mapImagePoint;

        typedef struct {
            Value* vText;
            Value* vLongitude;
            Value* vLatitude;
            Value* vEnabled;
            double size;
            double hRatio;
            double vRatio;
            bool enableScaling;
        } mapStringPoint;

class dcMap;

class dcMapTexture
{
    public:
        dcMapTexture(dcMap*);
        virtual ~dcMapTexture();

        void setTexture(const std::string &);
        void setYawOffset(const std::string &);
        void setSizeRatio(const std::string &);
        void setGhostTrail(const std::string &, const std::string &, const std::string &);
        void setMapImagePoint(const std::string &, const std::string &, const std::string &, const std::string &, 
            const std::string &, const std::string &, const std::string &);
        void setMapStringPoint(const std::string &, const std::string &, const std::string &, const std::string &, 
            const std::string &, const std::string &);

    protected:
        typedef struct {
            std::vector<std::pair<float,float>> ghostRatioHistory;
            double trailWidth;
            Kolor trailColor;
        } ghostTrailInfo;

        dcMap* mapInfo;
        
    public:
        tdTexture* textureID;
        double hRatio;
        double vRatio;
        double sizeRatio;
        double yawOffset;

        std::vector<std::pair<double,double>> ratioHistory;
        std::vector<ghostTrailInfo> ghostTrails;
        std::vector<mapImagePoint> imagePoints;
        std::vector<mapStringPoint> stringPoints;
        std::vector<std::pair<double,double>> zoneRatios;

        virtual void computePosRatios(void) = 0;
        virtual void addGhostTrail(std::vector<std::pair<double, double>>, double, Kolor) = 0;
        virtual void computeZoneRatios(void) = 0;
        virtual void computePointRatios(void) = 0;
};

#endif
