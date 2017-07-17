#ifndef _NODES_HH_
#define _NODES_HH_

#include <cstdio>
#include <cstdlib>
#include <string>
#include <list>
#include <map>
#include <vector>
#include "basicutils/timer.hh"
#include "PixelStream/PixelStream.hh"
#include "fontlib/fontlib.hh"
#include "dc.hh"
#include "alignment.hh"
#include "kolor.hh"
#include "comm.hh"
#include "animation.hh"
#include "types.hh"
#include "varstring.hh"

class PixelStreamItem
{
    public:
        PixelStreamItem() : psd(0x0), frame_count(0) {};
        ~PixelStreamItem() { if (psd) delete psd; };
        PixelStreamData *psd;
        unsigned frame_count;
};


#include "primitives/primitives.hh" // TODO: move and incorporate PixelStreamItem somewhere


typedef enum {
    Empty,
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

typedef enum { Equals, PlusEquals, MinusEquals } SetOperator;
typedef enum { Simple, IfEquals, IfNotEquals, IfGreaterThan, IfLessThan, IfGreaterOrEquals, IfLessOrEquals } IfOperator;

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
    Kolor color;
    float orthoX;
    float orthoY;
    float vwidth;
    float vheight;
    struct node *SubList;
};

struct Image
{
    dcTexture textureID;
};

struct PixelStreamView
{
    dcTexture textureID;
    PixelStreamItem *psi;
    void *pixels;
    size_t memallocation;
};

struct Rect
{
    float linewidth;
    int fill;
    int outline;
    Kolor FillColor;
    Kolor LineColor;
};

struct Circle
{
    float *radius;
    float linewidth;
    int fill;
    int outline;
    Kolor FillColor;
    Kolor LineColor;
    int segments;
};

struct Line
{
    float linewidth;
    Kolor color;
    struct node *Vertices;
};

struct Polygon
{
    float linewidth;
    int fill;
    int outline;
    Kolor FillColor;
    Kolor LineColor;
    struct node *Vertices;
};

struct String
{
    flFont *fontID;
    float *fontSize;
    Kolor color;
    Kolor bgcolor;
    bool background;
    float *shadowOffset;
//    char *format;
//    int datatype;
    flMonoOption forcemono;
//    void *value;
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

union objects
{
    struct Panel panel;
    struct Image image;
    struct PixelStreamView pixelstreamview;
    struct Container cont;
    struct Animate anim;
    struct Condition cond;
    struct Line line;
    struct Polygon poly;
    struct Rect rect;
    struct Circle circle;
    struct String string;
    struct ADI adi;
    struct MouseEvent me;
    struct KeyboardEvent ke;
    struct BezelEvent be;
    struct ModifyValue modval;
};

struct node
{
    struct
    {
        Type type;
        int halign;
        int valign;
        bool selected;

        float *x;
        float *y;
        float *w;
        float *h;
        float *containerW;
        float *containerH;
        float *rotate;
    } info;
    union objects object;
    // The two items below, and the above VarString class, should be defined in the String structure,
    // but the String structure is a member of the objects union, so it can't hold a variable size class
    std::vector<VarString *> vstring;
    std::vector<std::string> filler;
    struct node *p_next;
    struct node *p_tail;
    struct node *p_parent;
};

struct Window
{
    int *active_display;
    std::list<struct node *> panels;
    struct node *current_panel;
};

typedef struct
{
    float force_update;
    Timer *last_update;
    Timer *master_timer;
    std::list<Animation *> animators;
    std::list<CommModule *> commlist;
    std::list<PixelStreamItem *> pixelstreams;
    int *canbus_inhibited;
    struct Window window;
dcWindow *toplevel;
    void (*DisplayPreInit)(void *(*)(const char *));
    void (*DisplayInit)(void);
    void (*DisplayLogic)(void);
    void (*DisplayClose)(void);
} appdata;

#endif
