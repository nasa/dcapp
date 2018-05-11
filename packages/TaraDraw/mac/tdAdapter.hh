#import <Cocoa/Cocoa.h>
#import "tdView.hh"
#import "tdDefines.hh"

#define MAX_WINDOWS 128
#define EVENT_QUEUE 32

enum { tdNoEvent, tdConfigureEvent, tdWinCloseEvent, tdMouseEvent, tdKeyboardEvent };

typedef union
{
    ButtonEvent b;
    KeyboardEvent k;
    ConfigureEvent c;
    WinCloseEvent w;
} EventUnion;

typedef struct
{
    int type;
    EventUnion data;
} EventStruct;

@interface TaraDrawAdapter : NSObject
{
    NSOpenGLContext *curGLcontext;
    tdViewClass *curGLview;
    tdViewClass *winview[MAX_WINDOWS];
    int numwin;
    int curwin;
    EventStruct EventList[EVENT_QUEUE];
    int eventcount;
    NSTimer *myTimer;
    void (*RunRoutine)(void);
    void (*TerminateRoutine)(void);
}
// Initialization routines
- (id)init;
- (tdWindow)openWindow:(NSString *)mytitle withFrame:(NSRect)myrect aligned:(int)align;
- (tdWindow)openFullScreen:(NSString *)mytitle;
- (NSOpenGLContext *)glCreateContext:(tdWindow)winid;
- (void)glReshapeContext:(NSRect)myrect align:(int)align;
// Settings routines
- (void)setActiveWindow:(tdWindow)winid;
- (void)glSetContext:(NSOpenGLContext *)glContext;
- (void)setNeedsRedraw:(tdWindow)winid;
// Query routines
- (NSPoint)getPointerInWindow;
- (int)needsRedraw:(tdWindow)winid;
// Drawing routines
- (void)toggleFullScreen;
- (void)glSwapBuffers;
// Event processing
- (void)processEventsButtonHandler:(void (*)(ButtonEvent))button_handler
                     keyboardHandler:(void (*)(KeyboardEvent))keyboard_handler
                     configureHandler:(void (*)(ConfigureEvent))configure_handler
                     wincloseHandler:(void (*)(WinCloseEvent))winclose_handler;
// Termination routines
- (void)closeWindow;
- (void)glDestroyContext;
- (void)cleanUp;
// Public routines
- (void)setRunHandler:(void (*)(void))run_routine terminateHandler:(void (*)(void))terminate_routine;
- (void)runLoop:(NSTimer *)aTimer;
// Private routines
- (tdWindow)getWindowID:(id)mywindow;
- (void)mouseEvent:(NSEvent *)theEvent state:(int)state;
- (void)keyEvent:(NSEvent *)theEvent state:(int)state;
- (void)windowReconfigured:(NSNotification *)aNotification;
- (void)windowClosed:(NSNotification *)aNotification;
- (void)applicationTerminated:(NSNotification *)aNotification;
@end
