#include "nodes.hh"

static float GeomX(struct node *);
static float GeomY(struct node *);


Geometry GetGeometry(struct node *mynode)
{
    Geometry ret;
    float hwidth, hheight;

    ret.left = GeomX(mynode);
    ret.bottom = GeomY(mynode);
    if (mynode->info.w)
    {
        ret.width = *(mynode->info.w);
        hwidth = (0.5 * ret.width);
    }
    else
    {
        ret.width = 0;
        hwidth = 0;
    }
    if (mynode->info.h)
    {
        ret.height = *(mynode->info.h);
        hheight = (0.5 * ret.height);
    }
    else
    {
        ret.height = 0;
        hheight = 0;
    }
    ret.right = ret.left + ret.width;
    ret.top = ret.bottom + ret.height;
    ret.center = ret.left + hwidth;
    ret.middle = ret.bottom + hheight;
    switch (mynode->info.halign)
    {
        case AlignLeft:
            ret.refx = ret.left;
            ret.delx = 0;
            break;
        case AlignCenter:
            ret.refx = ret.center;
            ret.delx = hwidth;
            break;
        case AlignRight:
            ret.refx = ret.right;
            ret.delx = ret.width;
            break;
        default:
            break;
    }
    switch (mynode->info.valign)
    {
        case AlignBottom:
            ret.refy = ret.bottom;
            ret.dely = 0;
            break;
        case AlignMiddle:
            ret.refy = ret.middle;
            ret.dely = hheight;
            break;
        case AlignTop:
            ret.refy = ret.top;
            ret.dely = ret.height;
            break;
        default:
            break;
    }

    return ret;
}

static float GeomX(struct node *mynode)
{
    float val;

    if (mynode->info.x == 0)
    {
        switch (mynode->info.halign)
        {
            case AlignLeft:
                val = 0;
                break;
            case AlignCenter:
                val = (*(mynode->info.containerW)/2);
                break;
            case AlignRight:
                val = *(mynode->info.containerW);
                break;
            default:
                break;
        }
    }
    else if (*(mynode->info.x) < 0) val = *(mynode->info.x) + *(mynode->info.containerW);
    else val = *(mynode->info.x);

    switch (mynode->info.halign)
    {
        case AlignLeft:
            return val;
        case AlignCenter:
            return (val - (*(mynode->info.w)/2));
        case AlignRight:
            return (val - *(mynode->info.w));
        default:
            return 0;
    }
}

static float GeomY(struct node *mynode)
{
    float val;

    if (mynode->info.y == 0)
    {
        switch (mynode->info.valign)
        {
            case AlignBottom:
                val = 0;
                break;
            case AlignMiddle:
                val = (*(mynode->info.containerH)/2);
                break;
            case AlignTop:
                val = *(mynode->info.containerH);
                break;
            default:
                break;
        }
    }
    else if (*(mynode->info.y) < 0) val = *(mynode->info.y) + *(mynode->info.containerH);
    else val = *(mynode->info.y);

    switch (mynode->info.valign)
    {
        case AlignBottom:
            return val;
        case AlignMiddle:
            return (val - (*(mynode->info.h)/2));
        case AlignTop:
            return (val - *(mynode->info.h));
        default:
            return 0;
    }
}









static float GeomX(float *x, float w, float containerW, int halign)
{
    float val;

    if (!x)
    {
        switch (halign)
        {
            case AlignLeft:
                val = 0;
                break;
            case AlignCenter:
                val = (containerW/2);
                break;
            case AlignRight:
                val = containerW;
                break;
            default:
                break;
        }
    }
    else if (*x < 0) val = *x + containerW;
    else val = *x;

    switch (halign)
    {
        case AlignLeft:
            return val;
        case AlignCenter:
            return (val - (w/2));
        case AlignRight:
            return (val - w);
        default:
            return 0;
    }
}

static float GeomY(float *y, float h, float containerH, int valign)
{
    float val;

    if (!y)
    {
        switch (valign)
        {
            case AlignBottom:
                val = 0;
                break;
            case AlignMiddle:
                val = (containerH/2);
                break;
            case AlignTop:
                val = containerH;
                break;
            default:
                break;
        }
    }
    else if (*y < 0) val = *y + containerH;
    else val = *y;

    switch (valign)
    {
        case AlignBottom:
            return val;
        case AlignMiddle:
            return (val - (h/2));
        case AlignTop:
            return (val - h);
        default:
            return 0;
    }
}

Geometry GetGeometry(float *x, float *y, float *w, float *h, float containerW, float containerH, int halign, int valign)
{
    Geometry ret;
    float hwidth, hheight;

    ret.left = GeomX(x, *w, containerW, halign);
    ret.bottom = GeomY(y, *h, containerH, valign);
    if (w)
    {
        ret.width = *w;
        hwidth = (0.5 * ret.width);
    }
    else
    {
        ret.width = 0;
        hwidth = 0;
    }
    if (h)
    {
        ret.height = *h;
        hheight = (0.5 * ret.height);
    }
    else
    {
        ret.height = 0;
        hheight = 0;
    }
    ret.right = ret.left + ret.width;
    ret.top = ret.bottom + ret.height;
    ret.center = ret.left + hwidth;
    ret.middle = ret.bottom + hheight;
    switch (halign)
    {
        case AlignLeft:
            ret.refx = ret.left;
            ret.delx = 0;
            break;
        case AlignCenter:
            ret.refx = ret.center;
            ret.delx = hwidth;
            break;
        case AlignRight:
            ret.refx = ret.right;
            ret.delx = ret.width;
            break;
        default:
            break;
    }
    switch (valign)
    {
        case AlignBottom:
            ret.refy = ret.bottom;
            ret.dely = 0;
            break;
        case AlignMiddle:
            ret.refy = ret.middle;
            ret.dely = hheight;
            break;
        case AlignTop:
            ret.refy = ret.top;
            ret.dely = ret.height;
            break;
        default:
            break;
    }

    return ret;
}
