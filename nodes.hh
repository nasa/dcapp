#ifndef _NODES_HH_
#define _NODES_HH_

#include <list>
#include <vector>
#include "basicutils/timer.hh"
#include "primitives/primitives.hh"
#include "dc.hh"
#include "animation.hh"
#include "comm.hh"
#include "psi.hh"

typedef struct
{
    float force_update;
    Timer *last_update;
    Timer *master_timer;
    std::list<Animation *> animators;
    std::list<CommModule *> commlist;
    std::list<PixelStreamItem *> pixelstreams;
    std::list<dcObject *> events;
    std::list<dcObject *> mouseheld;
    std::vector<float> vertices;
    dcWindow *toplevel;
    void (*DisplayPreInit)(void *(*)(const char *));
    void (*DisplayInit)(void);
    void (*DisplayLogic)(void);
    void (*DisplayClose)(void);
    int *canbus_inhibited;
} appdata;

#endif
