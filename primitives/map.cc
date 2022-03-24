#include <string>
#include <sstream>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "app_data.hh"
#include "commonutils.hh"
#include "map.hh"
#include "basicutils/pathinfo.hh"
#include <algorithm>
#include <fstream>
#include "basicutils/msg.hh"

extern appdata AppData;

dcMap::dcMap(dcParent *myparent) :  dcGeometric(myparent), vTextureIndex(0x0), vLatitude(0x0), vLongitude(0x0), vZoom(0x0), vYaw(0x0),
                                    longitude(0), latitude(0), zoom(1), trajAngle(0), yawOffset(0),
                                    enableTrail(true), trailWidth(25), trailResolution(.005), fnClearTrail(0x0), ghostTrailWidth(25),
                                    enableIcon(true), enableCustomIcon(false), iconRotationOffset(0), iconTextureID(0x0),
                                    enableCircularMap(0), enableTrackUp(0), enableZone(false), unlocked(false), selected(false)
{
    trailColor.set(1, 0, 0, .5);
    ghostTrailColor.set(0, 1, 0, .5);

    PressList = new dcParent;
    ReleaseList = new dcParent;
    PressList->setParent(this);
    ReleaseList->setParent(this);
}

dcMap::~dcMap()
{
    return;
}

void dcMap::setTexture(const std::string &pos, const std::string &filename)
{
    int index = getValue(pos)->getInteger();
    if (!pos.empty() && !filename.empty()) 
        mapLayerInfos[index].textureID = tdLoadTexture(filename);
}

void dcMap::setTextureIndex(const std::string &inval)
{
    if (!inval.empty()) vTextureIndex = getValue(inval);
}

void dcMap::setLonLat(const std::string &lat1, const std::string &lon1)
{
    if (!lat1.empty() && !lon1.empty())
    {
        vLatitude = getValue(lat1);
        vLongitude = getValue(lon1);
    }
    else
    {
        printf("map.setLonLat: missing values\n");
    }
}

void dcMap::setZoom(const std::string &inval)
{
    if (!inval.empty()) vZoom = getValue(inval);
}

void dcMap::setSizeRatio(const std::string &pos, const std::string &sizeRatio)
{
    int index = getValue(pos)->getInteger();
    if (!pos.empty() && !sizeRatio.empty()) 
        mapLayerInfos[index].sizeRatio = getValue(sizeRatio)->getDecimal();
    else 
        mapLayerInfos[index].sizeRatio = 1;
}

void dcMap::setYaw(const std::string &inval1, const std::string &inval2)
{
    if (!inval1.empty()) 
    {
        vYaw = getValue(inval1);
        if (!inval2.empty()) yawOffset = getValue(inval2)->getDecimal();
    }
}

void dcMap::setEnableCircularMap(const std::string &inval)
{
    if (!inval.empty()) enableCircularMap = getValue(inval)->getBoolean();
}

void dcMap::setEnableTrackUp(const std::string &inval)
{
    if (!inval.empty()) enableTrackUp = getValue(inval)->getBoolean();
}

void dcMap::setFnClearTrail(const std::string &inval)
{
    if (!inval.empty()) fnClearTrail = getValue(inval);
}

void dcMap::setEnableIcon(const std::string &inval)
{
    if (!inval.empty()) enableIcon = getValue(inval)->getBoolean();
}

void dcMap::setEnableTrail(const std::string &inval)
{
    if (!inval.empty()) enableTrail = getValue(inval)->getBoolean();
}

void dcMap::setTrailColor(const std::string &cspec)
{
    if (!cspec.empty())
    {
        trailColor.set(cspec);
        enableTrail = true;
    }
}

void dcMap::setTrailWidth(const std::string &inval)
{
    if (!inval.empty()) trailWidth = getValue(inval)->getDecimal();
}

void dcMap::setTrailResolution(const std::string &inval)
{
    if (!inval.empty()) trailResolution = getValue(inval)->getDecimal();
}

void dcMap::setGhostTrail(const std::string &filename)
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

        computeGhostTrailRatios(result);
        enableGhostTrail = true;
    }
}

void dcMap::setGhostTrailColor(const std::string &cspec)
{
    if (!cspec.empty())
    {
        ghostTrailColor.set(cspec);
    }
}

void dcMap::setGhostTrailWidth(const std::string &inval)
{
    if (!inval.empty()) ghostTrailWidth = getValue(inval)->getDecimal();
}

void dcMap::setIconTexture(const std::string &filename) 
{
    if (!filename.empty()) {
        this->iconTextureID = tdLoadTexture(filename);
        enableCustomIcon = true;
    }
}

