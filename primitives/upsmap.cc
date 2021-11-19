#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "commonutils.hh"
#include "upsmap.hh"


dcUpsMap::dcUpsMap(dcParent *myparent) : dcMap(myparent), enableInverseThetaMultiplier(1)
{
    return;
}

dcUpsMap::~dcUpsMap()
{
    return;
}

void dcUpsMap::setLonLatParams(const std::string &pos, const std::string &loPolarAxis, const std::string &laOrigin, const std::string &laOuter)
{
    if (!pos.empty()) 
    {
        int index = getValue(pos)->getInteger();
        if (!loPolarAxis.empty() && !laOrigin.empty() && !laOuter.empty())
        {
            double polarAxisOffset = getValue(loPolarAxis)->getDecimal();
            double latOrigin = getValue(laOrigin)->getDecimal();
            double latOuter = getValue(laOuter)->getDecimal();

            upsLayerInfos.insert({
                index,
                {polarAxisOffset, latOrigin, latOuter}
            });
        }
    }
    else
    {
        printf("upsMap.setLonLatParams: missing values\n");
    }
}

void dcUpsMap::setEnableInverseTheta(const std::string &inval)
{
    if (!inval.empty()) {        
        if (getValue(inval)->getInteger()) enableInverseThetaMultiplier = -1;
        else enableInverseThetaMultiplier = 1;
    }
}

void dcUpsMap::fetchLonLat(void) 
{
    longitude = vLongitude->getDecimal() * enableInverseThetaMultiplier;
    latitude = vLatitude->getDecimal();
}

void dcUpsMap::fetchChildParams(void) 
{
    uliCurrent = &(upsLayerInfos[textureIndex]);
}


// compute positional ratios, as well as the current trajectory
void dcUpsMap::computePosRatios(void) 
{
    // save previous ratios
    double prevHRatio = mliCurrent->hRatio;
    double prevVRatio = mliCurrent->vRatio;

    // compute unit ratios for x and y
    for (auto const& pair : upsLayerInfos) 
    {
        int id = pair.first;
        const upsLayerInfo* uli = &(pair.second);
        mapLayerInfo* mli = &(mapLayerInfos[id]);

        double theta = (longitude + uli->polarAxisOffset) * M_PI / 180;
        double radius = fabs((uli->latOrigin - latitude) / (uli->latOrigin - uli->latOuter));    // scale of 0..1

        if ( radius > 1 ) radius = 1;

        mli->hRatio = radius * cos(theta) * .5 + .5;
        mli->vRatio = radius * sin(theta) * .5 + .5;
    }

    // compute trajectory
    if ( vYaw )
        trajAngle = vYaw->getDecimal() + yawOffset;
    else if ( prevVRatio != mliCurrent->vRatio || prevHRatio != mliCurrent->hRatio)
        trajAngle = atan2((mliCurrent->vRatio - prevVRatio), ( mliCurrent->hRatio - prevHRatio)) * 180 / M_PI;
}

void dcUpsMap::computeZoneRatios(void)
{
    zoneLonLatRatios.clear();
    for (uint i = 0; i < zoneLonLatVals.size(); i++) {

        // compute unit ratios for x and y
        double theta = (zoneLonLatVals.at(i).first->getDecimal() * enableInverseThetaMultiplier + uliCurrent->polarAxisOffset) * M_PI / 180;
        double radius = fabs((uliCurrent->latOrigin - zoneLonLatVals.at(i).second->getDecimal() ) / (uliCurrent->latOrigin - uliCurrent->latOuter));

        // add calculated value
        zoneLonLatRatios.push_back({radius * cos(theta) * .5 + .5, radius * sin(theta) * .5 + .5});
    }
}

// void dcUpsMap::computePointRatios(void)
// {
//     for (uint i = 0; i < mapImagePoints.size(); i++) {

//         // compute unit ratios for x and y
//         mapImagePoint& mip = mapImagePoints.at(i);
//         double theta = (mip.vLongitude->getDecimal() * enableInverseThetaMultiplier + polarAxisOffset) * M_PI / 180;
//         double radius = fabs((latOrigin - mip.vLatitude->getDecimal()  ) / (latOrigin - latOuter));    // scale of 0..1

//         // update position ratios
//         mip.hRatio = radius * cos(theta) * .5 + .5;
//         mip.vRatio = radius * sin(theta) * .5 + .5;
//     }

//     for (uint i = 0; i < mapStringPoints.size(); i++) {

//         // compute unit ratios for x and y
//         mapStringPoint& msp = mapStringPoints.at(i);
//         double theta = (msp.vLongitude->getDecimal() * enableInverseThetaMultiplier + polarAxisOffset) * M_PI / 180;
//         double radius = fabs((latOrigin - msp.vLatitude->getDecimal()  ) / (latOrigin - latOuter));    // scale of 0..1

//         // update position ratios
//         msp.hRatio = radius * cos(theta) * .5 + .5;
//         msp.vRatio = radius * sin(theta) * .5 + .5;
//     }
// }
