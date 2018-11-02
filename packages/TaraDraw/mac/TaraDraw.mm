#import "tdDefines.hh"
#import "tdAdapter.hh"

static TaraDrawAdapter *tda;

// Initialization routines

int tdInitialize(char * /* unused */)
{
    [ NSApplication sharedApplication ];
    if ( ![ NSApp isRunning ] )
    {
        [ NSApp setActivationPolicy:NSApplicationActivationPolicyRegular ];
        [ NSApp activateIgnoringOtherApps:YES ];
    }

    tda = [[ TaraDrawAdapter alloc ] init ];
    if (tda) return 0;
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

tdGLContext *tdGLCreateContext(tdWindow winid)
{
    return [ tda glCreateContext:winid ];
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

// Settings routines

void tdSetActiveWindow(tdWindow winid)
{
    [ tda setActiveWindow:winid ];
}

void tdGLSetContext(tdGLContext *glContext)
{
    [ tda glSetContext:(NSOpenGLContext *)glContext ];
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

void tdGLSwapBuffers(void)
{
    [ tda glSwapBuffers ];
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
