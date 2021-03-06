#ifndef _APP_DATA_HH_
#define _APP_DATA_HH_

#include <string>
#include <list>
#include <vector>
#include "basicutils/timer.hh"
#include "primitives/primitives.hh"
#include "animation.hh"
#include "comm.hh"
#include "device.hh"
#include "psi.hh"

typedef struct
{
    std::string dcapphome;
    std::string defaultfont;
    float force_update;
    Timer *last_update;
    Timer *master_timer;
    std::list<Animation *> animators;
    std::list<CommModule *> commlist;
    std::list<DeviceModule *> devicelist;
    std::list<PixelStreamItem *> pixelstreams;
    std::list<dcObject *> events;
    std::vector<float> vertices;
    dcWindow *toplevel;
    void (*DisplayPreInit)(void *(*)(const char *));
    void (*DisplayInit)(void);
    void (*DisplayLogic)(void);
    void (*DisplayClose)(void);
} appdata;

#endif
