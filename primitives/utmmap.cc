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

            utmLayerInfos[index] = {lonMin, lonMax, latMin, latMax};
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

// void dcUtmMap::computeZoneRatios(void)
// {
//     zoneLonLatRatios.clear();
//     for (uint i = 0; i < zoneLonLatVals.size(); i++) {
//         // add calculated value
//         zoneLonLatRatios.push_back({
//             (zoneLonLatVals.at(i).first->getDecimal() - lonMin) / (lonMax - lonMin), 
//             (zoneLonLatVals.at(i).second->getDecimal() - latMin) / (latMax - latMin)
//         });
//     }
// }

// void dcUtmMap::computePointRatios(void)
// {
//     for (uint i = 0; i < mapImagePoints.size(); i++) {
//         mapImagePoint& mip = mapImagePoints.at(i);
//         mip.hRatio = (mip.vLongitude->getDecimal() - lonMin) / (lonMax - lonMin);
//         mip.vRatio = (mip.vLatitude->getDecimal() - latMin) / (latMax - latMin);
//     }

//     for (uint i = 0; i < mapStringPoints.size(); i++) {
//         mapStringPoint& msp = mapStringPoints.at(i);
//         msp.hRatio = (msp.vLongitude->getDecimal() - lonMin) / (lonMax - lonMin);
//         msp.vRatio = (msp.vLatitude->getDecimal() - latMin) / (latMax - latMin);
//     }
// }