void dcMap::setIconRotationOffset(const std::string &inval) 
{
    if (!inval.empty()) iconRotationOffset = getValue(inval)->getDecimal();
}

void dcMap::setIconSize(const std::string &inw, const std::string &inh)
{
    if (enableCustomIcon)
    {
        if (!inw.empty() && !inh.empty())
        {
            iconWidth = getValue(inw)->getDecimal();
            iconHeight= getValue(inh)->getDecimal();
        }
        else
        {
            printf("map.cc: Missing dimensions for icon\n");
        }
    }
}

void dcMap::setZoneLonLat(const std::string &lon1, const std::string &lat1, const std::string &lon2, const std::string &lat2, 
    const std::string &lon3, const std::string &lat3, const std::string &lon4, const std::string &lat4) {

    if (!lon1.empty() && !lat1.empty() && !lon2.empty() && !lat2.empty() && !lon3.empty() && !lat3.empty() && !lon4.empty() && !lat4.empty())
    {
        zoneLonLatVals.clear();
        zoneLonLatVals.push_back({getValue(lon1), getValue(lat1)});
        zoneLonLatVals.push_back({getValue(lon2), getValue(lat2)});
        zoneLonLatVals.push_back({getValue(lon3), getValue(lat3)});
        zoneLonLatVals.push_back({getValue(lon4), getValue(lat4)});
        enableZone = true;
    }
}

void dcMap::setMapImagePoint(const std::string &filename, const std::string &lon, const std::string &lat, const std::string &enable, 
    const std::string &w, const std::string &h, const std::string &enableScaling, const std::string &layers) {

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

    if (!layers.empty()) {
        std::stringstream ss(layers);
        for (int temp; ss >> temp;) {
            mip.layers.push_back(temp);
            if (ss.peek() == ',')
                ss.ignore();
        }
    }

    mapImagePoints.push_back(mip);
}

void dcMap::setMapStringPoint(const std::string &text, const std::string &lon, const std::string &lat, const std::string &enable, 
    const std::string &size, const std::string &enableScaling, const std::string &layers) {
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

    if (!layers.empty()) {
        std::stringstream ss(layers);
        for (int temp; ss >> temp;) {
            msp.layers.push_back(temp);
            if (ss.peek() == ',')
                ss.ignore();
        }
    }

    mapStringPoints.push_back(msp);
}

void dcMap::setUnlocked(const std::string &inval) {
    if (!inval.empty()) vUnlocked = getValue(inval);
}

void dcMap::computeGeometry(void)
{
    if (w) displayWidth = w->getDecimal();
    else displayWidth = 0;

    if (h) displayHeight = h->getDecimal();
    else displayHeight = 0;

    /* true width/height of map is square to stay proportional and not cutoff portions when rotating */
    width = std::max(displayWidth, displayHeight) * SQRT_2;
    height = width;

    double hwidth = (0.5 * width);
    double hheight = (0.5 * height);
    
    widthOffset = (width-displayWidth)/2;
    heightOffset = (height-displayHeight)/2;
    left = GeomX(x, displayWidth, containerw->getDecimal(), halign) - widthOffset;
    bottom = GeomY(y, displayHeight, containerh->getDecimal(), valign) - heightOffset;

    right = left + width;
    top = bottom + height;
    center = left + hwidth;
    middle = bottom + hheight;

    switch (halign)
    {
        case dcLeft:
            refx = left;
            delx = 0;
            break;
        case dcCenter:
            refx = center;
            delx = hwidth;
            break;
        case dcRight:
            refx = right;
            delx = width;
            break;
        default:
            break;
    }
    switch (valign)
    {
        case dcBottom:
            refy = bottom;
            dely = 0;
            break;
        case dcMiddle:
            refy = middle;
            dely = hheight;
            break;
        case dcTop:
            refy = top;
            dely = height;
            break;
        default:
            break;
    }
}

void dcMap::fetchBaseParams(void)
{
    if (vTextureIndex) textureIndex = vTextureIndex->getInteger();
    else textureIndex = 0;

    mliCurrent = &(mapLayerInfos[textureIndex]);

    if (vZoom) zoom = vZoom->getDecimal();
    else zoom = 1;
    if (zoom < 1) 
        zoom = 1;

    //if (vUnlocked) unlocked = vUnlocked->getBoolean();
}

void dcMap::updateCurrentParams(void)
{
    hRatio = mliCurrent->hRatio;
    vRatio = mliCurrent->vRatio;
}

