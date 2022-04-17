#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "commonutils.hh"
#include "upsmap.hh"


dcUpsMap::dcUpsMap(dcParent *myparent) : dcMap(myparent), thetaFactor(1)
{
    return;
}

dcUpsMap::~dcUpsMap()
{
    return;
}

// convert latlon to x,y position relative to UPS scaling
double dcUpsMap::latlonToUnitX(double lat, double lon) {
    return 2 * tan(M_PI_4 - fabs(lat)*M_PI_2/180) * cos((-1*lon+90)*M_PI/180) ;
}

double dcUpsMap::latlonToUnitY(double lat, double lon) {
    return 2 * tan(M_PI_4 - fabs(lat)*M_PI_2/180) * sin((-1*lon+90)*M_PI/180);
}

void dcUpsMap::setLonLatParams(const std::string &pos, 
    const std::string &topLeftLat, const std::string &topLeftLon, 
    const std::string &bottomRightLat, const std::string &bottomRightLon) 
{
    if (!pos.empty()) 
    {
        int index = getValue(pos)->getInteger();
        if (!topLeftLat.empty() && !topLeftLon.empty() && !bottomRightLat.empty() && !bottomRightLon.empty()) {
            double tllat = getValue(topLeftLat)->getDecimal();
            double tllon = getValue(topLeftLon)->getDecimal();
            double brlat = getValue(bottomRightLat)->getDecimal();
            double brlon = getValue(bottomRightLon)->getDecimal();

            double tlx = latlonToUnitX(tllat, tllon);
            double tly = latlonToUnitY(tllat, tllon);
            double brx = latlonToUnitX(brlat, brlon);
            double bry = latlonToUnitY(brlat, brlon);

            upsLayerInfos.insert({
                index, {
                    tlx, tly, 
                    brx, bry
                }
            });
        }
    } else {
        printf("upsMap.setLonLatParams: missing values\n");
    }
}

void dcUpsMap::setEnableInverseTheta(const std::string &inval)
{
    if (!inval.empty()) {        
        if (getValue(inval)->getBoolean()) thetaFactor = -1;
    }
}

void dcUpsMap::fetchLonLat(void) 
{
    longitude = vLongitude->getDecimal();
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

    double ux = latlonToUnitX(latitude, thetaFactor * longitude);
    double uy = latlonToUnitY(latitude, thetaFactor * longitude);

    // compute unit ratios for x and y
    for (auto const& pair : upsLayerInfos) 
    {
        int id = pair.first;
        const upsLayerInfo* uli = &(pair.second);
        mapLayerInfo* mli = &(mapLayerInfos[id]);

        mli->hRatio = (ux - uli->topLeftUnitX) / (uli->bottomRightUnitX - uli->topLeftUnitX);
        mli->vRatio = (uy - uli->bottomRightUnitY) / (uli->topLeftUnitY - uli->bottomRightUnitY);
    }

    // compute trajectory
    if ( vYaw )
        trajAngle = vYaw->getDecimal() + yawOffset;
    else if ( prevVRatio != mliCurrent->vRatio || prevHRatio != mliCurrent->hRatio)
        trajAngle = atan2((mliCurrent->vRatio - prevVRatio), ( mliCurrent->hRatio - prevHRatio)) * 180 / M_PI;
}

// compute positional ratios, as well as the current trajectory
void dcUpsMap::computeGhostTrailRatios(std::vector<std::pair<double, double>> latlons) 
{
    // compute unit ratios for x and y
    for (auto const& pair : upsLayerInfos) 
    {
        int id = pair.first;
        const upsLayerInfo* uli = &(pair.second);
        mapLayerInfo* mli = &(mapLayerInfos[id]);

        for (auto const& latlon : latlons) {
            double ux = latlonToUnitX(latlon.first, thetaFactor * latlon.second);
            double uy = latlonToUnitY(latlon.first, thetaFactor * latlon.second);

            mli->ghostRatioHistory.push_back({
                (ux - uli->topLeftUnitX) / (uli->bottomRightUnitX - uli->topLeftUnitX), 
                (uy - uli->bottomRightUnitY) / (uli->topLeftUnitY - uli->bottomRightUnitY)
            });
        }
    }
}

void dcUpsMap::computeZoneRatios(void)
{
    zoneLonLatRatios.clear();
    for (uint i = 0; i < zoneLonLatVals.size(); i++) {
        double lon = zoneLonLatVals.at(i).first->getDecimal();
        double lat = zoneLonLatVals.at(i).second->getDecimal();

        double ux = latlonToUnitX(lat, thetaFactor * lon);
        double uy = latlonToUnitY(lat, thetaFactor * lon);

        zoneLonLatRatios.push_back({
            (ux - uliCurrent->topLeftUnitX) / (uliCurrent->bottomRightUnitX - uliCurrent->topLeftUnitX),
            (uy - uliCurrent->bottomRightUnitY) / (uliCurrent->topLeftUnitY - uliCurrent->bottomRightUnitY)
        });
    }
}

void dcUpsMap::computePointRatios(void)
{
    double mipLatitude, mipLongitude;
    for (uint i = 0; i < mapImagePoints.size(); i++) {
        mapImagePoint& mip = mapImagePoints.at(i);
        mipLatitude = (mip.vLatitude)->getDecimal();
        mipLongitude = (mip.vLongitude)->getDecimal();

        double ux = latlonToUnitX(mipLatitude, thetaFactor * mipLongitude);
        double uy = latlonToUnitY(mipLatitude, thetaFactor * mipLongitude);

        mip.hRatio = (ux - uliCurrent->topLeftUnitX) / (uliCurrent->bottomRightUnitX - uliCurrent->topLeftUnitX);
        mip.vRatio = (uy - uliCurrent->bottomRightUnitY) / (uliCurrent->topLeftUnitY - uliCurrent->bottomRightUnitY);
    }

    double mspLatitude, mspLongitude;
    for (uint i = 0; i < mapStringPoints.size(); i++) {

        mapStringPoint& msp = mapStringPoints.at(i);
        mspLatitude = (msp.vLatitude)->getDecimal();
        mspLongitude = (msp.vLongitude)->getDecimal();

        double ux = latlonToUnitX(mspLatitude, thetaFactor * mspLongitude);
        double uy = latlonToUnitY(mspLatitude, thetaFactor * mspLongitude);

        msp.hRatio = (ux - uliCurrent->topLeftUnitX) / (uliCurrent->bottomRightUnitX - uliCurrent->topLeftUnitX);
        msp.vRatio = (uy - uliCurrent->bottomRightUnitY) / (uliCurrent->topLeftUnitY - uliCurrent->bottomRightUnitY);
    }
}
