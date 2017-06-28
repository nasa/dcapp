#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include "basicutils/timer.hh"
#include "nodes.hh"
#include "geometry.hh"
#include "opengl_draw.hh"

extern void render_string(struct node *);
extern void draw_adi(struct node *);
extern bool CheckCondition(struct node *);
extern void UpdateValue(struct node *);
extern void SwapBuffers(void);

static void render_primitives(struct node *);

extern appdata AppData;

/*********************************************************************************
 *
 *  The main draw routine.
 *
 *********************************************************************************/
void Draw(void)
{
    if( AppData.window.current_panel != nullptr )
    {
        struct node *current = AppData.window.current_panel;
        setup_panel(current->object.panel.orthoX, current->object.panel.orthoY, -1, 1, (*(current->object.panel.color.R)), (*(current->object.panel.color.G)), (*(current->object.panel.color.B)), (*(current->object.panel.color.A)));
        render_primitives(current->object.panel.SubList);
        SwapBuffers();
        AppData.last_update->restart();
    }
}

static void render_primitives(struct node *list)
{
    struct node *current, *sublist;
    Geometry geo;
    size_t nbytes, origbytes, newh, pad, padbytes, offset, offsetbytes;

    for (current = list; current; current = current->p_next)
    {
        switch (current->info.type)
        {
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
                line_start(current->object.line.linewidth, (*(current->object.line.color.R)), (*(current->object.line.color.G)), (*(current->object.line.color.B)), (*(current->object.line.color.A)));
                for (sublist = current->object.line.Vertices; sublist; sublist = sublist->p_next)
                {
                    geo = GetGeometry(sublist);
                    gfx_vertex(geo.refx, geo.refy);
                }
                line_end();
                break;
            case Polygon:
                if (current->object.poly.fill)
                {
                    polygon_fill_start((*(current->object.poly.FillColor.R)), (*(current->object.poly.FillColor.G)), (*(current->object.poly.FillColor.B)), (*(current->object.poly.FillColor.A)));
                    for (sublist = current->object.poly.Vertices; sublist; sublist = sublist->p_next)
                    {
                        geo = GetGeometry(sublist);
                        gfx_vertex(geo.refx, geo.refy);
                    }
                    polygon_fill_end();
                }
                if (current->object.poly.outline)
                {
                    polygon_outline_start(current->object.poly.linewidth, (*(current->object.poly.LineColor.R)), (*(current->object.poly.LineColor.G)), (*(current->object.poly.LineColor.B)), (*(current->object.poly.LineColor.A)));
                    for (sublist = current->object.poly.Vertices; sublist; sublist = sublist->p_next)
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
                if (current->object.rect.fill) rectangle_fill((*(current->object.rect.FillColor.R)), (*(current->object.rect.FillColor.G)), (*(current->object.rect.FillColor.B)), (*(current->object.rect.FillColor.A)),
                    0, 0, 0, geo.height, geo.width, geo.height, geo.width, 0);
                if (current->object.rect.outline) rectangle_outline(current->object.rect.linewidth, (*(current->object.rect.LineColor.R)), (*(current->object.rect.LineColor.G)), (*(current->object.rect.LineColor.B)), (*(current->object.rect.LineColor.A)),
                    0, 0, 0, geo.height, geo.width, geo.height, geo.width, 0);
                container_end();
                break;
            case Circle:
                geo = GetGeometry(current);
                if (current->object.circle.fill) circle_fill(geo.refx, geo.refy, *(current->object.circle.radius), current->object.circle.segments,
                    (*(current->object.circle.FillColor.R)), (*(current->object.circle.FillColor.G)), (*(current->object.circle.FillColor.B)), (*(current->object.circle.FillColor.A)));
                if (current->object.circle.outline) circle_outline(geo.refx, geo.refy, *(current->object.circle.radius), current->object.circle.segments,
                    (*(current->object.circle.LineColor.R)), (*(current->object.circle.LineColor.G)), (*(current->object.circle.LineColor.B)), (*(current->object.circle.LineColor.A)), current->object.circle.linewidth);
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
            case PixelStreamView:
                geo = GetGeometry(current);
                container_start(geo.refx, geo.refy, geo.delx, geo.dely, 1, 1, *(current->info.rotate));
                newh = (size_t)((float)(current->object.pixelstreamview.psi->psd->width) * (*(current->info.h)) / (*(current->info.w)));
                if (newh > current->object.pixelstreamview.psi->psd->height)
                {
                    origbytes = current->object.pixelstreamview.psi->psd->width * current->object.pixelstreamview.psi->psd->height * 3;
                    // overpad the pad by 1 so that no unfilled rows show up
                    pad = ((newh - current->object.pixelstreamview.psi->psd->height) / 2) + 1;
                    padbytes = current->object.pixelstreamview.psi->psd->width * pad * 3;
                    nbytes = (size_t)(current->object.pixelstreamview.psi->psd->width * newh * 3);
                    if (nbytes > current->object.pixelstreamview.memallocation)
                    {
                        current->object.pixelstreamview.pixels = realloc(current->object.pixelstreamview.pixels, nbytes);
                        current->object.pixelstreamview.memallocation = nbytes;
                    }
                    bzero(current->object.pixelstreamview.pixels, padbytes);
                    bzero((void *)((size_t)(current->object.pixelstreamview.pixels) + nbytes - padbytes), padbytes);
                    bcopy(current->object.pixelstreamview.psi->psd->pixels, (void *)((size_t)(current->object.pixelstreamview.pixels) + padbytes), origbytes);
                }
                else
                {
                    nbytes = (size_t)(current->object.pixelstreamview.psi->psd->width * current->object.pixelstreamview.psi->psd->height * 3);
                    if (nbytes > current->object.pixelstreamview.memallocation)
                    {
                        current->object.pixelstreamview.pixels = realloc(current->object.pixelstreamview.pixels, nbytes);
                        current->object.pixelstreamview.memallocation = nbytes;
                    }
                    bcopy(current->object.pixelstreamview.psi->psd->pixels, current->object.pixelstreamview.pixels, nbytes);
                }

                if (newh < current->object.pixelstreamview.psi->psd->height)
                {
                    offset = (current->object.pixelstreamview.psi->psd->height - newh) / 2;
                    offsetbytes = current->object.pixelstreamview.psi->psd->width * offset * 3;
                    set_texture(current->object.pixelstreamview.textureID, current->object.pixelstreamview.psi->psd->width, newh, (void *)((size_t)(current->object.pixelstreamview.pixels) + offsetbytes));
                }
                else
                    set_texture(current->object.pixelstreamview.textureID, current->object.pixelstreamview.psi->psd->width, newh, current->object.pixelstreamview.pixels);
                draw_image(current->object.pixelstreamview.textureID, geo.width, geo.height);
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
