/*******************************************************************************
Description: Provides XRotDrawString functionality, which was based largely on
    "xvertext 5.0", built by Alan Richardson.  Reference:
    http://lists.gnu.org/archive/html/bug-gnustep/2002-03/msg00108.html
Programmer: M. McFarlane, March 2005
*******************************************************************************/

/* ********************************************************************** */
/* xvertext 5.0, Copyright (c) 1993 Alan Richardson (address@bogus.example.com)
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  All work developed as a consequence of the use of
 * this program should duly acknowledge such use. No representations are
 * made about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 */
 /* Modified 25 Mar 02 by S. V. Ramanan for GNUStep
  * 1. independent x and y scaling added
  * 2. image cache-ing removed
  * 3. user-stippling, background and alignment removed
 */
/* ********************************************************************** */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "rotate.hh"

extern int tdAlignLeft;
extern int tdAlignCenter;
extern int tdAlignRight;
extern int tdAlignBottom;
extern int tdAlignMiddle;
extern int tdAlignTop;
extern int tdAlignBaseline;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* A structure holding everything needed for a rotated string */
typedef struct rotated_text_item_template
{
    Pixmap bitmap;
    int cols_in;
    int rows_in;
    int cols_out;
    int rows_out;
} RotatedTextItem;

/* ---------------------------------------------------------------------- */

static XImage *MakeXImage(Display *, int, int);
int XRotDrawString(Display *, XFontStruct *, Drawable, GC, int, int,  float, float, float, int, const char *);
static RotatedTextItem *XRotCreateTextItem(Display *, XFontStruct *, float, float, float, float, const char *);
static void XRotFreeTextItem(Display *, RotatedTextItem *);
static XImage *XRotMagnifyImage(Display *, XImage *, float, float);


/**************************************************************************/
/*  Create an XImage structure and allocate memory for it                 */
/**************************************************************************/
static XImage *MakeXImage(Display *dpy, int w, int h)
{
    /* reserve memory for image */
    char *data = (char *)calloc((unsigned)(((w-1)/8+1)*h), 1);
    if (!data) return 0x0;

    /* create the XImage */
    XImage *I = XCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)), 1, XYBitmap, 0, data, w, h, 8, 0);
    if (!I)
    {
        free(data);
        return 0x0;
    }

    I->byte_order = I->bitmap_bit_order = MSBFirst;
    return I;
}


