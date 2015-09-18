#import "tdWindow.hh"
#import "tdAdapter.hh"

@implementation tdWindowClass

- (id)initWithContentRect:(NSRect)contentRect adapter:(id)myadapter
{
    unsigned int mymask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;

    if ((self = [ super initWithContentRect:contentRect styleMask:mymask backing:NSBackingStoreBuffered defer:YES ]) != nil)
    {
        tdAdapter = myadapter;
    }

    return self;
}

- (void)keyDown:(NSEvent *)theEvent
{
    [ tdAdapter keyEvent:theEvent state:tdPressed ];
}

- (void)keyUp:(NSEvent *)theEvent
{
    [ tdAdapter keyEvent:theEvent state:tdReleased ];
}

- (void)print:(id)sender
{
    /* set printing defaults (one window per page, etc.) */
    NSPrintInfo *printinfo = [ NSPrintInfo sharedPrintInfo ];
    NSRect pb = [ printinfo imageablePageBounds ];
    NSSize ps = [ printinfo paperSize ];
    [ printinfo setLeftMargin:pb.origin.x ];
    [ printinfo setRightMargin:(ps.width-pb.size.width-pb.origin.x) ];
    [ printinfo setBottomMargin:pb.origin.y ];
    [ printinfo setTopMargin:(ps.height-pb.size.height-pb.origin.y) ];
//    NSMutableDictionary *printdictionary = [ printinfo dictionary ];
//    [ printdictionary setValue:[ NSNumber numberWithBool:YES ] forKey:NSPrintHeaderAndFooter ];
    [ printinfo setHorizontalPagination:NSFitPagination ];
    [ printinfo setVerticalPagination:NSFitPagination ];
    [ printinfo setHorizontallyCentered:YES ];
    [ printinfo setVerticallyCentered:YES ];
    [ NSPrintInfo setSharedPrintInfo:printinfo ];

    [[ self contentView ] print:sender ];
}

@end
