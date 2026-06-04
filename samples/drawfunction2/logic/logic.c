#include "dcapp.h"
#include <math.h>

// DrawFunction 2 is a compact API reference. The XML file only creates the
// window/panel and calls draw_reference_grid(); everything inside the grid is
// rendered from this C file.
//
// Each numbered cell maps to one draw_example_## function below. The intent is
// that you can look at the rendered cell, find the same number in this file,
// and see the smallest useful snippet for that API feature.

typedef void (*ExampleFn)(DcDrawContext *ctx, const DcDrawFuncArgs *args);

typedef struct Example {
    const char *label;
    ExampleFn draw;
} Example;

static DcTextureId g_image_texture;
static DcVec2      g_image_size;
static const char *g_image_status = "image not loaded";

// The generated header exposes DcVec4 as a generic vector with rgba aliases.
// This helper keeps the examples short while still showing that colors are just
// normal values passed into the draw API.
static DcVec4 color(float r, float g, float b, float a) {
    DcVec4 out = {0};
    out.r = r;
    out.g = g;
    out.b = b;
    out.a = a;
    return out;
}

// Stroke controls the outline color and width for non-filled primitives.
// Fill primitives take a DcVec4 directly instead.
static DcStroke stroke(DcVec4 stroke_color, float width) {
    DcStroke out = {0};
    out.color = stroke_color;
    out.width = width;
    return out;
}

// Text rendering uses a style struct so the API can grow later without making
// text() calls unwieldy. Zeroed fields mean "not specified"; this sample sets
// size and color explicitly so the rendered result is predictable.
static DcTextStyle text_style(float size, DcVec4 text_color) {
    DcTextStyle out = {0};
    out.color = text_color;
    out.size = size;
    return out;
}

// The XML passes PHASE into draw_reference_grid() as a DrawFunction argument.
// A few cells use it to animate rotation or size, which proves args are live.
static float phase_degrees(const DcDrawFuncArgs *args) {
    if (!args || args->count == 0) return 0.0f;
    return (float)args->values[0].value_double;
}

// C's trig functions use radians. The XML-side PHASE variable is easier to
// think about as degrees, so examples convert it here when they need sine.
static float phase_radians(const DcDrawFuncArgs *args) {
    return phase_degrees(args) * 0.01745329251994329577f;
}

// Small explanatory text inside a cell. The on-screen label is intentionally
// tiny because the real documentation lives in this source file.
static void note(DcDrawContext *ctx, const char *text) {
    dc_draw->text(ctx, (DcVec2){12.0f, 18.0f}, text, text_style(8.5f, color(0.54f, 0.64f, 0.70f, 1.0f)));
}

// DcPlacement has separate parent/local alignment. For the common "place this
// object around its own center, but keep the given position local" case, only
// local alignment should be set. Using dc_place_center() here would also align
// to the parent center, which is not what most examples want.
static DcPlacement local_center(void) {
    DcPlacement placement = {0};
    placement.local_align_x = DC_ALIGN_CENTER;
    placement.local_align_y = DC_ALIGN_MIDDLE;
    return placement;
}

// 01: Basic line drawing. The simple draw functions take coordinates directly
// in the current draw area; there is no placement/result metadata.
static void draw_example_01_line(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->line(ctx, (DcVec2){24.0f, 30.0f}, (DcVec2){190.0f, 78.0f}, stroke(color(0.62f, 0.86f, 1.0f, 1.0f), 2.0f));
    dc_draw->line(ctx, (DcVec2){28.0f, 78.0f}, (DcVec2){184.0f, 32.0f}, stroke(color(0.30f, 0.52f, 0.74f, 1.0f), 1.0f));
}

// 02: _ex functions add placement and optional result output. Here the line is
// defined around local zero, placed at the cell center, and rotated around its
// own center using pivot alignment.
static void draw_example_02_line_ex(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    DcPlacement placement = local_center();
    placement.pivot_align_x = DC_ALIGN_CENTER;
    placement.pivot_align_y = DC_ALIGN_MIDDLE;
    placement.rotation = phase_degrees(args) * 2.0f;
    dc_draw->line_ex(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){96.0f, 0.0f}, stroke(color(0.74f, 0.92f, 1.0f, 1.0f), 3.0f), (DcVec2){110.0f, 58.0f}, placement, NULL);
}

