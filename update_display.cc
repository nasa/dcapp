#include <stdio.h>
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
    for (struct node *p_panel = AppData.window->p_head; p_panel; p_panel = p_panel->p_next_list)
    {
        if (*(AppData.window->object.win.active_display) == p_panel->object.panel.displayID)
        {
            AppData.window->p_current = p_panel;
            break;
        }
    }

    AppData.DisplayLogic();

    SetNeedsRedraw();
}
