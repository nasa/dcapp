#import <AppKit/AppKit.h>
#include <string.h>

@interface MyApp : NSObject
{
    id window;
    id specfile;
    id args;
}
- (id)init;
- (NSString *)getSpecfile;
- (NSString *)getArgs;
- (void)setSpecfile:(NSString *)value;
- (void)setArgs:(NSString *)value;
@end

void appLauncher(char *inSpecfile, char **outSpecfile, char *inArgs, char **outArgs)
{
    MyApp *myapp = [[ MyApp alloc ] init ];

    if (inSpecfile) [ myapp setSpecfile:[[ NSString stringWithCString:inSpecfile encoding:NSASCIIStringEncoding ] stringByStandardizingPath ]];
    if (inArgs) [ myapp setArgs:[ NSString stringWithCString:inArgs encoding:NSASCIIStringEncoding ]];

    [ NSApp run ];

    *outSpecfile = strdup([[[ myapp getSpecfile ] stringByStandardizingPath ] cStringUsingEncoding:NSASCIIStringEncoding ]);
    *outArgs = strdup([[ myapp getArgs ] cStringUsingEncoding:NSASCIIStringEncoding ]);
}

@implementation MyApp

- (id)init
{
    if ((self = [ super init ]))
    {
        id appName = [[NSProcessInfo processInfo] processName];

        window = [[[ NSWindow alloc ] initWithContentRect:NSMakeRect(0, 0, 1000, 234)
                                      styleMask:NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask
                                      backing:NSBackingStoreBuffered
                                      defer:NO ] autorelease ];

        [ window cascadeTopLeftFromPoint:NSMakePoint(20,20) ];
        [ window setTitle:appName ];
        [ window makeKeyAndOrderFront:nil ] ;

        id box1 = [ self uiBoxParent:[ window contentView ] x:17 y:123 w:966 h:93 ];
        [ self uiLabelParent:box1 x:5 y:49 w:200 h:17 label:@"Specification File:" ];
        specfile = [ self uiTextFieldParent:box1 x:5 y:25 w:850 h:22 ];
        [ self uiButtonParent:box1 x:864 y:50 w:92 h:28 label:@"Create..." target:self action:@selector(createFile:) ];
        [ self uiButtonParent:box1 x:864 y:25 w:92 h:28 label:@"Select..." target:self action:@selector(selectFile:) ];
        [ self uiButtonParent:box1 x:864 y:0 w:92 h:28 label:@"Edit..." target:self action:@selector(editFile:) ];

        id box2 = [ self uiBoxParent:[ window contentView ] x:17 y:56 w:966 h:65 ];
        [ self uiLabelParent:box2 x:5 y:34 w:200 h:17 label:@"Command-Line Arguments:" ];
        args = [ self uiTextFieldParent:box2 x:5 y:10 w:944 h:22 ];

        id proceed = [ self uiButtonParent:[ window contentView ] x:430 y:12 w:140 h:32 label:@"Proceed" target:self action:@selector(buttonClicked:) ];

        // Set the key view loop so that the user can switch between fields using the "tab" key
        [ specfile setNextKeyView:args ];
        [ args setNextKeyView:specfile ];

        // Set the "Proceed" button to activate by default with the "return" key
        [ window setDefaultButtonCell:[ proceed cell ]];
    }
    return self;
}

- (id)uiBoxParent:(id)parent x:(float)x y:(float)y w:(float)w h:(float)h
{
    id mybox = [[[ NSBox alloc] initWithFrame:NSMakeRect(x, y, w, h) ] autorelease ];
    [ mybox setTitlePosition:NSNoTitle ];
    [ parent addSubview:mybox ];
    return mybox;
}

