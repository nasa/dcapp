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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "tdDefines.hh"

typedef struct
{
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long inputMode;
    unsigned long status;
} WmHints;

typedef struct windowspec
{
    int height;
    int width;
    Window win;
    GC gc;
    Pixmap buffer;
    int needsRedraw;
    struct windowspec *next;
} WindowSpec;

typedef struct
{
    GLXContext context;
    Window win;
    WindowSpec *parent;
} GLSpec;

void tdSetActiveWindow(tdWindow);
void tdCloseWindow(void);

int tdAlignLeft = 0x00;
int tdAlignCenter = 0x01;
int tdAlignRight = 0x02;
int tdAlignBottom = 0x00;
int tdAlignMiddle = 0x04;
int tdAlignTop = 0x08;

static WindowSpec *startwin = 0x0;
static WindowSpec *current = 0x0;
static GLSpec *curGL = 0x0;
static Display *display;
static int screen_num;
static Atom wm_delete_window, wm_hints;

// Initialization routines

int tdInitialize(char *xdisplay)
{
    display = XOpenDisplay(xdisplay);
    if (!display)
    {
        printf("tdInitialize: Can't connect to X server\n");
        return (-1);
    }

    screen_num = DefaultScreen(display);
    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    wm_hints = XInternAtom(display, "_MOTIF_WM_HINTS", False);

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
        (int)width, (int)height, 0, BlackPixel(display, screen_num), WhitePixel(display, screen_num));

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

    // set default linestyle
//    XSetLineAttributes(display, newwin->gc, 1, LineSolid, CapButt, JoinRound);

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

void tdGLReshapeContext(float x, float y, int /* align */, float width, float height)
{
    XMoveResizeWindow(display, curGL->win, (int)x, curGL->parent->height-(int)(height+y), (int)width, (int)height);
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

void tdGLSwapBuffers(void)
{
    glXSwapBuffers(display, curGL->win);
}

// Event processing

void tdMainLoop(void (run_routine)(void), void (*)(void))
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
                                    case XK_Left:      kevent.specialkey = tdArrowLeft;  break;
                                    case XK_Right:     kevent.specialkey = tdArrowRight; break;
                                    case XK_Down:      kevent.specialkey = tdArrowDown;  break;
                                    case XK_Up:        kevent.specialkey = tdArrowUp;    break;
                                    case XK_Help:      kevent.specialkey = tdHelpKey;    break;
                                    case XK_Home:      kevent.specialkey = tdHomeKey;    break;
                                    case XK_End:       kevent.specialkey = tdEndKey;     break;
                                    case XK_Page_Up:   kevent.specialkey = tdPageUp;     break;
                                    case XK_Page_Down: kevent.specialkey = tdPageDown;   break;
                                    default:           kevent.specialkey = tdUnknownKey; break;
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

    XCloseDisplay(display);
}
