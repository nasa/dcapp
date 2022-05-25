#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "commonutils.hh"
#include "upsmaptexture.hh"


dcUpsMapTexture::dcUpsMapTexture(dcMap* myparent) : dcMapTexture(myparent), thetaFactor(1)
{
    return;
}

dcUpsMapTexture::~dcUpsMapTexture()
{
    return;
}

// convert latlon to x,y position relative to UPS scaling
static double latlonToUnitX(double lat, double lon) {
    return 2 * tan(M_PI_4 - fabs(lat)*M_PI_2/180) * cos((-1*lon+90)*M_PI/180) ;
}

static double latlonToUnitY(double lat, double lon) {
    return 2 * tan(M_PI_4 - fabs(lat)*M_PI_2/180) * sin((-1*lon+90)*M_PI/180);
}

void dcUpsMapTexture::setParams(const std::string &topLeftLat, const std::string &topLeftLon, 
    const std::string &bottomRightLat, const std::string &bottomRightLon, const std::string &invTheta) 
{
    if (!topLeftLat.empty() && !topLeftLon.empty() && !bottomRightLat.empty() && !bottomRightLon.empty()) {
        double tllat = getValue(topLeftLat)->getDecimal();
        double tllon = getValue(topLeftLon)->getDecimal();
        double brlat = getValue(bottomRightLat)->getDecimal();
        double brlon = getValue(bottomRightLon)->getDecimal();

        topLeftUnitX = latlonToUnitX(tllat, tllon);
        topLeftUnitY = latlonToUnitY(tllat, tllon);
        bottomRightUnitX = latlonToUnitX(brlat, brlon);
        bottomRightUnitY = latlonToUnitY(brlat, brlon);

        if (!invTheta.empty()) {        
            if (getValue(invTheta)->getBoolean()) thetaFactor = -1;
        }
    } else {
        printf("upsMapTexture.setParams: missing latlon values\n");
    }
}

// compute ghost trail ratios
void dcUpsMapTexture::addGhostTrail(std::vector<std::pair<double, double>> latlons, double trailWidth, Kolor trailColor) 
{
    std::vector<std::pair<float,float>> ghostRatioHistory;

    for (auto const& latlon : latlons) {
        double ux = latlonToUnitX(latlon.first, thetaFactor * latlon.second);
        double uy = latlonToUnitY(latlon.first, thetaFactor * latlon.second);

        ghostRatioHistory.push_back({
            (ux - topLeftUnitX) / (bottomRightUnitX - topLeftUnitX), 
            (uy - bottomRightUnitY) / (topLeftUnitY - bottomRightUnitY)
        });
    }

    ghostTrails.push_back({ghostRatioHistory, trailWidth, trailColor});
}

// compute positional ratios, as well as the current trajectory
void dcUpsMapTexture::computePosRatios(void) 
{
    // save previous ratios
    double prevHRatio = hRatio;
    double prevVRatio = vRatio;

    double ux = latlonToUnitX(mapInfo->latitude, thetaFactor * mapInfo->longitude);
    double uy = latlonToUnitY(mapInfo->latitude, thetaFactor * mapInfo->longitude);

    hRatio = (ux - topLeftUnitX) / (bottomRightUnitX - topLeftUnitX);
    vRatio = (uy - bottomRightUnitY) / (topLeftUnitY - bottomRightUnitY);

    // compute trajectory
    if ( mapInfo->vYaw )
        mapInfo->trajAngle = mapInfo->vYaw->getDecimal() + yawOffset;
    else if ( prevVRatio != vRatio || prevHRatio != hRatio)
        mapInfo->trajAngle = atan2((vRatio - prevVRatio), ( hRatio - prevHRatio)) * 180 / M_PI;
}

void dcUpsMapTexture::computeZoneRatios(void)
{
    zoneRatios.clear();
    for (unsigned int i = 0; i < mapInfo->zoneVals.size(); i++) {
        double lon = mapInfo->zoneVals.at(i).first->getDecimal();
        double lat = mapInfo->zoneVals.at(i).second->getDecimal();

        double ux = latlonToUnitX(lat, thetaFactor * lon);
        double uy = latlonToUnitY(lat, thetaFactor * lon);

        zoneRatios.push_back({
            (ux - topLeftUnitX) / (bottomRightUnitX - topLeftUnitX),
            (uy - bottomRightUnitY) / (topLeftUnitY - bottomRightUnitY)
        });
    }
}

void dcUpsMapTexture::computePointRatios(void)
{
    double mipLatitude, mipLongitude;
    for (unsigned int i = 0; i < imagePoints.size(); i++) {
        mapImagePoint& mip = imagePoints.at(i);
        mipLatitude = (mip.vLatitude)->getDecimal();
        mipLongitude = (mip.vLongitude)->getDecimal();

        double ux = latlonToUnitX(mipLatitude, thetaFactor * mipLongitude);
        double uy = latlonToUnitY(mipLatitude, thetaFactor * mipLongitude);

        mip.hRatio = (ux - topLeftUnitX) / (bottomRightUnitX - topLeftUnitX);
        mip.vRatio = (uy - bottomRightUnitY) / (topLeftUnitY - bottomRightUnitY);
    }

    double mspLatitude, mspLongitude;
    for (unsigned int i = 0; i < stringPoints.size(); i++) {

        mapStringPoint& msp = stringPoints.at(i);
        mspLatitude = (msp.vLatitude)->getDecimal();
        mspLongitude = (msp.vLongitude)->getDecimal();

        double ux = latlonToUnitX(mspLatitude, thetaFactor * mspLongitude);
        double uy = latlonToUnitY(mspLatitude, thetaFactor * mspLongitude);

        msp.hRatio = (ux - topLeftUnitX) / (bottomRightUnitX - topLeftUnitX);
        msp.vRatio = (uy - bottomRightUnitY) / (topLeftUnitY - bottomRightUnitY);
    }
}