// 03: A polyline draws connected line segments through every point.
static void draw_example_03_polyline(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {22.0f, 34.0f},
        {54.0f, 72.0f},
        {90.0f, 44.0f},
        {126.0f, 80.0f},
        {166.0f, 38.0f},
        {196.0f, 68.0f},
    };
    dc_draw->polyline(ctx, points, 6, stroke(color(0.54f, 0.95f, 1.0f, 1.0f), 2.0f));
}

// 04: polyline_ex keeps the points local to the shape, then places the whole
// shape as one unit. That is useful when the point set should move/rotate
// without rewriting every point.
static void draw_example_04_polyline_ex(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {0.0f, 2.0f},
        {36.0f, 40.0f},
        {70.0f, 14.0f},
        {106.0f, 44.0f},
        {144.0f, 4.0f},
    };
    dc_draw->polyline_ex(ctx, points, 5, stroke(color(0.72f, 0.96f, 1.0f, 1.0f), 2.0f), (DcVec2){110.0f, 58.0f}, local_center(), NULL);
}

// 05: polygon outlines close the point list automatically.
static void draw_example_05_polygon(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {72.0f, 28.0f},
        {154.0f, 34.0f},
        {184.0f, 74.0f},
        {104.0f, 90.0f},
        {52.0f, 62.0f},
    };
    dc_draw->polygon(ctx, points, 5, stroke(color(0.72f, 0.92f, 1.0f, 1.0f), 2.0f));
}

// 06: filled polygons use the same point list, but take a fill color instead
// of a stroke.
static void draw_example_06_polygon_filled(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {72.0f, 28.0f},
        {154.0f, 34.0f},
        {184.0f, 74.0f},
        {104.0f, 90.0f},
        {52.0f, 62.0f},
    };
    dc_draw->polygon_filled(ctx, points, 5, color(0.20f, 0.42f, 0.58f, 0.80f));
}

// 07: rounded polygons soften the corners by a radius. The radius is in the
// current draw area's coordinate system.
static void draw_example_07_rounded_polygon(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {72.0f, 28.0f},
        {150.0f, 28.0f},
        {188.0f, 60.0f},
        {112.0f, 88.0f},
        {44.0f, 58.0f},
    };
    dc_draw->rounded_polygon(ctx, points, 5, 10.0f, stroke(color(0.86f, 0.78f, 1.0f, 1.0f), 2.0f));
}

// 08: filled rounded polygons follow the same shape rules as rounded_polygon(),
// but they render a filled surface.
static void draw_example_08_rounded_polygon_filled(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {72.0f, 28.0f},
        {150.0f, 28.0f},
        {188.0f, 60.0f},
        {112.0f, 88.0f},
        {44.0f, 58.0f},
    };
    dc_draw->rounded_polygon_filled(ctx, points, 5, 10.0f, color(0.28f, 0.28f, 0.60f, 0.82f));
}

// 09: quads are explicit four-point shapes. They are useful when a rectangle is
// not enough, but a full polygon point array would be noisy.
static void draw_example_09_quad(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->quad(ctx, (DcVec2){54.0f, 28.0f}, (DcVec2){176.0f, 36.0f}, (DcVec2){154.0f, 86.0f}, (DcVec2){34.0f, 74.0f}, stroke(color(0.92f, 0.82f, 1.0f, 1.0f), 2.0f));
}

// 10: filled quads are the fill equivalent of quad().
static void draw_example_10_quad_filled(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->quad_filled(ctx, (DcVec2){54.0f, 28.0f}, (DcVec2){176.0f, 36.0f}, (DcVec2){154.0f, 86.0f}, (DcVec2){34.0f, 74.0f}, color(0.48f, 0.32f, 0.70f, 0.82f));
}

// 11: rounded quads give the four-point form the same corner treatment as
// rounded polygons.
static void draw_example_11_rounded_quad(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rounded_quad(ctx, (DcVec2){54.0f, 30.0f}, (DcVec2){174.0f, 30.0f}, (DcVec2){182.0f, 80.0f}, (DcVec2){42.0f, 86.0f}, 12.0f, stroke(color(0.94f, 0.78f, 1.0f, 1.0f), 2.0f));
}

