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

void dcUpsMap::setEnableInverseTheta(const std::string &inval)
{
    if (!inval.empty()) enableInverseTheta = getValue(inval)->getInteger();
}

void dcUpsMap::computeLonLat(void) 
{
    longitude = lon->getDecimal();
    latitude = lat->getDecimal();

    if (enableInverseTheta) longitude *= -1;
}

void dcUpsMap::computePosRatios(void) 
{
    // save previous ratios
    double prevHRatio = hRatio;
    double prevVRatio = vRatio;

    // compute unit ratios for x and y
    double theta = (longitude + lonPolarAxis) * M_PI / 180;
    double radius = fabs((latOrigin - latitude) / (latOrigin - latOuter));    // scale of 0..1

    if ( radius > 1 ) radius = 1;

    hRatio = radius * cos(theta) * .5 + .5;
    vRatio = radius * sin(theta) * .5 + .5;

    // calculate current angle of trajectory
    if ( prevVRatio != vRatio || prevHRatio != hRatio)
        trajAngle = atan2((vRatio - prevVRatio), ( hRatio - prevHRatio)) * 180 / M_PI;

//printf("%f %f\n", radius, theta);
printf("%f %f\n", longitude, latitude);
}

void dcUpsMap::computeZoneRatios(void)
{
    zoneLonLatRatios.clear();
    for (uint i = 0; i < zoneLonLatVals.size(); i++) {

        // compute unit ratios for x and y
        double theta = (zoneLonLatVals.at(i).first->getDecimal() + lonPolarAxis) * M_PI / 180;
        double radius = fabs((latOrigin - zoneLonLatVals.at(i).second->getDecimal() ) / (latOrigin - latOuter));    // scale of 0..1
        if ( radius > 1 ) radius = 1;

        // add calculated value
        zoneLonLatRatios.push_back({radius * cos(theta) * .5 + .5, radius * sin(theta) * .5 + .5});
    }
}
