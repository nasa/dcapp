#ifndef _MAP_HH_
#define _MAP_HH_

#include <string>
#include <vector>
#include <map>
#include "RenderLib/RenderLib.hh"
#include "geometric.hh"
#include "parent.hh"
#include "kolor.hh"
#include "maptexture.hh"

#define SQRT_2 1.414213562373095048802

class dcMapTexture;

class dcMap : public dcGeometric
{
    public:
        dcMap(dcParent *);
        virtual ~dcMap();

        void setTexture(const std::string &, dcMapTexture*);
        void setTextureIndex(const std::string &);
        void setLonLat(const std::string &, const std::string &);
        void setZoom(const std::string &);
        void setSizeRatio(const std::string &, const std::string &);
        void setYaw(const std::string &);
        void setEnableCircularMap(const std::string &);
        void setEnableTrackUp(const std::string &);
        void setEnableIcon(const std::string &);
        void setEnableTrail(const std::string &);
        void setTrailColor(const std::string &);
        void setTrailWidth(const std::string &);
        void setTrailResolution(const std::string &);
        void setFnClearTrail(const std::string &);
        void setIconRotationOffset(const std::string &);
        void setIconTexture(const std::string &);
        void setIconSize(const std::string &, const std::string &);
        void setZoneLonLat(const std::string &, const std::string &, const std::string &, const std::string &, const std::string &, const std::string &, 
            const std::string &, const std::string &);
        void setUnlocked(const std::string &);

        void draw(void);
        void processPreCalculations(void);
        void processPostCalculations(void);
        void handleMousePress(double, double);
        void handleMouseMotion(double, double);
        void handleMouseRelease(void);

    private:
        void displayIcon(void);
        void displayTrail(void);
        void displayGhostTrail(void);
        void displayZone(void);
        void displayPoints(void);
        void computeGeometry(void);
        void computeTextureBounds(void);
        void updateTrail(void);

        /* live variable from dcapp panel */
        Value* vTextureIndex;
        Value* vLatitude;
        Value* vLongitude;
        Value* vZoom;
    public:
        Value* vYaw;    // needed for computing yaw in texture classes

        /* variables calculated from above */
        double longitude;
        double latitude;
        double zoom;
        double textureIndex;
        double trajAngle;
    private:
        dcMapTexture* mtCurrent;

        /* list of textures */
        std::map<int, dcMapTexture*> mapTextures;

        /* variables used for render params */
        double displayWidth;
        double displayHeight;
        double widthOffset;
        double heightOffset;

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
        Value* vEnableTrackUp;
        bool enableTrackUp;

        /* zone parameters */
    public:
        std::vector<std::pair<Value*,Value*>> zoneVals;
    private:
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
        void fetchDisplayRatios(void);
};

#endif
