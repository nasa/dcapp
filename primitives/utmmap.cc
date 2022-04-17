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

void dcUtmMap::setLonLatParams(const std::string &pos, const std::string &loMin, const std::string &loMax, const std::string &laMin, const std::string &laMax)
{
    if (!pos.empty()) 
    {
        int index = getValue(pos)->getInteger();
        if (!laMin.empty() && !laMax.empty() && !loMin.empty() && !loMax.empty())
        {
            double lonMin = getValue(loMin)->getDecimal();
            double lonMax = getValue(loMax)->getDecimal();
            double latMin = getValue(laMin)->getDecimal();
            double latMax = getValue(laMax)->getDecimal();

            utmLayerInfos.insert({
                index,
                {lonMin, lonMax, latMin, latMax}
            });
        }
    }
    else
    {
        printf("utmMap.setLonLatRange: missing values\n");
    }
}

void dcUtmMap::fetchLonLat(void) 
{
    longitude = vLongitude->getDecimal();
    latitude = vLatitude->getDecimal();
}

void dcUtmMap::fetchChildParams(void) 
{
    uliCurrent = &(utmLayerInfos[textureIndex]);
}


void dcUtmMap::computePosRatios(void) 
{
    // save previous ratios
    double prevHRatio = mliCurrent->hRatio;
    double prevVRatio = mliCurrent->vRatio;
    
    // compute unit ratios for x and y
    for (auto const& pair : utmLayerInfos) 
    {
        int id = pair.first;
        const utmLayerInfo* uli = &(pair.second);
        mapLayerInfo* mli = &(mapLayerInfos[id]);

        mli->hRatio = (longitude - uli->lonMin) / (uli->lonMax - uli->lonMin);
        mli->vRatio = (latitude - uli->latMin) / (uli->latMax - uli->latMin);
    }

    // calculate current angle of trajectory
    if ( vYaw ) 
        trajAngle = vYaw->getDecimal() + yawOffset;
    else if ( prevVRatio != mliCurrent->vRatio || prevHRatio != mliCurrent->hRatio)
        trajAngle = atan2((mliCurrent->vRatio - prevVRatio), (mliCurrent->hRatio - prevHRatio)) * 180 / M_PI;
}

// compute positional ratios, as well as the current trajectory
std::vector<std::pair<float,float>>
dcUtmMap::computeGhostTrailRatios(int index, std::vector<std::pair<double, double>> latlons) 
{
    std::vector<std::pair<float,float>> ghostRatioHistory;
    utmLayerInfo* uli = &(utmLayerInfos[index]);

    for (auto const& latlon : latlons) {
        double lat = latlon.first;
        double lon = latlon.second;

        ghostRatioHistory.push_back({
            (lon - uli->lonMin) / (uli->lonMax - uli->lonMin),
            (lat - uli->latMin) / (uli->latMax - uli->latMin)
        });
    }

    return ghostRatioHistory;
}

void dcUtmMap::computeZoneRatios(void)
{
    zoneLonLatRatios.clear();
    for (uint i = 0; i < zoneLonLatVals.size(); i++) {
        // add calculated value
        zoneLonLatRatios.push_back({
            (zoneLonLatVals.at(i).first->getDecimal() - uliCurrent->lonMin) / (uliCurrent->lonMax - uliCurrent->lonMin), 
            (zoneLonLatVals.at(i).second->getDecimal() - uliCurrent->latMin) / (uliCurrent->latMax - uliCurrent->latMin)
        });
    }
}

void dcUtmMap::computePointRatios(void)
{
    double mipLatitude, mipLongitude;
    for (uint i = 0; i < mliCurrent->imagePoints.size(); i++) {
        mapImagePoint& mip = mliCurrent->imagePoints.at(i);
        mipLatitude = (mip.vLatitude)->getDecimal();
        mipLongitude = (mip.vLongitude)->getDecimal();

        mip.hRatio = (mipLongitude- uliCurrent->lonMin) / (uliCurrent->lonMax - uliCurrent->lonMin);
        mip.vRatio = (mipLatitude - uliCurrent->latMin) / (uliCurrent->latMax - uliCurrent->latMin);
    }

    double mspLatitude, mspLongitude;
    for (uint i = 0; i < mliCurrent->stringPoints.size(); i++) {
        mapStringPoint& msp = mliCurrent->stringPoints.at(i);
        mspLatitude = (msp.vLatitude)->getDecimal();
        mspLongitude = (msp.vLongitude)->getDecimal();

        msp.hRatio = (mspLongitude - uliCurrent->lonMin) / (uliCurrent->lonMax - uliCurrent->lonMin);
        msp.vRatio = (mspLatitude - uliCurrent->latMin) / (uliCurrent->latMax - uliCurrent->latMin);
    }
}