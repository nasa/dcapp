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
        void setLatLong(const std::string &, const std::string &);
        void setLatLongRange(const std::string &, const std::string &, const std::string &, const std::string &);
        void setZoom(const std::string &);
        void computeGeometry(void);
        void draw(void);

        // possibly implement mouse press + release for resetting the rover path
        //void handleMousePress(double, double);
        //void handleMouseRelease(void);

    private:
        tdTexture *textureID;
        Value* lat;
        Value* lon;
        Value *zu;

        double latitude;
        double longitude;

        double latMin;
        double latMax;
        double longMin;
        double longMax;
        double basew;
        double baseh;

        double zoom;

        bool selected;
};

#endif
