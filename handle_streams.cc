#include "nodes.hh"

extern appdata AppData;

extern bool CheckCondition(struct node *);
extern void SetNeedsRedraw(void);

static void traverse_list(struct node *);

static unsigned passnum=0;

void UpdateStreams(void)
{
    passnum++;
    struct node *current = AppData.window.current_panel;
    if( current == nullptr )
        return;
    
    traverse_list(current->object.panel.SubList);
}

static void traverse_list(struct node *list)
{
    struct node *current;

    for (current = list; current; current = current->p_next)
    {
        switch (current->info.type)
        {
            case Condition:
                if (CheckCondition(current))
                    traverse_list(current->object.cond.TrueList);
                else
                    traverse_list(current->object.cond.FalseList);
                break;
            case Container:
                traverse_list(current->object.cont.SubList);
                break;
            case PixelStreamView:
                if (current->object.pixelstreamview.psi->frame_count != passnum)
                {
                    if (current->object.pixelstreamview.psi->psd->reader()) SetNeedsRedraw();
                    current->object.pixelstreamview.psi->frame_count = passnum;
                }
                break;
            default:
                break;
        }
    }
}