// get bounds for texture on 0 to 1 range
void dcMap::computeTextureBounds(void)
{
    double mapWidthRatio = 1/(mliCurrent->sizeRatio)/zoom/2;

    texUp = vRatio + mapWidthRatio;
    texDown = vRatio - mapWidthRatio;
    texLeft = hRatio - mapWidthRatio;
    texRight = hRatio + mapWidthRatio;

    if (texUp > 1) {
        texUp = 1;
        texDown = 1 - 2*mapWidthRatio;
    } else if (texDown < 0) {
        texDown = 0;
        texUp = 2*mapWidthRatio;
    }

    if (texRight > 1) {
        texRight = 1;
        texLeft = 1 - 2*mapWidthRatio;
    } else if (texLeft < 0) {
        texLeft = 0;
        texRight = 2*mapWidthRatio;
    }
}

void dcMap::displayIcon(void) {
    float mx, my, mwidth, mheight, mdelx, mdely;

    if (enableCustomIcon) 
    {
        mwidth = iconWidth;
        mheight = iconHeight;
    }
    else
    {
        mwidth = mheight = 25;
    }

    mx = (hRatio - texLeft) / (texRight - texLeft) * width;
    my = (vRatio - texDown) / (texUp - texDown) * height;
    mdelx = mwidth/2;
    mdely = mheight/2;

    if (enableCustomIcon) 
    {
        container_start(mx, my, mdelx, mdely, 1, 1, iconRotationOffset + trajAngle);

        draw_image(this->iconTextureID, mwidth, mheight);
        container_end();
    }
    else
    {
        circle_fill(mx, my, mwidth, 80, 1, 0, 0, 1);
        circle_outline(mx, my, mwidth, 80, 0, 0, 0, 1, 10, 0xFFFF, 1);
    }
}

void dcMap::displayTrail(void)
{
    std::vector<float> pntsA;
    if (mliCurrent->ratioHistory.size() > 1) {
        for (uint i = 1; i < mliCurrent->ratioHistory.size(); i++) {
            std::pair<float,float> p1 = mliCurrent->ratioHistory.at(i-1);
            std::pair<float,float> p2 = mliCurrent->ratioHistory.at(i);
            if ( (p1.first > texLeft && p1.first < texRight && p1.second > texDown && p1.second < texUp) ||
                 (p2.first > texLeft && p2.first < texRight && p2.second > texDown && p2.second < texUp) ) {

                // calculate mx, my for points
                float mx1 = (p1.first - texLeft) / (texRight - texLeft) * width;
                float my1 = (p1.second - texDown) / (texUp - texDown) * height;
                float mx2 = (p2.first - texLeft) / (texRight - texLeft) * width;
                float my2 = (p2.second - texDown) / (texUp - texDown) * height;

                // add to set of points
                pntsA.insert(pntsA.end(),{mx1, my1, mx2, my2});
            } else {
                if (!pntsA.empty()) {
                    draw_line(pntsA, trailWidth, trailColor.R->getDecimal(), trailColor.G->getDecimal(), trailColor.B->getDecimal(), trailColor.A->getDecimal(), 0xFF00, 1);
                    pntsA.clear();
                }
            }
        }

        // plot any points remaining 
        if (!pntsA.empty()) {
            draw_line(pntsA, trailWidth, trailColor.R->getDecimal(), trailColor.G->getDecimal(), trailColor.B->getDecimal(), trailColor.A->getDecimal(), 0xFF00, 1);
            pntsA.clear();
        }

        // draw last position to current point
        if (!unlocked && !selected) {
            std::pair<float,float> p = mliCurrent->ratioHistory.back();

            float mx1 = (p.first - texLeft) / (texRight - texLeft) * width;
            float my1 = (p.second - texDown) / (texUp - texDown) * height;
            float mx2 = (hRatio - texLeft) / (texRight - texLeft) * width;
            float my2 = (vRatio - texDown) / (texUp - texDown) * height;

            pntsA.insert(pntsA.end(),{mx1, my1, mx2, my2});
            draw_line(pntsA, trailWidth, trailColor.R->getDecimal(), trailColor.G->getDecimal(), trailColor.B->getDecimal(), trailColor.A->getDecimal(), 0xFFFF, 1);
        }
    }
}

