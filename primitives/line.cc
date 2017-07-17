#include "line.hh"

dcLine::dcLine(float inlwidth, Kolor *incolor)
{
    linewidth = inlwidth;
    color.R = incolor->R;
    color.G = incolor->G;
    color.B = incolor->B;
    color.A = incolor->A;
}

void dcLine::draw(void)
{
    line_start(linewidth, *(color.R), *(color.G), *(color.B), *(color.A));
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->draw();
    }
    line_end();
}
