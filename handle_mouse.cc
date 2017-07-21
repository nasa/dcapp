#include <list>
#include "basicutils/timer.hh"
#include "primitives/primitives.hh"
#include "nodes.hh"

extern void ProcessEvents(void);
extern void UpdateDisplay(void);

extern appdata AppData;

std::list<dcObject *> mouseheld;

int mousebouncemode = 0;

void HandleMousePress(float xpct, float ypct)
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
