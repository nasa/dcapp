#include "opengl_draw.hh"
#include "panel.hh"

dcPanel::dcPanel(int id, float inox, float inoy, Kolor *incolor)
{
    displayID = id;
    orthoX = inox;
    orthoY = inoy;
    color.R = incolor->R;
    color.G = incolor->G;
    color.B = incolor->B;
    color.A = incolor->A;
}

bool dcPanel::checkID(int id)
{
    if (id == displayID) return true;
    else return false;
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
