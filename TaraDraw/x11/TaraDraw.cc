/*******************************************************************************
Description: TaraDraw adapter for X11.
Programmer: M. McFarlane, March 2005
*******************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "tdDefines.hh"
#include "rotate.hh"
#include "image.hh"
#include "xfonts.hh"

#define MAX_FONTS 512
#define MAX_COLORS 256
#define BORDER_WIDTH 0

#define RTD 57.2957795131

typedef struct
{
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long inputMode;
    unsigned long status;
} WmHints;

typedef struct
{
    XFontStruct *info;
    char *face;
    int size;
} FontStruct;

typedef struct windowspec
{
    int height;
    int width;
    Window win;
    GC gc;
    Pixmap buffer;
    XPoint storedpoint;
    unsigned long bgcolor;
    unsigned long fgcolor;
    unsigned int linewidth;
    unsigned int capstyle;
    FontStruct *curfont;
    int needsRedraw;
    struct windowspec *next;
} WindowSpec;

typedef struct
{
    GLXContext context;
    Window win;
    WindowSpec *parent;
} GLSpec;

extern TDImage *tdLoadBmpImage(char *);

int tdRegisterColor(int, float, float, float);
int tdRegisterXFont(const char *, int, char *);
void tdSetActiveWindow(tdWindow);
void tdSetFont(const char *, float);
void tdRenderGraphics(void);
void tdCloseWindow(void);

int tdButtLineCapStyle = CapButt;
int tdRoundLineCapStyle = CapRound;
int tdSquareLineCapStyle = CapProjecting;
int tdAlignLeft = 0x00;
int tdAlignCenter = 0x01;
int tdAlignRight = 0x02;
int tdAlignBottom = 0x00;
int tdAlignMiddle = 0x04;
int tdAlignTop = 0x08;
int tdAlignBaseline = 0x10;

static WindowSpec *startwin = 0x0;
static WindowSpec *current = 0x0;
static GLSpec *curGL = 0x0;
static Display *display;
static int screen_num;
static Atom wm_delete_window, wm_hints;
static Colormap default_cmap;
static unsigned long cmap[MAX_COLORS];
static FontStruct font[MAX_FONTS];
static int fontnum = 0;

// Initialization routines

int tdInitialize(char *xdisplay)
{
    int i, j, len, maxlen = 0;
    char *fontspec;

    display = XOpenDisplay(xdisplay);
    if (!display)
    {
        printf("tdInitialize: Can't connect to X server\n");
        return (-1);
    }

    screen_num = DefaultScreen(display);
    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    wm_hints = XInternAtom(display, "_MOTIF_WM_HINTS", False);
    default_cmap = DefaultColormap(display, screen_num);

    tdRegisterColor(0, 1.0, 1.0, 1.0); // white
    tdRegisterColor(1, 0.0, 0.0, 0.0); // black

    for (i=0; i<tdXFontCount; i++)
    {
        for (j=0; j<tdXFontSizeCount; j++)
        {
            len = strlen(tdXFont[i].xspec);
            if (len > maxlen) maxlen = len;
        }
    }

    fontspec = (char *)malloc(maxlen);
    if (!fontspec)
    {
        printf("tdInitialize: Can't allocate memory for font definitions\n");
        return (-1);
    }

    for (i=0; i<tdXFontCount; i++)
    {
        for (j=0; j<tdXFontSizeCount; j++)
        {
            sprintf(fontspec, tdXFont[i].xspec, tdXFontSize[j]);
            tdRegisterXFont(tdXFont[i].tdspec, tdXFontSize[j], fontspec);
        }
    }
    free(fontspec);

    return 0;
}

tdWindow tdOpenWindow(const char *title, float xpos, float ypos, float width, float height, int align)
{
    XGCValues values;
    XTextProperty textprop;
    char *strlist;
    XWindowAttributes attr;
    XSizeHints sizehints;
    WindowSpec *newwin;

    newwin = (WindowSpec *)malloc(sizeof(WindowSpec));

    if (align & tdAlignCenter)
        xpos -= width/2;
    else if (align & tdAlignRight)
        xpos -= width;

    if (align & tdAlignMiddle)
        ypos = DisplayHeight(display, screen_num) - ypos - height/2;
    else if (align & tdAlignTop)
        ypos = DisplayHeight(display, screen_num) - ypos;
    else
        ypos = DisplayHeight(display, screen_num) - ypos - height;

    newwin->win = XCreateSimpleWindow(display, RootWindow(display, screen_num), (int)xpos, (int)ypos,
        (int)width, (int)height, BORDER_WIDTH, BlackPixel(display, screen_num), WhitePixel(display, screen_num));

    sizehints.flags = USPosition | USSize;
    sizehints.x = (int)xpos;
    sizehints.y = (int)ypos;
    sizehints.width = (int)width;
    sizehints.height = (int)height;
    XSetStandardProperties(display, newwin->win, title, title, None, 0x0, 0, &sizehints);

    strlist = (char *)malloc(strlen(title)+1);
    strcpy(strlist, title);
    XStringListToTextProperty(&strlist, 1, &textprop);
    XSetWMName(display, newwin->win, &textprop);
    XSetWMIconName(display, newwin->win, &textprop);
    free(strlist);

    XGetWindowAttributes(display, newwin->win, &attr);
    newwin->gc = XCreateGC(display, newwin->win, 0, &values);
    newwin->buffer = XCreatePixmap(display, newwin->win, (unsigned int)width, (unsigned int)height, attr.depth);

    XSelectInput(display, newwin->win,
        KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | ExposureMask);
    XMapWindow(display, newwin->win);
    XSetWMProtocols(display, newwin->win, &wm_delete_window, 1);

    newwin->height = (int)height;
    newwin->width = (int)width;

    // set default colors and linestyle
    newwin->fgcolor = BlackPixel(display, screen_num);
    newwin->bgcolor = WhitePixel(display, screen_num);
    newwin->linewidth = 1;
    newwin->capstyle = CapButt;
    XSetLineAttributes(display, newwin->gc, newwin->linewidth, LineSolid, newwin->capstyle, JoinRound);

    if (!current)
    {
        newwin->next = 0x0;
        startwin = newwin;
    }
    else
    {
        newwin->next = current->next;
        current->next = newwin;
    }

    current = newwin;
    tdSetFont("Helvetica", 12);

    return newwin->win;
}

tdWindow tdOpenFullScreen(const char *title)
{
    WmHints hints;

    tdWindow mywin = tdOpenWindow(title, 0, 0, (float)DisplayWidth(display, screen_num), (float)DisplayHeight(display, screen_num), tdAlignLeft | tdAlignBottom);

    hints.flags = 2;        // Specify that we're changing the window decorations
    hints.functions = 0;
    hints.decorations = 0;  // Specify window decorations: 1=on, 0=off
    hints.inputMode = 0;
    hints.status = 0;

    XChangeProperty(display, mywin, wm_hints, wm_hints, 32, PropModeReplace, (unsigned char *)&hints, 5);

    return mywin;
}

tdImage *tdLoadImage(char *filespec, tdSize *size)
{
    unsigned i;
    XColor mycolor;
    TDImage *image = tdLoadBmpImage(filespec);

    if (image == 0)
    {
        if (size)
        {
            size->width = 0;
            size->height = 0;
        }
        return 0;
    }

    // convert RGBA color table to X colors
    image->xcolor = (unsigned long *)calloc(image->ncolors, sizeof(unsigned long));
    for (i=0; i<image->ncolors; i++)
    {
        mycolor.red = (int)(image->color[i].red*65535);
        mycolor.green = (int)(image->color[i].green*65535);
        mycolor.blue = (int)(image->color[i].blue*65535);
        XAllocColor(display, default_cmap, &mycolor);
        image->xcolor[i] = mycolor.pixel;
    }
    free(image->color);

    if (size)
    {
        size->width = image->width;
        size->height = image->height;
    }

    return (tdImage *)image;
}

tdGLContext *tdGLCreateContext(tdWindow winid)
{
    XVisualInfo *visual;

    int attr_list[] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        GLX_RED_SIZE, 4,
        GLX_GREEN_SIZE, 4,
        GLX_BLUE_SIZE, 4,
        GLX_DEPTH_SIZE, 16,
        None };

    curGL = (GLSpec *)malloc(sizeof(GLSpec));
    if (!curGL)
    {
        printf("tdGLCreateContext: Can't allocate memory for OpenGL context\n");
        return 0x0;
    }

    for (curGL->parent = startwin; curGL->parent; curGL->parent = curGL->parent->next)
    {
        if (curGL->parent->win == (Window)winid) break;
    }
    if (!(curGL->parent))
    {
        printf("tdGLCreateContext: Invalid window specified\n");
        return 0x0;
    }

    visual = glXChooseVisual(display, screen_num, attr_list);
    if (!visual)
    {
        printf("tdGLCreateContext: Can't create OpenGL context\n");
        return 0x0;
    }

    curGL->context = glXCreateContext(display, visual, 0, True);
    curGL->win = XCreateSimpleWindow(display, curGL->parent->win, 0, 0, curGL->parent->width, curGL->parent->height, 0, 0, 0);
    XMapWindow(display, curGL->win);
    glXMakeCurrent(display, curGL->win, curGL->context);

    return (tdGLContext *)curGL;
}

void tdGLReshapeContext(float x, float y, int align, float width, float height)
{
    XMoveResizeWindow(display, curGL->win, (int)x, curGL->parent->height-(int)(height+y), (int)width, (int)height);
}

int tdRegisterColor(int index, float r, float g, float b)
{
    XColor mycolor;

    if (index < 0 || index >= MAX_COLORS)
    {
        printf("tdRegisterColor: Color index %d is out of bounds\n", index);
        return (-1);
    }

    mycolor.red = (int)(r*65535);
    mycolor.green = (int)(g*65535);
    mycolor.blue = (int)(b*65535);
    XAllocColor(display, default_cmap, &mycolor);
    cmap[index] = mycolor.pixel;

    return 0;
}

int tdRegisterXFont(const char *face, int size, char *spec)
{
    int i, index = fontnum;

    for (i=0; i<fontnum; i++)
    {
        if (!strcmp(font[i].face, face) && font[i].size == size)
        {
            index = i;
            break;
        }
    }

    if (index >= MAX_FONTS)
    {
        printf("tdRegisterXFont: Maximum allowable number of fonts exceeded\n");
        return (-1);
    }

    font[index].info = XLoadQueryFont(display, spec);

    if (!(font[index].info))
    {
        printf("tdRegisterXFont: Cannot open font %s\n", spec);
        return (-1);
    }

    font[index].face = (char *)malloc(strlen(face)+1);
    strcpy(font[index].face, face);
    font[index].size = size;
    if (index == fontnum) fontnum++;

    return 0;
}

// Settings routines

void tdSetActiveWindow(tdWindow winid)
{
    for (WindowSpec *win = startwin; win; win = win->next)
    {
        if (win->win == (Window)winid)
        {
            current = win;
            return;
        }
    }
}

void tdGLSetContext(tdGLContext *glContext)
{
    curGL = (GLSpec *)glContext;
    glXMakeCurrent(display, curGL->win, curGL->context);
}

void tdSetBackgroundColor(int color)
{
    if (!current || color < 0 || color >= MAX_COLORS) return;
    current->bgcolor = cmap[color];
}

void tdSetColor(int color)
{
    if (!current || color < 0 || color >= MAX_COLORS) return;
    current->fgcolor = cmap[color];
    XSetForeground(display, current->gc, current->fgcolor);
}

void tdSetColorRGB(float r, float g, float b)
{
    if (!current) return;

    XColor mycolor;
    mycolor.red = (int)(r*65535);
    mycolor.green = (int)(g*65535);
    mycolor.blue = (int)(b*65535);
    XAllocColor(display, default_cmap, &mycolor);
    current->fgcolor = mycolor.pixel;
    XSetForeground(display, current->gc, current->fgcolor);
}

void tdSetFont(const char *fontface, float fontsize)
{
    if (!current) return;

    for (int i=0; i<fontnum; i++)
    {
        if (!strcmp(font[i].face, fontface) && font[i].size == (int)fontsize)
        {
            current->curfont = &font[i];
            XSetFont(display, current->gc, current->curfont->info->fid);
            return;
        }
    }
}

void tdSetLineCap(int style)
{
    if (!current) return;
    current->capstyle = style;
    XSetLineAttributes(display, current->gc, current->linewidth, LineSolid, current->capstyle, JoinRound);
}

void tdSetLineWidth(float linewidth)
{
    if (!current) return;
    current->linewidth = (int)(linewidth+0.5);
    XSetLineAttributes(display, current->gc, current->linewidth, LineSolid, current->capstyle, JoinRound);
}

void tdSetNeedsRedraw(tdWindow winid)
{
    for (WindowSpec *win = startwin; win; win = win->next)
    {
        if (win->win == (Window)winid)
        {
            win->needsRedraw = 1;
            return;
        }
    }
}

// Query routines

void tdGetScreenData(tdSize *total, tdRegion *usable)
{
    if (total)
    {
        total->width = (float)DisplayWidth(display, screen_num);
        total->height = (float)DisplayHeight(display, screen_num);
    }
    if (usable) // assume that the full screen is usable
    {
        usable->left = 0;
        usable->right = (float)DisplayWidth(display, screen_num);
        usable->center = (usable->right/2);
        usable->width = usable->right;
        usable->bottom = 0;
        usable->top = (float)DisplayHeight(display, screen_num);
        usable->middle = (usable->top/2);
        usable->height = usable->top;
    }
}

tdColorRGB tdGetColor(int index)
{
    tdColorRGB mycolor;
    XColor xcolor;

    if (index < 0 || index >= MAX_COLORS)
    {
        printf("tdGetColor: Color index %d is out of bounds\n", index);
        mycolor.red = 0;
        mycolor.green = 0;
        mycolor.blue = 0;
    }
    else
    {
        xcolor.pixel = cmap[index];
        XQueryColor(display, default_cmap, &xcolor);
        mycolor.red = (float)xcolor.red/65535;
        mycolor.green = (float)xcolor.green/65535;
        mycolor.blue = (float)xcolor.blue/65535;
    }

    return mycolor;
}

tdSize tdGetStringBounds(char *string)
{
    tdSize mysize;

    if (current)
    {
        XFontStruct *finfo = current->curfont->info;
        mysize.width = (float)XTextWidth(finfo, string, strlen(string));
        mysize.height = (float)(finfo->descent + finfo->ascent);
    }
    else
    {
        mysize.width = 0;
        mysize.height = 0;
    }

    return mysize;
}

void tdGetPointer(tdPosition *screenpos, tdPosition *windowpos)
{
    Window root, child;
    int screenx, screeny, winx, winy;
    unsigned int keys_buttons;

    XQueryPointer(display, current->win, &root, &child, &screenx, &screeny, &winx, &winy, &keys_buttons);
    if (screenpos)
    {
        screenpos->x = (float)screenx;
        screenpos->y = (float)(DisplayHeight(display, screen_num) - screeny);
    }
    if (windowpos)
    {
        windowpos->x = (float)winx;
        windowpos->y = (float)(current->height - winy);
    }
}

int tdNeedsRedraw(tdWindow winid)
{
    for (WindowSpec *win = startwin; win; win = win->next)
    {
        if (win->win == (Window)winid)
        {
            if (win->needsRedraw)
            {
                win->needsRedraw = 0;
                return 1;
            }
            else return 0;
        }
    }

    return 0;
}

// Drawing routines

void tdToggleFullScreen(void)
{
}

void tdClearWindow(void)
{
    if (!current) return;

    XSetForeground(display, current->gc, current->bgcolor);
    XFillRectangle(display, current->buffer, current->gc, 0, 0, current->width, current->height);
    XSetForeground(display, current->gc, current->fgcolor);
}

void tdDrawFilledPoly(float x[], float y[], int vertices)
{
    if (!current) return;

    XPoint *point = (XPoint *)malloc(vertices * sizeof(XPoint));
    if (!point) return;

    for (int i=0; i<vertices; i++)
    {
        point[i].x = (int)x[i];
        point[i].y = current->height - (int)y[i];
    }

    XFillPolygon(display, current->buffer, current->gc, point, vertices, Complex, CoordModeOrigin);

    free(point);
}

void tdDrawFilledRect(float x, float y, int align, float width, float height)
{
    int xpos, ypos;

    if (!current) return;

    if (align & tdAlignCenter)
        xpos = (int)(x - (width/2));
    else if (align & tdAlignRight)
        xpos = (int)(x - width);
    else
        xpos = (int)x;

    if (align & tdAlignMiddle)
        ypos = current->height - (int)y - (int)(height/2);
    else if (align & tdAlignTop)
        ypos = current->height - (int)y;
    else
        ypos = current->height - (int)y - (int)height;

    XFillRectangle(display, current->buffer, current->gc, xpos, ypos, (int)width, (int)height);
}

void tdDrawString(char *string, float x, float y, float rotate, int align)
{
    int xpos = (int)x;
    int ypos = (int)y;
    XFontStruct *finfo;

    if (!current) return;

    finfo = current->curfont->info;

    if (rotate == 0)
    {
        if (align & tdAlignCenter)
            xpos -= (XTextWidth(finfo, string, strlen(string))/2);
        else if (align & tdAlignRight)
            xpos -= XTextWidth(finfo, string, strlen(string));

        if (align & tdAlignMiddle)
            ypos -= (finfo->ascent - finfo->descent)/2;
        else if (align & tdAlignTop)
            ypos -= finfo->ascent;
        else if (!(align & tdAlignBaseline))
            ypos += finfo->descent;

        XDrawString(display, current->buffer, current->gc,
                    xpos, current->height-ypos, string, strlen(string));
    }
    else
    {
        XRotDrawString(display, finfo, current->buffer, current->gc, xpos,
                       current->height-ypos, rotate, 1.0, 1.0, align, string);
    }
}

void tdLineStart(float x, float y)
{
    if (!current) return;

    current->storedpoint.x = (int)x;
    current->storedpoint.y = (int)y;
}

void tdLineAppend(float x, float y)
{
    if (!current) return;

    XDrawLine(display, current->buffer, current->gc, current->storedpoint.x,
              current->height-current->storedpoint.y, (int)x, current->height-(int)y);
    current->storedpoint.x = (int)x;
    current->storedpoint.y = (int)y;
}

void tdDrawImage(tdImage *myimage, float xpos, float ypos, int align, float scalex, float scaley, float rotate)
{
    int i, j, index;
    float startx, endx, starty, endy, dx, dy, cosang, sinang, deltax, deltay;
    unsigned prevcolor=-1;
    TDImage *image = (TDImage *)myimage;

    cosang = cosf(rotate/RTD);
    sinang = sinf(rotate/RTD);

    if (align & tdAlignCenter) deltax = -image->width/2;
    else if (align & tdAlignRight) deltax = -image->width;
    else deltax = 0;

    if (align & tdAlignMiddle) deltay = -image->height/2;
    else if (align & tdAlignTop) deltay = -image->height;
    else deltay = 0;

    XSetLineAttributes(display, current->gc, (unsigned int)(scaley+0.9999), LineSolid, tdButtLineCapStyle, JoinRound);

    for (i=0; i<(int)(image->height); i++)
    {
        for (j=0; j<(int)(image->width); j++)
        {
            index = ((int)(image->width)*i)+j;

            if (image->pixel[index] != prevcolor)
            {
                XSetForeground(display, current->gc, image->xcolor[image->pixel[index]]);
                prevcolor = image->pixel[index];
            }

            dx = (scalex * ((float)j + deltax));
            dy = (scaley * ((float)i + deltay));
            startx = xpos + (dx * cosang) - (dy * sinang);
            starty = ypos + (dx * sinang) + (dy * cosang);
            endx = xpos + ((dx+scalex+0.9999) * cosang) - (dy * sinang);
            endy = ypos + ((dx+scalex+0.9999) * sinang) + (dy * cosang);
            XDrawLine(display, current->buffer, current->gc, (int)startx, (int)(current->height-starty), (int)endx, (int)(current->height-endy));
        }
    }

    // restore the previous graphics state
    XSetLineAttributes(display, current->gc, current->linewidth, LineSolid, current->capstyle, JoinRound);
    XSetForeground(display, current->gc, current->fgcolor);
}

void tdGLSwapBuffers(void)
{
    glXSwapBuffers(display, curGL->win);
}

void tdRenderGraphics(void)
{
    if (!current) return;
    XCopyArea(display, current->buffer, current->win, current->gc, 0, 0, current->width, current->height, 0, 0);
}

// Event processing

void tdMainLoop(void (run_routine)(void), void (terminate_routine)(void))
{
    while (1) run_routine();
}

void tdProcessEvents(void (button_handler)(ButtonEvent),
                     void (keyboard_handler)(KeyboardEvent),
                     void (configure_handler)(ConfigureEvent),
                     void (winclose_handler)(WinCloseEvent))
{
    WindowSpec *win;
    ButtonEvent bevent;
    KeyboardEvent kevent;
    ConfigureEvent cevent;
    WinCloseEvent wevent;
    XEvent report;
    XWindowAttributes attr;
    Window changedwin;
    KeySym keysym;

    while(XPending(display))
    {
        XNextEvent(display, &report);
        switch(report.type)
        {
            case ButtonPress:
            case ButtonRelease:
                if (button_handler)
                {
                    for (win = startwin; win; win = win->next)
                    {
                        if (win->win == report.xbutton.window)
                        {
                            if (report.type == ButtonPress) bevent.state = tdPressed;
                            else bevent.state = tdReleased;

                            bevent.window = report.xbutton.window;
                            bevent.pos.x = (float)report.xbutton.x;
                            bevent.pos.y = (float)(win->height-report.xbutton.y);
                            button_handler(bevent);
                            break;
                        }
                    }
                }
                break;
            case KeyPress:
            case KeyRelease:
                if (keyboard_handler)
                {
                    for (win = startwin; win; win = win->next)
                    {
                        if (win->win == report.xkey.window)
                        {
                            if (report.type == KeyPress) kevent.state = tdPressed;
                            else kevent.state = tdReleased;

                            XLookupString(&report.xkey, &kevent.key, 1, 0x0, 0x0);
                            if (kevent.key == 0)
                            {
                                keysym = XkbKeycodeToKeysym(display, report.xkey.keycode, 0, 0);
                                switch (keysym)
                                {
                                    case XK_Left: kevent.specialkey = tdArrowLeft; break;
                                    case XK_Right: kevent.specialkey = tdArrowRight; break;
                                    case XK_Down: kevent.specialkey = tdArrowDown; break;
                                    case XK_Up: kevent.specialkey = tdArrowUp; break;
                                    case XK_Help: kevent.specialkey = tdHelpKey; break;
                                    case XK_Home: kevent.specialkey = tdHomeKey; break;
                                    case XK_End: kevent.specialkey = tdEndKey; break;
                                    case XK_Page_Up: kevent.specialkey = tdPageUp; break;
                                    case XK_Page_Down: kevent.specialkey = tdPageDown; break;
                                    default: kevent.specialkey = tdUnknownKey; break;
                                }
                            }
                            else kevent.specialkey = 0;

                            kevent.window = report.xkey.window;
                            kevent.pos.x = (float)report.xkey.x;
                            kevent.pos.y = (float)(win->height-report.xkey.y);
                            keyboard_handler(kevent);
                            break;
                        }
                    }
                }
                break;
            case ConfigureNotify:
                changedwin = report.xconfigure.window;

                // only process the most recent reconfig event, so if there's already one for this window in the queue, replace it
                while (XCheckWindowEvent(display, changedwin, StructureNotifyMask, &report)) continue;

                for (win = startwin; win; win = win->next)
                {
                    if (win->win == changedwin)
                    {
                        XGetWindowAttributes(display, win->win, &attr);

                        win->width = attr.width;
                        win->height = attr.height;
                        XFreePixmap(display, win->buffer);
                        win->buffer = XCreatePixmap(display, win->win, win->width, win->height, attr.depth);

                        if (configure_handler)
                        {
                            cevent.window = changedwin;
                            cevent.size.width = attr.width;
                            cevent.size.height = attr.height;
                            configure_handler(cevent);
                        }

                        XSync(display, 0);

                        win->needsRedraw = 1;
                        break;
                    }
                }
                break;
            case Expose:
                if (report.xexpose.count == 0)
                {
                    for (win = startwin; win; win = win->next)
                    {
                        if (win->win == report.xexpose.window)
                        {
                            win->needsRedraw = 1;
                            break;
                        }
                    }
                }
                break;
            case ClientMessage:
                if (winclose_handler)
                {
                    wevent.window = report.xclient.window;
                    winclose_handler(wevent);
                }
                break;
        }
    }
}

// Termination routines

void tdCloseWindow(void)
{
    WindowSpec *win, *active = 0x0;

    if (!current) return;

    XFreePixmap(display, current->buffer);
    XFreeGC(display, current->gc);
    XDestroyWindow(display, current->win);

    // Reset current to a valid window
    if (current == startwin) active = startwin = startwin->next;
    else
    {
        for (win = startwin; win && !active; win = win->next)
        {
            if (win->next == current)
            {
                win->next = current->next;
                active = win;
            }
        }
    }

    free(current);
    current = active;
}

void tdGLDestroyContext(void)
{
    if (curGL)
    {
        glXMakeCurrent(display, None, 0x0);
        glXDestroyContext(display, curGL->context);
        XDestroyWindow(display, curGL->win);
        free(curGL);
        curGL = 0x0;
    }
}

void tdTerminate(void)
{
    WindowSpec *win, *tmp;

    tdGLDestroyContext();

    for (win = startwin; win;)
    {
        XFreePixmap(display, win->buffer);
        XFreeGC(display, win->gc);
        XDestroyWindow(display, win->win);
        tmp = win;
        win = win->next;
        free(tmp);
    }

    for (int i=0; i<fontnum; i++)
    {
        XUnloadFont(display, font[i].info->fid);
        free(font[i].face);
    }

    XCloseDisplay(display);
}
