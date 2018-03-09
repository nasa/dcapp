#include <list>
#include "basicutils/timer.hh"
#include "primitives/primitives.hh"
#include "nodes.hh"

extern void ProcessEvents(void);
extern void UpdateDisplay(void);

extern appdata AppData;

int mousebouncemode = 0;

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
}

void CheckMouseBounce(void)
{
    if (AppData.mouseheld.empty()) return;

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
            for (mh = AppData.mouseheld.begin(); mh != AppData.mouseheld.end(); mh++)
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
            for (mh = AppData.mouseheld.begin(); mh != AppData.mouseheld.end(); mh++)
            {
                (*mh)->updateData();
            }
            UpdateDisplay();
        }
    }
}
