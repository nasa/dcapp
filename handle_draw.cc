#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include "nodes.hh"
#include "geometry.hh"
#include "opengl_draw.hh"

extern void render_string(struct node *);
extern void draw_adi(struct node *);
extern void SwapBuffers(void);

extern appdata AppData;

static void render_primitives(struct node *);

extern int CheckCondition(struct node *);
extern void UpdateValue(struct node *);


/*********************************************************************************
 *
 *  The main draw routine.
 *
 *********************************************************************************/
void Draw(void)
{
    render_primitives(AppData.window->p_current);
    SwapBuffers();
    gettimeofday(&(AppData.last_update), NULL);
}

static void render_primitives(struct node *list)
{
    struct node *current, *sublist;
    Geometry geo;

    for (current = list; current != NULL; current = current->p_next)
    {
        switch (current->info.type)
        {
            case Panel:
                setup_panel(current->object.panel.orthoX, current->object.panel.orthoY, -1, 1, current->object.panel.color.R, current->object.panel.color.G, current->object.panel.color.B, current->object.panel.color.A);
                break;
            case Condition:
                if (CheckCondition(current))
                    render_primitives(current->object.cond.TrueList);
                else
                    render_primitives(current->object.cond.FalseList);
                break;
            case Container:
                geo = GetGeometry(current);
                container_start(geo.refx, geo.refy, geo.delx, geo.dely, (*(current->info.w))/(*(current->object.cont.vwidth)), (*(current->info.h))/(*(current->object.cont.vheight)), *(current->info.rotate));
                render_primitives(current->object.cont.SubList);
                container_end();
                break;
            case Line:
                line_start(current->object.line.linewidth, current->object.line.color.R, current->object.line.color.G, current->object.line.color.B, current->object.line.color.A);
                for (sublist = current->object.line.Vertices; sublist != NULL; sublist = sublist->p_next)
                {
                    geo = GetGeometry(sublist);
                    gfx_vertex(geo.refx, geo.refy);
                }
                line_end();
                break;
            case Polygon:
                if (current->object.poly.fill)
                {
                    polygon_fill_start(current->object.poly.FillColor.R, current->object.poly.FillColor.G, current->object.poly.FillColor.B, current->object.poly.FillColor.A);
                    for (sublist = current->object.poly.Vertices; sublist != NULL; sublist = sublist->p_next)
                    {
                        geo = GetGeometry(sublist);
                        gfx_vertex(geo.refx, geo.refy);
                    }
                    polygon_fill_end();
                }
                if (current->object.poly.outline)
                {
                    polygon_outline_start(current->object.poly.linewidth, current->object.poly.LineColor.R, current->object.poly.LineColor.G, current->object.poly.LineColor.B, current->object.poly.LineColor.A);
                    for (sublist = current->object.poly.Vertices; sublist != NULL; sublist = sublist->p_next)
                    {
                        geo = GetGeometry(sublist);
                        gfx_vertex(geo.refx, geo.refy);
                    }
                    polygon_outline_end();
                }
                break;
            case Rectangle:
                geo = GetGeometry(current);
                container_start(geo.refx, geo.refy, geo.delx, geo.dely, 1, 1, *(current->info.rotate));
                if (current->object.rect.fill) rectangle_fill(current->object.rect.FillColor.R, current->object.rect.FillColor.G, current->object.rect.FillColor.B, current->object.rect.FillColor.A,
                    0, 0, 0, geo.height, geo.width, geo.height, geo.width, 0);
                if (current->object.rect.outline) rectangle_outline(current->object.rect.linewidth, current->object.rect.LineColor.R, current->object.rect.LineColor.G, current->object.rect.LineColor.B, current->object.rect.LineColor.A,
                    0, 0, 0, geo.height, geo.width, geo.height, geo.width, 0);
                container_end();
                break;
            case Circle:
                geo = GetGeometry(current);
                if (current->object.circle.fill) circle_fill(geo.refx, geo.refy, *(current->object.circle.radius), current->object.circle.segments,
                    current->object.circle.FillColor.R, current->object.circle.FillColor.G, current->object.circle.FillColor.B, current->object.circle.FillColor.A);
                if (current->object.circle.outline) circle_outline(geo.refx, geo.refy, *(current->object.circle.radius), current->object.circle.segments,
                    current->object.circle.LineColor.R, current->object.circle.LineColor.G, current->object.circle.LineColor.B, current->object.circle.LineColor.A, current->object.circle.linewidth);
                break;
            case String:
                render_string(current);
                break;
            case ADI:
                draw_adi(current);
                break;
            case Image:
                geo = GetGeometry(current);
                container_start(geo.refx, geo.refy, geo.delx, geo.dely, 1, 1, *(current->info.rotate));
                draw_image(current->object.image.textureID, geo.width, geo.height);
                container_end();
                break;
            case PixelStream:
                geo = GetGeometry(current);
                container_start(geo.refx, geo.refy, geo.delx, geo.dely, 1, 1, *(current->info.rotate));
                draw_image(current->object.pixelstream.textureID, geo.width, geo.height);
                container_end();
                break;
            case SetValue:
                UpdateValue(current);
                break;
            default:
                break;
        }
    }
}
