#include <cstdio>
#include "nodes.hh"

extern void SetNeedsRedraw(void);

extern appdata AppData;


/*********************************************************************************
 *
 * Data has changed, so update active display, run DisplayLogic, and mark the
 * window as needing to be redrawn.
 *
 *********************************************************************************/
void UpdateDisplay(void)
{
    // Get the pointer to the current active panel
    std::list<struct node *>::iterator panel;
    for (panel = AppData.window.panels.begin(); panel != AppData.window.panels.end(); panel++)
    {
        if (*(AppData.window.active_display) == (*panel)->object.panel.displayID)
        {
            AppData.window.current_panel = *panel;
            break;
        }
    }

    AppData.DisplayLogic();

    SetNeedsRedraw();
}
