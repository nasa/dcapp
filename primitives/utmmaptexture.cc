#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "commonutils.hh"
#include "utmmaptexture.hh"


dcUtmMapTexture::dcUtmMapTexture(dcMap *mymap) : dcMapTexture(mymap)
{
    return;
}

dcUtmMapTexture::~dcUtmMapTexture()
{
    return;
}

void dcUtmMapTexture::setParams(const std::string &loMin, const std::string &loMax, const std::string &laMin, const std::string &laMax)
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
        printf("utmMap.setLonLatRange: missing values\n");
    }
}

// compute positional ratios, as well as the current trajectory
void dcUtmMapTexture::addGhostTrail(std::vector<std::pair<double, double>> latlons, double trailWidth, Kolor trailColor) 
{
    std::vector<std::pair<float,float>> ghostRatioHistory;

    for (auto const& latlon : latlons) {
        ghostRatioHistory.push_back({
            (latlon.first - lonMin) / (lonMax - lonMin),
            (latlon.second - latMin) / (latMax - latMin)
        });
    }

    ghostTrails.push_back({ghostRatioHistory, trailWidth, trailColor});
}

void dcUtmMapTexture::computePosRatios(void) 
{
    // save previous ratios
    double prevHRatio = hRatio;
    double prevVRatio = vRatio;

    hRatio = (mapInfo->longitude - lonMin) / (lonMax - lonMin);
    vRatio = (mapInfo->latitude - latMin) / (latMax - latMin);

    // calculate current angle of trajectory
    if ( mapInfo->vYaw ) 
        mapInfo->trajAngle = mapInfo->vYaw->getDecimal() + yawOffset;
    else if ( prevVRatio != vRatio || prevHRatio != hRatio)
        mapInfo->trajAngle = atan2((vRatio - prevVRatio), (hRatio - prevHRatio)) * 180 / M_PI;
}

void dcUtmMapTexture::computeZoneRatios(void)
{
    zoneRatios.clear();
    for (unsigned int i = 0; i < mapInfo->zoneVals.size(); i++) {
        // add calculated value
        zoneRatios.push_back({
            (mapInfo->zoneVals.at(i).first->getDecimal() - lonMin) / (lonMax - lonMin),
            (mapInfo->zoneVals.at(i).second->getDecimal() - latMin) / (latMax - latMin)
        });
    }
}

void dcUtmMapTexture::computePointRatios(void)
{
    double mipLatitude, mipLongitude;
    for (unsigned int i = 0; i < imagePoints.size(); i++) {
        mapImagePoint& mip = imagePoints.at(i);
        mipLatitude = (mip.vLatitude)->getDecimal();
        mipLongitude = (mip.vLongitude)->getDecimal();

        mip.hRatio = (mipLongitude - lonMin) / (lonMax - lonMin);
        mip.vRatio = (mipLatitude - latMin) / (latMax - latMin);
    }

    double mspLatitude, mspLongitude;
    for (unsigned int i = 0; i < stringPoints.size(); i++) {
        mapStringPoint& msp = stringPoints.at(i);
        mspLatitude = (msp.vLatitude)->getDecimal();
        mspLongitude = (msp.vLongitude)->getDecimal();

        msp.hRatio = (mspLongitude - lonMin) / (lonMax - lonMin);
        msp.vRatio = (mspLatitude - latMin) / (latMax - latMin);
    }
}