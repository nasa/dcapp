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

void dcMap::setLonLat(const std::string &lat1, const std::string &lon1)
{
    if (!lat1.empty() and !lon1.empty())
    {
        lat = getValue(lat1);
        lon = getValue(lon1);
    }
    else
    {
        printf("map.setLonLat: missing values\n");
    }
}

void dcMap::setLonLatRange(const std::string &loMin, const std::string &loMax, const std::string &laMin, const std::string &laMax)
{
    if (!laMin.empty() && !laMax.empty() && !loMin.empty() && !loMax.empty())
    {
        lonMin = getValue(loMin)->getDecimal();
        lonMax = getValue(loMax)->getDecimal();
        latMin = getValue(laMin)->getDecimal();
        latMax = getValue(laMax)->getDecimal();
    }
    else
    {
        printf("map.setLonLatRange: missing values\n");
    }
}

void dcMap::setZoom(const std::string &inval)
{
    if (!inval.empty()) zu = getValue(inval);
}

void dcMap::computeGeometry(void)
{
    if (w) width = w->getDecimal();
    else width = 0;

    if (h) height = h->getDecimal();
    else height = 0;

    double hwidth = (0.5 * width);
    double hheight = (0.5 * height);
    
    left = GeomX(x, width, containerw->getDecimal(), halign);
    bottom = GeomY(y, height, containerh->getDecimal(), valign);

    right = left + width;
    top = bottom + height;
    center = left + hwidth;
    middle = bottom + hheight;

    switch (halign)
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
    }
    switch (valign)
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
    }

    setTextureBounds();
}

// get bounds for texture on 0 to 1 range
void dcMap::setTextureBounds(void)
{
    // compute unit location of texture to draw (0 .. 1)
    double mapWidthRatio, lonRatio, latRatio;

    if (lon) longitude = lon->getDecimal();
    else longitude = (lonMin + lonMax)/2;

    if (lat) latitude = lat->getDecimal();
    else latitude = (latMin + latMax)/2;

    lonRatio = (longitude - lonMin) / (lonMax - lonMin);
    latRatio = (latitude - latMin) / (latMax - latMin);

    // compute unit offset for position
    if (zu) zoom = zu->getDecimal();
    else zoom = 1;

    if (zoom < 1) 
        zoom = 1;

    mapWidthRatio = 1/zoom/2;

    texUp = latRatio + mapWidthRatio;
    texDown = latRatio - mapWidthRatio;
    texLeft = lonRatio - mapWidthRatio;
    texRight = lonRatio + mapWidthRatio;

    if (texUp > 1) {
        texUp = 1;
        texDown = 1 - 2*mapWidthRatio;
    } else if (texDown < 0) {
        texDown = 0;
        texUp = 2*mapWidthRatio;
    }

    if (texRight > 1) {
        texRight = 1;
        texLeft = 1 - 2*mapWidthRatio;
    } else if (texLeft < 0) {
        texLeft = 0;
        texRight = 2*mapWidthRatio;
    }

    printf("%f, %f\n", texLeft, texDown);

}

void dcMap::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, 1, 1, rotate->getDecimal());
    draw_map(this->textureID, width, height, texUp, texDown, texLeft, texRight);
    container_end();
}
