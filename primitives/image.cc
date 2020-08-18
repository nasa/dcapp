#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "commonutils.hh"
#include "image.hh"

extern void RegisterPressedPrimitive(dcParent *);

dcImage::dcImage(dcParent *myparent) : dcGeometric(myparent), textureID(0x0), selected(false)
{
    PressList = new dcParent;
    ReleaseList = new dcParent;
    PressList->setParent(this);
    ReleaseList->setParent(this);
}

dcImage::~dcImage()
{
    delete PressList;
    delete ReleaseList;
}


void dcImage::setTexture(const std::string &filename)
{
    this->textureID = tdLoadTexture(filename);
}

void dcImage::handleMousePress(double inx, double iny)
{
    if (this->PressList->children.empty() && this->ReleaseList->children.empty()) return;

    double finalx, finaly;

    computeGeometry();
    if (rotate->getDecimal())
    {
        double ang = (rotate->getDecimal()) * 0.01745329252;
        double originx = refx - ((delx * cos(-ang)) + (dely * sin(-ang)));
        double originy = refy - ((dely * cos(-ang)) - (delx * sin(-ang)));
        double tmpx = inx - originx;
        double tmpy = iny - originy;
        finalx = (tmpx * cos(ang)) + (tmpy * sin(ang));
        finaly = (tmpy * cos(ang)) - (tmpx * sin(ang));
    }
    else
    {
        finalx = inx + delx - refx;
        finaly = iny + dely - refy;
    }

    if ((0 < finalx) && (finalx < width) && (0 < finaly) && (finaly < height))
    {
        float xpct = finalx / width;
        float ypct = finaly / height;
        unsigned char rgba[4];

        // get_image_pixel is very slow. An alternative would be to check the image when the handler is first assigned
        // to see if it is opaque. If so, then the below check can be skipped. If not, then the image's pixel array
        // could be stored for later use to speed up each instance of get_image_pixel_RGBA.
        get_image_pixel(rgba, this->textureID, xpct, ypct);

        if (rgba[3])
        {
            this->selected = true;
            this->PressList->handleEvent();
            RegisterPressedPrimitive(this->PressList);
        }
    }
}

void dcImage::handleMouseRelease(void)
{
    if (this->selected)
    {
        this->selected = false;
        this->ReleaseList->handleEvent();
    }
}

void dcImage::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, 1, 1, rotate->getDecimal());
    draw_image(this->textureID, width, height);
    container_end();
}