// 12: filled rounded quads are the filled form of rounded_quad().
static void draw_example_12_rounded_quad_filled(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rounded_quad_filled(ctx, (DcVec2){54.0f, 30.0f}, (DcVec2){174.0f, 30.0f}, (DcVec2){182.0f, 80.0f}, (DcVec2){42.0f, 86.0f}, 12.0f, color(0.42f, 0.24f, 0.58f, 0.88f));
}

// 13: rect() takes a bottom-left position and dimensions.
static void draw_example_13_rect(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rect(ctx, (DcVec2){48.0f, 32.0f}, (DcVec2){124.0f, 56.0f}, stroke(color(0.90f, 0.82f, 1.0f, 1.0f), 2.0f));
}

// 14: rect_filled() is the simplest filled primitive for block shapes.
static void draw_example_14_rect_filled(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rect_filled(ctx, (DcVec2){48.0f, 32.0f}, (DcVec2){124.0f, 56.0f}, color(0.34f, 0.24f, 0.62f, 0.84f));
}

// 15: rounded_rect() is usually the easiest way to draw outlined panels,
// buttons, and badges from logic code.
static void draw_example_15_rounded_rect(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rounded_rect(ctx, (DcVec2){44.0f, 30.0f}, (DcVec2){132.0f, 60.0f}, 12.0f, stroke(color(0.84f, 0.80f, 1.0f, 1.0f), 2.0f));
}

// 16: rounded_rect_filled() is the filled counterpart.
static void draw_example_16_rounded_rect_filled(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rounded_rect_filled(ctx, (DcVec2){44.0f, 30.0f}, (DcVec2){132.0f, 60.0f}, 12.0f, color(0.26f, 0.22f, 0.52f, 0.88f));
}

// 17: circle() draws the outline at center/radius.
static void draw_example_17_circle(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->circle(ctx, (DcVec2){110.0f, 60.0f}, 34.0f, stroke(color(0.70f, 1.0f, 0.76f, 1.0f), 2.0f));
}

// 18: circle_filled() uses the same center/radius form with a fill color.
static void draw_example_18_circle_filled(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->circle_filled(ctx, (DcVec2){110.0f, 60.0f}, 34.0f, color(0.20f, 0.56f, 0.34f, 0.88f));
}

// 19: text_size() lets logic measure text before drawing it. This cell draws a
// measured rectangle first, then draws the text at the same placement.
static void draw_example_19_text_size(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const char *text = "text_size";
    DcTextStyle style = text_style(10.0f, color(0.70f, 0.96f, 0.76f, 1.0f));
    DcVec2 size = dc_draw->text_size(ctx, text, style);
    DcPlacement placement = local_center();

    dc_draw->rect_ex(ctx, (DcVec2){110.0f, 60.0f}, size, stroke(color(0.42f, 0.64f, 0.46f, 1.0f), 1.0f), placement, NULL);
    dc_draw->text_ex(ctx, (DcVec2){110.0f, 60.0f}, text, style, placement, NULL);
}

// 20: text() is the simple path: position, string, style.
static void draw_example_20_text(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->text(ctx, (DcVec2){38.0f, 64.0f}, "dc_draw->text", text_style(14.0f, color(0.88f, 1.0f, 0.90f, 1.0f)));
    dc_draw->text(ctx, (DcVec2){38.0f, 42.0f}, "plain position", text_style(9.0f, color(0.54f, 0.72f, 0.58f, 1.0f)));
}

// 21: text_ex() adds placement. The guide line shows the actual anchor; the
// local-center placement centers the text on that anchor.
static void draw_example_21_text_ex(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->text_ex(ctx, (DcVec2){110.0f, 60.0f}, "centered", text_style(13.0f, color(0.88f, 1.0f, 0.90f, 1.0f)), local_center(), NULL);
    dc_draw->line(ctx, (DcVec2){70.0f, 60.0f}, (DcVec2){150.0f, 60.0f}, stroke(color(0.32f, 0.46f, 0.34f, 0.8f), 1.0f));
}

