#import "tdAdapter.hh"
#import "tdWindow.hh"

int tdButtLineCapStyle = NSButtLineCapStyle;
int tdRoundLineCapStyle = NSRoundLineCapStyle;
int tdSquareLineCapStyle = NSSquareLineCapStyle;
int tdAlignLeft = 0x00;
int tdAlignCenter = 0x01;
int tdAlignRight = 0x02;
int tdAlignBottom = 0x00;
int tdAlignMiddle = 0x04;
int tdAlignTop = 0x08;
int tdAlignBaseline = 0x10;

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
        [ center addObserver:self selector:@selector(windowReconfigured:) name:NSWindowDidEnterFullScreenNotification object:nil ];
        [ center addObserver:self selector:@selector(windowReconfigured:) name:NSWindowDidExitFullScreenNotification object:nil ];
        [ center addObserver:self selector:@selector(windowReconfigured:) name:NSWindowDidEndLiveResizeNotification object:nil ];
        [ center addObserver:self selector:@selector(windowReconfigured:) name:NSWindowDidResizeNotification object:nil ];
        [ center addObserver:self selector:@selector(windowClosed:) name:NSWindowWillCloseNotification object:nil ];
        [ center addObserver:self selector:@selector(applicationTerminated:) name:NSApplicationWillTerminateNotification object:nil ];

        myTimer = [[ NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector(runLoop:) userInfo:nil repeats:YES ] retain ];
        [[ NSRunLoop currentRunLoop ] addTimer:myTimer forMode:NSRunLoopCommonModes ];
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    id menubar = [[ NSMenu new ] autorelease ];
    [ NSApp setMainMenu:menubar ];
    id appName = [[ NSProcessInfo processInfo ] processName ];

    /*** default application menu ***/
    id appMenu = [ self addMenu:menubar title:nil ];
    [ self addMenuItem:appMenu title:[ @"About " stringByAppendingString:appName ] action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@"" modifier:0 ];
    [ self addMenuSeparator:appMenu ];
    [ self addMenuItem:appMenu title:[ @"Hide " stringByAppendingString:appName ] action:@selector(hide:) keyEquivalent:@"h" modifier:0 ];
    [ self addMenuItem:appMenu title:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h" modifier:NSCommandKeyMask|NSAlternateKeyMask ];
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
}

- (id)addMenu:(id)bar title:(NSString *)mytitle
{
    id mymenu;
    id myitem = [[ NSMenuItem new ] autorelease ];

    [ bar addItem:myitem ];
    if (mytitle)
        mymenu = [[[ NSMenu new ] initWithTitle:mytitle ] autorelease ];
    else
        mymenu = [[ NSMenu new ] autorelease ];
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
    if (RunRoutine) RunRoutine();
}

- (tdWindow)openWindow:(NSString *)mytitle withFrame:(NSRect)myrect aligned:(int)align
{
    tdWindowClass *mywin;

    curwin = numwin;

    if (align & tdAlignCenter)
        myrect.origin.x -= myrect.size.width/2;
    else if (align & tdAlignRight)
        myrect.origin.x -= myrect.size.width;

    if (align & tdAlignMiddle)
        myrect.origin.y -= myrect.size.height/2;
    else if (align & tdAlignTop)
        myrect.origin.y -= myrect.size.height;

    winview[curwin] = [[ tdViewClass alloc ] initWithFrame:myrect adapter:self ];
    mywin = [[ tdWindowClass alloc ] initWithContentRect:myrect adapter:self ];
    [ mywin setTitle:mytitle ];
    [ mywin setContentView:winview[curwin] ];
    [ mywin makeKeyAndOrderFront:nil ];
    [ mywin setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary ];
    [ self setFont:"Helvetica" size:12 ];
    [ self setActiveWindow:curwin ];

    numwin++;
    return curwin;
}

