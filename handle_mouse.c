#include <math.h>
#include "nodes.h"
#include "mappings.h"
#include "geometry.h"

extern void ProcessEventList(struct node *);
extern int CheckCondition(struct node *);

extern appdata AppData;

static int CheckRegion(struct node *, float, float);
static void MouseDown(struct node *, float, float);
static void MouseUp(struct node *);


/*********************************************************************************
 *
 *  Handle mouse events
 *
 *********************************************************************************/
void HandleMouse(int button, int state, float xpct, float ypct, int modifier)
{
    struct node *current;

    if (state == MOUSE_DOWN)
    {
        // Scale cursor x and y with ortho x and y
        MouseDown(AppData.window->p_current, AppData.window->p_current->object.panel.orthoX * xpct, AppData.window->p_current->object.panel.orthoY * ypct);
    }

    if (state == MOUSE_UP)
    { // Check all lists since MOUSE_DOWN may have been on a different active page
        for (current = AppData.window->p_head; current != NULL; current=current->p_next_list)
            MouseUp(current);
    }
}


/*********************************************************************************
 *
 * Handle movement of the mouse when a mouse button is depressed.
 *
 *********************************************************************************/
void HandleMouseMotion(int x, int y)
{
}


/*********************************************************************************
 *
 * Handle movement of the mouse when a mouse button is not depressed.
 *
 *********************************************************************************/
void HandlePassiveMotion(int x, int y)
{
}


/*********************************************************************************
 *
 * Pass MOUSE_DOWN event through the primitives list
 *
 *********************************************************************************/
static void MouseDown(struct node *list, float x, float y)
{
    struct node *current;
    Geometry geo;
    float ang, originx, originy, tmpx, tmpy, finalx, finaly;

    for (current = list; current != NULL; current = current->p_next)
    {
        switch (current->info.type)
        {
            case Container:
                geo = GetGeometry(current);
                ang = (*(current->info.rotate)) * 0.01745329252;
                originx = geo.refx - ((geo.delx * cosf(-ang)) + (geo.dely * sinf(-ang)));
                originy = geo.refy - ((geo.dely * cosf(-ang)) - (geo.delx * sinf(-ang)));
                tmpx = (x - originx) * (*(current->object.cont.vwidth)) / (*(current->info.w));
                tmpy = (y - originy) * (*(current->object.cont.vheight)) / (*(current->info.h));
                finalx = (tmpx * cosf(ang)) + (tmpy * sinf(ang));
                finaly = (tmpy * cosf(ang)) - (tmpx * sinf(ang));
                MouseDown(current->object.cont.SubList, finalx, finaly);
                break;
            case Condition:
                if (CheckCondition(current))
                {
                    MouseDown(current->object.cond.TrueList, x, y);
                }
                else
                {
                    MouseDown(current->object.cond.FalseList, x, y);
                }
                break;
            case MouseEvent:
                if (CheckRegion(current, x, y))
                {
                    current->info.selected = 1;
                    ProcessEventList(current->object.me.PressList);
                }
                else current->info.selected = 0;
                break;
            default:
                break;
        }
    }
}


/*********************************************************************************
 *
 * Pass MOUSE_UP event through the primitives list
 *
 *********************************************************************************/
static void MouseUp(struct node *list)
{
    struct node *current;

    for (current = list; current != NULL; current = current->p_next)
    {
        switch (current->info.type)
        {
            case Container:
                MouseUp(current->object.cont.SubList);
                break;
            case Condition:
                MouseUp(current->object.cond.TrueList);
                MouseUp(current->object.cond.FalseList);
                break;
            case MouseEvent:
                if (current->info.selected) ProcessEventList(current->object.me.ReleaseList);
                break;
            default:
                break;
        }

        // Deselect all objects when releasing mouse button when in runtime.
        current->info.selected = 0;
    }
}


/*********************************************************************************
 *
 * Check if x and y are within a node's bounds
 *
 *********************************************************************************/
static int CheckRegion(struct node *current, float x, float y)
{
    Geometry geo = GetGeometry(current);

    if ((geo.left < x) && (x < geo.right) && (geo.bottom < y) && (y < geo.top)) return 1;
    else return 0;
}
