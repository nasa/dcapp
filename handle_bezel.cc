#include "app_data.hh"

extern appdata AppData;

extern void ProcessEvents(void);

void HandleBezelPress(int itemid)
{
    AppData.toplevel->handleBezelPress(itemid);
    ProcessEvents();
}

void HandleBezelRelease(int itemid)
{
    AppData.toplevel->handleBezelRelease(itemid);
    ProcessEvents();
}