- (tdWindow)openFullScreen:(NSString *)mytitle
{
    tdWindowClass *mywin;
    NSRect myrect = [[ NSScreen mainScreen ] visibleFrame ];

    curwin = numwin;

    winview[curwin] = [[ tdViewClass alloc ] initWithFrame:myrect adapter:self ];
    mywin = [[ tdWindowClass alloc ] initWithContentRect:myrect adapter:self ];
    [ mywin setTitle:mytitle ];
    [ mywin setContentView:winview[curwin] ];
    [ mywin makeKeyAndOrderFront:nil ];
    [ mywin setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary ];
    [ mywin toggleFullScreen:self ];
    [ self setFont:"Helvetica" size:12 ];
    [ self setActiveWindow:curwin ];

    numwin++;
    return curwin;
}

- (NSImage *)loadImage:(NSString *)myfile size:(NSSize *)mysize
{
    NSImage *myimage;
    if ([ myfile characterAtIndex:0 ] == '/')
        myimage = [[ NSImage alloc ] initWithContentsOfFile:myfile ];
    else
    {
        NSString *resourcepath = [[ NSBundle mainBundle ] resourcePath ];
        NSString *fullpath = [ NSString stringWithFormat:@"%@/%@", resourcepath, myfile ];
        myimage = [[ NSImage alloc ] initWithContentsOfFile:fullpath ];
    }
    [ myimage setScalesWhenResized:YES ];
    *mysize = [ myimage size ];
    return myimage;
}