// 22: dc_load_image() is called once in display_init(). The returned
// DcTextureId can then be reused by image() and image_ex() every frame.
static void draw_example_22_image(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    if (g_image_texture) {
        DcVec2 image_size = {72.0f, 44.0f};
        if (g_image_size.x > 0.0f && g_image_size.y > 0.0f) {
            image_size.y = image_size.x * g_image_size.y / g_image_size.x;
        }

        DcPlacement placement = local_center();
        placement.pivot_align_x = DC_ALIGN_CENTER;
        placement.pivot_align_y = DC_ALIGN_MIDDLE;
        placement.rotation      = phase_degrees(args);

        dc_draw->image(ctx, g_image_texture, (DcVec2){30.0f, 38.0f}, image_size, color(1.0f, 1.0f, 1.0f, 1.0f));
        dc_draw->image_ex(ctx, g_image_texture, (DcVec2){150.0f, 60.0f}, image_size, color(1.0f, 1.0f, 1.0f, 0.85f), placement, NULL);
        dc_draw->text(ctx, (DcVec2){32.0f, 20.0f}, "image()", text_style(8.0f, color(0.62f, 0.74f, 0.82f, 1.0f)));
        dc_draw->text_ex(ctx, (DcVec2){150.0f, 20.0f}, "image_ex()", text_style(8.0f, color(0.62f, 0.74f, 0.82f, 1.0f)), local_center(), NULL);
    } else {
        dc_draw->rounded_rect(ctx, (DcVec2){48.0f, 34.0f}, (DcVec2){124.0f, 52.0f}, 8.0f, stroke(color(0.70f, 0.84f, 0.92f, 1.0f), 1.0f));
        note(ctx, g_image_status);
    }
}

// 23: Placement combines local alignment, pivot alignment, and rotation. Local
// alignment centers the rectangle on the given position; pivot alignment makes
// the rotation happen around that same center.
static void draw_example_23_placement(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    DcPlacement placement = local_center();
    placement.pivot_align_x = DC_ALIGN_CENTER;
    placement.pivot_align_y = DC_ALIGN_MIDDLE;
    placement.rotation = phase_degrees(args) * 2.0f;
    dc_draw->rect_filled_ex(ctx, (DcVec2){110.0f, 60.0f}, (DcVec2){62.0f, 22.0f}, color(0.86f, 0.78f, 0.30f, 0.95f), placement, NULL);
}

// 24: Every _ex draw can optionally return a DcDrawResult. The result contains
// the resolved draw area. Do not treat result.area.position as a local x/y; push
// the area when you want to draw children relative to the result.
static void draw_example_24_draw_result(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcDrawResult result = {0};
    dc_draw->circle_filled_ex(ctx, (DcVec2){110.0f, 60.0f}, 34.0f, color(0.18f, 0.44f, 0.34f, 0.84f), (DcPlacement){0}, &result);
    if (dc_draw->container_push_area(ctx, &result.area)) {
        const DcDrawArea *area = dc_draw->get_area(ctx);
        float width = area ? area->dimensions[0] : 68.0f;
        float height = area ? area->dimensions[1] : 68.0f;
        DcStroke area_stroke = stroke(color(0.94f, 0.82f, 0.28f, 1.0f), 1.0f);

        dc_draw->rect(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){width, height}, area_stroke);
        dc_draw->line(ctx, (DcVec2){0.0f, height * 0.5f}, (DcVec2){width, height * 0.5f}, area_stroke);
        dc_draw->line(ctx, (DcVec2){width * 0.5f, 0.0f}, (DcVec2){width * 0.5f, height}, area_stroke);
        dc_draw->rect_filled_ex(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){10.0f, 10.0f}, color(0.94f, 0.82f, 0.28f, 1.0f), dc_place_center(), NULL);
        dc_draw->container_pop(ctx);
    }
}

// 25: get_area() returns the current parent draw area. Because each grid cell
// is a container, the area here is the current cell's virtual coordinate space.
static void draw_example_25_get_area(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const DcDrawArea *area = dc_draw->get_area(ctx);
    if (!area) return;
    dc_draw->text(ctx, (DcVec2){30.0f, 64.0f}, "get_area(ctx)", text_style(12.0f, color(0.82f, 0.94f, 1.0f, 1.0f)));
    dc_draw->text(ctx, (DcVec2){30.0f, 44.0f}, "current parent area", text_style(8.5f, color(0.54f, 0.66f, 0.74f, 1.0f)));
}

