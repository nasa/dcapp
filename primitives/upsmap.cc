#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "commonutils.hh"
#include "upsmap.hh"


dcUpsMap::dcUpsMap(dcParent *myparent) : dcMap(myparent)
{
    return;
}

dcUpsMap::~dcUpsMap()
{
    return;
}

void dcUpsMap::setLonLatParams(const std::string &loPolarAxis, const std::string &laOrigin, const std::string &laOuter)
{
    if (!loPolarAxis.empty() && !laOrigin.empty() && !laOuter.empty())
    {
        lonPolarAxis = getValue(loPolarAxis)->getDecimal();
        latOrigin = getValue(laOrigin)->getDecimal();
        latOuter = getValue(laOuter)->getDecimal();
    }
    else
    {
        printf("map.setLonLatParams: missing values\n");
    }
}

void dcUpsMap::computeLonLat(void) 
{
    if (lon) longitude = lon->getDecimal();
    else longitude = lonPolarAxis;

    if (lat) latitude = lat->getDecimal();
    else latitude = latOrigin;
}

void dcUpsMap::computePosRatios(void) 
{
    // compute unit ratios for x and y
    double theta = (longitude - lonPolarAxis + 90) * M_PI / 180;
    double radius = (latOrigin - latitude) / (latOrigin - latOuter);    // scale of 0..1
    hRatio = radius * cos(theta) * .5 + .5;
    vRatio = radius * sin(theta) * .5 + .5;
}
