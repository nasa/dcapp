#include <stdio.h>
#include "nodes.hh"
#include "bezel.hh"

extern void ProcessEventList(struct node *);
extern int CheckCondition(struct node *);

extern appdata AppData;

static void BezelButtonPressed(struct node *, int);
static void BezelButtonReleased(struct node *, int);


void HandleBezelControl(int type, int itemid, int action)
{
    int ACTIVE_DISPLAY=0, PREVIOUS_DISPLAY=0;

    if (type == 0xaa && itemid == 0x01)
    {
        if (action) ACTIVE_DISPLAY = PREVIOUS_DISPLAY;
        else 
        {
            PREVIOUS_DISPLAY = ACTIVE_DISPLAY;
            ACTIVE_DISPLAY = 99;
        }
    }
}


void HandleBezelButton(int type, int itemid, int action)
{
    struct node *current;

#ifdef DEBUG
    printf("Type: ");
    switch (type)
    {
        case BEZEL_BUTTON:
            printf("Button");
            break;
        case BEZEL_KNOB:
            printf("Knob");
            break;
        default:
            printf("Unknown");
            break;
    }
    printf(", ID: %d, Action: ", itemid);
    switch (action)
    {
        case BEZEL_RELEASED:
            printf("Released\n");
            break;
        case BEZEL_PRESSED:
            printf("Pressed\n");
            break;
        case BEZEL_CLOCKWISE:
            printf("Clockwise Tick\n");
            break;
        case BEZEL_COUNTERCLOCKWISE:
            printf("Couterclockwise Tick\n");
            break;
        default:
            printf("Unknown\n");
            break;
    }
#endif

    if (type == BEZEL_BUTTON)
    {
        if (action == BEZEL_PRESSED)
        {
            BezelButtonPressed(AppData.window->p_current, itemid);
        }
        else if (action == BEZEL_RELEASED)
        { // Check all lists since BEZEL_PRESSED may have been on a different active page
            for (current = AppData.window->p_head; current != NULL; current=current->p_next_list)
                BezelButtonReleased(current, itemid);
        }
    }
}


static void BezelButtonPressed(struct node *list, int itemid)
{
    struct node *current;

    for (current = list; current != NULL; current = current->p_next)
    {
        switch (current->info.type)
        {
            case Container:
                BezelButtonPressed(current->object.cont.SubList, itemid);
                break;
            case Condition:
                if (CheckCondition(current))
                {
                    BezelButtonPressed(current->object.cond.TrueList, itemid);
                }
                else
                {
                    BezelButtonPressed(current->object.cond.FalseList, itemid);
                }
                break;
            case BezelEvent:
                if (current->object.be.key == itemid)
                {
                    current->info.selected = 1;
                    ProcessEventList(current->object.be.PressList);
                }
                else current->info.selected = 0;
                break;
            default:
                break;
        }
    }
}


static void BezelButtonReleased(struct node *list, int itemid)
{
    struct node *current;

    for (current = list; current != NULL; current = current->p_next)
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
        current->info.selected = 0;
    }
}
