#import "tdView.hh"
#import "tdAdapter.hh"

@implementation tdViewClass

- (id)initWithFrame:(NSRect)frameRect adapter:(id)myadapter
{
    if ((self = [ super initWithFrame:frameRect ]) != nil) tdAdapter = myadapter;
    return self;
}

- (void)tdSetNeedsRedraw
{
    needsRedraw = 1;
}

- (int)tdNeedsRedraw
{
    if (needsRedraw)
    {
        needsRedraw = 0;
        return 1;
    }
    else return 0;
}

- (void)setGlContext:(NSOpenGLContext *)mycontext
{
    glContext = mycontext;
}

- (NSOpenGLContext *)getGlContext
{
    return glContext;
}

- (void)mouseDown:(NSEvent *)theEvent
{
    [ tdAdapter mouseEvent:theEvent state:tdPressed ];
}

- (void)mouseUp:(NSEvent *)theEvent
{
    [ tdAdapter mouseEvent:theEvent state:tdReleased ];
}

@end
