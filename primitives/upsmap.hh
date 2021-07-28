#ifndef _UPSMAP_HH_
#define _UPSMAP_HH_

#include <string>
#include "RenderLib/RenderLib.hh"
#include "geometric.hh"
#include "parent.hh"

class dcUpsMap : public dcGeometric
{
    public:
        dcUpsMap(dcParent *);
        virtual ~dcUpsMap();

        void setTexture(const std::string &);
        void setLonLat(const std::string &, const std::string &);
        void setLonLatParams(const std::string &, const std::string &, const std::string &);
        void setZoom(const std::string &);
        void computeGeometry(void);
        void draw(void);

        // possibly implement mouse press + release for resetting the rover path
        //void handleMousePress(double, double);
        //void handleMouseRelease(void);

    private:
        void setTextureBounds(void);
        void displayCurrentPosition(void);

        tdTexture *textureID;
        Value* lat;
        Value* lon;
        Value *zu;

        double longitude;
        double latitude;
        
        double lonPolarAxis;
        double latOrigin;
        double latOuter;
        double zoom;

        double texUp;
        double texDown;
        double texLeft;
        double texRight;

        double hRatio;
        double vRatio;

        bool selected;
        double polarAxisPosition;       // position of polar axis in radians (TODO)
};

#endif
