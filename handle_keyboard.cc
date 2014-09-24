#include "nodes.hh"
#include "mappings.hh"

extern void ProcessEventList(struct node *);
extern int CheckCondition(struct node *);
extern void toggle_fullscreen(void);

extern appdata AppData;

static void KeyPressed(struct node *, char);


/*********************************************************************************
 *
 *  Handle regular keyboard events.
 *
 *********************************************************************************/
void HandleKeyboard(unsigned char key, int x, int y)
{
    // escape 0x1b toggles fullscreen mode
    if (key == 0x1b) toggle_fullscreen();
    else KeyPressed(AppData.window->p_current, key);
}

/*********************************************************************************
 *
 *  Handle keyboard events like the arrow keys and function keys.
 *
 *********************************************************************************/
void HandleSpecialKeyboard(int key, int x, int y)
{
    switch (key)
    {
        case KEY_LEFT:
            break;
        case KEY_RIGHT:
            break;
        case KEY_UP:
            break;
        case KEY_DOWN:
            break;
    }
}

static void KeyPressed(struct node *list, char key)
{
    struct node *current;

    for (current = list; current != NULL; current = current->p_next)
    {
        switch (current->info.type)
        {
            case Container:
                KeyPressed(current->object.cont.SubList, key);
                break;
            case Condition:
                if (CheckCondition(current))
                {
                    KeyPressed(current->object.cond.TrueList, key);
                }
                else
                {
                    KeyPressed(current->object.cond.FalseList, key);
                }
                break;
            case KeyboardEvent:
                if (current->object.ke.key == key)
                {
                    ProcessEventList(current->object.ke.PressList);
                    ProcessEventList(current->object.ke.ReleaseList);
                }
                break;
            default:
                break;
        }
    }
}
