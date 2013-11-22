#import <Cocoa/Cocoa.h>

@interface tdViewClass : NSView
{
    NSPoint storedPoint;
    NSFont *renderFont;
    NSColor *renderColor;
    NSOpenGLContext *glContext;
    float lineWidth;
    int capStyle;
    int needsRedraw;
    id tdAdapter;
}
- (id)initWithFrame:(NSRect)frameRect adapter:(id)myadapter;
- (void)tdSetColor: (NSColor *)mycolor;
- (void)tdSetFont: (NSFont *)myfont;
- (void)tdSetLineCap: (int)mystyle;
- (void)tdSetLineWidth: (float)mywidth;
- (void)tdSetNeedsRedraw;
- (NSSize)tdGetStringBounds:(NSString *)string;
- (int)tdNeedsRedraw;
- (void)tdClearWindow;
- (void)tdDrawFilledPoly: (NSPointArray)mypoints count: (int)mycount;
- (void)tdDrawFilledRect: (NSRect)myrect;
- (void)tdDrawString: (NSString *)mystring atPoint: (NSPoint)mypoint rotate: (float)rotate align: (int)align;
- (void)tdLineStart: (NSPoint)mypoint;
- (void)tdLineAppend: (NSPoint)mypoint;
- (void)tdDrawImage:(NSImage *)myimage atPoint:(NSPoint)mypoint align:(int)align scaleX:(float)scalex scaleY:(float)scaley rotate:(float)rotate;
- (void)setGlContext:(NSOpenGLContext *)mycontext;
- (NSOpenGLContext *)getGlContext;
@end