/**************************************************************************/
/*  paints a rotated string                                               */
/**************************************************************************/
int XRotDrawString(Display *dpy, XFontStruct *font, Drawable drawable,
                   GC gc, int x, int y, float angle,
                   float scalex, float scaley, int align, const char *text)
{
    GC my_gc;
    int xp, yp;
    float hot_x, hot_y;
    float hot_xp, hot_yp;
    float sin_angle, cos_angle;
    RotatedTextItem *item;

    /* return early for NULL/empty strings or bad scale factors */
    if (!text) return 0;
    if (strlen(text) == 0 || scalex <= 0 || scaley <= 0) return 0;

    /* horizontal text made easy */
    if (angle==0. && scalex==1. && scaley == 1.)
    {
        XDrawString(dpy, drawable, gc, x, y, text, strlen(text));
        return 0;
    }

    /* manipulate angle to 0<=angle<360 degrees */
    while (angle < 0) angle += 360;
    while (angle >= 360) angle -= 360;
    if (angle == 0)
    {
        cos_angle = 1.; sin_angle = 0.;
    }
    else if (angle == 90)
    {
        cos_angle = 0.; sin_angle = 1.;
    }
    else if (angle == 180)
    {
        cos_angle = -1.; sin_angle = 0.;
    }
    else if (angle == 270)
    {
        cos_angle = 0.; sin_angle = -1.;
    }
    else
    {
        angle *= M_PI/180.;
        cos_angle = cos(angle);
        sin_angle = sin(angle);
    }

    /* get a rotated bitmap */
    item = XRotCreateTextItem(dpy, font, sin_angle, cos_angle, scalex, scaley, text);
    if (!item) return 1;

    /* this gc has similar properties to the user's gc */
    my_gc = XCreateGC(dpy, drawable, 0, 0);
    XCopyGC(dpy, gc, GCForeground|GCBackground|GCFunction|GCPlaneMask, my_gc);

    /* which point (hot_x, hot_y) relative to bitmap centre
      coincides with user's specified point? */
    if (align & tdAlignCenter)
        hot_x = 0.;
    else if (align & tdAlignRight)
        hot_x = (float)(item->cols_in/2.)*scalex;
    else
        hot_x = -(float)(item->cols_in/2.)*scalex;

    if (align & tdAlignMiddle)
        hot_y = 0;
    else if (align & tdAlignTop)
        hot_y = (float)(item->rows_in/2.)*scaley;
    else if (align & tdAlignBaseline)
        hot_y = -((float)(item->rows_in/2.)-(float)font->descent)*scaley;
    else
        hot_y = -(float)(item->rows_in/2.)*scaley;

    /* rotate hot_x and hot_y around bitmap centre */
    hot_xp = hot_x*cos_angle - hot_y*sin_angle;
    hot_yp = hot_x*sin_angle + hot_y*cos_angle;

    /* where should top left corner of bitmap go ? */
    xp = x - (int)((float)item->cols_out/2. + hot_xp);
    yp = y - (int)((float)item->rows_out/2. - hot_yp);

    /* paint text using stipple technique */
    XSetFillStyle(dpy, my_gc, FillStippled);
    XSetStipple(dpy, my_gc, item->bitmap);
    XSetTSOrigin(dpy, my_gc, xp, yp);
    XFillRectangle(dpy, drawable, my_gc, xp, yp, item->cols_out, item->rows_out);

    /* free our resources */
    XFreeGC(dpy, my_gc);

    /*  destroy item completely */
    XRotFreeTextItem(dpy,item);

    /* we got to the end OK! */
    return 0;
}


