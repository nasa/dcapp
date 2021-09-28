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
        void setEnableIcon(const std::string &);
        void setEnableTrail(const std::string &);
        void setTrailColor(const std::string &);
        void setTrailWidth(const std::string &);
        void setTrailResolution(const std::string &);
        void setFnClearTrail(const std::string &);
        void setIconRotationOffset(const std::string &inval);
        void setIconTexture(const std::string &);
        void setIconSize(const std::string &, const std::string &);
        void setZoneLonLat(const std::string &, const std::string &, const std::string &, const std::string &, 
            const std::string &, const std::string &, const std::string &, const std::string &);
        void draw(void);
        void processPreCalculations(void);
        void processPostCalculations(void);

        // exists in all children, but different number of params 
        // virtual void setLonLatParams(...) = 0;

        // possibly implement mouse press + release for resetting the rover path
        //void handleMousePress(double, double);
        //void handleMouseRelease(void);

    protected:
        void displayIcon(void);
        void displayTrail(void);
        void displayZone(void);
        void computeGeometry(void);
        void computeTextureBounds(void);
        void updateTrail(void);

        virtual void computeLonLat(void) = 0;
        virtual void computePosRatios(void) = 0;
        virtual void computeZoneRatios(void) = 0;

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

        bool enableIcon;
        bool enableTrail;
        double trailWidth;
        Kolor trailColor;
        std::vector<std::pair<float,float>> positionHistory;
        Value* fnClearTrail;     // more of a function
        double trailResolution;

        tdTexture *iconTextureID;
        double iconHeight;
        double iconWidth;
        bool enableCustomIcon;
        double iconRotationOffset;

        std::vector<std::pair<Value*,Value*>> zoneLonLatVals;
        std::vector<std::pair<double,double>> zoneLonLatRatios;
        bool enableZone;

        bool selected;

    private:
        int prev_clear_state;
};

#endif
