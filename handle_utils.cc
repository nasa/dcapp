#include <list>
#include "app_data.hh"
#include "primitives/primitives.hh"

extern void UpdateDisplay(void);

extern appdata AppData;

void ProcessEvents(void)
{
    if (!AppData.events.empty())
    {
        std::list<dcObject *>::iterator event;
        for (event = AppData.events.begin(); event != AppData.events.end(); event++)
        {
            (*event)->updateData();
        }
        AppData.events.clear();
        UpdateDisplay();
    }
}
