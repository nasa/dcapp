#import "tdDefines.hh"

#import "GLKit/GLKit.h" // fix for bug in compiler generated file DCApp_IOS-Swift.h

#import "DCApp_IOS-Swift.h"
#include "imgload.hh"
#include "UIImageResizing.hh"

#include <iostream>
#include <vector>

@class AppDelegate;
@class ModelController;

void InitializeData( int argc, char **argv );
char *getFullPath(const char *fname);
void mainloop(void);
void Draw();

static void (*keyClick)(KeyboardEvent);

static void (*RunRoutine)(void)        = nullptr;
static void (*TerminateRoutine)(void)  = nullptr;

static std::vector< ButtonEvent > touchEvents;

extern "C" {

void Bridged_Initialize( char *fileNameA )
{
    char **args = new char *[2];
    
    args[0] = strdup( "DCApp_Mobile" );
    args[1] = strdup( getFullPath( fileNameA ) );
    
    InitializeData( 2, args );
    
    free( args[0] );
    free( args[1] );
    
    free( args );
    
    
    mainloop();
}
    
void Bridged_Input( float xA, float yA, int stateA )
{
    ButtonEvent eventL;
    
    eventL.pos.x    = xA;
    eventL.pos.y    = yA;
    eventL.state    = stateA;
    
    touchEvents.push_back( eventL );
}

void Bridged_TimerUpdate()
{
    
    if( RunRoutine )
        RunRoutine();
}

void Bridged_Draw()
{
    Draw();
}
    
}

ModelController *pModelController = nil;


// Initialization routines

int tdInitialize(char *unused)
{
    return 0;
}

tdWindow tdOpenWindow(const char *title, float xpos, float ypos, float width, float height, int align)
{
    CGRect myrect;
    myrect.origin.x = xpos;
    myrect.origin.y = ypos;
    myrect.size.width = width;
    myrect.size.height = height;
    
    int windowIdL = 0;
    @autoreleasepool {
        AppDelegate *pAppL = (AppDelegate *)([UIApplication sharedApplication].delegate);
        
        pModelController = [pAppL getModelController];
    }
    
    windowIdL = [pModelController addDisplayPageWithWidthA:width heightA:height ];
    
    return windowIdL;
}

tdWindow tdOpenFullScreen(const char *title)
{
    return -1;
}


int createTextureFromImage(ImageStruct *image);

static int computeNearestPowerOfTwo(int val)
{
    if (val & (val - 1))
    {
        float p2 = logf((float)val) / 0.69314718; /* logf(2.0f) = 0.69314718 */
        float rounded_p2 = floorf(p2 + 0.5);
        val = (int)(powf(2.0f, rounded_p2));
    }
    return val;
}

