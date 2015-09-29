#include "tdDefines.hh"
#include "tdAdapter.hh"

static TaraDrawAdapter *tda;

// Initialization routines

int tdInitialize(char *unused)
{
    [ NSApplication sharedApplication ];
    if ( ![ NSApp isRunning ] )
    {
        [ NSApp setActivationPolicy:NSApplicationActivationPolicyRegular ];
        [ NSApp activateIgnoringOtherApps:YES ];
    }

    tda = [[ TaraDrawAdapter alloc ] init ];
    if (tda)
    {
        [ tda registerColor:0 red:1.0 green:1.0 blue:1.0 ]; // white
        [ tda registerColor:1 red:0.0 green:0.0 blue:0.0 ]; // black
        return 0;
    }
    else return (-1);
}

tdWindow tdOpenWindow(const char *title, float xpos, float ypos, float width, float height, int align)
{
    NSRect myrect;
    myrect.origin.x = xpos;
    myrect.origin.y = ypos;
    myrect.size.width = width;
    myrect.size.height = height;
    return [ tda openWindow:[ NSString stringWithCString:title encoding:NSASCIIStringEncoding ] withFrame:myrect aligned:align ];
}

tdWindow tdOpenFullScreen(const char *title)
{
    return [ tda openFullScreen:[ NSString stringWithCString:title encoding:NSASCIIStringEncoding ]];
}

tdImage *tdLoadImage(char *filespec, tdSize *size)
{
    NSSize mysize;
    tdImage *retval = (tdImage *)[ tda loadImage:[ NSString stringWithCString:filespec encoding:NSASCIIStringEncoding ] size:&mysize ];
    if (size)
    {
        size->width = mysize.width;
        size->height = mysize.height;
    }
    return (retval);
}

tdGLContext *tdGLCreateContext(tdWindow win)
{
    return [ tda glCreateContext:win ];
}

void tdGLReshapeContext(float x, float y, int align, float width, float height)
{
    NSRect myrect;
    myrect.origin.x = x;
    myrect.origin.y = y;
    myrect.size.width = width;
    myrect.size.height = height;
    [ tda glReshapeContext:myrect align:align ];
}

int tdRegisterColor(int index, float r, float g, float b)
{
    return [ tda registerColor:index red:r green:g blue:b ];
}

int tdRegisterXFont(char *face, int size, char *spec)
{
    return 0;
}

// Settings routines

void tdSetActiveWindow(tdWindow winid)
{
    [ tda setActiveWindow:winid ];
}

void tdGLSetContext(tdGLContext *glContext)
{
    [ tda glSetContext:(NSOpenGLContext *)glContext ];
}

void tdSetBackgroundColor(int index)
{
    [ tda setBackgroundColor:index ];
}

void tdSetColor(int index)
{
    [ tda setColor:index ];
}

void tdSetColorRGB(float r, float g, float b)
{
    [ tda setColorRGBred:r green:g blue:b ];
}

void tdSetFont(const char *fontface, float fontsize)
{
    [ tda setFont:fontface size:fontsize ];
}

void tdSetLineCap(int capstyle)
{
    [ tda setLineCap:capstyle ];
}

void tdSetLineWidth(float linewidth)
{
    [ tda setLineWidth:linewidth ];
}

void tdSetNeedsRedraw(tdWindow winid)
{
    [ tda setNeedsRedraw:winid ];
}

// Query routines

void tdGetScreenData(tdSize *total, tdRegion *usable)
{
    NSScreen *mainscreen = [ NSScreen mainScreen ];
    if (total)
    {
        NSRect totalscreen = [ mainscreen frame ];
        total->width = totalscreen.size.width;
        total->height = totalscreen.size.height;
    }
    if (usable)
    {
        NSRect usablescreen = [ mainscreen visibleFrame ];
        usable->left = usablescreen.origin.x;
        usable->width = usablescreen.size.width;
        usable->right = usable->left + usable->width;
        usable->center = (usable->left + usable->right)/2;
        usable->bottom = usablescreen.origin.y;
        usable->height = usablescreen.size.height;
        usable->top = usable->bottom + usable->height;
        usable->middle = (usable->bottom + usable->top)/2;
    }
}

