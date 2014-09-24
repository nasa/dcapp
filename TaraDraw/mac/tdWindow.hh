#import <Cocoa/Cocoa.h>

@interface tdWindowClass : NSWindow
{
    id tdAdapter;
}
- (id)initWithContentRect:(NSRect)contentRect adapter:(id)myadapter;
@end
