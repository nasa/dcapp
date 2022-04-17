#ifndef _MAP_HH_
#define _MAP_HH_

#include <string>
#include <vector>
#include <map>
#include "RenderLib/RenderLib.hh"
#include "geometric.hh"
#include "parent.hh"
#include "kolor.hh"

#define SQRT_2 1.414213562373095048802

class dcMap : public dcGeometric
{
    public:
        dcMap(dcParent *);
        virtual ~dcMap();

        void setTexture(const std::string &, const std::string &);
        void setTextureIndex(const std::string &);
        void setLonLat(const std::string &, const std::string &);
        void setZoom(const std::string &);
        void setSizeRatio(const std::string &, const std::string &);
        void setYaw(const std::string &, const std::string &);
        void setEnableCircularMap(const std::string &);
        void setEnableTrackUp(const std::string &);
        void setEnableIcon(const std::string &);
        void setEnableTrail(const std::string &);
        void setTrailColor(const std::string &);
        void setTrailWidth(const std::string &);
        void setTrailResolution(const std::string &);
        void setFnClearTrail(const std::string &);
        void setGhostTrail(const std::string &, const std::string &, const std::string &, const std::string &);
        void setIconRotationOffset(const std::string &);
        void setIconTexture(const std::string &);
        void setIconSize(const std::string &, const std::string &);
        void setZoneLonLat(const std::string &, const std::string &, const std::string &, const std::string &, const std::string &, const std::string &, 
            const std::string &, const std::string &);
        void setMapImagePoint(const std::string &, const std::string &, const std::string &, const std::string &, 
            const std::string &, const std::string &, const std::string &, const std::string &);
        void setMapStringPoint(const std::string &, const std::string &, const std::string &, const std::string &, 
            const std::string &, const std::string &, const std::string &);
        void setUnlocked(const std::string &);

        void draw(void);
        void processPreCalculations(void);
        void processPostCalculations(void);
        void handleMousePress(double, double);
        void handleMouseMotion(double, double);
        void handleMouseRelease(void);

    protected:
        typedef struct {
            std::vector<std::pair<float,float>> ghostRatioHistory;
            double trailWidth;
            Kolor trailColor;
        } ghostTrailInfo;

        typedef struct {
            tdTexture* textureID;
            Value* vLongitude;
            Value* vLatitude;
            Value* vEnabled;
            double width;
            double height;
            double hRatio;
            double vRatio;
            std::vector<int> layers;
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
            std::vector<int> layers;
            bool enableScaling;
        } mapStringPoint;

        typedef struct {
            tdTexture* textureID;
            double hRatio;
            double vRatio;
            double sizeRatio;

            std::vector<std::pair<float,float>> ratioHistory;
            std::vector<ghostTrailInfo> ghostTrails;
            std::vector<mapImagePoint> imagePoints;
            std::vector<mapStringPoint> stringPoints;
        } mapLayerInfo;

        void displayIcon(void);
        void displayTrail(void);
        void displayGhostTrail(void);
        void displayZone(void);
        void displayPoints(void);
        void computeGeometry(void);
        void computeTextureBounds(void);
        void updateTrail(void);

        virtual void fetchLonLat(void) = 0;
        virtual void fetchChildParams(void) = 0;
        virtual void computePosRatios(void) = 0;
        virtual std::vector<std::pair<float,float>> computeGhostTrailRatios(int, std::vector<std::pair<double, double>>) = 0;
        virtual void computeZoneRatios(void) = 0;
        virtual void computePointRatios(void) = 0;

        /* live variable from dcapp panel */
        std::map<int,mapLayerInfo> mapLayerInfos;
        Value* vTextureIndex;
        Value* vLatitude;
        Value* vLongitude;
        Value* vZoom;
        Value* vYaw;

        /* variables calculated from above */
        double longitude;
        double latitude;
        double zoom;
        double textureIndex;
        mapLayerInfo* mliCurrent;

        /* variables used for render params */
        double displayWidth;
        double displayHeight;
        double widthOffset;
        double heightOffset;

        /* (optionally) calculated variables */
        double trajAngle;
        double yawOffset;

        /* calculated variables */
        double texUp;
        double texDown;
        double texLeft;
        double texRight;
        double hRatio;
        double vRatio;

        /* trail params */
        bool enableTrail;
        double trailWidth;
        double trailResolution;
        Kolor trailColor;
        Value* fnClearTrail;     // more akin to a function

        /* icon params */
        bool enableIcon;
        bool enableCustomIcon;
        double iconHeight;
        double iconWidth;
        double iconRotationOffset;
        tdTexture *iconTextureID;

        /* view params */
        bool enableCircularMap;
        bool enableTrackUp;

        /* zone parameters */
        std::vector<std::pair<Value*,Value*>> zoneLonLatVals;
        std::vector<std::pair<double,double>> zoneLonLatRatios;
        bool enableZone;

        /* map scrolling */
        Value* vUnlocked;
        bool unlocked;
        double scrollX;
        double scrollY;
        bool selected;

    private:
        int prev_clear_state;   // tied to fnClearTrail
        double mapWidthRatio;
        
        void fetchBaseParams(void);
        void updateCurrentParams(void);
};

#endif