/**************************************************************************/
/*  Create a rotated text item                                            */
/**************************************************************************/
static RotatedTextItem *XRotCreateTextItem(Display *dpy, XFontStruct *font,
                        float sin_angle, float cos_angle,
                        float scalex, float scaley, const char *text)
{
    RotatedTextItem *item=0x0;
    Pixmap canvas;
    GC font_gc;
    XImage *I_in, *ximage;
    register int i, j;
    int height;
    int byte_w_in, byte_w_out;
    int xp, yp;
    float tan_angle;
    int it, jt;
    float di, dj;
    float xl, xr, xinc;
    int byte_out;
    int dir, asc, desc;
    XCharStruct overall;
    int old_cols_in=0, old_rows_in=0;

    /* allocate memory */
    item = (RotatedTextItem *)malloc((unsigned)sizeof(RotatedTextItem));
    if (!item) return 0x0;
    item->bitmap = (Pixmap)0x0;

    XTextExtents(font, text, strlen(text), &dir, &asc, &desc, &overall);

    /* overall font height */
    /* dimensions horizontal text will have */
    item->cols_in=overall.rbearing;
    item->rows_in=height=font->ascent+font->descent;

    /* bitmap for drawing on */
    canvas=XCreatePixmap(dpy, DefaultRootWindow(dpy), item->cols_in, item->rows_in, 1);

    /* create a GC for the bitmap */
    font_gc=XCreateGC(dpy, canvas, 0, 0);
    XSetBackground(dpy, font_gc, 0);
    XSetFont(dpy, font_gc, font->fid);

    /* make sure the bitmap is blank */
    XSetForeground(dpy, font_gc, 0);
    XFillRectangle(dpy, canvas, font_gc, 0, 0, item->cols_in+1, item->rows_in+1);
    XSetForeground(dpy, font_gc, 1);

    /* draw text horizontally */

    /* where to draw section */
    yp=font->ascent;
    xp=0;

    /* draw string onto bitmap */
    XDrawString(dpy, canvas, font_gc, xp, yp, text, strlen(text));

    /* create image to hold horizontal text */
    I_in = MakeXImage(dpy, item->cols_in, item->rows_in);
    if (!I_in)
    {
        XFreeGC(dpy, font_gc);
        XFreePixmap(dpy, canvas);
        return 0x0;
    }

    /* extract horizontal text */
    XGetSubImage(dpy, canvas, 0, 0, item->cols_in, item->rows_in, 1, XYPixmap, I_in, 0, 0);
    I_in->format=XYBitmap;

    /* magnify horizontal text */
    if (scalex != 1. || scaley != 1.)
    {
        I_in = XRotMagnifyImage(dpy, I_in, scalex, scaley);
        old_cols_in=item->cols_in;
        old_rows_in=item->rows_in;
        item->cols_in = (int)((float)item->cols_in * scalex);
        item->rows_in = (int)((float)item->rows_in * scaley);
    }

    item->cols_out = overall.rbearing;
    item->rows_out = height = font->ascent + font->descent;

    /* how big will rotated text be ? */
    item->cols_out = (int)(fabs((float)item->rows_in*sin_angle) +
                     fabs((float)item->cols_in*cos_angle) + 2.99999);

    item->rows_out = (int)(fabs((float)item->rows_in*cos_angle) +
                     fabs((float)item->cols_in*sin_angle) + 2.99999);

    if (item->cols_out%2 == 0) item->cols_out++;
    if (item->rows_out%2 == 0) item->rows_out++;

    /* create image to hold rotated text */
    ximage = MakeXImage(dpy, item->cols_out, item->rows_out);
    if (!ximage)
    {
        XDestroyImage(I_in);
        XFreeGC(dpy, font_gc);
        XFreePixmap(dpy, canvas);
        return 0x0;
    }

    byte_w_in = (item->cols_in-1)/8 + 1;
    byte_w_out = (item->cols_out-1)/8 + 1;

    /* we try to make this bit as fast as possible - which is why it looks a bit over-the-top */

    /* vertical distance from centre */
    dj = 0.5 - (float)item->rows_out/2;

    /* where abouts does text actually lie in rotated image? */
    if (cos_angle == 0 || sin_angle == 0)
    {
        xl=0;
        xr=(float)item->cols_out;
        xinc=0;
    }
    else if (sin_angle > 0)
    {
        tan_angle = sin_angle/cos_angle;
        xl = (float)item->cols_out/2 +
             (dj-(float)item->rows_in/(2*cos_angle))/tan_angle - 2;
        xr = (float)item->cols_out/2 +
             (dj+(float)item->rows_in/(2*cos_angle))/tan_angle + 2;
        xinc = 1./tan_angle;
    }
    else
    {
        tan_angle = sin_angle/cos_angle;
        xl = (float)item->cols_out/2 +
             (dj+(float)item->rows_in/(2*cos_angle))/tan_angle - 2;
        xr = (float)item->cols_out/2 +
             (dj-(float)item->rows_in/(2*cos_angle))/tan_angle + 2;
        xinc = 1./tan_angle;
    }

    /* loop through all relevent bits in rotated image */
    for (j=0; j<item->rows_out; j++)
    {
        /* no point re-calculating these every pass */
        di = (float)((xl<0)?0:(int)xl) + 0.5 - (float)item->cols_out/2;
        byte_out = (item->rows_out-j-1) * byte_w_out;

        /* loop through meaningful columns */
        for (i = ((xl<0)?0:(int)xl); i < ((xr>=item->cols_out)?item->cols_out:(int)xr); i++)
        {
            /* rotate coordinates */
            it = (int)((float)item->cols_in/2. + ( di*cos_angle + dj*sin_angle));
            jt = (int)((float)item->rows_in/2. - (-di*sin_angle + dj*cos_angle));

            /* set pixel if required */
            if (it>=0 && it<item->cols_in && jt>=0 && jt<item->rows_in)
            {
                if ((I_in->data[jt*byte_w_in+it/8] & 128>>(it%8))>0) ximage->data[byte_out+i/8]|=128>>i%8;
            }

            di += 1;
        }
        dj += 1;
        xl += xinc;
        xr += xinc;
    }
    XDestroyImage(I_in);

    if ((scalex != 1.) || (scaley != 1.))
    {
        item->cols_in=old_cols_in;
        item->rows_in=old_rows_in;
    }

    /* create a bitmap to hold rotated text */
    item->bitmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), item->cols_out, item->rows_out, 1);

    /* make the text bitmap from XImage */
    XPutImage(dpy, item->bitmap, font_gc, ximage, 0, 0, 0, 0, item->cols_out, item->rows_out);

    XDestroyImage(ximage);

    XFreeGC(dpy, font_gc);
    XFreePixmap(dpy, canvas);

    return item;
}


