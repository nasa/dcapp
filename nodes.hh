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

typedef enum { Equals, PlusEquals, MinusEquals } SetOperator;
typedef enum { Simple, IfEquals, IfNotEquals, IfGreaterThan, IfLessThan, IfGreaterOrEquals, IfLessOrEquals } IfOperator;

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
