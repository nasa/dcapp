#include "RenderLib/RenderLib.hh"
#include "commonutils.hh"
#include "image.hh"

dcImage::dcImage(dcParent *myparent) : dcGeometric(myparent)
{
}

void dcImage::setTexture(const char *filename)
{
    textureID = tdLoadTexture(filename);
}

void dcImage::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, 1, 1, *rotate);
    draw_image(textureID, width, height);
    container_end();
}
