#ifndef _MAP_HH_
#define _MAP_HH_

#include <string>
#include <vector>
#include "RenderLib/RenderLib.hh"
#include "geometric.hh"
#include "parent.hh"
#include "kolor.hh"


class dcMap : public dcGeometric
{
    public:
        dcMap(dcParent *);
        virtual ~dcMap();

        void setTexture(const std::string &);
        void setLonLat(const std::string &, const std::string &);
        void setZoom(const std::string &);
        void setEnablePositionIndicator(const std::string &);
        void setEnablePositionTrail(const std::string &);
        void setTrailColor(const std::string &);
        void setTrailWidth(const std::string &);
        void setTrailClear(const std::string &);
        void setIconRotationOffset(const std::string &inval);
        void setIconTexture(const std::string &);
        void setIconSize(const std::string &, const std::string &);
        void draw(void);

        // exists in all children, but different number of params 
        // virtual void setLonLatParams(...) = 0;

        // possibly implement mouse press + release for resetting the rover path
        //void handleMousePress(double, double);
        //void handleMouseRelease(void);

    protected:
        void displayPositionIndicator(void);
        void displayPositionTrail(void);
        void computeGeometry(void);
        void computeTextureBounds(void);
        void updatePositionTrail(void);
        void remapXYBounds(std::pair<float,float>& p);

        virtual void computeLonLat(void) = 0;
        virtual void computePosRatios(void) = 0;

        tdTexture *textureID;
        Value* lat;
        Value* lon;
        Value* zu;

        double longitude;
        double latitude;
        double zoom;

        double texUp;
        double texDown;
        double texLeft;
        double texRight;

        double hRatio;
        double vRatio;
        double trajAngle;

        bool enablePositionIndicator;
        bool enablePositionTrail;
        double trailWidth;
        Kolor trailColor;
        std::vector<std::pair<float,float>> positionHistory;
        Value* clearTrails;     // more of a function

        tdTexture *iconTextureID;
        double iconHeight;
        double iconWidth;
        bool enableCustomIcon;
        double iconRotationOffset;

        bool selected;
};

#endif
