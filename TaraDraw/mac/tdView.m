#import "tdView.h"
#import "tdAdapter.h"

extern int tdAlignLeft;
extern int tdAlignCenter;
extern int tdAlignRight;
extern int tdAlignBottom;
extern int tdAlignMiddle;
extern int tdAlignTop;
extern int tdAlignBaseline;

@implementation tdViewClass

- (id)initWithFrame:(NSRect)frameRect adapter:(id)myadapter
{
	if ((self = [ super initWithFrame:frameRect ]) != nil)
    {
        tdAdapter = myadapter;
        renderColor = [ NSColor blackColor ];
	}
	return self;
}

- (void)tdSetColor:(NSColor *)mycolor
{
    renderColor = mycolor;
    [ renderColor set ];
}

- (void)tdSetFont:(NSFont *)myfont
{
    if (renderFont) [ renderFont release ];
    renderFont = myfont;
    [ renderFont retain ];
}

- (void)tdSetLineCap:(int)mystyle
{
    capStyle = mystyle;
    [ NSBezierPath setDefaultLineCapStyle:(NSLineCapStyle)capStyle ];
}

- (void)tdSetLineWidth:(float)mywidth
{
    lineWidth = mywidth;
    [ NSBezierPath setDefaultLineWidth:lineWidth ];
}

- (void)tdSetNeedsRedraw
{
    needsRedraw = 1;
}

- (NSSize)tdGetStringBounds:(NSString *)string
{
    NSDictionary *mydictionary;
    NSArray *attrs, *keys;

    attrs = [ NSArray arrayWithObjects: renderFont, nil ];
    keys = [ NSArray arrayWithObjects: NSFontAttributeName, nil ];
    mydictionary = [ NSDictionary dictionaryWithObjects:attrs forKeys:keys ];
    NSSize mysize = [ string sizeWithAttributes:mydictionary ];
    mysize.height = [ renderFont ascender ] - [ renderFont descender ];
    return mysize;
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

- (void)tdClearWindow
{
    [[[ self window ] backgroundColor ] set ];
    NSRectFill([ self frame ]);
    if (renderColor != nil) [ renderColor set ];
}

- (void)tdDrawFilledPoly:(NSPointArray)mypoints count:(int)mycount
{
    NSBezierPath *mypath = [ NSBezierPath bezierPath ];
    [ mypath appendBezierPathWithPoints:mypoints count:mycount ];
    [ mypath closePath ]; 
    [ mypath fill ]; 
}

- (void)tdDrawFilledRect:(NSRect)myrect
{
    NSRectFill(myrect);
}

- (void)tdDrawString:(NSString *)mystring atPoint:(NSPoint)mypoint rotate:(float)rotate align:(int)align
{
    NSDictionary *mydictionary;
    NSArray *attrs, *keys;
    NSSize mysize;
    float deltax, deltay;

    attrs = [ NSArray arrayWithObjects: renderColor, renderFont, nil ];
    keys = [ NSArray arrayWithObjects: NSForegroundColorAttributeName, NSFontAttributeName, nil ];
    mydictionary = [ NSDictionary dictionaryWithObjects:attrs forKeys:keys ];
    mysize = [ mystring sizeWithAttributes:mydictionary ];

    if (align & tdAlignCenter)
        deltax = -mysize.width/2;
    else if (align & tdAlignRight)
        deltax = -mysize.width;
    else
        deltax = 0;

    if (align & tdAlignMiddle)
        deltay = ([ renderFont descender ] - [ renderFont ascender ])/2;
    else if (align & tdAlignTop)
        deltay = [ renderFont descender ] - [ renderFont ascender ];
    else if (align & tdAlignBaseline)
        deltay = [ renderFont descender ];
    else
        deltay = 0;

    if (rotate == 0)
    {
        mypoint.x += deltax;
        mypoint.y += deltay;
        [ mystring drawAtPoint:mypoint withAttributes:mydictionary ];
    }
    else
    {
        NSPoint basepoint;
        basepoint.x = deltax;
        basepoint.y = deltay;

        NSAffineTransform *xform = [ NSAffineTransform transform ];
        [ xform translateXBy:mypoint.x yBy:mypoint.y ];
        [ xform rotateByDegrees:rotate ];
        [[ NSGraphicsContext currentContext ] saveGraphicsState ];
        [ xform concat ];
        [ mystring drawAtPoint:basepoint withAttributes:mydictionary ];
        [[ NSGraphicsContext currentContext ] restoreGraphicsState ];
    }
}

- (void)tdLineStart:(NSPoint)mypoint
{
    storedPoint = mypoint;
}

- (void)tdLineAppend:(NSPoint)mypoint
{
    [ NSBezierPath strokeLineFromPoint:storedPoint toPoint:mypoint ];
    storedPoint = mypoint;
}

- (void)tdDrawImage:(NSImage *)myimage atPoint:(NSPoint)mypoint align:(int)align scaleX:(float)scalex scaleY:(float)scaley rotate:(float)rotate
{
    NSSize origsize;
    NSRect drawrect;
    float deltax, deltay;

    origsize = [ myimage size ];
    drawrect.size.width = scalex * origsize.width;
    drawrect.size.height = scaley * origsize.height;

    if (align & tdAlignCenter) deltax = -drawrect.size.width/2;
    else if (align & tdAlignRight) deltax = -drawrect.size.width;
    else deltax = 0;

    if (align & tdAlignMiddle) deltay = -drawrect.size.height/2;
    else if (align & tdAlignTop) deltay = -drawrect.size.height;
    else deltay = 0;

    if (rotate == 0)
    {
        drawrect.origin = mypoint;
        drawrect.origin.x += deltax;
        drawrect.origin.y += deltay;
        [ myimage drawInRect:drawrect fromRect:NSZeroRect operation:NSCompositeCopy fraction:1 ];
    }
    else
    {
        NSPoint zeropoint;
        zeropoint.x = deltax; zeropoint.y = deltay;
        drawrect.origin = zeropoint;
        NSAffineTransform *xform = [ NSAffineTransform transform ];
        [ xform translateXBy:mypoint.x yBy:mypoint.y ];
        [ xform rotateByDegrees:rotate ];
        [[ NSGraphicsContext currentContext ] saveGraphicsState ];
        [ xform concat ];
        [ myimage drawInRect:drawrect fromRect:NSZeroRect operation:NSCompositeCopy fraction:1 ];
        [[ NSGraphicsContext currentContext ] restoreGraphicsState ];
    }
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

- (void)drawRect:(NSRect)rect
{
    needsRedraw = 1;
    [ tdAdapter runLoop:nil ];
}

@end
