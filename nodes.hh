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

typedef enum { Empty, Container } Type;
typedef enum { Equals, PlusEquals, MinusEquals } SetOperator;
typedef enum { Simple, IfEquals, IfNotEquals, IfGreaterThan, IfLessThan, IfGreaterOrEquals, IfLessOrEquals } IfOperator;

struct Panel
{
    struct node *SubList;
};

struct Line
{
    struct node *Vertices;
};

struct Polygon
{
    struct node *Vertices;
};

struct MouseEvent
{
    struct node *PressList;
    struct node *ReleaseList;
};

struct KeyboardEvent
{
    struct node *PressList;
    struct node *ReleaseList;
};

struct BezelEvent
{
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
    struct node *SubList;
};

struct Condition
{
    struct node *TrueList;
    struct node *FalseList;
};

struct Container
{
    struct node *SubList;
    float *vwidth;
    float *vheight;
};

union objects
{
    struct Panel panel;
    struct Container cont;
    struct Animate anim;
    struct Condition cond;
    struct Line line;
    struct Polygon poly;
    struct MouseEvent me;
    struct KeyboardEvent ke;
    struct BezelEvent be;
};

struct node
{
    struct
    {
        Type type;
        int halign;
        int valign;
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

typedef struct
{
    float force_update;
    Timer *last_update;
    Timer *master_timer;
    std::list<Animation *> animators;
    std::list<CommModule *> commlist;
    std::list<PixelStreamItem *> pixelstreams;
    int *canbus_inhibited;
    dcWindow *toplevel;
    void (*DisplayPreInit)(void *(*)(const char *));
    void (*DisplayInit)(void);
    void (*DisplayLogic)(void);
    void (*DisplayClose)(void);
} appdata;

#endif
