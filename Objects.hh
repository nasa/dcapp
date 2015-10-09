#ifndef _OBJECTS_HH_
#define _OBJECTS_HH_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <list>
#include <map>
#include "dc.hh"
#include "fontlib.hh"
#include "timer.hh"
#include "comm.hh"
#include "PixelStream.hh"
#include "animation.hh"

typedef std::map<std::string, std::string> ConstantsList;

typedef enum {
    Empty,
    Panel,
    Image,
    Vertex,
    Rectangle,
    Circle,
    Line,
    Polygon,
    ADI,
    String,
    MouseEvent,
    KeyboardEvent,
    BezelEvent,
    SetValue,
    Animate,
    Condition,
    Container,
    PixelStreamView
} Type;
typedef enum { AlignLeft, AlignCenter, AlignRight } HAlignment;
typedef enum { AlignBottom, AlignMiddle, AlignTop } VAlignment;
typedef enum { Equals, PlusEquals, MinusEquals } SetOperator;
typedef enum { Simple, IfEquals, IfNotEquals, IfGreaterThan, IfLessThan, IfGreaterOrEquals, IfLessOrEquals } IfOperator;

struct kolor
{
    float *R;
    float *G;
    float *B;
    float *A;
};

typedef struct
{
    float refx;
    float refy;
    float delx;
    float dely;
    float width;
    float height;
    float left;
    float right;
    float bottom;
    float top;
    float center;
    float middle;
} Geometry;

struct Panel
{
    int displayID;
    struct kolor color;
    float orthoX;
    float orthoY;
    float vwidth;
    float vheight;
    struct node *PrimitivesList;
};

struct Image
{
    dcTexture textureID;
};

struct PixelStreamView
{
    dcTexture textureID;
    PixelStreamData *psd;
    void *pixels;
    size_t memallocation;
};

struct Rect
{
    float linewidth;
    int fill;
    int outline;
    struct kolor FillColor;
    struct kolor LineColor;
};

struct Circle
{
    float *radius;
    float linewidth;
    int fill;
    int outline;
    struct kolor FillColor;
    struct kolor LineColor;
    int segments;
};

struct Line
{
    float linewidth;
    struct kolor color;
    struct node *Vertices;
};

struct Polygon
{
    float linewidth;
    int fill;
    int outline;
    struct kolor FillColor;
    struct kolor LineColor;
    struct node *Vertices;
};

struct String
{
    flFont *fontID;
    float *fontSize;
    struct kolor color;
    struct kolor bgcolor;
    bool background;
    float *shadowOffset;
    char *format;
    int datatype;
    flMonoOption forcemono;
    void *value;
    HAlignment halign;
    VAlignment valign;
};

struct MouseEvent
{
    struct node *PressList;
    struct node *ReleaseList;
};

struct KeyboardEvent
{
    char key;
    struct node *PressList;
    struct node *ReleaseList;
};

struct BezelEvent
{
    int key;
    struct node *PressList;
    struct node *ReleaseList;
};

struct ModifyValue
{
    int optype;
    int datatype1;
    int datatype2;
    int mindatatype;
    int maxdatatype;
    void *var;
    void *val;
    void *min;
    void *max;
};

struct Animate
{
    float duration;
    struct node *SubList;
};

struct Condition
{
    int opspec;
    int datatype1;
    int datatype2;
    void *val1;
    void *val2;
    struct node *TrueList;
    struct node *FalseList;
};

struct Container
{
    struct node *SubList;
    float *vwidth;
    float *vheight;
};

struct ADI
{
    dcTexture bkgdID;
    dcTexture ballID;
    float *outerradius;
    float *ballradius;
    float *chevronW;
    float *chevronH;
    float *roll;
    float *pitch;
    float *yaw;
    float *rollError;
    float *pitchError;
    float *yawError;
};

struct Window
{
    int *active_display;
};

typedef struct
{
    float force_update;
    Timer *last_update;
    Timer *master_timer;
    std::list<Animation *> animators;
    std::list<CommModule *> commlist;
    std::list<PixelStreamData *> pixelstreams;
    ConstantsList arglist;
    int *canbus_inhibited;
    struct node *window;
    void (*DisplayPreInit)(void *(*)(const char *));
    void (*DisplayInit)(void);
    void (*DisplayLogic)(void);
    void (*DisplayClose)(void);
} appdata;

#endif
