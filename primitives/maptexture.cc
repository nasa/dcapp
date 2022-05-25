#include <fstream>


#include "maptexture.hh"
#include "commonutils.hh"
#include "values.hh"
#include "basicutils/pathinfo.hh"
#include "basicutils/msg.hh"

dcMapTexture::dcMapTexture(dcMap *mymap) : mapInfo(mymap), textureID(NULL), 
    hRatio(.5), vRatio(.5), sizeRatio(1), yawOffset(0)
{
    return;
}

dcMapTexture::~dcMapTexture()
{
    return;
}

void dcMapTexture::setTexture(const std::string &filename)
{
    if (!filename.empty()) {
        textureID = tdLoadTexture(filename);
    }
}

void dcMapTexture::setYawOffset(const std::string &yo)
{
    if (!yo.empty()) yawOffset = getValue(yo)->getDecimal();
}

void dcMapTexture::setSizeRatio(const std::string &sr)
{
    if (!sr.empty()) 
        sizeRatio = getValue(sr)->getDecimal();
    else 
        sizeRatio = 1;
}

void dcMapTexture::setGhostTrail(const std::string &filename, const std::string &cspec, const std::string &linewidth)
{
    if (!filename.empty()) 
    {
        PathInfo mypath(filename);
        if (!(mypath.isValid()))
        {
            warning_msg("Unable to locate ghost trail file at " + filename);
            return;
        }
        
        std::string line, word;
        double lat, lon;
        std::vector<std::pair<double, double>> result;
        std::ifstream file(mypath.getFullPath());
        while ( std::getline(file, line) )
        {
            std::stringstream sline(line);

            getline(sline, word, ',');
            lat = std::stod(word);
            getline(sline, word, ',');
            lon = std::stod(word);

            result.push_back({lat, lon});
        }

        if (result.size() > 1)
        {
            Kolor k = Kolor();
            double w = 25;
            if (!cspec.empty()) k.set(cspec);
            if (!linewidth.empty()) w = getValue(linewidth)->getDecimal();

            addGhostTrail(result, w, k);
        }
        else
        {
            printf("dcMap.setGhostTrail(): empty trail file; possible bad format\n");
        }
    }
    else
    {
        printf("dcMap.setGhostTrail(): missing parameters\n");
    }
}

void dcMapTexture::setMapImagePoint(const std::string &filename, const std::string &lon, const std::string &lat, 
    const std::string &enable, const std::string &w, const std::string &h, const std::string &enableScaling)
{
    mapImagePoint mip;
    if (!filename.empty() && !lon.empty() && !lat.empty() && !w.empty() && !h.empty()) 
    {
        mip.textureID = tdLoadTexture(filename);
        mip.vLongitude = getValue(lon);
        mip.vLatitude = getValue(lat);
        mip.width = getValue(w)->getDecimal();
        mip.height = getValue(h)->getDecimal();
    }
    else
    {
        printf("setMapImagePoint: missing a parameter, ignoring. Check documentation\n");
        return;
    }

    if (!enable.empty()) mip.vEnabled = getValue(enable);
    else mip.vEnabled = getValue("1");

    if (!enableScaling.empty()) mip.enableScaling = getValue(enableScaling)->getBoolean();
    else mip.enableScaling = 0;

    imagePoints.push_back(mip);
}

void dcMapTexture::setMapStringPoint(const std::string &text, const std::string &lon, const std::string &lat, 
    const std::string &enable, const std::string &size, const std::string &enableScaling) {

    mapStringPoint msp;
    if (!text.empty() && !lon.empty() && !lat.empty() && !size.empty()) 
    {
        msp.vText = getValue(text);
        msp.vLongitude = getValue(lon);
        msp.vLatitude = getValue(lat);
        msp.size = getValue(size)->getDecimal();
    }
    else
    {
        printf("setMapStringPoint: missing a parameter, ignoring. Check documentation\n");
        return;
    }

    if (!enable.empty()) msp.vEnabled = getValue(enable);
    else msp.vEnabled = getValue("1");

    if (!enableScaling.empty()) msp.enableScaling = getValue(enableScaling)->getBoolean();
    else msp.enableScaling = 0;

    stringPoints.push_back(msp);
}