- (id)uiButtonParent:(id)parent x:(float)x y:(float)y w:(float)w h:(float)h label:(NSString *)mystring target:(id)mytarget action:(SEL)myaction
{
    id mybutton = [[ NSButton alloc ] initWithFrame:NSMakeRect(x, y, w, h) ];
    [ mybutton setBezelStyle:NSRoundedBezelStyle ];
    [ mybutton setTitle:mystring ];
    [ mybutton setTarget:mytarget ];
    [ mybutton setAction:myaction ];
    [ parent addSubview:mybutton ];
    return mybutton;
}

- (id)uiLabelParent:(id)parent x:(float)x y:(float)y w:(float)w h:(float)h label:(NSString *)mystring
{
    id mylabel = [[[ NSTextField alloc] initWithFrame:NSMakeRect(x, y, w, h) ] autorelease ];
    [ mylabel setStringValue:mystring ];
    [ mylabel setEditable:NO ];
    [ mylabel setDrawsBackground:NO ];
    [ mylabel setBordered:NO ];
    [ parent addSubview:mylabel ];
    return mylabel;
}

- (id)uiTextFieldParent:(id)parent x:(float)x y:(float)y w:(float)w h:(float)h
{
    id myfield = [[[ NSTextField alloc] initWithFrame:NSMakeRect(x, y, w, h) ] retain ];
    [[ myfield cell ] setScrollable:YES ];
    [ parent addSubview:myfield ];
    return myfield;
}

- (void)createFile:(id)sender
{
    NSSavePanel	*panel = [ NSSavePanel savePanel ];
    FILE *fp;
    char *cmd;
    
    [ panel setNameFieldLabel:@"Create:" ];
    [ panel setPrompt:@"Create" ];

    if ([ panel runModal ] == NSFileHandlingPanelOKButton)
    {
        [ specfile setStringValue:[[ panel URL ] path ]];
        fp = fopen([[ specfile stringValue ] cStringUsingEncoding:NSASCIIStringEncoding ], "w");
        fclose(fp);
        cmd = (char *)malloc(strlen([[ specfile stringValue ] cStringUsingEncoding:NSASCIIStringEncoding ])+8);
        sprintf(cmd, "open \"%s\"", [[ specfile stringValue ] cStringUsingEncoding:NSASCIIStringEncoding ]);
        system(cmd);
        free(cmd);
    }
}

- (void)selectFile:(id)sender
{
    NSOpenPanel	*panel = [ NSOpenPanel openPanel ];
    
    [ panel setCanChooseFiles:YES ];
    [ panel setCanChooseDirectories:NO ];
    [ panel setResolvesAliases:YES ];
    [ panel setAllowsMultipleSelection:NO ];

#ifdef NSAppKitVersionNumber10_9
    if ([ panel runModal ] == NSModalResponseOK)
#else
    if ([ panel runModal ] == NSOKButton)
#endif
        [ specfile setStringValue:[[[ panel URLs ] objectAtIndex: 0 ] path ]];
}

- (void)editFile:(id)sender
{
    char *cmd = (char *)malloc(strlen([[ specfile stringValue ] cStringUsingEncoding:NSASCIIStringEncoding ])+8);
    sprintf(cmd, "open \"%s\"", [[ specfile stringValue ] cStringUsingEncoding:NSASCIIStringEncoding ]);
    system(cmd);
    free(cmd);
}

- (void)buttonClicked:(id)sender
{
    [ window makeFirstResponder:nil ];
    [ window close ];
    [[ NSApplication sharedApplication ] stop:sender ];
}

- (NSString *)getSpecfile
{
    return [[ specfile stringValue ] stringByTrimmingCharactersInSet:[ NSCharacterSet whitespaceCharacterSet ]];
}

- (NSString *)getArgs
{
    return [[ args stringValue ] stringByTrimmingCharactersInSet:[ NSCharacterSet whitespaceCharacterSet ]];
}

- (void)setSpecfile:(NSString *)value
{
    [ specfile setStringValue:value ];
}

- (void)setArgs:(NSString *)value
{
    [ args setStringValue:value ];
}

@end
