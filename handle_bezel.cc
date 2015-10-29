#include <sstream>
#include "nodes.hh"
#include "bezel.hh"
#include "msg.hh"

extern void ProcessEventList(struct node *);
extern bool CheckCondition(struct node *);

extern appdata AppData;

static void BezelButtonPressed(struct node *, int);
static void BezelButtonReleased(struct node *, int);


void HandleBezelControl(int type, int itemid, int action)
{
    if (type == 0xaa && itemid == 0x01)
    {
        if (action) *(AppData.canbus_inhibited) = 0;
        else *(AppData.canbus_inhibited) = 1;
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
    debug_msg(debug_string);
#endif

    if (type == BEZEL_BUTTON)
    {
        if (action == BEZEL_PRESSED) BezelButtonPressed(AppData.window->p_current, itemid);
        else if (action == BEZEL_RELEASED)
        { // Check all lists since BEZEL_PRESSED may have been on a different active page
            for (struct node *current = AppData.window->p_head; current; current=current->p_next_list)
                BezelButtonReleased(current, itemid);
        }
    }
}


static void BezelButtonPressed(struct node *list, int itemid)
{
    for (struct node *current = list; current; current = current->p_next)
    {
        switch (current->info.type)
        {
            case Container:
                BezelButtonPressed(current->object.cont.SubList, itemid);
                break;
            case Condition:
                if (CheckCondition(current))
                    BezelButtonPressed(current->object.cond.TrueList, itemid);
                else
                    BezelButtonPressed(current->object.cond.FalseList, itemid);
                break;
            case BezelEvent:
                if (current->object.be.key == itemid)
                {
                    current->info.selected = true;
                    ProcessEventList(current->object.be.PressList);
                }
                else current->info.selected = false;
                break;
            default:
                break;
        }
    }
}


static void BezelButtonReleased(struct node *list, int itemid)
{
    for (struct node *current = list; current; current = current->p_next)
    {
        switch (current->info.type)
        {
            case Container:
                BezelButtonReleased(current->object.cont.SubList, itemid);
                break;
            case Condition:
                BezelButtonReleased(current->object.cond.TrueList, itemid);
                BezelButtonReleased(current->object.cond.FalseList, itemid);
                break;
            case BezelEvent:
                if (current->info.selected) ProcessEventList(current->object.be.ReleaseList);
                break;
            default:
                break;
        }

        // Deselect all objects when releasing bezel button when in runtime.
        current->info.selected = false;
    }
}