// 26: container_push() creates a child draw area. All draw calls inside the
// push/pop pair use the container's local coordinate system.
static void draw_example_26_container_push(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    if (dc_draw->container_push(ctx, (DcVec2){44.0f, 30.0f}, (DcVec2){132.0f, 66.0f}, (DcVec2){132.0f, 66.0f})) {
        dc_draw->rounded_rect_filled(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){132.0f, 66.0f}, 8.0f, color(0.24f, 0.18f, 0.14f, 0.92f));
        dc_draw->rect_filled_ex(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){18.0f, 18.0f}, color(1.0f, 0.74f, 0.42f, 1.0f), dc_place_center(), NULL);
        dc_draw->container_pop(ctx);
    }
}

// 27: container_push_ex() adds placement to a container. This is the container
// equivalent of the primitive _ex functions.
static void draw_example_27_container_push_ex(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    DcPlacement placement = local_center();
    placement.pivot_align_x = DC_ALIGN_CENTER;
    placement.pivot_align_y = DC_ALIGN_MIDDLE;
    placement.rotation = sinf(phase_radians(args)) * 10.0f;
    if (dc_draw->container_push_ex(ctx, (DcVec2){110.0f, 60.0f}, (DcVec2){126.0f, 58.0f}, (DcVec2){126.0f, 58.0f}, placement, NULL)) {
        dc_draw->rounded_rect_filled(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){126.0f, 58.0f}, 8.0f, color(0.26f, 0.18f, 0.13f, 0.90f));
        dc_draw->line(ctx, (DcVec2){0.0f, 29.0f}, (DcVec2){126.0f, 29.0f}, stroke(color(1.0f, 0.74f, 0.42f, 1.0f), 2.0f));
        dc_draw->container_pop(ctx);
    }
}

// 28: container_push_area() is the bridge between a result area and child
// drawing. Here a circle returns an area, then a square is centered inside it.
static void draw_example_28_container_push_area(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcDrawResult result = {0};
    dc_draw->circle_filled_ex(ctx, (DcVec2){110.0f, 60.0f}, 34.0f, color(0.24f, 0.18f, 0.14f, 0.92f), (DcPlacement){0}, &result);
    if (dc_draw->container_push_area(ctx, &result.area)) {
        dc_draw->rect_filled_ex(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){18.0f, 18.0f}, color(1.0f, 0.74f, 0.42f, 1.0f), dc_place_center(), NULL);
        dc_draw->container_pop(ctx);
    }
}

// Stencil examples draw striped content so it is obvious which pixels made it
// through the stencil. The mask geometry itself uses dc_stencil_color(), which
// is the logic-side equivalent of XML's #_stencil_color_ constant.
static void draw_stripes(DcDrawContext *ctx, DcVec2 position, DcVec2 size, DcVec4 a, DcVec4 b) {
    const int stripe_count = 7;
    float stripe_w = size.x / (float)stripe_count;
    for (int i = 0; i < stripe_count; i++) {
        DcVec4 stripe_color = (i % 2 == 0) ? a : b;
        dc_draw->rect_filled(ctx, (DcVec2){position.x + stripe_w * (float)i, position.y}, (DcVec2){stripe_w + 1.0f, size.y}, stripe_color);
    }
}

// 29: stencil_add() writes mask geometry. stencil_draw() then renders normal
// content clipped to the accumulated mask.
static void draw_example_29_stencil_add(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rect(ctx, (DcVec2){44.0f, 28.0f}, (DcVec2){132.0f, 64.0f}, stroke(color(0.40f, 0.28f, 0.22f, 0.55f), 1.0f));

    if (dc_draw->stencil_begin(ctx)) {
        dc_draw->stencil_add(ctx);
        dc_draw->rounded_rect_filled(ctx, (DcVec2){56.0f, 34.0f}, (DcVec2){108.0f, 52.0f}, 12.0f, dc_stencil_color());

        dc_draw->stencil_draw(ctx);
        draw_stripes(ctx, (DcVec2){44.0f, 28.0f}, (DcVec2){132.0f, 64.0f}, color(0.98f, 0.50f, 0.28f, 1.0f), color(1.0f, 0.72f, 0.36f, 1.0f));
        dc_draw->stencil_end(ctx);
    }

    dc_draw->rounded_rect(ctx, (DcVec2){56.0f, 34.0f}, (DcVec2){108.0f, 52.0f}, 12.0f, stroke(color(1.0f, 0.84f, 0.56f, 0.85f), 1.0f));
}

