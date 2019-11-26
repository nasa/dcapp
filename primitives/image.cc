#include "RenderLib/RenderLib.hh"
#include "commonutils.hh"
#include "image.hh"

dcImage::dcImage(dcParent *myparent) : dcGeometric(myparent), textureID(0x0)
{
}

void dcImage::setTexture(const char *filename)
{
    if (filename) textureID = tdLoadTexture(filename);
}

void dcImage::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, 1, 1, *rotate);
    draw_image(textureID, width, height);
    container_end();
}
