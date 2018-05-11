#import <Cocoa/Cocoa.h>

@interface tdViewClass : NSView
{
    NSOpenGLContext *glContext;
    int needsRedraw;
    id tdAdapter;
}
- (id)initWithFrame:(NSRect)frameRect adapter:(id)myadapter;
- (void)tdSetNeedsRedraw;
- (int)tdNeedsRedraw;
- (void)setGlContext:(NSOpenGLContext *)mycontext;
- (NSOpenGLContext *)getGlContext;
@end