// 30: stencil_remove() subtracts from the current mask. The result is a ring:
// the larger circle was added, then the smaller circle was removed.
static void draw_example_30_stencil_remove(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rect(ctx, (DcVec2){52.0f, 28.0f}, (DcVec2){116.0f, 64.0f}, stroke(color(0.40f, 0.28f, 0.22f, 0.55f), 1.0f));

    if (dc_draw->stencil_begin(ctx)) {
        dc_draw->stencil_add(ctx);
        dc_draw->circle_filled(ctx, (DcVec2){110.0f, 60.0f}, 40.0f, dc_stencil_color());

        dc_draw->stencil_remove(ctx);
        dc_draw->circle_filled(ctx, (DcVec2){110.0f, 60.0f}, 18.0f, dc_stencil_color());

        dc_draw->stencil_draw(ctx);
        draw_stripes(ctx, (DcVec2){52.0f, 28.0f}, (DcVec2){116.0f, 64.0f}, color(0.92f, 0.58f, 0.28f, 1.0f), color(1.0f, 0.76f, 0.42f, 1.0f));
        dc_draw->stencil_end(ctx);
    }

    dc_draw->circle(ctx, (DcVec2){110.0f, 60.0f}, 40.0f, stroke(color(1.0f, 0.84f, 0.56f, 0.85f), 1.0f));
    dc_draw->circle(ctx, (DcVec2){110.0f, 60.0f}, 18.0f, stroke(color(1.0f, 0.84f, 0.56f, 0.85f), 1.0f));
}

// 31: Stencils can nest. The inner stencil is evaluated while the outer stencil
// is active, so the visible striped content is constrained by both masks.
static void draw_example_31_nested_stencil(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rect(ctx, (DcVec2){44.0f, 28.0f}, (DcVec2){132.0f, 68.0f}, stroke(color(0.40f, 0.28f, 0.22f, 0.55f), 1.0f));

    if (dc_draw->stencil_begin(ctx)) {
        dc_draw->stencil_add(ctx);
        dc_draw->rounded_rect_filled(ctx, (DcVec2){52.0f, 34.0f}, (DcVec2){104.0f, 52.0f}, 10.0f, dc_stencil_color());

        dc_draw->stencil_draw(ctx);
        if (dc_draw->stencil_begin(ctx)) {
            dc_draw->stencil_add(ctx);
            dc_draw->circle_filled(ctx, (DcVec2){142.0f, 60.0f}, 42.0f, dc_stencil_color());

            dc_draw->stencil_draw(ctx);
            draw_stripes(ctx, (DcVec2){44.0f, 28.0f}, (DcVec2){132.0f, 68.0f}, color(0.96f, 0.42f, 0.26f, 1.0f), color(1.0f, 0.68f, 0.34f, 1.0f));
            dc_draw->stencil_end(ctx);
        }
        dc_draw->stencil_end(ctx);
    }

    dc_draw->rounded_rect(ctx, (DcVec2){52.0f, 34.0f}, (DcVec2){104.0f, 52.0f}, 10.0f, stroke(color(1.0f, 0.84f, 0.56f, 0.85f), 1.0f));
    dc_draw->circle(ctx, (DcVec2){142.0f, 60.0f}, 42.0f, stroke(color(1.0f, 0.84f, 0.56f, 0.85f), 1.0f));
}

// 32: Mouse hit registration is separate from drawing. The ID ties the hit
// target to later event queries; multiple shapes can intentionally share an ID.
static void draw_example_32_mouse_rect(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const char *id = "drawfunction2_mouse_rect";
    dc_mouse->rect(ctx, id, (DcVec2){48.0f, 34.0f}, (DcVec2){124.0f, 52.0f});
    DcVec4 fill = dc_mouse->hovered(ctx, id) ? color(0.72f, 0.38f, 0.24f, 0.94f) : color(0.34f, 0.22f, 0.16f, 0.88f);
    dc_draw->rounded_rect_filled(ctx, (DcVec2){48.0f, 34.0f}, (DcVec2){124.0f, 52.0f}, 8.0f, fill);
    note(ctx, "hover target");
}

