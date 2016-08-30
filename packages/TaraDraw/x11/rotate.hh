/*******************************************************************************
Description: Header file for the XRotDrawString routine, which was based largely
    on "xvertext 5.0", built by Alan Richardson.  Reference:
    http://lists.gnu.org/archive/html/bug-gnustep/2002-03/msg00108.html
Programmer: M. McFarlane, March 2005
*******************************************************************************/

#ifndef _ROTATE_HH_
#define _ROTATE_HH_

int XRotDrawString(Display*, XFontStruct*, Drawable, GC, int, int, float, float, float, int, const char *);

#endif
