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
    if (vLongitude) {
        longitude = vLongitude->getDecimal();
        if (longitude < lonMin) 
            longitude = lonMin;
        else if (longitude > lonMax) 
            longitude = lonMax;
    } else 
        longitude = (lonMin + lonMax)/2;

    if (vLatitude) {
        latitude = vLatitude->getDecimal();
        if (latitude < latMin) 
            latitude = latMin;
        else if 
            (latitude > latMax)
            latitude = latMax;
    } else 
        latitude = (latMin + latMax)/2;
}

void dcUtmMap::computePosRatios(void) 
{
    // save previous ratios
    double prevHRatio = hRatio;
    double prevVRatio = vRatio;
    
    hRatio = (longitude - lonMin) / (lonMax - lonMin);
    vRatio = (latitude - latMin) / (latMax - latMin);

    // calculate current angle of trajectory
    if ( vYaw ) 
        trajAngle = vYaw->getDecimal() + yawOffset;
    else if ( prevVRatio != vRatio || prevHRatio != hRatio)
        trajAngle = atan2((vRatio - prevVRatio), ( hRatio - prevHRatio)) * 180 / M_PI;
}

void dcUtmMap::computeZoneRatios(void)
{
    zoneLonLatRatios.clear();
    for (uint i = 0; i < zoneLonLatVals.size(); i++) {
        // add calculated value
        zoneLonLatRatios.push_back({
            (zoneLonLatVals.at(i).first->getDecimal() - lonMin) / (lonMax - lonMin), 
            (zoneLonLatVals.at(i).second->getDecimal() - latMin) / (latMax - latMin)
        });
    }
}

void dcUtmMap::computePointRatios(void)
{
    for (uint i = 0; i < mapImagePoints.size(); i++) {
        mapImagePoint& mip = mapImagePoints.at(i);
        mip.hRatio = (mip.vLongitude->getDecimal() - lonMin) / (lonMax - lonMin);
        mip.vRatio = (mip.vLatitude->getDecimal() - latMin) / (latMax - latMin);
    }

    for (uint i = 0; i < mapStringPoints.size(); i++) {
        mapStringPoint& msp = mapStringPoints.at(i);
        msp.hRatio = (msp.vLongitude->getDecimal() - lonMin) / (lonMax - lonMin);
        msp.vRatio = (msp.vLatitude->getDecimal() - latMin) / (latMax - latMin);
    }
}