// 33: The mouse API has shape helpers that match common draw primitives. This
// registers a circular hit target and uses hovered() to change the fill.
static void draw_example_33_mouse_circle(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const char *id = "drawfunction2_mouse_circle";
    dc_mouse->circle(ctx, id, (DcVec2){110.0f, 60.0f}, 34.0f);
    DcVec4 fill = dc_mouse->hovered(ctx, id) ? color(0.72f, 0.38f, 0.24f, 0.94f) : color(0.34f, 0.22f, 0.16f, 0.88f);
    dc_draw->circle_filled(ctx, (DcVec2){110.0f, 60.0f}, 34.0f, fill);
    note(ctx, "circle hit");
}

// 34: Polygon hit targets use the same point data as polygon drawing. The same
// point array is used for both hit registration and rendering in this example.
static void draw_example_34_mouse_polygon(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const char *id = "drawfunction2_mouse_polygon";
    DcVec2 points[] = {
        {76.0f, 30.0f},
        {166.0f, 42.0f},
        {136.0f, 88.0f},
        {52.0f, 72.0f},
    };
    dc_mouse->polygon(ctx, id, points, 4, (DcVec2){0.0f, 0.0f});
    DcVec4 fill = dc_mouse->hovered(ctx, id) ? color(0.72f, 0.38f, 0.24f, 0.94f) : color(0.34f, 0.22f, 0.16f, 0.88f);
    dc_draw->polygon_filled(ctx, points, 4, fill);
    note(ctx, "polygon hit");
}

// 35: Event queries are ID based. Registration happens as the frame is drawn;
// event queries report the last resolved mouse state for that ID.
static void draw_example_35_mouse_events(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const char *id = "drawfunction2_mouse_events";
    dc_mouse->rect(ctx, id, (DcVec2){48.0f, 34.0f}, (DcVec2){124.0f, 52.0f});

    bool hovered = dc_mouse->hovered(ctx, id);
    bool pressed = dc_mouse->pressed(ctx, id);
    bool released = dc_mouse->released(ctx, id);
    bool active = dc_mouse->active(ctx, id);
    bool clicked = dc_mouse->clicked(ctx, id);

    DcVec4 fill = color(0.34f, 0.22f, 0.16f, 0.88f);
    const char *state = "hover/click/hold";
    if (released) {
        fill = color(0.92f, 0.70f, 0.30f, 1.0f);
        state = "released";
    } else if (pressed) {
        fill = color(1.0f, 0.86f, 0.42f, 1.0f);
        state = "pressed";
    } else if (clicked) {
        fill = color(0.96f, 0.72f, 0.34f, 1.0f);
        state = "clicked";
    } else if (active) {
        fill = color(0.98f, 0.62f, 0.32f, 1.0f);
        state = "active";
    } else if (hovered) {
        fill = color(0.72f, 0.38f, 0.24f, 0.94f);
        state = "hovered";
    }

    dc_draw->rounded_rect_filled(ctx, (DcVec2){48.0f, 34.0f}, (DcVec2){124.0f, 52.0f}, 8.0f, fill);
    note(ctx, state);
}

// 36: DrawFunction args and global mouse state are also available. PHASE comes
// from XML, while get_state() exposes the current mouse snapshot.
static void draw_example_36_args_and_state(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    float phase = phase_degrees(args);
    float radius = 20.0f + 8.0f * (0.5f + 0.5f * sinf(phase_radians(args)));
    const DcMouse *mouse = dc_mouse->get_state(ctx);
    dc_draw->circle_filled(ctx, (DcVec2){70.0f, 60.0f}, radius, color(0.40f, 0.62f, 0.90f, 0.90f));
    dc_draw->text(ctx, (DcVec2){112.0f, 66.0f}, "Arg PHASE", text_style(9.0f, color(0.88f, 0.96f, 1.0f, 1.0f)));
    dc_draw->text(ctx, (DcVec2){112.0f, 46.0f}, mouse && mouse->position_valid ? "mouse valid" : "mouse invalid", text_style(8.5f, color(0.58f, 0.68f, 0.76f, 1.0f)));
}

