#import <AppKit/AppKit.h>
#import <vector>
#import <string>
#import <sstream>
#import <sys/stat.h>

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

extern void ui_init(const std::string &);

/* Get default arguments from the Application Support folder */
std::vector<std::string> getArgs(void)
{
    std::vector<std::string> retvec;
    NSString *applicationSupportDirectory = [ NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) firstObject ];
    NSString *dcappSupportDirectory = [[ applicationSupportDirectory stringByAppendingString:@"/dcapp" ] stringByStandardizingPath ];
    NSString *dcappPrefsFile = [[ dcappSupportDirectory stringByAppendingString:@"/Preferences" ] stringByStandardizingPath ];

    MyApp *myapp = [[ MyApp alloc ] init ];

    NSString *contents = [ NSString stringWithContentsOfFile:dcappPrefsFile encoding:NSASCIIStringEncoding error:nil ];
    if (contents)
    {
        NSScanner *myscanner = [ NSScanner scannerWithString:contents ];
        NSCharacterSet *lbrace = [ NSCharacterSet characterSetWithCharactersInString:@"{" ];
        NSCharacterSet *rbrace = [ NSCharacterSet characterSetWithCharactersInString:@"}" ];
        NSString *mypath, *myargs;

        [ myscanner scanCharactersFromSet:lbrace intoString:nil ];
        if ([ myscanner scanUpToCharactersFromSet:rbrace intoString:&mypath ]) [ myapp setSpecfile:mypath ];
        [ myscanner scanCharactersFromSet:rbrace intoString:nil ];
        [ myscanner scanCharactersFromSet:lbrace intoString:nil ];
        if ([ myscanner scanUpToCharactersFromSet:rbrace intoString:&myargs ]) [ myapp setArgs:myargs ];

        [ NSApp run ];

        retvec.push_back([[[ myapp getSpecfile ] stringByStandardizingPath ] cStringUsingEncoding:NSASCIIStringEncoding ]);

        // These lines strip out multiple-consecuctive white spaces...
        std::stringstream ss([[ myapp getArgs ] cStringUsingEncoding:NSASCIIStringEncoding ]);
        std::string item;
        while (std::getline(ss >> std::ws, item, ' ')) retvec.push_back(item);
    }
    else [ NSApp run ];

    return retvec;
}

/* Store default arguments to the Application Support folder */
void storeArgs(std::vector<std::string> &arglist)
{
    NSString *applicationSupportDirectory = [ NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) firstObject ];
    NSString *dcappSupportDirectory = [[ applicationSupportDirectory stringByAppendingString:@"/dcapp" ] stringByStandardizingPath ];
    NSString *dcappPrefsFile = [[ dcappSupportDirectory stringByAppendingString:@"/Preferences" ] stringByStandardizingPath ];

    [[ NSFileManager defaultManager ] createDirectoryAtPath:dcappSupportDirectory withIntermediateDirectories:YES attributes:nil error:nil ];
    NSString *fullpath = [[ NSURL fileURLWithPath:[ NSString stringWithCString:arglist[0].c_str() encoding:NSASCIIStringEncoding ]] path ];

    std::string cmdargs;
    if (!arglist.empty())
    {
        for (std::vector<std::string>::iterator it = arglist.begin()+1; it != arglist.end(); it++)
        {
            cmdargs += *it;
            cmdargs += ' ';
        }
        if (cmdargs.back() == ' ') cmdargs.pop_back();
    }

    NSString *mydata = [ NSString stringWithFormat:@"{%@}{%s}", fullpath, cmdargs.c_str() ];
    [ mydata writeToFile:dcappPrefsFile atomically:NO encoding:NSASCIIStringEncoding error:nil ];
}

std::vector<std::string> OSgetArgs(int argc, char **argv)
{
    std::vector<std::string> arglist;

    ui_init("");

    arglist.assign(argv + 1, argv + argc);

    if (arglist.empty() || (arglist.size() == 1 && !arglist[0].compare(0, 4, "-psn"))) arglist = getArgs();

    storeArgs(arglist);

    return arglist;
}

@implementation MyApp

- (id)init
{
    if ((self = [ super init ]))
    {
        id appName = [[NSProcessInfo processInfo] processName];

        window = [[[ NSWindow alloc ] initWithContentRect:NSMakeRect(0, 0, 1000, 234)
#ifdef MAC_OS_X_VERSION_10_12
                                      styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable
#else
                                      styleMask:NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask
#endif
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
#ifdef MAC_OS_X_VERSION_10_12
    [ mybutton setBezelStyle:NSBezelStyleRounded ];
#else
    [ mybutton setBezelStyle:NSRoundedBezelStyle ];
#endif
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
    (void)sender; // variable unused

    NSSavePanel *panel = [ NSSavePanel savePanel ];

    [ panel setNameFieldLabel:@"Create:" ];
    [ panel setPrompt:@"Create" ];

#ifdef MAC_OS_X_VERSION_10_13
    if ([ panel runModal ] == NSModalResponseOK)
#else
    if ([ panel runModal ] == NSFileHandlingPanelOKButton)
#endif
    {
        [ specfile setStringValue:[[ panel URL ] path ]];
        NSString *content = @"<?xml version=\"1.0\"?>\n<DCAPP>\n\n</DCAPP>\n";
        NSData *fileContents = [ content dataUsingEncoding:NSASCIIStringEncoding ];
        [[ NSFileManager defaultManager ] createFileAtPath:[ specfile stringValue ] contents:fileContents attributes:nil ];
        [[ NSWorkspace sharedWorkspace ] openFile:[ specfile stringValue ]];
    }
}

- (void)selectFile:(id)sender
{
    (void)sender; // variable unused

    NSOpenPanel *panel = [ NSOpenPanel openPanel ];

    [ panel setCanChooseFiles:YES ];
    [ panel setCanChooseDirectories:NO ];
    [ panel setResolvesAliases:YES ];
    [ panel setAllowsMultipleSelection:NO ];

#ifdef MAC_OS_X_VERSION_10_9
    if ([ panel runModal ] == NSModalResponseOK)
#else
    if ([ panel runModal ] == NSOKButton)
#endif
        [ specfile setStringValue:[[[ panel URLs ] objectAtIndex: 0 ] path ]];
}

- (void)editFile:(id)sender
{
    (void)sender; // variable unused
    [[ NSWorkspace sharedWorkspace ] openFile:[ specfile stringValue ]];
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
