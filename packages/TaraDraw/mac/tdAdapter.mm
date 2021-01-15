#import "tdAdapter.hh"
#import "tdWindow.hh"

int tdAlignLeft = 0x00;
int tdAlignCenter = 0x01;
int tdAlignRight = 0x02;
int tdAlignBottom = 0x00;
int tdAlignMiddle = 0x04;
int tdAlignTop = 0x08;

@implementation TaraDrawAdapter

// Initialization routines

- (id)init
{
    if ((self = [ super init ]))
    {
        numwin = 0;
        curwin = 0;
        eventcount = 0;
        RunRoutine = nil;
        TerminateRoutine = nil;

        NSNotificationCenter *center = [ NSNotificationCenter defaultCenter ];
        [ center addObserver:self selector:@selector(applicationDidFinishLaunching:) name:NSApplicationDidFinishLaunchingNotification object:nil ];
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    (void)notification; // variable unused

    id menubar = [[ NSMenu new ] autorelease ];
    [ NSApp setMainMenu:menubar ];
    id appName = [[ NSProcessInfo processInfo ] processName ];

    /*** default application menu ***/
    id appMenu = [ self addMenu:menubar title:nil ];
    [ self addMenuItem:appMenu title:[ @"About " stringByAppendingString:appName ] action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@"" modifier:0 ];
    [ self addMenuSeparator:appMenu ];
    [ self addMenuItem:appMenu title:[ @"Hide " stringByAppendingString:appName ] action:@selector(hide:) keyEquivalent:@"h" modifier:0 ];
#ifdef MAC_OS_X_VERSION_10_12
    [ self addMenuItem:appMenu title:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h" modifier:NSEventModifierFlagCommand|NSEventModifierFlagOption ];
#else
    [ self addMenuItem:appMenu title:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h" modifier:NSCommandKeyMask|NSAlternateKeyMask ];
#endif
    [ self addMenuItem:appMenu title:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@"" modifier:0 ];
    [ self addMenuSeparator:appMenu ];
    [ self addMenuItem:appMenu title:[ @"Quit " stringByAppendingString:appName ] action:@selector(terminate:) keyEquivalent:@"q" modifier:0 ];

    /*** Edit menu ***/
    id editMenu = [ self addMenu:menubar title:@"Edit" ];
    [ self addMenuItem:editMenu title:@"Undo" action:@selector(undo:) keyEquivalent:@"z" modifier:0 ];
    [ self addMenuItem:editMenu title:@"Redo" action:@selector(redo:) keyEquivalent:@"Z" modifier:0 ];
    [ self addMenuSeparator:editMenu ];
    [ self addMenuItem:editMenu title:@"Cut" action:@selector(cut:) keyEquivalent:@"x" modifier:0 ];
    [ self addMenuItem:editMenu title:@"Copy" action:@selector(copy:) keyEquivalent:@"c" modifier:0 ];
    [ self addMenuItem:editMenu title:@"Paste" action:@selector(paste:) keyEquivalent:@"v" modifier:0 ];
    [ self addMenuItem:editMenu title:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a" modifier:0 ];

    /*** Window menu ***/
    id winMenu = [ self addMenu:menubar title:@"Window" ];
    [ self addMenuItem:winMenu title:@"Minimize" action:@selector(miniaturize:) keyEquivalent:@"m" modifier:0 ];
    [ self addMenuItem:winMenu title:@"Zoom" action:@selector(zoom:) keyEquivalent:@"" modifier:0 ];
    [ self addMenuSeparator:winMenu ];
    [ self addMenuItem:winMenu title:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@"" modifier:0 ];
    [ self addMenuSeparator:winMenu ];
    [ self addMenuItem:winMenu title:@"Close" action:@selector(performClose:) keyEquivalent:@"w" modifier:0 ];

    NSNotificationCenter *center = [ NSNotificationCenter defaultCenter ];
    [ center addObserver:self selector:@selector(windowReconfigured:) name:NSWindowDidEnterFullScreenNotification object:nil ];
    [ center addObserver:self selector:@selector(windowReconfigured:) name:NSWindowDidExitFullScreenNotification object:nil ];
    [ center addObserver:self selector:@selector(windowReconfigured:) name:NSWindowDidEndLiveResizeNotification object:nil ];
    [ center addObserver:self selector:@selector(windowReconfigured:) name:NSWindowDidResizeNotification object:nil ];
    [ center addObserver:self selector:@selector(windowClosed:) name:NSWindowWillCloseNotification object:nil ];
    [ center addObserver:self selector:@selector(applicationTerminated:) name:NSApplicationWillTerminateNotification object:nil ];

    myTimer = [[ NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector(runLoop:) userInfo:nil repeats:YES ] retain ];
    [[ NSRunLoop currentRunLoop ] addTimer:myTimer forMode:NSRunLoopCommonModes ];
}

- (id)addMenu:(id)bar title:(NSString *)mytitle
{
    id mymenu;
    id myitem = [[ NSMenuItem new ] autorelease ];

    [ bar addItem:myitem ];
    if (mytitle) mymenu = [[[ NSMenu new ] initWithTitle:mytitle ] autorelease ];
    else mymenu = [[ NSMenu new ] autorelease ];
    [ myitem setSubmenu:mymenu ];

    return mymenu;
}

- (void)addMenuItem:(id)menu title:(NSString *)mytitle action:(SEL)myaction keyEquivalent:(NSString *)mykey modifier:(NSUInteger)mask
{
    id myitem = [[[ NSMenuItem alloc ] initWithTitle:mytitle action:myaction keyEquivalent:mykey ] autorelease ];
    if (mask) [ myitem setKeyEquivalentModifierMask:mask ];
    [ menu addItem:myitem ];
}

- (void)addMenuSeparator:(id)menu
{
    [ menu addItem:[ NSMenuItem separatorItem ]];
}

- (void)setRunHandler:(void (*)(void))run_routine terminateHandler:(void (*)(void))terminate_routine
{
    RunRoutine = run_routine;
    TerminateRoutine = terminate_routine;
}

- (void)runLoop:(NSTimer *)aTimer
{
    (void)aTimer; // variable unused
    if (RunRoutine) RunRoutine();
}

- (tdWindow)openWindow:(NSString *)mytitle withFrame:(NSRect)myrect aligned:(int)align
{
    if (align & tdAlignCenter)
        myrect.origin.x -= myrect.size.width/2;
    else if (align & tdAlignRight)
        myrect.origin.x -= myrect.size.width;

    if (align & tdAlignMiddle)
        myrect.origin.y -= myrect.size.height/2;
    else if (align & tdAlignTop)
        myrect.origin.y -= myrect.size.height;

    winview[numwin] = [[ tdViewClass alloc ] initWithFrame:myrect adapter:self ];
    tdWindowClass *mywin = [[ tdWindowClass alloc ] initWithContentRect:myrect adapter:self ];

    [ mywin setTitle:mytitle ];
    [ mywin setContentView:winview[numwin] ];
    [ mywin makeKeyAndOrderFront:nil ];
    [ mywin setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary ];
    [ self setActiveWindow:numwin ];

    curwin = numwin;
    numwin++;
    return curwin;
}

- (tdWindow)openFullScreen:(NSString *)mytitle
{
    NSRect myrect = [[ NSScreen mainScreen ] visibleFrame ];

    winview[numwin] = [[ tdViewClass alloc ] initWithFrame:myrect adapter:self ];
    tdWindowClass *mywin = [[ tdWindowClass alloc ] initWithContentRect:myrect adapter:self ];

    [ mywin setTitle:mytitle ];
    [ mywin setContentView:winview[numwin] ];
    [ mywin makeKeyAndOrderFront:nil ];
    [ mywin setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary ];
    [ mywin toggleFullScreen:self ];
    [ self setActiveWindow:numwin ];

    curwin = numwin;
    numwin++;
    return curwin;
}

- (NSOpenGLContext *)glCreateContext:(tdWindow)winid
{
    if (!(winview[winid])) return nil;

    NSOpenGLView *myview = [[ NSOpenGLView alloc ] initWithFrame:[ winview[winid] frame ] pixelFormat:[ NSOpenGLView defaultPixelFormat ]];
    NSOpenGLPixelFormatAttribute attrs[] = { NSOpenGLPFADoubleBuffer, NSOpenGLPFADepthSize, 32, 0 };
    NSOpenGLPixelFormat *pixfmt = [[ NSOpenGLPixelFormat alloc ] initWithAttributes:attrs ];
    curGLcontext = [[ NSOpenGLContext alloc ] initWithFormat:pixfmt shareContext:nil ];
    [ curGLcontext makeCurrentContext ];
    [ myview setOpenGLContext:curGLcontext ];
    [ winview[winid] addSubview:myview ];
    [ curGLcontext setView:myview ];
    [ pixfmt release ];

    return curGLcontext;
}

- (void)glReshapeContext:(NSRect)myrect align:(int)align
{
    if (align & tdAlignCenter)
        myrect.origin.x -= (myrect.size.width/2);
    else if (align & tdAlignRight)
        myrect.origin.x -= myrect.size.width;

    if (align & tdAlignMiddle)
        myrect.origin.y -= (myrect.size.height/2);
    else if (align & tdAlignTop)
        myrect.origin.y -= myrect.size.height;

    [[ curGLcontext view ] setFrame:myrect ];
}

// Settings routines

- (void)setActiveWindow:(tdWindow)winid
{
    curwin = winid;
    if (winview[curwin])
    {
        NSView *focusView = [ NSView focusView ];
        if (focusView != winview[curwin])
        {
            if (focusView) [ focusView unlockFocus ];
            [ winview[curwin] lockFocus ];
        }
    }
}

- (void)glSetContext:(NSOpenGLContext *)glContext
{
    curGLcontext = glContext;
    [ curGLcontext makeCurrentContext ];
}

- (void)setNeedsRedraw:(tdWindow)winid
{
    [ winview[winid] tdSetNeedsRedraw ];
}

// Query routines

- (NSPoint)getPointerInWindow
{
    return [[ winview[curwin] window ] mouseLocationOutsideOfEventStream ];
}

- (int)needsRedraw:(tdWindow)winid
{
    return [ winview[winid] tdNeedsRedraw ];
}

// Drawing routines

- (void)toggleFullScreen
{
    [[ winview[curwin] window ] toggleFullScreen:self ];
}

- (void)glSwapBuffers
{
    [ curGLcontext flushBuffer ];
}

// Event processing

- (void)processEventsButtonHandler:(void (*)(ButtonEvent))button_handler
                     keyboardHandler:(void (*)(KeyboardEvent))keyboard_handler
                     configureHandler:(void (*)(ConfigureEvent))configure_handler
                     wincloseHandler:(void (*)(WinCloseEvent))winclose_handler
{
    int i;
    int event_type;

    for (i=0; i<eventcount; i++)
    {
        event_type = EventList[i].type;
        // set EventList[i] to tdNoEvent to prevent recursive event handling
        EventList[i].type = tdNoEvent;

        switch (event_type)
        {
            case tdMouseEvent:
                if (button_handler) button_handler(EventList[i].data.b);
                break;
            case tdKeyboardEvent:
                if (keyboard_handler) keyboard_handler(EventList[i].data.k);
                break;
            case tdConfigureEvent:
                if (configure_handler) configure_handler(EventList[i].data.c);
                break;
            case tdWinCloseEvent:
                if (winclose_handler) winclose_handler(EventList[i].data.w);
                break;
            default:
                break;
        }
    }
    eventcount = 0;
}

// Termination routines

- (void)closeWindow
{
    if (winview[curwin])
    {
        tdWindowClass *mywin = (tdWindowClass *)[ winview[curwin] window ];
        if (mywin)
        {
            NSOpenGLContext *mycontext = [ winview[curwin] getGlContext ];
            if (mycontext)
            {
                if (mycontext == curGLcontext) [ self glDestroyContext ];
                else [ mycontext release ];
            }
            [ winview[curwin] unlockFocus ];
            [ mywin close ];
        }
        winview[curwin] = nil;
    }

    // Reset curwin to a valid window
    for (int i=0; i < MAX_WINDOWS; i++)
    {
        if (winview[i])
        {
            curwin = i;
            break;
        }
    }
}

- (void)glDestroyContext
{
    [ NSOpenGLContext clearCurrentContext ];
    [ curGLcontext release ];
    curGLcontext = nil;
}

- (void)cleanUp
{
    if (myTimer)
    {
        [ myTimer invalidate ];
        [ myTimer release ];
        myTimer = nil;
    }

    NSNotificationCenter *center = [ NSNotificationCenter defaultCenter ];
    [ center removeObserver:self ];

    for (int i=0; i<numwin; i++)
    {
        if (winview[i]) [ winview[i] release ];
    }
}

// Private routines

- (tdWindow)getWindowID:(id)mywindow
{
    int i;
    for (i=0; i<numwin; i++)
    {
        if (mywindow == [ winview[i] window ]) return i;
    }
    return (-1);
}

- (void)mouseEvent:(NSEvent *)theEvent state:(int)state
{
    EventUnion data;
    NSPoint mypoint;
    int i;

    if (eventcount >= EVENT_QUEUE) return;

    for (i=0; i<numwin; i++)
    {
        if ([ theEvent window ] == [ winview[i] window ])
        {
            mypoint = [ theEvent locationInWindow ];
            data.b.pos.x = mypoint.x;
            data.b.pos.y = mypoint.y;
            data.b.state = state;
            data.b.window = i;
            EventList[eventcount].type = tdMouseEvent;
            EventList[eventcount].data = data;
            eventcount++;
            break;
        }
    }
}

- (void)keyEvent:(NSEvent *)theEvent state:(int)state
{
    EventUnion data;
    NSPoint mypoint;
    int i;
    unichar ukey;

    if (eventcount >= EVENT_QUEUE) return;

    for (i=0; i<numwin; i++)
    {
        if ([ theEvent window ] == [ winview[i] window ])
        {
            NSRect mouserect;
            mouserect.origin = [ NSEvent mouseLocation ];
            NSRect myrect = [[ theEvent window ] convertRectFromScreen: mouserect ];
            mypoint = myrect.origin;

            ukey = [[ theEvent characters ] characterAtIndex:0 ];
            if (ukey > 0xff)
            {
                data.k.key = 0;
                switch ([ theEvent keyCode ])
                {
                    case 0x7b: data.k.specialkey = tdArrowLeft; break;
                    case 0x7c: data.k.specialkey = tdArrowRight; break;
                    case 0x7d: data.k.specialkey = tdArrowDown; break;
                    case 0x7e: data.k.specialkey = tdArrowUp; break;
                    case 0x72: data.k.specialkey = tdHelpKey; break;
                    case 0x73: data.k.specialkey = tdHomeKey; break;
                    case 0x77: data.k.specialkey = tdEndKey; break;
                    case 0x74: data.k.specialkey = tdPageUp; break;
                    case 0x79: data.k.specialkey = tdPageDown; break;
                    default:   data.k.specialkey = tdUnknownKey; break;
                }
            }
            else
            {
                data.k.key = ukey;
                data.k.specialkey = 0;
            }

            data.k.pos.x = mypoint.x;
            data.k.pos.y = mypoint.y;
            data.k.state = state;
            data.k.window = i;
            EventList[eventcount].type = tdKeyboardEvent;
            EventList[eventcount].data = data;
            eventcount++;
            break;
        }
    }
}

- (void)windowReconfigured:(NSNotification *)aNotification
{
    EventUnion data;
    int i, j, eventnum;

    if (eventcount >= EVENT_QUEUE) return;

    for (i=0; i<numwin; i++)
    {
        if ([ aNotification object ] == [ winview[i] window ])
        {
            // only process the most recent reconfig event, so if there's already one for this window in the queue, replace it
            eventnum = -1;
            for (j=0; j<eventcount; j++)
            {
                if (EventList[j].type == tdConfigureEvent && EventList[j].data.c.window == i) eventnum = j;
            }
            if (eventnum == -1)
            {
                eventnum = eventcount;
                eventcount++;
            }

            NSRect myrect = [ winview[i] frame ];
// May need to get backing bounds instead so that OpenGL renders correctly:
//     NSRect myrect = [ winview[i] convertRectToBacking:[ winview[i] bounds ]];
            data.c.window = i;
            data.c.size.width = myrect.size.width;
            data.c.size.height = myrect.size.height;
            EventList[eventnum].type = tdConfigureEvent;
            EventList[eventnum].data = data;
            [ winview[i] tdSetNeedsRedraw ];
            break;
        }
    }
}

- (void)windowClosed:(NSNotification *)aNotification
{
    EventUnion data;

    if (eventcount >= EVENT_QUEUE) return;

    for (int i=0; i<numwin; i++)
    {
        if ([ aNotification object ] == [ winview[i] window ])
        {
            winview[i] = nil;
            data.w.window = i;
            EventList[eventcount].type = tdWinCloseEvent;
            EventList[eventcount].data = data;
            eventcount++;
            break;
        }
    }
}

- (void)applicationTerminated:(NSNotification *)aNotification
{
    (void)aNotification; // variable unused
    if (TerminateRoutine) TerminateRoutine();
    [ self cleanUp ];
}

@end
