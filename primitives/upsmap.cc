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

void dcUpsMap::setLonLatParams(const std::string &loPolarAxis, const std::string &laOrigin, const std::string &laOuter)
{
    if (!loPolarAxis.empty() && !laOrigin.empty() && !laOuter.empty())
    {
        polarAxisOffset = getValue(loPolarAxis)->getDecimal();
        latOrigin = getValue(laOrigin)->getDecimal();
        latOuter = getValue(laOuter)->getDecimal();
    }
    else
    {
        printf("map.setLonLatParams: missing values\n");
    }
}

void dcUpsMap::setEnableInverseTheta(const std::string &inval)
{
    if (!inval.empty()) {        
        if (getValue(inval)->getInteger()) enableInverseThetaMultiplier = -1;
        else enableInverseThetaMultiplier = 1;
    }
}

void dcUpsMap::computeLonLat(void) 
{
    longitude = vLongitude->getDecimal() * enableInverseThetaMultiplier;
    latitude = vLatitude->getDecimal();
}

void dcUpsMap::computePosRatios(void) 
{
    // save previous ratios
    double prevHRatio = hRatio;
    double prevVRatio = vRatio;

    // compute unit ratios for x and y
    double theta = (longitude + polarAxisOffset) * M_PI / 180;
    double radius = fabs((latOrigin - latitude) / (latOrigin - latOuter));    // scale of 0..1

    if ( radius > 1 ) radius = 1;

    hRatio = radius * cos(theta) * .5 + .5;
    vRatio = radius * sin(theta) * .5 + .5;

    // calculate current angle of trajectory
    if ( prevVRatio != vRatio || prevHRatio != hRatio)
        trajAngle = atan2((vRatio - prevVRatio), ( hRatio - prevHRatio)) * 180 / M_PI;
}

void dcUpsMap::computeZoneRatios(void)
{
    zoneLonLatRatios.clear();
    for (uint i = 0; i < zoneLonLatVals.size(); i++) {

        // compute unit ratios for x and y
        double theta = (zoneLonLatVals.at(i).first->getDecimal() * enableInverseThetaMultiplier + polarAxisOffset) * M_PI / 180;
        double radius = fabs((latOrigin - zoneLonLatVals.at(i).second->getDecimal() ) / (latOrigin - latOuter));    // scale of 0..1

        // add calculated value
        zoneLonLatRatios.push_back({radius * cos(theta) * .5 + .5, radius * sin(theta) * .5 + .5});
    }
}

void dcUpsMap::computePointRatios(void)
{
    for (uint i = 0; i < mapImagePoints.size(); i++) {

        // compute unit ratios for x and y
        mapImagePoint& mip = mapImagePoints.at(i);
        double theta = (mip.vLongitude->getDecimal() * enableInverseThetaMultiplier + polarAxisOffset) * M_PI / 180;
        double radius = fabs((latOrigin - mip.vLatitude->getDecimal()  ) / (latOrigin - latOuter));    // scale of 0..1

        // update position ratios
        mip.hRatio = radius * cos(theta) * .5 + .5;
        mip.vRatio = radius * sin(theta) * .5 + .5;
    }

    for (uint i = 0; i < mapStringPoints.size(); i++) {

        // compute unit ratios for x and y
        mapStringPoint& msp = mapStringPoints.at(i);
        double theta = (msp.vLongitude->getDecimal() * enableInverseThetaMultiplier + polarAxisOffset) * M_PI / 180;
        double radius = fabs((latOrigin - msp.vLatitude->getDecimal()  ) / (latOrigin - latOuter));    // scale of 0..1

        // update position ratios
        msp.hRatio = radius * cos(theta) * .5 + .5;
        msp.vRatio = radius * sin(theta) * .5 + .5;
    }
}