void dcMap::updateTrail(void)
{
    // clear stored trails on fnClearTrail value change
    bool doClear = false;
    if (fnClearTrail) {
        int curr_clear_state = fnClearTrail->getInteger();
        if (prev_clear_state != curr_clear_state) {
            doClear = true;
        }
        prev_clear_state = curr_clear_state;
    }

    // compute unit ratios for x and y
    for (auto const& pair : mapLayerInfos) 
    {
        mapLayerInfo* mli = &(mapLayerInfos[pair.first]);

        // clear trail 
        if (doClear) mli->ratioHistory.clear();

        // add position to list
        if (mli->ratioHistory.empty()) 
            mli->ratioHistory.push_back({(float)mli->hRatio, (float)mli->vRatio});
        else
        {
            std::pair<float,float> last_pos = mli->ratioHistory.back();
            float dist = sqrt(pow(mli->hRatio-last_pos.first, 2) + pow(mli->vRatio-last_pos.second, 2)*1.0);
            if ( dist > trailResolution) {
                mli->ratioHistory.push_back({mli->hRatio, mli->vRatio});
            }
        }
    }
}

void dcMap::displayGhostTrail(void)
{
    std::vector<float> pntsA;
    if (mliCurrent->ghostRatioHistory.size() > 1) {
        for (uint i = 1; i < mliCurrent->ghostRatioHistory.size(); i++) {
            std::pair<float,float> p1 = mliCurrent->ghostRatioHistory.at(i-1);
            std::pair<float,float> p2 = mliCurrent->ghostRatioHistory.at(i);
            if ( (p1.first > texLeft && p1.first < texRight && p1.second > texDown && p1.second < texUp) ||
                 (p2.first > texLeft && p2.first < texRight && p2.second > texDown && p2.second < texUp) ) {

                // calculate mx, my for points
                float mx1 = (p1.first - texLeft) / (texRight - texLeft) * width;
                float my1 = (p1.second - texDown) / (texUp - texDown) * height;
                float mx2 = (p2.first - texLeft) / (texRight - texLeft) * width;
                float my2 = (p2.second - texDown) / (texUp - texDown) * height;

                // add to set of points
                pntsA.insert(pntsA.end(),{mx1, my1, mx2, my2});
            } else {
                if (!pntsA.empty()) {
                    draw_line(pntsA, ghostTrailWidth, ghostTrailColor.R->getDecimal(), ghostTrailColor.G->getDecimal(), ghostTrailColor.B->getDecimal(), ghostTrailColor.A->getDecimal(), 0xFFFF, 1);
                    pntsA.clear();
                }
            }
        }

        // plot any points remaining 
        if (!pntsA.empty()) {
            draw_line(pntsA, ghostTrailWidth, ghostTrailColor.R->getDecimal(), ghostTrailColor.G->getDecimal(), ghostTrailColor.B->getDecimal(), ghostTrailColor.A->getDecimal(), 0xFFFF, 1);
        }
    }
}

void dcMap::displayZone(void)
{
    computeZoneRatios();
    std::vector<float> pntsA;
    for (uint ii = 0; ii < zoneLonLatRatios.size(); ii++) {
        std::pair<float,float> p = zoneLonLatRatios.at(ii);
        pntsA.push_back( (p.first - texLeft) / (texRight - texLeft) * width );
        pntsA.push_back( (p.second - texDown) / (texUp - texDown) * height );
    }

    draw_quad(pntsA, 1, .5, .5, .5);
}