tdColorRGB tdGetColor(int index)
{
    tdColorRGB mycolor;
    [ tda getColor:index red:&(mycolor.red) green:&(mycolor.green) blue:&(mycolor.blue) ];
    return mycolor;
}

tdSize tdGetStringBounds(char *string)
{
    tdSize retsize;
    NSSize mysize = [ tda getStringBounds:[ NSString stringWithCString:string encoding:NSASCIIStringEncoding ]];
    retsize.width = mysize.width;
    retsize.height = mysize.height;
    return retsize;
}

void tdGetPointer(tdPosition *screenpos, tdPosition *windowpos)
{
    if (screenpos)
    {
        NSPoint mypoint = [ NSEvent mouseLocation ];
        screenpos->x = mypoint.x;
        screenpos->y = mypoint.y;
    }
    if (windowpos)
    {
        NSPoint mypoint = [ tda getPointerInWindow ];
        windowpos->x = mypoint.x;
        windowpos->y = mypoint.y;
    }
}

int tdNeedsRedraw(tdWindow winid)
{
    return [ tda needsRedraw:winid ];
}

// Drawing routines

void tdToggleFullScreen(void)
{
    [ tda toggleFullScreen ];
}

void tdClearWindow(void)
{
    [ tda clearWindow ];
}

void tdDrawFilledPoly(float x[], float y[], int count)
{
    NSPointArray myarray = (NSPointArray)malloc(count * sizeof(NSPoint));
    int i;
    for (i=0; i<count; i++)
    {
        myarray[i].x = x[i];
        myarray[i].y = y[i];
    }
    [ tda drawFilledPoly:myarray vertices:count ];
    free(myarray);
}

void tdDrawFilledRect(float x, float y, int align, float width, float height)
{
    NSRect myrect;
    myrect.origin.x = x;
    myrect.origin.y = y;
    myrect.size.width = width;
    myrect.size.height = height;
    [ tda drawFilledRect:myrect align:align ];
}

void tdDrawString(char *mystring, float x, float y, float rotate, int align)
{
    NSPoint mypoint;
    mypoint.x = x;
    mypoint.y = y;
    [ tda drawString:[ NSString stringWithCString:mystring encoding:NSASCIIStringEncoding ] atPoint:mypoint rotated:rotate aligned:align ];
}

void tdLineStart(float x, float y)
{
    NSPoint point;
    point.x = x;
    point.y = y;
    [ tda lineStart:point ];
}

void tdLineAppend(float x, float y)
{
    NSPoint point;
    point.x = x;
    point.y = y;
    [ tda lineAppend:point ];
}

void tdDrawImage(tdImage *myimage, float xpos, float ypos, int align, float scalex, float scaley, float rotate)
{
    NSPoint point;
    point.x = xpos;
    point.y = ypos;
    [ tda drawImage:(NSImage *)myimage atPoint:point align:align scaleX:scalex scaleY:scaley rotate:rotate ];
}

void tdGLSwapBuffers(void)
{
    [ tda glSwapBuffers ];
}

void tdRenderGraphics(void)
{
    [ tda renderGraphics ];
}

// Event processing

void tdMainLoop(void (run_routine)(void), void (terminate_routine)(void))
{
    [ tda setRunHandler:run_routine terminateHandler:terminate_routine ];
    if ( ![ NSApp isRunning ] ) [ NSApp run ];
}

void tdProcessEvents(void (button_handler)(ButtonEvent),
                     void (keyboard_handler)(KeyboardEvent),
                     void (configure_handler)(ConfigureEvent),
                     void (winclose_handler)(WinCloseEvent))
{
    [ tda processEventsButtonHandler:button_handler
                       keyboardHandler:keyboard_handler
                       configureHandler:configure_handler
                       wincloseHandler:winclose_handler ];
}

// Termination routines

void tdCloseWindow(void)
{
    [ tda closeWindow ];
}

void tdGLDestroyContext(void)
{
    [ tda glDestroyContext ];
}

void tdTerminate(void)
{
    [ tda cleanUp ];
}