- (NSOpenGLContext *)glCreateContext:(tdWindow)winid
{
    if ( winview[winid] == nil) return nil;

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

- (int)registerColor:(int)index red:(float)r green:(float)g blue:(float)b
{
    if (index < 0 || index >= MAX_COLORS)
    {
        printf("tdRegisterColor: Color index %d is out of bounds\n", index);
        return (-1);
    }

    ColorMap[index] = [[ NSColor colorWithCalibratedRed:r green:g blue:b alpha:1.0 ] retain ];
    return 0;
}

// Settings routines

- (void)setActiveWindow:(tdWindow)winid
{
    curwin = winid;
    if (winview[curwin] != nil)
    {
        NSView *focusView = [ NSView focusView ];
        if (focusView != winview[curwin])
        {
            if (focusView != nil) [ focusView unlockFocus ];
            [ winview[curwin] lockFocus ];
        }
    }
}

- (void)glSetContext:(NSOpenGLContext *)glContext
{
    curGLcontext = glContext;
    [ curGLcontext makeCurrentContext ];
}

- (void)setBackgroundColor:(int)index
{
    [[ winview[curwin] window ] setBackgroundColor:ColorMap[index] ];
}

- (void)setColor:(int)index
{
    [ winview[curwin] tdSetColor:ColorMap[index] ];
}

- (void)setColorRGBred:(float)r green:(float)g blue:(float)b
{
    NSColor *mycolor = [ NSColor colorWithCalibratedRed:r green:g blue:b alpha:1.0 ];
    [ winview[curwin] tdSetColor:mycolor ];
}

- (void)setFont:(const char *)fontface size:(float)fontsize
{
    NSFont *myfont = [ NSFont fontWithName:[ NSString stringWithCString:fontface encoding:NSASCIIStringEncoding ] size:fontsize ];
    if (myfont) [ winview[curwin] tdSetFont:myfont ];
}

- (void)setLineCap:(int)capstyle
{
    [ winview[curwin] tdSetLineCap: capstyle ];
}

- (void)setLineWidth:(float)linewidth
{
    [ winview[curwin] tdSetLineWidth: linewidth ];
}

- (void)setNeedsRedraw:(tdWindow)winid
{
    [ winview[winid] tdSetNeedsRedraw ];
}

// Query routines

- (int)getColor:(int)index red:(float *)r green:(float *)g blue:(float *)b
{
    if (index < 0 || index >= MAX_COLORS)
    {
        *r = 0;
        *g = 0;
        *b = 0;
        printf("tdGetColor: Color index %d is out of bounds\n", index);
        return (-1);
    }

    *r = [ ColorMap[index] redComponent ];
    *g = [ ColorMap[index] greenComponent ];
    *b = [ ColorMap[index] blueComponent ];
    return 0;
}

- (NSSize)getStringBounds:(NSString *)string
{
    return [ winview[curwin] tdGetStringBounds:string ];
}

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

- (void)clearWindow
{
    [ winview[curwin] tdClearWindow ];
}

- (void)drawFilledPoly:(NSPointArray)myarray vertices:(int)count
{
    [ winview[curwin] tdDrawFilledPoly:myarray count:count ];
}

- (void)drawFilledRect:(NSRect)myrect align:(int)align
{
    if (align & tdAlignCenter)
        myrect.origin.x -= (myrect.size.width/2);
    else if (align & tdAlignRight)
        myrect.origin.x -= myrect.size.width;

    if (align & tdAlignMiddle)
        myrect.origin.y -= (myrect.size.height/2);
    else if (align & tdAlignTop)
        myrect.origin.y -= myrect.size.height;

    [ winview[curwin] tdDrawFilledRect:myrect ];
}

- (void)drawString:(NSString *)renderString atPoint:(NSPoint)mypoint rotated:(float)rotate aligned:(int)align
{
    [ winview[curwin] tdDrawString:renderString atPoint:mypoint rotate:rotate align:align ];
}

- (void)lineStart:(NSPoint)mypoint
{
    [ winview[curwin] tdLineStart:mypoint ];
}

- (void)lineAppend:(NSPoint)mypoint
{
    [ winview[curwin] tdLineAppend:mypoint ];
}

- (void)drawImage:(NSImage *)myimage atPoint:(NSPoint)mypoint align:(int)align scaleX:(float)scalex scaleY:(float)scaley rotate:(float)rotate
{
    [ winview[curwin] tdDrawImage:(NSImage *)myimage atPoint:mypoint align:align scaleX:scalex scaleY:scaley rotate:rotate ];
}

- (void)glSwapBuffers
{
    [ curGLcontext flushBuffer ];
}

- (void)renderGraphics
{
    [[ winview[curwin] window ] flushWindow ];
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
                if (button_handler != NULL) button_handler(EventList[i].data.b);
                break;
            case tdKeyboardEvent:
                if (keyboard_handler != NULL) keyboard_handler(EventList[i].data.k);
                break;
            case tdConfigureEvent:
                if (configure_handler != NULL) configure_handler(EventList[i].data.c);
                break;
            case tdWinCloseEvent:
                if (winclose_handler != NULL) winclose_handler(EventList[i].data.w);
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
    NSOpenGLContext *mycontext;
    int i;

    if (winview[curwin] != nil)
    {
        tdWindowClass *mywin = (tdWindowClass *)[ winview[curwin] window ];
        if (mywin != nil)
        {
            mycontext = [ winview[curwin] getGlContext ];
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
    for (i=0; i < MAX_WINDOWS; i++)
    {
        if (winview[i] != nil)
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

- (void)terminate
{
    int i;
    
    for (i=0; i<MAX_COLORS; i++)
    {
        if (ColorMap[i] != nil) [ ColorMap[i] release ];
    }

    for (i=0; i<numwin; i++)
    {
        if (winview[i] != nil) [ winview[i] release ];
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
            if (NSPointInRect(mypoint, [ winview[i] frame ]))
            {
                data.b.pos.x = mypoint.x;
                data.b.pos.y = mypoint.y;
                data.b.state = state;
                data.b.window = i;
                EventList[eventcount].type = tdMouseEvent;
                EventList[eventcount].data = data;
                eventcount++;
            }
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
            mypoint = [[ theEvent window ] convertScreenToBase: [ NSEvent mouseLocation ]];
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
                    default: data.k.specialkey = tdUnknownKey; break;
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
    int i;

    if (eventcount >= EVENT_QUEUE) return;

    for (i=0; i<numwin; i++)
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
    if (myTimer != nil)
    {
        [myTimer invalidate];
        [myTimer release];
        myTimer = nil;
    }
    if (TerminateRoutine) TerminateRoutine();
}

@end