int tdLoadImageAsTexture( const char *fileNameA )
{
    NSString *stringL = [NSString stringWithCString: fileNameA encoding:NSASCIIStringEncoding ];
    
    UIImage *imageL = [[UIImage alloc] initWithContentsOfFile: stringL];
    
    ImageStruct imageStructL;
    
    if( LoadImageFile( fileNameA, &imageStructL ) == false )
        return -1;
    
    int maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    int newW = computeNearestPowerOfTwo( imageStructL.width );
    int newH = computeNearestPowerOfTwo( imageStructL.height );
    
    newW = std::fmin( newW, maxTextureSize );
    newH = std::fmin( newH, maxTextureSize );
    
    std::vector< PixelSpec > pSpecL = { PixelUnknown, PixelLuminance, PixelLuminanceAlpha, PixelRGB, PixelRGBA };
    
    ImageStruct iStructL;
    
#if false
    if (newW != imageStructL.width || newH != imageStructL.height )
    {
  //      imageL = [imageL scaleToSize:CGSizeMake( newW, newH )];
        GLubyte* textureData = (GLubyte *)malloc(imageStructL.width * imageStructL.height * 4); // if 4 components per pixel (RGBA)
        
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        NSUInteger bytesPerPixel = 4;
        NSUInteger bytesPerRow = bytesPerPixel * imageStructL.width;
        NSUInteger bitsPerComponent = 8;
        CGContextRef context = CGBitmapContextCreate(textureData, imageStructL.width, imageStructL.height,
                                                     bitsPerComponent, bytesPerRow, colorSpace,
                                                     kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
        
        CGColorSpaceRelease(colorSpace);
        
        CGContextDrawImage(context, CGRectMake(0, 0, imageStructL.width, imageStructL.height), imageRef);
        CGContextRelease(context);
    }
    
    CGImageRef cgImageRefL = imageL.CGImage;
    if( cgImageRefL != nullptr )
    {
        size_t bitsPerComponentL    = CGImageGetBitsPerComponent( cgImageRefL );
        size_t bitsPerPixel         = CGImageGetBitsPerPixel( cgImageRefL );
        
        std::cout << "bytes per pixel = " << bitsPerPixel / bitsPerComponentL << std::endl;
        
    }
    GLKTextureLoader *textureLoader = [[GLKTextureLoader alloc] init];
    
    [textureLoader textureWithContentsOfFile: stringL options: nil queue: nil completionHandler: ^(GLKTextureInfo * _Nullable textureInfo, NSError * _Nullable outError ) { std::cout << "in here" << std::endl; } ];
    
    
    
    CGBitmapInfo info = CGImageGetBitmapInfo(cgImageRefL);
    if (info & kCGBitmapByteOrder32Little)
    {
    }
    else
    {
        
    }
#endif
    
    CGImageRef image = [imageL CGImage];
    
    CFDataRef data = CGDataProviderCopyData(CGImageGetDataProvider(image));
    iStructL.data =  (unsigned char *)CFDataGetBytePtr(data);
    iStructL.width  = imageL.size.width;
    iStructL.height = imageL.size.height;
    
    iStructL.pixelspec   = PixelRGBA; // guess?
    
    return createTextureFromImage( &iStructL );
}

tdImage *tdLoadImage(const char *filespec, tdSize *size)
{
    return nullptr;
}

tdGLContext *tdGLCreateContext(tdWindow winid)
{
    return nullptr;
}

void tdGLReshapeContext(float x, float y, int align, float width, float height)
{
    CGRect myrect;
    myrect.origin.x = x;
    myrect.origin.y = y;
    myrect.size.width = width;
    myrect.size.height = height;
 //   [ tda glReshapeContext:myrect align:align ];
}

int tdRegisterColor(int index, float r, float g, float b)
{
    return 0;
}

int tdRegisterXFont(const char *face, int size, char *spec)
{
    return 0;
}

// Settings routines

void tdSetActiveWindow(tdWindow winid)
{
//    [ tda setActiveWindow:winid ];
}

void tdGLSetContext(tdGLContext *glContext)
{
   // [ tda glSetContext:(__bridge EAGLContext *)glContext ];
}

void tdSetBackgroundColor(int index)
{
//    [ tda setBackgroundColor:index ];
}

void tdSetColor(int index)
{
//    [ tda setColor:index ];
}

void tdSetColorRGB(float r, float g, float b)
{
//    [ tda setColorRGBred:r green:g blue:b ];
}

void tdSetFont(const char *fontface, float fontsize)
{
//    [ tda setFont:fontface size:fontsize ];
}

void tdSetLineCap(int capstyle)
{
//    [ tda setLineCap:capstyle ];
}

void tdSetLineWidth(float linewidth)
{
//    [ tda setLineWidth:linewidth ];
}

void tdSetNeedsRedraw(tdWindow winid)
{
    [pModelController setNeedsDisplayWithIndexA: winid];
}

// Query routines

void tdGetScreenData(tdSize *total, tdRegion *usable)
{
#if 0
    UIScreen *mainscreen = [ UIScreen mainScreen ];
    if (total)
    {
        CGRect totalscreen = [ mainscreen frame ];
        total->width = totalscreen.size.width;
        total->height = totalscreen.size.height;
    }
    if (usable)
    {
        CGRect usablescreen = [ mainscreen visibleFrame ];
        usable->left = usablescreen.origin.x;
        usable->width = usablescreen.size.width;
        usable->right = usable->left + usable->width;
        usable->center = (usable->left + usable->right)/2;
        usable->bottom = usablescreen.origin.y;
        usable->height = usablescreen.size.height;
        usable->top = usable->bottom + usable->height;
        usable->middle = (usable->bottom + usable->top)/2;
    }
#endif
}

tdColorRGB tdGetColor(int index)
{
    tdColorRGB mycolor;
//    [ tda getColor:index red:&(mycolor.red) green:&(mycolor.green) blue:&(mycolor.blue) ];
    return mycolor;
}

tdSize tdGetStringBounds(char *string)
{
    tdSize retsize;
//    CGSize mysize = [ tda getStringBounds:[ NSString stringWithCString:string encoding:NSASCIIStringEncoding ]];
//    retsize.width = mysize.width;
 //   retsize.height = mysize.height;
    return retsize;
}

void tdGetPointer(tdPosition *screenpos, tdPosition *windowpos)
{
#if 0
    if (screenpos)
    {
        CGPoint mypoint = [ NSEvent mouseLocation ];
        screenpos->x = mypoint.x;
        screenpos->y = mypoint.y;
    }
    if (windowpos)
    {
        CGPoint mypoint = [ tda getPointerInWindow ];
        windowpos->x = mypoint.x;
        windowpos->y = mypoint.y;
    }
#endif
}

int tdNeedsRedraw(tdWindow winid)
{
    return [pModelController doesNeedDisplayWithIndexA: winid];
    // return [ tda needsRedraw:winid ];
}

// Drawing routines

void tdToggleFullScreen(void)
{
 //   [ tda toggleFullScreen ];
}

void tdClearWindow(void)
{
//    [ tda clearWindow ];
}

void tdDrawFilledPoly(float x[], float y[], int count)
{
#if 0
    NSPointArray *myarray = [[NSPointArray alloc] init];
    
    
 //   (NSPointArray)malloc(count * sizeof(NSPoint));
    int i;
    for (i=0; i<count; i++)
    {
        myarray[i].x = x[i];
        myarray[i].y = y[i];
    }
    [ tda drawFilledPoly:myarray vertices:count ];
    free(myarray);
#endif
}

void tdDrawFilledRect(float x, float y, int align, float width, float height)
{
    CGRect myrect;
    myrect.origin.x = x;
    myrect.origin.y = y;
    myrect.size.width = width;
    myrect.size.height = height;
//    [ tda drawFilledRect:myrect align:align ];
}

void tdDrawString(char *mystring, float x, float y, float rotate, int align)
{
    CGPoint mypoint;
    mypoint.x = x;
    mypoint.y = y;
 //   [ tda drawString:[ NSString stringWithCString:mystring encoding:NSASCIIStringEncoding ] atPoint:mypoint rotated:rotate aligned:align ];
}

void tdLineStart(float x, float y)
{
    CGPoint point;
    point.x = x;
    point.y = y;
 //   [ tda lineStart:point ];
}

void tdLineAppend(float x, float y)
{
    CGPoint point;
    point.x = x;
    point.y = y;
//    [ tda lineAppend:point ];
}

void tdDrawImage(tdImage *myimage, float xpos, float ypos, int align, float scalex, float scaley, float rotate)
{
    CGPoint point;
    point.x = xpos;
    point.y = ypos;
 //   [ tda drawImage:(__bridge UIImage *)myimage atPoint:point align:align scaleX:scalex scaleY:scaley rotate:rotate ];
}

void tdGLSwapBuffers(void)
{
//    [ tda glSwapBuffers ];
}

void tdRenderGraphics(void)
{
//    [ tda renderGraphics ];
}

// Event processing

void tdMainLoop(void (run_routine)(void), void (terminate_routine)(void))
{
    RunRoutine          = run_routine;
    TerminateRoutine    = terminate_routine;

    if( RunRoutine )
        RunRoutine();
}

void tdProcessEvents(void (button_handler)(ButtonEvent),
                     void (keyboard_handler)(KeyboardEvent),
                     void (configure_handler)(ConfigureEvent),
                     void (winclose_handler)(WinCloseEvent))
{
    keyClick    = keyboard_handler;
   
    if( button_handler )
    {
        for( const auto &touchL : touchEvents )
        {
            button_handler( touchL );
        }
        
        touchEvents.clear();
    }
}

// Termination routines

void tdCloseWindow(void)
{
//    [ tda closeWindow ];
}

void tdGLDestroyContext(void)
{
//    [ tda glDestroyContext ];
}

void tdTerminate(void)
{
//    [ tda cleanUp ];
}

void tdResizeImage( unsigned char *pDataA, int &widthA, int &heightA )
{
}