/**************************************************************************/
/*  Free the resources used by a text item                                */
/**************************************************************************/
static void XRotFreeTextItem(Display *dpy, RotatedTextItem *item)
{
    if (item->bitmap) XFreePixmap(dpy, item->bitmap);
    free(item);
}


/**************************************************************************/
/* Magnify an XImage using bilinear interpolation                         */
/**************************************************************************/
static XImage *XRotMagnifyImage(Display *dpy, XImage *ximage, float scalex, float scaley)
{
    int i, j;
    float x, y;
    float u,t;
    XImage *I_out;
    int cols_in, rows_in;
    int cols_out, rows_out;
    register int i2, j2;
    float z1, z2, z3, z4;
    int byte_width_in, byte_width_out;
    float magx_inv, magy_inv;

    /* size of input image */
    cols_in = ximage->width;
    rows_in = ximage->height;

    /* size of final image */
    cols_out = (int)((float)cols_in * scalex);
    rows_out = (int)((float)rows_in * scaley);

    /* this will hold final image */
    I_out = MakeXImage(dpy, cols_out, rows_out);
    if (!I_out) return 0x0;

    /* width in bytes of input, output images */
    byte_width_in=(cols_in-1)/8 + 1;
    byte_width_out=(cols_out-1)/8 + 1;

    /* for speed */
    magx_inv = 1./scalex;
    magy_inv = 1./scaley;

    y = 0.;

    /* loop over magnified image */
    for(j2=0; j2<rows_out; j2++)
    {
        x = 0;
        j = (int)y;

        for(i2=0; i2<cols_out; i2++)
        {
            i = (int)x;

            /* bilinear interpolation - where are we on bitmap ? */
            /* right edge */
            if (i == cols_in-1 && j != rows_in-1)
            {
                t = 0;
                u = y-(float)j;

                z1 = (ximage->data[j*byte_width_in+i/8] & 128>>(i%8))>0;
                z2 = z1;
                z3 = (ximage->data[(j+1)*byte_width_in+i/8] & 128>>(i%8))>0;
                z4 = z3;
            }
            /* top edge */
            else if (i != cols_in-1 && j == rows_in-1)
            {
                t = x-(float)i;
                u = 0;

                z1 = (ximage->data[j*byte_width_in+i/8] & 128>>(i%8))>0;
                z2 = (ximage->data[j*byte_width_in+(i+1)/8] & 128>>((i+1)%8))>0;
                z3 = z2;
                z4 = z1;
            }
            /* top right corner */
            else if (i == cols_in-1 && j == rows_in-1)
            {
                u = 0;
                t = 0;

                z1 = (ximage->data[j*byte_width_in+i/8] & 128>>(i%8))>0;
                z2 = z1;
                z3 = z1;
                z4 = z1;
            }
            /* somewhere `safe' */
            else
            {
                t = x-(float)i;
                u = y-(float)j;

                z1 = (ximage->data[j*byte_width_in+i/8] & 128>>(i%8))>0;
                z2 = (ximage->data[j*byte_width_in+(i+1)/8] & 128>>((i+1)%8))>0;
                z3 = (ximage->data[(j+1)*byte_width_in+(i+1)/8] & 128>>((i+1)%8))>0;
                z4 = (ximage->data[(j+1)*byte_width_in+i/8] & 128>>(i%8))>0;
            }

            /* if interpolated value is greater than 0.5, set bit */
            if (((1-t)*(1-u)*z1 + t*(1-u)*z2 + t*u*z3 + (1-t)*u*z4) > 0.5)
                I_out->data[j2*byte_width_out+i2/8]|=128>>i2%8;

            x += magx_inv;
        }
        y += magy_inv;
    }

    /* destroy original */
    XDestroyImage(ximage);

    /* return big image */
    return I_out;
}

#undef M_PI
