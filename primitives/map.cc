#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "commonutils.hh"
#include "map.hh"


dcMap::dcMap(dcParent *myparent) : dcGeometric(myparent), textureID(0x0), zoom(1), selected(false)
{
    return;
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

    setTextureBounds();
}

// get bounds for texture on 0 to 1 range
void dcMap::setTextureBounds(void)
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

void dcMap::displayCurrentPosition(void) {
    float mx, my, mleft, mbottom, mright, mtop, mcenter, mmiddle, mwidth;

    mleft = left + (hRatio - texLeft) / (texRight - texLeft) * width;
    mbottom = bottom + (vRatio - texDown) / (texUp - texDown) * height;
    mwidth = 25;

    mright = mleft + mwidth;
    mtop = mbottom + mwidth;
    mcenter = mleft + mwidth/2;
    mmiddle = mbottom + mwidth/2;

    switch (halign)
    {
        case dcLeft:
            mx = mleft;
            break;
        case dcCenter:
            mx = mcenter;
            break;
        case dcRight:
            mx = mright;
            break;
        default:
            break;
    }
    switch (valign)
    {
        case dcBottom:
            my = mbottom;
            break;
        case dcMiddle:
            my = mmiddle;
            break;
        case dcTop:
            my = mtop;
            break;
        default:
            break;
    }

    circle_fill(mx, my, mwidth, 80, 1, 0, 0, 1);
    circle_outline(mx, my, mwidth, 80, 0, 0, 0, 1, 10, 0xFFFF, 1);
}

void dcMap::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, 1, 1, 0);   // disable rotation for now
    draw_map(this->textureID, width, height, texUp, texDown, texLeft, texRight);
    container_end();

    displayCurrentPosition();
}
