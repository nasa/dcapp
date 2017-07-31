#include "opengl_draw.hh"
#include "string_utils.hh"
#include "loadUtils.hh"
#include "panel.hh"

dcPanel::dcPanel(dcParent *myparent) : displayID(0), orthoX(100), orthoY(100)
{
    myparent->addChild(this);
    color.R = dcLoadConstant(0.0f);
    color.G = dcLoadConstant(0.0f);
    color.B = dcLoadConstant(0.0f);
    color.A = dcLoadConstant(0.0f);
}

void dcPanel::setID(const char *inval)
{
    if (inval) displayID = StrToInt(inval, 0);
}

void dcPanel::setColor(const char *cspec)
{
    if (cspec) color = StrToColor(cspec, 0, 0, 0, 1);
}

void dcPanel::setOrtho(const char *inw, const char *inh)
{
    if (inw) orthoX = StrToFloat(inw, 100);
    if (inh) orthoY = StrToFloat(inh, 100);
}

bool dcPanel::checkID(int id)
{
    if (id == displayID) return true;
    else return false;
}

float * dcPanel::getContainerWidth(void)
{
    return &orthoX;
}

float * dcPanel::getContainerHeight(void)
{
    return &orthoY;
}

void dcPanel::draw(void)
{
    setup_panel(orthoX, orthoY, -1, 1, *(color.R), *(color.G), *(color.B), *(color.A));

    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->draw();
    }
}

void dcPanel::handleMousePress(float x, float y)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleMousePress(orthoX * x, orthoY * y);
    }
}