// The table is the source of truth for the rendered reference grid. Adding a
// new example means adding the draw function above and one row here.
static const Example examples[] = {
    {"01 line", draw_example_01_line},
    {"02 line_ex", draw_example_02_line_ex},
    {"03 polyline", draw_example_03_polyline},
    {"04 polyline_ex", draw_example_04_polyline_ex},
    {"05 polygon", draw_example_05_polygon},
    {"06 polygon_filled", draw_example_06_polygon_filled},
    {"07 rounded_polygon", draw_example_07_rounded_polygon},
    {"08 rounded_polygon_filled", draw_example_08_rounded_polygon_filled},
    {"09 quad", draw_example_09_quad},
    {"10 quad_filled", draw_example_10_quad_filled},
    {"11 rounded_quad", draw_example_11_rounded_quad},
    {"12 rounded_quad_filled", draw_example_12_rounded_quad_filled},
    {"13 rect", draw_example_13_rect},
    {"14 rect_filled", draw_example_14_rect_filled},
    {"15 rounded_rect", draw_example_15_rounded_rect},
    {"16 rounded_rect_filled", draw_example_16_rounded_rect_filled},
    {"17 circle", draw_example_17_circle},
    {"18 circle_filled", draw_example_18_circle_filled},
    {"19 text_size", draw_example_19_text_size},
    {"20 text", draw_example_20_text},
    {"21 text_ex", draw_example_21_text_ex},
    {"22 image/image_ex", draw_example_22_image},
    {"23 placement", draw_example_23_placement},
    {"24 draw result", draw_example_24_draw_result},
    {"25 get_area", draw_example_25_get_area},
    {"26 container_push", draw_example_26_container_push},
    {"27 container_push_ex", draw_example_27_container_push_ex},
    {"28 container_push_area", draw_example_28_container_push_area},
    {"29 stencil_add", draw_example_29_stencil_add},
    {"30 stencil_remove", draw_example_30_stencil_remove},
    {"31 nested stencil", draw_example_31_nested_stencil},
    {"32 mouse rect", draw_example_32_mouse_rect},
    {"33 mouse circle", draw_example_33_mouse_circle},
    {"34 mouse polygon", draw_example_34_mouse_polygon},
    {"35 mouse events", draw_example_35_mouse_events},
    {"36 args/state", draw_example_36_args_and_state},
};

void display_init(void) {
    if (!dc_texture) {
        g_image_status = "texture api unavailable";
        return;
    }

    g_image_texture = dc_load_image("../../assets/nasa.png", &g_image_size);
    if (!g_image_texture) {
        g_image_status = "dc_load_image failed";
        return;
    }

    if (!dc_get_texture_size(g_image_texture, &g_image_size)) {
        g_image_status = "texture size unavailable";
    }
}

// PHASE is an XML variable passed to draw_reference_grid(). Incrementing it in
// display_draw() gives the C examples a tiny bit of animation without adding a
// separate timing API to the sample.
void display_draw(void) {
    if (!PHASE) return;
    *PHASE += 1.0;
}

void display_close(void) {
}

// The grid itself is also drawn from C. Each cell is a pushed container with a
// 220x120 virtual coordinate system, so every draw_example_## can use the same
// local coordinates no matter where the cell appears on screen.
void draw_reference_grid(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    if (!dc_draw || !dc_mouse) return;

    const int columns = 6;
    const int count = (int)(sizeof(examples) / sizeof(examples[0]));
    const float cell_w = 160.0f;
    const float cell_h = 90.0f;
    const float step_x = 174.0f;
    const float step_y = 104.0f;
    const float start_x = 34.0f;
    const float start_y = 590.0f;

    for (int i = 0; i < count; i++) {
        int col = i % columns;
        int row = i / columns;
        float x = start_x + (float)col * step_x;
        float y = start_y - (float)row * step_y;

        if (!dc_draw->container_push(ctx, (DcVec2){x, y}, (DcVec2){cell_w, cell_h}, (DcVec2){220.0f, 120.0f})) {
            continue;
        }

        // Draw the card background and label before handing control to the
        // example. The example only needs to care about its own feature.
        float band = (float)row / 5.0f;
        dc_draw->rounded_rect_filled(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){220.0f, 120.0f}, 7.0f, color(0.075f + band * 0.025f, 0.088f, 0.105f + band * 0.020f, 1.0f));
        dc_draw->rounded_rect(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){220.0f, 120.0f}, 7.0f, stroke(color(0.24f, 0.32f, 0.40f, 1.0f), 1.0f));
        dc_draw->text(ctx, (DcVec2){10.0f, 100.0f}, examples[i].label, text_style(9.0f, color(0.76f, 0.88f, 0.96f, 1.0f)));
        examples[i].draw(ctx, args);
        dc_draw->container_pop(ctx);
    }
}