void dcMap::displayPoints(void)
{
    float mx, my, mwidth, mheight, msize, mdelx, mdely;

    computePointRatios();

    // process images
    for (uint ii = 0; ii < mapImagePoints.size(); ii++) {

        mapImagePoint& mip = mapImagePoints.at(ii);
        if (mip.vEnabled->getInteger() && std::count(mip.layers.begin(), mip.layers.end(), textureIndex) ) {

            mx = (mip.hRatio - texLeft) / (texRight - texLeft) * width;
            my = (mip.vRatio - texDown) / (texUp - texDown) * height;
            mwidth = mip.width;
            mheight = mip.height;
            if (mip.enableScaling)
            {
                mwidth *= zoom;
                mheight *= zoom;
            }
            mdelx = mwidth/2;
            mdely = mheight/2;

            // draw image
            container_start(mx, my, mdelx, mdely, 1, 1, 0);
            draw_image(mip.textureID, mwidth, mheight);
            container_end();
        }
    }

    // process strings
    static tdFont* fontID = tdLoadFont(AppData.defaultfont, "");
    for (uint ii = 0; ii < mapStringPoints.size(); ii++) {

        mapStringPoint& msp = mapStringPoints.at(ii);
        if (msp.vEnabled->getInteger() && std::count(msp.layers.begin(), msp.layers.end(), textureIndex) ) {

            // get string with variables, constants, formatting
            std::string mystring = msp.vText->getString();
            // break mystring into a vector of 1-line substrings
            std::vector <std::string> lines;
            size_t endptr, strptr=0;
            do {
                endptr = mystring.find("\\n", strptr);
                lines.push_back(mystring.substr(strptr, endptr - strptr));
                strptr = endptr + 2;
            } while (endptr != std::string::npos);

            mx = (msp.hRatio - texLeft) / (texRight - texLeft) * width;
            my = (msp.vRatio - texDown) / (texUp - texDown) * height;
            msize = msp.size;
            if (msp.enableScaling)
            {
                msize *= zoom;
            }

            for (uint jj=0; jj<lines.size(); jj++) {
                float stringWidth = fontID->getAdvance(lines[jj], flMonoNone) * msize / fontID->getBaseSize();
                double myleft = -0.5 * stringWidth;
                double mybottom = msize * (((double)lines.size()/2) - (double)(jj + 1));

                translate_start(mx, my);

                    // draw background
                    double mytop = mybottom + msize;
                    double myright = myleft + stringWidth;
                    std::vector<float> pointsL;
                    addPoint(pointsL, myleft, mybottom);
                    addPoint(pointsL, myleft, mytop);
                    addPoint(pointsL, myright, mytop);
                    addPoint(pointsL, myright, mybottom);
                    draw_quad(pointsL, 0, 0, 0, .5);

                    // draw outline
                    draw_string(myleft, mybottom, msize, 0, 0, 0, 1, fontID, flMonoNone, true, lines[jj]);
                
                    // draw string
                    draw_string(myleft, mybottom, msize, 1, 1, 1, 1, fontID, flMonoNone, false, lines[jj]);
                
                translate_end();
            }
        }
    }
}

void dcMap::handleMousePress(double inx, double iny) {
    double truex = refx + widthOffset - delx;
    double truey = refy + heightOffset - dely;

    if (inx > truex && inx < truex + displayWidth && iny > truey && iny < truey + displayHeight) {
        selected = true;
        scrollX = inx;
        scrollY = iny;
    }
}

void dcMap::handleMouseMotion(double inx, double iny) {
    if (selected) {
        hRatio -= (inx - scrollX) / width / mliCurrent->sizeRatio / zoom;
        vRatio -= (iny - scrollY) / height / mliCurrent->sizeRatio / zoom;

        printf("%f %f\n", hRatio, vRatio);

        scrollX = inx;
        scrollY = iny;
    }
}

void dcMap::handleMouseRelease(void) {
    selected = false;
}

void dcMap::processPreCalculations(void) {
    computeGeometry();
    fetchBaseParams();
    fetchChildParams();

    // only run the following if the user is not scrolling
    if (!selected && !unlocked) {
        fetchLonLat();      // dependent on UPS/UTM
        computePosRatios();
        updateCurrentParams();
    }
}

void dcMap::processPostCalculations(void) {
    if (!selected && !unlocked) {
        updateTrail();
    }
}

void dcMap::draw(void)
{
    computeTextureBounds();

    stencil_begin();        // enable stencil, clear existing buffer
    stencil_init_dest();    // setup stencil test to write 1's into destination area 

        container_start(refx, refy, delx, dely, 1, 1, 0);   // start container for full map

        // draw masking shape
        container_start(widthOffset, heightOffset, 0, 0, 1, 1, 0);
        if (enableCircularMap) {
            draw_ellipse(displayWidth/2, displayHeight/2, displayWidth/2, displayHeight/2, 100, 1, 1, 1, 1);
        } else {
            std::vector<float> pointsL;
            pointsL.reserve(8);
            addPoint(pointsL, 0, 0);
            addPoint(pointsL, 0, displayHeight);
            addPoint(pointsL, displayWidth, displayHeight);
            addPoint(pointsL, displayWidth, 0);
            draw_quad(pointsL, 0, 0, 0, 0);
        }
        container_end();

    stencil_init_proj();    // set stencil to only keep fragments with reference != 0

        if (enableTrackUp) {
            translate_start(width/2, height/2);
            rotate_start(-1 * (trajAngle + iconRotationOffset));
            translate_start(-1 * width/2, -1 * height/2);
        }

        draw_map(mliCurrent->textureID, width, height, texUp, texDown, texLeft, texRight);
        if ( enableZone )   displayZone();
        displayPoints();    // always display points
        if ( enableGhostTrail ) displayGhostTrail();
        if ( enableTrail )  displayTrail();
        if ( enableIcon && !selected  && !unlocked) displayIcon();

        if (enableTrackUp) {
            translate_end();
            rotate_end();
            translate_end();
        }

        container_end();


    stencil_end();
}
