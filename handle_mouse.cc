#include <list>
#include "basicutils/timer.hh"
#include "primitives/primitives.hh"
#include "app_data.hh"

extern void ProcessEvents(void);
extern void UpdateDisplay(void);

extern appdata AppData;

static std::list<dcObject *> mouseheld;
static unsigned mousebouncemode = 0;

void HandleMouseMotion(double xpct, double ypct)
{
    static double prevx=0, prevy=0;
    if (xpct != prevx || ypct != prevy)
    {
        AppData.toplevel->handleMouseMotion(xpct, ypct);
        prevx = xpct;
        prevy = ypct;
    }
}

void HandleMousePress(double xpct, double ypct)
{
    AppData.toplevel->handleMousePress(xpct, ypct);
    ProcessEvents();
}

void HandleMouseRelease(void)
{
    AppData.toplevel->handleMouseRelease();
    ProcessEvents();
    mouseheld.clear();
}

void RegisterPressedPrimitive(dcParent *mylist)
{
    mouseheld.push_back(mylist);
    mousebouncemode = 1;
}

void CheckMouseBounce(void)
{
    if (mouseheld.empty()) return;

    static Timer *mousetimer = new Timer;

    if (mousebouncemode == 1)
    {
        mousetimer->restart();
        mousebouncemode = 2;
    }
    else if (mousebouncemode == 2)
    {
        if (mousetimer->getSeconds() > 1)
        {
            mousetimer->restart();
            std::list<dcObject *>::iterator mh;
            for (mh = mouseheld.begin(); mh != mouseheld.end(); mh++)
            {
                (*mh)->updateData();
            }
            UpdateDisplay();
            mousebouncemode = 3;
        }
    }
    else if (mousebouncemode == 3)
    {
        if (mousetimer->getSeconds() > .1)
        {
            mousetimer->restart();
            std::list<dcObject *>::iterator mh;
            for (mh = mouseheld.begin(); mh != mouseheld.end(); mh++)
            {
                (*mh)->updateData();
            }
            UpdateDisplay();
        }
    }
}
