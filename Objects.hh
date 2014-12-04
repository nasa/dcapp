#ifndef _OBJECTS_HH_
#define _OBJECTS_HH_

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/shm.h>
#include "PixelStream.hh"
#include "fontlib.hh"

typedef enum { Empty, Panel, Image, Vertex, Rectangle, Circle, Line, Polygon, ADI, String, MouseEvent, KeyboardEvent, BezelEvent, SetValue, Condition, Container, PixelStream } Type;
typedef enum { AlignLeft, AlignCenter, AlignRight } HAlignment;
typedef enum { AlignBottom, AlignMiddle, AlignTop } VAlignment;
typedef enum { Equals, PlusEquals, MinusEquals } SetOperator;
typedef enum { Simple, IfEquals, IfNotEquals, IfGreaterThan, IfLessThan, IfGreaterOrEquals, IfLessOrEquals } IfOperator;
typedef enum { AppTerminate, AppReconnect } DisconnectAction;

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
    unsigned int textureID;
};

struct PixelStream
{
    unsigned int textureID;
    char *filename;
    FILE *fp;
    uint32_t buffercount;
    void *pixels;
    PixelStreamData *shm;
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
    void *fontID;
    float fontSize;
    struct kolor color;
    struct kolor bgcolor;
    int background;
    float shadowoffset;
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
    int bkgdID;
    int ballID;
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

struct Fonts
{
    void *fontID;
    char *fontFile;
    char *fontFace;
};

struct ShMem
{
    void *shm;
    key_t shm_key;
};

struct Textures
{
    unsigned int textureID;
    char *textureFile;
};

struct Constants
{
    int datatype;
    union
    {
        float f;
        int i;
        char *s;
    } val;
};

struct PreProcessConstant
{
    char *name;
    char *value;
};

struct Style
{
    char *name;
    void *node;
};

struct Default
{
    void *node;
};

struct Window
{
    int *active_display;
};

typedef struct
{
    struct
    {
        char *host;
        int port;
        char *datarate;
        DisconnectAction disconnectaction;
    } simcomm;
    float force_update;
    struct timeval last_update;
    struct node *window;
    struct node *ArgList;
    struct node *FontList;
    struct node *TextureList;
    struct node *ShMemList;
    struct node *ConstantList;
    void (*DisplayPreInit)(void *(*)(const char *));
    void (*DisplayInit)(void);
    void (*DisplayLogic)(void);
    void (*DisplayClose)(void);
} appdata;

#endif
