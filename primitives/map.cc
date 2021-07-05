#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "commonutils.hh"
#include "map.hh"


dcMap::dcMap(dcParent *myparent) : dcGeometric(myparent), textureID(0x0), zoom(1), selected(false)
{
    return;
}

dcMap::~dcMap()
{
    return;
}

void dcMap::setTexture(const std::string &filename)
{
    this->textureID = tdLoadTexture(filename);
}

void dcMap::setLatLong(const std::string &lat1, const std::string &lon1)
{
    if (!lat1.empty() and !lon1.empty())
    {
        lat = getValue(lat1);
        lon = getValue(lon1);
    }
    else
    {
        printf("map.setLatLong: missing values\n");
    }
}

void dcMap::setLatLongRange(const std::string &laMin, const std::string &laMax, const std::string &loMin, const std::string &loMax)
{
    if (!laMin.empty() && !laMax.empty() && !loMin.empty() && !loMax.empty())
    {
        latMin = getValue(laMin)->getDecimal();
        latMax = getValue(laMax)->getDecimal();
        longMin = getValue(loMin)->getDecimal();
        longMax = getValue(loMax)->getDecimal();
    }
    else
    {
        printf("map.setLatLongRange: missing values\n");
    }
}

void dcMap::setZoom(const std::string &inval)
{
    if (!inval.empty()) zu = getValue(inval);
}

void dcMap::computeGeometry(void)
{
    if (w) basew = w->getDecimal();
    else basew = 0;

    if (h) baseh = h->getDecimal();
    else baseh = 0;

    if (zu) zoom = zu->getDecimal();
    else zoom = 1; 

    width = basew * zoom;
    height = baseh * zoom;

    double hwidth = (0.5 * width);
    double hheight = (0.5 * height);

    // calculate x and y position using latlong
    double val, llx, lly;

    latitude = lat->getDecimal();
    longitude = lon->getDecimal();
    lly = (baseh) / (latMax - latMin)* (latitude - latMin);
    llx = (basew) / (longMax - longMin) * (longitude - longMin);

    // find x value
    if (originx == dcRight) val = containerw->getDecimal() - llx;
        else val = llx;
    left = (val - (width/2));

    // find y value
    if (originy == dcTop) val = containerh->getDecimal() - lly;
        else val = lly;
    bottom = (val - (height/2));

    right = left + width;
    top = bottom + height;
    center = left + hwidth;
    middle = bottom + hheight;

/*    switch (halign)
    {
        case dcLeft:
            refx = left;
            delx = 0;
            break;
        case dcCenter:
            refx = center;
            delx = hwidth;
            break;
        case dcRight:
            refx = right;
            delx = width;
            break;
        default:
            break;
    }*/
    refx = center;
    delx = hwidth;

    /*switch (valign)
    {
        case dcBottom:
            refy = bottom;
            dely = 0;
            break;
        case dcMiddle:
            refy = middle;
            dely = hheight;
            break;
        case dcTop:
            refy = top;
            dely = height;
            break;
        default:
            break;
    }*/
    refy = middle;
    dely = hheight;
}

void dcMap::draw(void)
{
    computeGeometry();

    // adjust the 1,1 scale here (vertical/horizontal scaling)
    container_start(refx, refy, delx, dely, 1, 1, rotate->getDecimal());

    draw_map(this->textureID, width, height);
    container_end();
}
