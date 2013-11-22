#import <Cocoa/Cocoa.h>
#import "tdView.h"
#import "tdDefines.h"

#define MAX_COLORS 256
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
    NSColor *ColorMap[MAX_COLORS];
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
- (NSImage *)loadImage:(NSString *)myfile size:(NSSize *)mysize;
- (NSOpenGLContext *)glCreateContext:(tdWindow)winid;
- (void)glReshapeContext:(NSRect)myrect align:(int)align;
- (int)registerColor:(int)index red:(float)r green:(float)g blue:(float)b;
// Settings routines
- (void)setActiveWindow:(tdWindow)winid;
- (void)glSetContext:(NSOpenGLContext *)glContext;
- (void)setBackgroundColor:(int)index;
- (void)setColor:(int)index;
- (void)setColorRGBred:(float)r green:(float)g blue:(float)b;
- (void)setFont:(char *)fontface size:(float)fontsize;
- (void)setLineCap:(int)capstyle;
- (void)setLineWidth:(float)linewidth;
- (void)setNeedsRedraw:(tdWindow)winid;
// Query routines
- (int)getColor:(int)index red:(float *)r green:(float *)g blue:(float *)b;
- (NSSize)getStringBounds:(NSString *)string;
- (NSPoint)getPointerInWindow;
- (int)needsRedraw:(tdWindow)winid;
// Drawing routines
- (void)toggleFullScreen;
- (void)clearWindow;
- (void)drawFilledPoly:(NSPointArray)myarray vertices:(int)count;
- (void)drawFilledRect:(NSRect)myrect align:(int)align;
- (void)drawString:(NSString *)renderString atPoint:(NSPoint)mypoint rotated:(float)rotate aligned:(int)align;
- (void)lineStart:(NSPoint)mypoint;
- (void)lineAppend:(NSPoint)mypoint;
- (void)drawImage:(NSImage *)myimage atPoint:(NSPoint)mypoint align:(int)align scaleX:(float)scalex scaleY:(float)scaley rotate:(float)rotate;
- (void)glSwapBuffers;
- (void)renderGraphics;
// Event processing
- (void)processEventsButtonHandler:(void (*)(ButtonEvent))button_handler
                     keyboardHandler:(void (*)(KeyboardEvent))keyboard_handler
                     configureHandler:(void (*)(ConfigureEvent))configure_handler
                     wincloseHandler:(void (*)(WinCloseEvent))winclose_handler;
// Termination routines
- (void)closeWindow;
- (void)glDestroyContext;
- (void)terminate;
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
