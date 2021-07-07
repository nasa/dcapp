#ifndef _MAP_HH_
#define _MAP_HH_

#include <string>
#include "RenderLib/RenderLib.hh"
#include "geometric.hh"
#include "parent.hh"

class dcMap : public dcGeometric
{
    public:
        dcMap(dcParent *);
        virtual ~dcMap();

        void setTexture(const std::string &);
        void setLonLat(const std::string &, const std::string &);
        void setLonLatRange(const std::string &, const std::string &, const std::string &, const std::string &);
        void setZoom(const std::string &);
        void computeGeometry(void);
        void draw(void);

        // possibly implement mouse press + release for resetting the rover path
        //void handleMousePress(double, double);
        //void handleMouseRelease(void);

    private:
        void setTextureBounds(void);

        tdTexture *textureID;
        Value* lat;
        Value* lon;
        Value *zu;

        double longitude;
        double latitude;
        
        double lonMin;
        double lonMax;
        double latMin;
        double latMax;
        double zoom;

        double texUp;
        double texDown;
        double texLeft;
        double texRight;

        bool selected;
};

#endif
