#ifndef _NODES_HH_
#define _NODES_HH_

#include <cstdio>
#include <cstdlib>
#include <string>
#include <list>
#include <map>
#include <vector>
#include "fontlib/fontlib.hh"
#include "utils/timer.hh"
#include "dc.hh"
#include "alignment.hh"
#include "kolor.hh"
#include "comm.hh"
#include "PixelStream.hh"
#include "animation.hh"

#define UNDEFINED_TYPE 0
#define STRING_TYPE    1
#define FLOAT_TYPE     2
#define INTEGER_TYPE   3

class VarString
{
    public:
        VarString(int a, void *b, const char *c)
        {
            if (a == UNDEFINED_TYPE) datatype = STRING_TYPE;
            else datatype = a;
            value = b;
            if (c) format = c;
            else
            {
                switch (datatype)
                {
                    case FLOAT_TYPE:   format = "%g"; break;
                    case INTEGER_TYPE: format = "%d";   break;
                    case STRING_TYPE:  format = "%s";   break;
                }
            }
        };
        std::string get(void)
        {
            char *tmp_str = 0x0;
            switch (datatype)
            {
                case FLOAT_TYPE:   asprintf(&tmp_str, format.c_str(), *(float *)(value)); break;
                case INTEGER_TYPE: asprintf(&tmp_str, format.c_str(), *(int *)(value));   break;
                case STRING_TYPE:  asprintf(&tmp_str, format.c_str(), (char *)value);     break;
            }
            std::string ret_str = tmp_str;
            free(tmp_str);
            return ret_str;
        };
    private:
        int datatype;
        void *value;
        std::string format;
};

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

union objects
{
    struct Window win;
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
    std::vector<VarString *>vstring;
    std::vector<std::string>filler;
    struct node *p_prev;
    struct node *p_prev_list;
    struct node *p_next;
    struct node *p_next_list;
    struct node *p_head;
    struct node *p_tail;
    struct node *p_current;
    struct node *p_parent;
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
