#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "commonutils.hh"
#include "map.hh"


dcMap::dcMap(dcParent *myparent) :  dcGeometric(myparent), textureID(0x0), vLatitude(0x0), vLongitude(0x0), vZoom(0x0),
                                    longitude(0), latitude(0), zoom(1), trajAngle(0),
                                    enableTrail(true), trailWidth(25), trailResolution(.005), fnClearTrail(0x0),
                                    enableIcon(true), enableCustomIcon(false), iconRotationOffset(0), iconTextureID(0x0),
                                    enableCircularMap(0), enableTrackUp(0), enableZone(false), selected(false)
{
    trailColor.set(1, 0, 0, .5);
}

dcMap::~dcMap()
{
    return;
}

void dcMap::setTexture(const std::string &filename)
{
    this->textureID = tdLoadTexture(filename);
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

void dcMap::setTrailResolution(const std::string &inval)
{
    if (!inval.empty()) trailResolution = getValue(inval)->getDecimal();
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

void dcMap::computeGeometry(void)
{
    if (w) width = w->getDecimal();
    else width = 0;

    if (h) height = h->getDecimal();
    else height = 0;

    double hwidth = (0.5 * width);
    double hheight = (0.5 * height);
    
    left = GeomX(x, width, containerw->getDecimal(), halign);
    bottom = GeomY(y, height, containerh->getDecimal(), valign);

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

    computeTextureBounds();
}

// get bounds for texture on 0 to 1 range
void dcMap::computeTextureBounds(void)
{
    // compute unit offset for position
    if (vZoom) zoom = vZoom->getDecimal();
    else zoom = 1;

    if (zoom < 1) 
        zoom = 1;

    computeLonLat();
    computePosRatios();

    double mapWidthRatio = 1/zoom/2;

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
    if (positionHistory.size() > 1) {
        for (uint i = 1; i < positionHistory.size(); i++) {
            std::pair<float,float> p1 = positionHistory.at(i-1);
            std::pair<float,float> p2 = positionHistory.at(i);
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
                    draw_line(pntsA, trailWidth, trailColor.R->getDecimal(), trailColor.G->getDecimal(), trailColor.B->getDecimal(), trailColor.A->getDecimal(), 0xFFFF, 1);
                    pntsA.clear();
                }
            }
        }

        // plot any points remaining 
        if (!pntsA.empty()) {
            draw_line(pntsA, trailWidth, trailColor.R->getDecimal(), trailColor.G->getDecimal(), trailColor.B->getDecimal(), trailColor.A->getDecimal(), 0xFFFF, 1);
            pntsA.clear();
        }

        // draw last position to current point
        std::pair<float,float> p = positionHistory.back();

        float mx1 = (p.first - texLeft) / (texRight - texLeft) * width;
        float my1 = (p.second - texDown) / (texUp - texDown) * height;
        float mx2 = (hRatio - texLeft) / (texRight - texLeft) * width;
        float my2 = (vRatio - texDown) / (texUp - texDown) * height;

        pntsA.insert(pntsA.end(),{mx1, my1, mx2, my2});
        draw_line(pntsA, trailWidth, trailColor.R->getDecimal(), trailColor.G->getDecimal(), trailColor.B->getDecimal(), trailColor.A->getDecimal(), 0xFFFF, 1);
    }

}

void dcMap::updateTrail(void)
{
    // clear stored trails on fnClearTrail value change
    if (fnClearTrail) {
        int curr_clear_state = fnClearTrail->getInteger();
        if (prev_clear_state != curr_clear_state) {
            positionHistory.clear();
        }
        prev_clear_state = curr_clear_state;
    }

    // add position to list
    if (positionHistory.empty()) 
    {
        positionHistory.push_back({(float)hRatio, (float)vRatio});
    }
    else
    {
        std::pair<float,float> last_pos = positionHistory.back();
        float dist = sqrt(pow(hRatio-last_pos.first, 2) + pow(vRatio-last_pos.second, 2)*1.0);
        if ( dist > trailResolution) {
            positionHistory.push_back({hRatio, vRatio});
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

void dcMap::processPreCalculations(void) {
    computeGeometry();
}

void dcMap::processPostCalculations(void) {
    updateTrail();
}

void dcMap::draw(void)
{
    stencil_begin();        // enable stencil, clear existing buffer
    stencil_init_dest();    // setup stencil test to write 1's into destination area 

        container_start(refx, refy, delx, dely, 1, 1, 0);       // start container for masking shape (no rotation)

        if (enableCircularMap) {                                // draw circle or rectangle dest
            draw_ellipse(width/2, height/2, width/2, height/2, 100, 1, 1, 1, 1);
        } else {
            std::vector<float> pointsL;
            pointsL.reserve(8);
            addPoint(pointsL, 0, 0);
            addPoint(pointsL, 0, height);
            addPoint(pointsL, width, height);
            addPoint(pointsL, width, 0);
            draw_quad(pointsL, 0, 0, 0, 0);
        }

    stencil_init_proj();    // set stencil to only keep fragments with reference != 0

        if (enableTrackUp) {
            translate_start(width/2, height/2);
            rotate_start(-1 * (trajAngle + iconRotationOffset));
            translate_start(-1 * width/2, -1 * height/2);
        }
    
        // draw remaining projected fragments
        draw_map(this->textureID, width, height, texUp, texDown, texLeft, texRight);
        if ( enableZone )   displayZone();
        if ( enableTrail )  displayTrail();
        if ( enableIcon )   displayIcon();

        if (enableTrackUp) {
            translate_end();
            rotate_end();
            translate_end();
        }

        container_end();


    stencil_end();
}
