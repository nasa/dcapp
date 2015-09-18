#ifndef _TDDEFINES_HH_
#define _TDDEFINES_HH_

typedef long tdWindow;
typedef void tdImage;
typedef void tdGLContext;

enum { tdPressed, tdReleased };
enum { tdUnknownKey, tdArrowLeft, tdArrowRight, tdArrowDown, tdArrowUp, tdHelpKey, tdHomeKey, tdEndKey, tdPageUp, tdPageDown };

typedef struct
{
    float x;
    float y;
} tdPosition;

typedef struct
{
    float width;
    float height;
} tdSize;

typedef struct
{
    float left;
    float center;
    float right;
    float width;
    float bottom;
    float middle;
    float top;
    float height;
} tdRegion;

typedef struct
{
    float red;
    float green;
    float blue;
} tdColorRGB;

typedef struct
{
    tdWindow window;
    tdPosition pos;
    int state;
} ButtonEvent;

typedef struct
{
    tdWindow window;
    tdPosition pos;
    char key;
    char specialkey;
    int state;
} KeyboardEvent;

typedef struct
{
    tdWindow window;
    tdSize size;
} ConfigureEvent;

typedef struct
{
    tdWindow window;
} WinCloseEvent;

#endif
