#include <list>
#include "nodes.hh"

extern void ProcessEventList(struct node *);
extern bool CheckCondition(struct node *);

extern appdata AppData;

static void KeyPressed(struct node *, char);

static std::list<struct node *> actionList;

void HandleKeyboard(unsigned char key)
{
    KeyPressed(AppData.window.current_panel->object.panel.SubList, key);
AppData.toplevel->handleKeyboard(key);
    std::list<struct node *>::iterator action;
    for (action = actionList.begin(); action != actionList.end(); action++)
    {
        ProcessEventList(*action);
    }
    actionList.clear();
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
