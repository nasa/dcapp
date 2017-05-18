#include <list>
#include "nodes.hh"
#include "mappings.hh"

extern void ProcessEventList(struct node *);
extern bool CheckCondition(struct node *);
extern void toggle_fullscreen(void);

extern appdata AppData;

static void KeyPressed(struct node *, char);

static std::list<struct node *> actionList;


void HandleKeyboard(unsigned char key)
{
    // escape (0x1b) toggles fullscreen mode
    if (key == 0x1b) toggle_fullscreen();
    else
    {
        KeyPressed(AppData.window.current_panel->object.panel.SubList, key);
        std::list<struct node *>::iterator action;
        for (action = actionList.begin(); action != actionList.end(); action++)
        {
            ProcessEventList(*action);
        }
        actionList.clear();
    }
}


void HandleSpecialKeyboard(int key)
{
    switch (key)
    {
        case KEY_LEFT:  break;
        case KEY_RIGHT: break;
        case KEY_UP:    break;
        case KEY_DOWN:  break;
    }
}


static void KeyPressed(struct node *list, char key)
{
    for (struct node *current = list; current; current = current->p_next)
    {
        switch (current->info.type)
        {
            case Container:
                KeyPressed(current->object.cont.SubList, key);
                break;
            case Condition:
                if (CheckCondition(current))
                    KeyPressed(current->object.cond.TrueList, key);
                else
                    KeyPressed(current->object.cond.FalseList, key);
                break;
            case KeyboardEvent:
                if (current->object.ke.key == key)
                {
                    actionList.push_back(current->object.ke.PressList);
                    actionList.push_back(current->object.ke.ReleaseList);
                }
                break;
            default:
                break;
        }
    }
}
