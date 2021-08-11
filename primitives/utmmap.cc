#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "commonutils.hh"
#include "utmmap.hh"


dcUtmMap::dcUtmMap(dcParent *myparent) : dcMap(myparent)
{
    return;
}

dcUtmMap::~dcUtmMap()
{
    return;
}

void dcUtmMap::setLonLatParams(const std::string &loMin, const std::string &loMax, const std::string &laMin, const std::string &laMax)
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

void dcUtmMap::computeLonLat(void) 
{
    if (lon) longitude = lon->getDecimal();
    else longitude = (lonMin + lonMax)/2;

    if (lat) latitude = lat->getDecimal();
    else latitude = (latMin + latMax)/2;
}

void dcUtmMap::computePosRatios(void) 
{
    // save previous ratios
    double prevHRatio = hRatio;
    double prevVRatio = vRatio;
    
    hRatio = (longitude - lonMin) / (lonMax - lonMin);
    vRatio = (latitude - latMin) / (latMax - latMin);

    // calculate current angle of trajectory
    if ( prevVRatio != vRatio || prevHRatio != hRatio)
        trajAngle = atan2((vRatio - prevVRatio), ( hRatio - prevHRatio)) * 180 / M_PI;
}
