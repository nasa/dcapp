#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "commonutils.hh"
#include "map.hh"


dcMap::dcMap(dcParent *myparent) :  dcGeometric(myparent), textureID(0x0), zoom(1), trailWidth(25), fnClearTrail(NULL),
                                    trailResolution(.005), enableCustomIcon(false), iconRotationOffset(0), selected(false)
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
    if (!lat1.empty() and !lon1.empty())
    {
        lat = getValue(lat1);
        lon = getValue(lon1);
    }
    else
    {
        printf("map.setLonLat: missing values\n");
    }
}

void dcMap::setZoom(const std::string &inval)
{
    if (!inval.empty()) zu = getValue(inval);
}

void dcMap::setFnClearTrail(const std::string &inval)
{
    if (!inval.empty()) 
        fnClearTrail = getValue(inval);
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
    this->iconTextureID = tdLoadTexture(filename);
    enableCustomIcon = true;
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
    if (zu) zoom = zu->getDecimal();
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
    float mx, my, mleft, mbottom, mright, mtop, mcenter, mmiddle, mwidth, mheight, mdelx, mdely;

    if (enableCustomIcon) 
    {
        mwidth = iconWidth;
        mheight = iconHeight;
    }
    else
    {
        mwidth = mheight = 25;
    }

    mleft = left + (hRatio - texLeft) / (texRight - texLeft) * width;
    mbottom = bottom + (vRatio - texDown) / (texUp - texDown) * height;

    mright = mleft;//mleft + mwidth/2;
    mtop = mbottom;//mbottom + mheight/2;
    mcenter = mleft;
    mmiddle = mbottom;

    switch (halign)
    {
        case dcLeft:
            mx = mleft;
            mdelx = 0;
            break;
        case dcCenter:
            mx = mcenter;
            mdelx = mwidth/2;
            break;
        case dcRight:
            mx = mright;
            mdelx = mwidth;
            break;
        default:
            break;
    }
    switch (valign)
    {
        case dcBottom:
            my = mbottom;
            mdely = 0;
            break;
        case dcMiddle:
            my = mmiddle;
            mdely = mheight/2;
            break;
        case dcTop:
            my = mtop;
            mdely = mheight;
            break;
        default:
            break;
    }

    mx = mleft;
    my = mbottom;
    mdelx = mwidth/2;
    mdely = mheight/2;

    if (enableCustomIcon) 
    {
        container_start(mx, my, mdelx/2, mdely/2, 1, 1, iconRotationOffset + trajAngle);
        draw_image(this->iconTextureID, mwidth, mheight);
        container_end();
    }
    else
    {
        circle_fill(mx, my, mwidth, 80, 1, 0, 0, 1);
        circle_outline(mx, my, mwidth, 80, 0, 0, 0, 1, 10, 0xFFFF, 1);
    }
}

// bind x,y points to the visible view
void dcMap::remapXYBounds(std::pair<float,float>& p) 
{
    if (p.first < texLeft)
        p.first = texLeft;
    else if (p.first > texRight)
        p.first = texRight;

    if (p.second < texDown)
        p.second = texDown;
    else if (p.second > texUp)
        p.second = texUp;
}

void dcMap::displayTrail(void)
{
    if (positionHistory.size() > 1) {
        for (uint i = 1; i < positionHistory.size(); i++) {
            std::pair<float,float> p1 = positionHistory.at(i-1);
            std::pair<float,float> p2 = positionHistory.at(i);
            if ( (p1.first > texLeft && p1.first < texRight && p1.second > texDown && p1.second < texUp) ||
                 (p2.first > texLeft && p2.first < texRight && p2.second > texDown && p2.second < texUp) ) {

                remapXYBounds(p1);
                remapXYBounds(p2);

                // calculate mx, my for points
                float mx1 = left + (p1.first - texLeft) / (texRight - texLeft) * width;
                float my1 = bottom + (p1.second - texDown) / (texUp - texDown) * height;
                float mx2= left + (p2.first - texLeft) / (texRight - texLeft) * width;
                float my2 = bottom + (p2.second - texDown) / (texUp - texDown) * height;

                // plot line for current set
                std::vector<float> pntsA = {mx1, my1, mx2, my2};
                draw_line(pntsA, trailWidth, trailColor.R->getDecimal(), trailColor.G->getDecimal(), trailColor.B->getDecimal(), trailColor.A->getDecimal(), 0xFFFF, 1);
            }
        }

        std::pair<float,float> p = positionHistory.back();
        remapXYBounds(p);

        float mx1 = left + (p.first - texLeft) / (texRight - texLeft) * width;
        float my1 = bottom + (p.second - texDown) / (texUp - texDown) * height;
        float mx2 = left + (hRatio - texLeft) / (texRight - texLeft) * width;
        float my2 = bottom + (vRatio - texDown) / (texUp - texDown) * height;

        std::vector<float> pntsA = {mx1, my1, mx2, my2};
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

void dcMap::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, 1, 1, 0);   // disable rotation for now
    draw_map(this->textureID, width, height, texUp, texDown, texLeft, texRight);
    container_end();

    if (enableTrail)
        displayTrail();

    if (enableIcon)
        displayIcon();

    updateTrail();
}
