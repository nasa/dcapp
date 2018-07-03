#include <sstream>
#include "basicutils/msg.hh"
#include "nodes.hh"
#include "bezel.hh"

extern appdata AppData;

static int *canbus_inhibited = 0x0;

extern void ProcessEvents(void);

void HandleBezelInit(int *inhibit_ptr)
{
    canbus_inhibited = inhibit_ptr;
}

void HandleBezelControl(int type, int itemid, int action)
{
    if (type == 0xaa && itemid == 0x01 && canbus_inhibited)
    {
        if (action) *canbus_inhibited = 0;
        else *canbus_inhibited = 1;
    }
}

void HandleBezelButton(int type, int itemid, int action)
{
#ifdef DEBUG
    std::stringstream debug_string;
    debug_string << "Type: ";
    switch (type)
    {
        case BEZEL_BUTTON:           debug_string << "Button";               break;
        case BEZEL_KNOB:             debug_string << "Knob";                 break;
        default:                     debug_string << "Unknown";              break;
    }
    debug_string << ", ID: " << itemid << ", Action: ";
    switch (action)
    {
        case BEZEL_RELEASED:         debug_string << "Released";             break;
        case BEZEL_PRESSED:          debug_string << "Pressed";              break;
        case BEZEL_CLOCKWISE:        debug_string << "Clockwise Tick";       break;
        case BEZEL_COUNTERCLOCKWISE: debug_string << "Couterclockwise Tick"; break;
        default:                     debug_string << "Unknown";              break;
    }
    debug_msg(debug_string.str());
#endif

    if (type == BEZEL_BUTTON)
    {
        if (action == BEZEL_PRESSED)
        {
            AppData.toplevel->handleBezelPress(itemid);
            ProcessEvents();
        }
        else if (action == BEZEL_RELEASED)
        {
            AppData.toplevel->handleBezelRelease(itemid);
            ProcessEvents();
        }
    }
}
