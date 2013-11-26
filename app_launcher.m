#import <AppKit/AppKit.h>
 
@interface MyApp : NSObject
{
    id window;
    id specfile;
    id host;
    id port;
    id args;
}
- (id)init;
- (NSString *)getSpecfile;
- (NSString *)getHost;
- (NSString *)getPort;
- (NSString *)getArgs;
- (void)setSpecfile:(NSString *)value;
@end

char *appLauncher(char *defaultSpecfile)
{
    MyApp *myapp = [[ MyApp alloc ] init ];
    [ myapp setSpecfile:[ NSString stringWithCString:defaultSpecfile encoding:NSASCIIStringEncoding ]];
    [ NSApp run ];
    return (char *)[[ myapp getSpecfile ] cStringUsingEncoding:NSASCIIStringEncoding ];
}

@implementation MyApp

- (id)init
{
    if ((self = [ super init ]))
    {
        id appName = [[NSProcessInfo processInfo] processName];

        window = [[[ NSWindow alloc ] initWithContentRect:NSMakeRect(0, 0, 716, 284)
                                      styleMask:NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask|NSTexturedBackgroundWindowMask
                                      backing:NSBackingStoreBuffered
                                      defer:NO ] autorelease ];
        [ window cascadeTopLeftFromPoint:NSMakePoint(20,20) ];
        [ window setTitle:appName ];
        [ window makeKeyAndOrderFront:nil ] ;

        id box1 = [[[ NSBox alloc] initWithFrame:NSMakeRect(17, 173, 682, 93) ] autorelease ];
        [ box1 setTitlePosition:NSNoTitle ];
        [[ window contentView ] addSubview:box1 ];

        id speclabel = [[[ NSTextField alloc] initWithFrame:NSMakeRect(5, 49, 114, 17) ] autorelease ];
        [ speclabel setStringValue:@"Specification File:" ];
        [ speclabel setEditable:NO ];
        [ speclabel setDrawsBackground:NO ];
        [ speclabel setBordered:NO ];
        [ box1 addSubview:speclabel ];

        specfile = [[[ NSTextField alloc] initWithFrame:NSMakeRect(5, 25, 566, 22) ] retain ];
        [ box1 addSubview:specfile ];

        id createbutton = [[ NSButton alloc ] initWithFrame:NSMakeRect(580, 50, 92, 28) ];
        [ createbutton setBezelStyle:NSRoundedBezelStyle ];
        [ createbutton setTitle:@"Create..." ];
        [ createbutton setTarget:self ];
        [ createbutton setAction:@selector(createFile:) ];
        [ box1 addSubview:createbutton ];

        id selectbutton = [[ NSButton alloc ] initWithFrame:NSMakeRect(580, 25, 92, 28) ];
        [ selectbutton setBezelStyle:NSRoundedBezelStyle ];
        [ selectbutton setTitle:@"Select..." ];
        [ selectbutton setTarget:self ];
        [ selectbutton setAction:@selector(selectFile:) ];
        [ box1 addSubview:selectbutton ];

        id editbutton = [[ NSButton alloc ] initWithFrame:NSMakeRect(580, 0, 92, 28) ];
        [ editbutton setBezelStyle:NSRoundedBezelStyle ];
        [ editbutton setTitle:@"Edit..." ];
        [ editbutton setTarget:self ];
        [ editbutton setAction:@selector(editFile:) ];
        [ box1 addSubview:editbutton ];

        id box2 = [[[ NSBox alloc] initWithFrame:NSMakeRect(17, 56, 682, 115) ] autorelease ];
        [ box2 setTitlePosition:NSNoTitle ];
        [[ window contentView ] addSubview:box2 ];

        id hostlabel = [[[ NSTextField alloc] initWithFrame:NSMakeRect(5, 84, 38, 17) ] autorelease ];
        [ hostlabel setStringValue:@"Host:" ];
        [ hostlabel setEditable:NO ];
        [ hostlabel setDrawsBackground:NO ];
        [ hostlabel setBordered:NO ];
        [ box2 addSubview:hostlabel ];

        host = [[[ NSTextField alloc] initWithFrame:NSMakeRect(5, 60, 414, 22) ] retain ];
        [ box2 addSubview:host ];

        id portlabel = [[[ NSTextField alloc] initWithFrame:NSMakeRect(435, 84, 40, 17) ] autorelease ];
        [ portlabel setStringValue:@"Port:" ];
        [ portlabel setEditable:NO ];
        [ portlabel setDrawsBackground:NO ];
        [ portlabel setBordered:NO ];
        [ box2 addSubview:portlabel ];

        port = [[[ NSTextField alloc] initWithFrame:NSMakeRect(435, 60, 136, 22) ] retain ];
        [ box2 addSubview:port ];

        id defbutton = [[ NSButton alloc ] initWithFrame:NSMakeRect(580, 56, 92, 28) ];
        [ defbutton setBezelStyle:NSRoundedBezelStyle ];
        [ defbutton setTitle:@"Defaults" ];
        [ defbutton setTarget:self ];
        [ defbutton setAction:@selector(setDefaults:) ];
        [ box2 addSubview:defbutton ];

        id argslabel = [[[ NSTextField alloc] initWithFrame:NSMakeRect(5, 34, 136, 17) ] autorelease ];
        [ argslabel setStringValue:@"Optional Arguments:" ];
        [ argslabel setEditable:NO ];
        [ argslabel setDrawsBackground:NO ];
        [ argslabel setBordered:NO ];
        [ box2 addSubview:argslabel ];

        args = [[[ NSTextField alloc] initWithFrame:NSMakeRect(5, 10, 660, 22) ] retain ];
        [ box2 addSubview:args ];

        id mybutton = [[ NSButton alloc ] initWithFrame:NSMakeRect(288, 12, 140, 32) ];
        [ mybutton setBezelStyle:NSRoundedBezelStyle ];
        [ mybutton setTitle:@"Proceed" ];
        [ mybutton setTarget:self ];
        [ mybutton setAction:@selector(buttonClicked:) ];
        [ window setDefaultButtonCell:[ mybutton cell ]];
        [[ window contentView ] addSubview:mybutton ];
    }
    return self;
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

    if ([ panel runModal ] == NSOKButton)
        [ specfile setStringValue:[[[ panel URLs ] objectAtIndex: 0 ] path ]];
}

- (void)editFile:(id)sender
{
    char *cmd = (char *)malloc(strlen([[ specfile stringValue ] cStringUsingEncoding:NSASCIIStringEncoding ])+8);
    sprintf(cmd, "open \"%s\"", [[ specfile stringValue ] cStringUsingEncoding:NSASCIIStringEncoding ]);
    system(cmd);
    free(cmd);
}

- (void)setDefaults:(id)sender
{
    [ host setStringValue:[[ NSProcessInfo processInfo ] hostName ]];
    [ port setStringValue:@"7000" ];
}

- (void)buttonClicked:(id)sender
{
    [ window close ];
    [[ NSApplication sharedApplication ] stop:sender ];
}

- (NSString *)getSpecfile
{
    return [ specfile stringValue ];
}

- (NSString *)getHost
{
    return [ host stringValue ];
}

- (NSString *)getPort
{
    return [ port stringValue ];
}

- (NSString *)getArgs
{
    return [ args stringValue ];
}

- (void)setSpecfile:(NSString *)value
{
    [ specfile setStringValue:value ];
}

@end
