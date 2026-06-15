#include "dcapp.h"
#include <math.h>

// DrawFunction 2 is a compact API reference. The XML file only creates the
// window/panel and calls draw_reference_grid(); everything inside the grid is
// rendered from this C file.
//
// Each numbered cell maps to one draw_example_## function below. The intent is
// that you can look at the rendered cell, find the same number in this file,
// and see the smallest useful snippet for that API feature.

typedef void (*ExampleFn)(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args);

typedef struct Example {
    const char *label;
    ExampleFn draw;
} Example;

static DcTextureId g_image_texture;
static DcVec2      g_image_size;
static const char *g_image_status = "image not loaded";

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

// 01: Basic line drawing. The simple draw functions take coordinates directly
// in the current draw area; there is no placement/result metadata.
static void draw_example_01_line(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->line(draw_ctx, (DcVec2){24.0f, 30.0f}, (DcVec2){190.0f, 78.0f}, (DcStroke){ .color = (DcVec4){ .r = 0.62f, .g = 0.86f, .b = 1.0f, .a = 1.0f }, .width = 2.0f });
    dc_draw->line(draw_ctx, (DcVec2){28.0f, 78.0f}, (DcVec2){184.0f, 32.0f}, (DcStroke){ .color = (DcVec4){ .r = 0.30f, .g = 0.52f, .b = 0.74f, .a = 1.0f }, .width = 1.0f });
}

// 02: _ex functions add placement and optional result output. Here the line is
// defined around local zero, placed at the cell center, and rotated around its
// own center using pivot alignment.
static void draw_example_02_line_ex(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    DcPlacement placement = (DcPlacement){ .local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE };
    placement.pivot_align_x = DC_ALIGN_CENTER;
    placement.pivot_align_y = DC_ALIGN_MIDDLE;
    placement.rotation = phase_degrees(args) * 2.0f;
    dc_draw->line_ex(draw_ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){96.0f, 0.0f}, (DcStroke){ .color = (DcVec4){ .r = 0.74f, .g = 0.92f, .b = 1.0f, .a = 1.0f }, .width = 3.0f }, (DcVec2){110.0f, 58.0f}, placement, NULL);
}

// 03: A polyline draws connected line segments through every point.
static void draw_example_03_polyline(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {22.0f, 34.0f},
        {54.0f, 72.0f},
        {90.0f, 44.0f},
        {126.0f, 80.0f},
        {166.0f, 38.0f},
        {196.0f, 68.0f},
    };
    dc_draw->polyline(draw_ctx, points, 6, (DcStroke){ .color = (DcVec4){ .r = 0.54f, .g = 0.95f, .b = 1.0f, .a = 1.0f }, .width = 2.0f });
}

// 04: polyline_ex keeps the points local to the shape, then places the whole
// shape as one unit. That is useful when the point set should move/rotate
// without rewriting every point.
static void draw_example_04_polyline_ex(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {0.0f, 2.0f},
        {36.0f, 40.0f},
        {70.0f, 14.0f},
        {106.0f, 44.0f},
        {144.0f, 4.0f},
    };
    dc_draw->polyline_ex(draw_ctx, points, 5, (DcStroke){ .color = (DcVec4){ .r = 0.72f, .g = 0.96f, .b = 1.0f, .a = 1.0f }, .width = 2.0f }, (DcVec2){110.0f, 58.0f}, (DcPlacement){ .local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE }, NULL);
}

// 05: polygon outlines close the point list automatically.
static void draw_example_05_polygon(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {72.0f, 28.0f},
        {154.0f, 34.0f},
        {184.0f, 74.0f},
        {104.0f, 90.0f},
        {52.0f, 62.0f},
    };
    dc_draw->polygon(draw_ctx, points, 5, (DcStroke){ .color = (DcVec4){ .r = 0.72f, .g = 0.92f, .b = 1.0f, .a = 1.0f }, .width = 2.0f });
}

// 06: filled polygons use the same point list, but take a fill color instead
// of a stroke.
static void draw_example_06_polygon_filled(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {72.0f, 28.0f},
        {154.0f, 34.0f},
        {184.0f, 74.0f},
        {104.0f, 90.0f},
        {52.0f, 62.0f},
    };
    dc_draw->polygon_filled(draw_ctx, points, 5, (DcVec4){ .r = 0.20f, .g = 0.42f, .b = 0.58f, .a = 0.80f });
}

// 07: rounded polygons soften the corners by a radius. The radius is in the
// current draw area's coordinate system.
static void draw_example_07_rounded_polygon(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {72.0f, 28.0f},
        {150.0f, 28.0f},
        {188.0f, 60.0f},
        {112.0f, 88.0f},
        {44.0f, 58.0f},
    };
    dc_draw->rounded_polygon(draw_ctx, points, 5, 10.0f, (DcStroke){ .color = (DcVec4){ .r = 0.86f, .g = 0.78f, .b = 1.0f, .a = 1.0f }, .width = 2.0f });
}

// 08: filled rounded polygons follow the same shape rules as rounded_polygon(),
// but they render a filled surface.
static void draw_example_08_rounded_polygon_filled(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcVec2 points[] = {
        {72.0f, 28.0f},
        {150.0f, 28.0f},
        {188.0f, 60.0f},
        {112.0f, 88.0f},
        {44.0f, 58.0f},
    };
    dc_draw->rounded_polygon_filled(draw_ctx, points, 5, 10.0f, (DcVec4){ .r = 0.28f, .g = 0.28f, .b = 0.60f, .a = 0.82f });
}

// 09: quads are explicit four-point shapes. They are useful when a rectangle is
// not enough, but a full polygon point array would be noisy.
static void draw_example_09_quad(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->quad(draw_ctx, (DcVec2){54.0f, 28.0f}, (DcVec2){176.0f, 36.0f}, (DcVec2){154.0f, 86.0f}, (DcVec2){34.0f, 74.0f}, (DcStroke){ .color = (DcVec4){ .r = 0.92f, .g = 0.82f, .b = 1.0f, .a = 1.0f }, .width = 2.0f });
}

// 10: filled quads are the fill equivalent of quad().
static void draw_example_10_quad_filled(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->quad_filled(draw_ctx, (DcVec2){54.0f, 28.0f}, (DcVec2){176.0f, 36.0f}, (DcVec2){154.0f, 86.0f}, (DcVec2){34.0f, 74.0f}, (DcVec4){ .r = 0.48f, .g = 0.32f, .b = 0.70f, .a = 0.82f });
}

// 11: rounded quads give the four-point form the same corner treatment as
// rounded polygons.
static void draw_example_11_rounded_quad(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rounded_quad(draw_ctx, (DcVec2){54.0f, 30.0f}, (DcVec2){174.0f, 30.0f}, (DcVec2){182.0f, 80.0f}, (DcVec2){42.0f, 86.0f}, 12.0f, (DcStroke){ .color = (DcVec4){ .r = 0.94f, .g = 0.78f, .b = 1.0f, .a = 1.0f }, .width = 2.0f });
}

// 12: filled rounded quads are the filled form of rounded_quad().
static void draw_example_12_rounded_quad_filled(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rounded_quad_filled(draw_ctx, (DcVec2){54.0f, 30.0f}, (DcVec2){174.0f, 30.0f}, (DcVec2){182.0f, 80.0f}, (DcVec2){42.0f, 86.0f}, 12.0f, (DcVec4){ .r = 0.42f, .g = 0.24f, .b = 0.58f, .a = 0.88f });
}

// 13: rect() takes a bottom-left position and dimensions.
static void draw_example_13_rect(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rect(draw_ctx, (DcVec2){48.0f, 32.0f}, (DcVec2){124.0f, 56.0f}, (DcStroke){ .color = (DcVec4){ .r = 0.90f, .g = 0.82f, .b = 1.0f, .a = 1.0f }, .width = 2.0f });
}

// 14: rect_filled() is the simplest filled primitive for block shapes.
static void draw_example_14_rect_filled(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rect_filled(draw_ctx, (DcVec2){48.0f, 32.0f}, (DcVec2){124.0f, 56.0f}, (DcVec4){ .r = 0.34f, .g = 0.24f, .b = 0.62f, .a = 0.84f });
}

// 15: rounded_rect() is usually the easiest way to draw outlined panels,
// buttons, and badges from logic code.
static void draw_example_15_rounded_rect(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rounded_rect(draw_ctx, (DcVec2){44.0f, 30.0f}, (DcVec2){132.0f, 60.0f}, 12.0f, (DcStroke){ .color = (DcVec4){ .r = 0.84f, .g = 0.80f, .b = 1.0f, .a = 1.0f }, .width = 2.0f });
}

// 16: rounded_rect_filled() is the filled counterpart.
static void draw_example_16_rounded_rect_filled(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rounded_rect_filled(draw_ctx, (DcVec2){44.0f, 30.0f}, (DcVec2){132.0f, 60.0f}, 12.0f, (DcVec4){ .r = 0.26f, .g = 0.22f, .b = 0.52f, .a = 0.88f });
}

// 17: circle() draws the outline at center/radius.
static void draw_example_17_circle(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->circle(draw_ctx, (DcVec2){110.0f, 60.0f}, 34.0f, (DcStroke){ .color = (DcVec4){ .r = 0.70f, .g = 1.0f, .b = 0.76f, .a = 1.0f }, .width = 2.0f });
}

// 18: circle_filled() uses the same center/radius form with a fill color.
static void draw_example_18_circle_filled(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->circle_filled(draw_ctx, (DcVec2){110.0f, 60.0f}, 34.0f, (DcVec4){ .r = 0.20f, .g = 0.56f, .b = 0.34f, .a = 0.88f });
}

// 19: text_size() lets logic measure text before drawing it. This cell draws a
// measured rectangle first, then draws the text at the same placement.
static void draw_example_19_text_size(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const char *text = "text_size";
    DcTextStyle style = (DcTextStyle){ .size = 10.0f, .color = (DcVec4){ .r = 0.70f, .g = 0.96f, .b = 0.76f, .a = 1.0f } };
    DcVec2 size = dc_draw->text_size(draw_ctx, text, style);
    DcPlacement placement = (DcPlacement){ .local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE };

    dc_draw->rect_ex(draw_ctx, (DcVec2){110.0f, 60.0f}, size, (DcStroke){ .color = (DcVec4){ .r = 0.42f, .g = 0.64f, .b = 0.46f, .a = 1.0f }, .width = 1.0f }, placement, NULL);
    dc_draw->text_ex(draw_ctx, (DcVec2){110.0f, 60.0f}, text, style, placement, NULL);
}

// 20: text() is the simple path: position, string, style.
static void draw_example_20_text(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->text(draw_ctx, (DcVec2){38.0f, 64.0f}, "dc_draw->text", (DcTextStyle){ .size = 14.0f, .color = (DcVec4){ .r = 0.88f, .g = 1.0f, .b = 0.90f, .a = 1.0f } });
    dc_draw->text(draw_ctx, (DcVec2){38.0f, 42.0f}, "plain position", (DcTextStyle){ .size = 9.0f, .color = (DcVec4){ .r = 0.54f, .g = 0.72f, .b = 0.58f, .a = 1.0f } });
}

// 21: text_ex() adds placement. The guide line shows the actual anchor; the
// local-center placement centers the text on that anchor.
static void draw_example_21_text_ex(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->text_ex(draw_ctx, (DcVec2){110.0f, 60.0f}, "centered", (DcTextStyle){ .size = 13.0f, .color = (DcVec4){ .r = 0.88f, .g = 1.0f, .b = 0.90f, .a = 1.0f } }, (DcPlacement){ .local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE }, NULL);
    dc_draw->line(draw_ctx, (DcVec2){70.0f, 60.0f}, (DcVec2){150.0f, 60.0f}, (DcStroke){ .color = (DcVec4){ .r = 0.32f, .g = 0.46f, .b = 0.34f, .a = 0.8f }, .width = 1.0f });
}

// 22: dc_load_image() is called once in display_init(). The returned
// DcTextureId can then be reused by image() and image_ex() every frame.
static void draw_example_22_image(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    if (g_image_texture) {
        DcVec2 image_size = {72.0f, 44.0f};
        if (g_image_size.x > 0.0f && g_image_size.y > 0.0f) {
            image_size.y = image_size.x * g_image_size.y / g_image_size.x;
        }

        DcPlacement placement = (DcPlacement){ .local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE };
        placement.pivot_align_x = DC_ALIGN_CENTER;
        placement.pivot_align_y = DC_ALIGN_MIDDLE;
        placement.rotation      = phase_degrees(args);

        dc_draw->image(draw_ctx, g_image_texture, (DcVec2){30.0f, 38.0f}, image_size, (DcVec4){ .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f });
        dc_draw->image_ex(draw_ctx, g_image_texture, (DcVec2){150.0f, 60.0f}, image_size, (DcVec4){ .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 0.85f }, placement, NULL);
        dc_draw->text(draw_ctx, (DcVec2){32.0f, 20.0f}, "image()", (DcTextStyle){ .size = 8.0f, .color = (DcVec4){ .r = 0.62f, .g = 0.74f, .b = 0.82f, .a = 1.0f } });
        dc_draw->text_ex(draw_ctx, (DcVec2){150.0f, 20.0f}, "image_ex()", (DcTextStyle){ .size = 8.0f, .color = (DcVec4){ .r = 0.62f, .g = 0.74f, .b = 0.82f, .a = 1.0f } }, (DcPlacement){ .local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE }, NULL);
    } else {
        dc_draw->rounded_rect(draw_ctx, (DcVec2){48.0f, 34.0f}, (DcVec2){124.0f, 52.0f}, 8.0f, (DcStroke){ .color = (DcVec4){ .r = 0.70f, .g = 0.84f, .b = 0.92f, .a = 1.0f }, .width = 1.0f });
        dc_draw->text(draw_ctx, (DcVec2){12.0f, 18.0f}, g_image_status, (DcTextStyle){ .size = 8.5f, .color = (DcVec4){ .r = 0.54f, .g = 0.64f, .b = 0.70f, .a = 1.0f } });
    }
}

// 23: Placement combines local alignment, pivot alignment, and rotation. Local
// alignment centers the rectangle on the given position; pivot alignment makes
// the rotation happen around that same center.
static void draw_example_23_placement(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    DcPlacement placement = (DcPlacement){ .local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE };
    placement.pivot_align_x = DC_ALIGN_CENTER;
    placement.pivot_align_y = DC_ALIGN_MIDDLE;
    placement.rotation = phase_degrees(args) * 2.0f;
    dc_draw->rect_filled_ex(draw_ctx, (DcVec2){110.0f, 60.0f}, (DcVec2){62.0f, 22.0f}, (DcVec4){ .r = 0.86f, .g = 0.78f, .b = 0.30f, .a = 0.95f }, placement, NULL);
}

// 24: Every _ex draw can optionally return a DcDrawResult. The result contains
// the resolved draw area. Do not treat result.area.position as a local x/y; push
// the area when you want to draw children relative to the result.
static void draw_example_24_draw_result(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcDrawResult result = {0};
    dc_draw->circle_filled_ex(draw_ctx, (DcVec2){110.0f, 60.0f}, 34.0f, (DcVec4){ .r = 0.18f, .g = 0.44f, .b = 0.34f, .a = 0.84f }, (DcPlacement){0}, &result);
    if (dc_draw->container_push_area(draw_ctx, &result.area)) {
        const DcDrawArea *area = dc_draw->get_area(draw_ctx);
        float width = area ? area->dimensions[0] : 68.0f;
        float height = area ? area->dimensions[1] : 68.0f;
        DcStroke area_stroke = (DcStroke){ .color = (DcVec4){ .r = 0.94f, .g = 0.82f, .b = 0.28f, .a = 1.0f }, .width = 1.0f };

        dc_draw->rect(draw_ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){width, height}, area_stroke);
        dc_draw->line(draw_ctx, (DcVec2){0.0f, height * 0.5f}, (DcVec2){width, height * 0.5f}, area_stroke);
        dc_draw->line(draw_ctx, (DcVec2){width * 0.5f, 0.0f}, (DcVec2){width * 0.5f, height}, area_stroke);
        dc_draw->rect_filled_ex(draw_ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){10.0f, 10.0f}, (DcVec4){ .r = 0.94f, .g = 0.82f, .b = 0.28f, .a = 1.0f }, dc_place_center(), NULL);
        dc_draw->container_pop(draw_ctx);
    }
}

// 25: get_area() returns the current parent draw area. Because each grid cell
// is a container, the area here is the current cell's virtual coordinate space.
static void draw_example_25_get_area(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const DcDrawArea *area = dc_draw->get_area(draw_ctx);
    if (!area) return;
    dc_draw->text(draw_ctx, (DcVec2){30.0f, 64.0f}, "get_area(draw_ctx)", (DcTextStyle){ .size = 12.0f, .color = (DcVec4){ .r = 0.82f, .g = 0.94f, .b = 1.0f, .a = 1.0f } });
    dc_draw->text(draw_ctx, (DcVec2){30.0f, 44.0f}, "current parent area", (DcTextStyle){ .size = 8.5f, .color = (DcVec4){ .r = 0.54f, .g = 0.66f, .b = 0.74f, .a = 1.0f } });
}

// 26: container_push() creates a child draw area. All draw calls inside the
// push/pop pair use the container's local coordinate system.
static void draw_example_26_container_push(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    if (dc_draw->container_push(draw_ctx, (DcVec2){44.0f, 30.0f}, (DcVec2){132.0f, 66.0f}, (DcVec2){132.0f, 66.0f})) {
        dc_draw->rounded_rect_filled(draw_ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){132.0f, 66.0f}, 8.0f, (DcVec4){ .r = 0.24f, .g = 0.18f, .b = 0.14f, .a = 0.92f });
        dc_draw->rect_filled_ex(draw_ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){18.0f, 18.0f}, (DcVec4){ .r = 1.0f, .g = 0.74f, .b = 0.42f, .a = 1.0f }, dc_place_center(), NULL);
        dc_draw->container_pop(draw_ctx);
    }
}

// 27: container_push_ex() adds placement to a container. This is the container
// equivalent of the primitive _ex functions.
static void draw_example_27_container_push_ex(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    DcPlacement placement = (DcPlacement){ .local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE };
    placement.pivot_align_x = DC_ALIGN_CENTER;
    placement.pivot_align_y = DC_ALIGN_MIDDLE;
    placement.rotation = sinf(phase_radians(args)) * 10.0f;
    if (dc_draw->container_push_ex(draw_ctx, (DcVec2){110.0f, 60.0f}, (DcVec2){126.0f, 58.0f}, (DcVec2){126.0f, 58.0f}, placement, NULL)) {
        dc_draw->rounded_rect_filled(draw_ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){126.0f, 58.0f}, 8.0f, (DcVec4){ .r = 0.26f, .g = 0.18f, .b = 0.13f, .a = 0.90f });
        dc_draw->line(draw_ctx, (DcVec2){0.0f, 29.0f}, (DcVec2){126.0f, 29.0f}, (DcStroke){ .color = (DcVec4){ .r = 1.0f, .g = 0.74f, .b = 0.42f, .a = 1.0f }, .width = 2.0f });
        dc_draw->container_pop(draw_ctx);
    }
}

// 28: container_push_area() is the bridge between a result area and child
// drawing. Here a circle returns an area, then a square is centered inside it.
static void draw_example_28_container_push_area(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    DcDrawResult result = {0};
    dc_draw->circle_filled_ex(draw_ctx, (DcVec2){110.0f, 60.0f}, 34.0f, (DcVec4){ .r = 0.24f, .g = 0.18f, .b = 0.14f, .a = 0.92f }, (DcPlacement){0}, &result);
    if (dc_draw->container_push_area(draw_ctx, &result.area)) {
        dc_draw->rect_filled_ex(draw_ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){18.0f, 18.0f}, (DcVec4){ .r = 1.0f, .g = 0.74f, .b = 0.42f, .a = 1.0f }, dc_place_center(), NULL);
        dc_draw->container_pop(draw_ctx);
    }
}

// Stencil examples draw striped content so it is obvious which pixels made it
// through the stencil. The mask geometry itself uses dc_stencil_color(), which
// is the logic-side equivalent of XML's #_stencil_color_ constant.
static void draw_stripes(DcDrawContext *draw_ctx, DcVec2 position, DcVec2 size, DcVec4 a, DcVec4 b) {
    const int stripe_count = 7;
    float stripe_w = size.x / (float)stripe_count;
    for (int i = 0; i < stripe_count; i++) {
        DcVec4 stripe_color = (i % 2 == 0) ? a : b;
        dc_draw->rect_filled(draw_ctx, (DcVec2){position.x + stripe_w * (float)i, position.y}, (DcVec2){stripe_w + 1.0f, size.y}, stripe_color);
    }
}

// 29: stencil_add() writes mask geometry. stencil_draw() then renders normal
// content clipped to the accumulated mask.
static void draw_example_29_stencil_add(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rect(draw_ctx, (DcVec2){44.0f, 28.0f}, (DcVec2){132.0f, 64.0f}, (DcStroke){ .color = (DcVec4){ .r = 0.40f, .g = 0.28f, .b = 0.22f, .a = 0.55f }, .width = 1.0f });

    if (dc_draw->stencil_begin(draw_ctx)) {
        dc_draw->stencil_add(draw_ctx);
        dc_draw->rounded_rect_filled(draw_ctx, (DcVec2){56.0f, 34.0f}, (DcVec2){108.0f, 52.0f}, 12.0f, dc_stencil_color());

        dc_draw->stencil_draw(draw_ctx);
        draw_stripes(draw_ctx, (DcVec2){44.0f, 28.0f}, (DcVec2){132.0f, 64.0f}, (DcVec4){ .r = 0.98f, .g = 0.50f, .b = 0.28f, .a = 1.0f }, (DcVec4){ .r = 1.0f, .g = 0.72f, .b = 0.36f, .a = 1.0f });
        dc_draw->stencil_end(draw_ctx);
    }

    dc_draw->rounded_rect(draw_ctx, (DcVec2){56.0f, 34.0f}, (DcVec2){108.0f, 52.0f}, 12.0f, (DcStroke){ .color = (DcVec4){ .r = 1.0f, .g = 0.84f, .b = 0.56f, .a = 0.85f }, .width = 1.0f });
}

// 30: stencil_remove() subtracts from the current mask. The result is a ring:
// the larger circle was added, then the smaller circle was removed.
static void draw_example_30_stencil_remove(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rect(draw_ctx, (DcVec2){52.0f, 28.0f}, (DcVec2){116.0f, 64.0f}, (DcStroke){ .color = (DcVec4){ .r = 0.40f, .g = 0.28f, .b = 0.22f, .a = 0.55f }, .width = 1.0f });

    if (dc_draw->stencil_begin(draw_ctx)) {
        dc_draw->stencil_add(draw_ctx);
        dc_draw->circle_filled(draw_ctx, (DcVec2){110.0f, 60.0f}, 40.0f, dc_stencil_color());

        dc_draw->stencil_remove(draw_ctx);
        dc_draw->circle_filled(draw_ctx, (DcVec2){110.0f, 60.0f}, 18.0f, dc_stencil_color());

        dc_draw->stencil_draw(draw_ctx);
        draw_stripes(draw_ctx, (DcVec2){52.0f, 28.0f}, (DcVec2){116.0f, 64.0f}, (DcVec4){ .r = 0.92f, .g = 0.58f, .b = 0.28f, .a = 1.0f }, (DcVec4){ .r = 1.0f, .g = 0.76f, .b = 0.42f, .a = 1.0f });
        dc_draw->stencil_end(draw_ctx);
    }

    dc_draw->circle(draw_ctx, (DcVec2){110.0f, 60.0f}, 40.0f, (DcStroke){ .color = (DcVec4){ .r = 1.0f, .g = 0.84f, .b = 0.56f, .a = 0.85f }, .width = 1.0f });
    dc_draw->circle(draw_ctx, (DcVec2){110.0f, 60.0f}, 18.0f, (DcStroke){ .color = (DcVec4){ .r = 1.0f, .g = 0.84f, .b = 0.56f, .a = 0.85f }, .width = 1.0f });
}

// 31: Stencils can nest. The inner stencil is evaluated while the outer stencil
// is active, so the visible striped content is constrained by both masks.
static void draw_example_31_nested_stencil(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    dc_draw->rect(draw_ctx, (DcVec2){44.0f, 28.0f}, (DcVec2){132.0f, 68.0f}, (DcStroke){ .color = (DcVec4){ .r = 0.40f, .g = 0.28f, .b = 0.22f, .a = 0.55f }, .width = 1.0f });

    if (dc_draw->stencil_begin(draw_ctx)) {
        dc_draw->stencil_add(draw_ctx);
        dc_draw->rounded_rect_filled(draw_ctx, (DcVec2){52.0f, 34.0f}, (DcVec2){104.0f, 52.0f}, 10.0f, dc_stencil_color());

        dc_draw->stencil_draw(draw_ctx);
        if (dc_draw->stencil_begin(draw_ctx)) {
            dc_draw->stencil_add(draw_ctx);
            dc_draw->circle_filled(draw_ctx, (DcVec2){142.0f, 60.0f}, 42.0f, dc_stencil_color());

            dc_draw->stencil_draw(draw_ctx);
            draw_stripes(draw_ctx, (DcVec2){44.0f, 28.0f}, (DcVec2){132.0f, 68.0f}, (DcVec4){ .r = 0.96f, .g = 0.42f, .b = 0.26f, .a = 1.0f }, (DcVec4){ .r = 1.0f, .g = 0.68f, .b = 0.34f, .a = 1.0f });
            dc_draw->stencil_end(draw_ctx);
        }
        dc_draw->stencil_end(draw_ctx);
    }

    dc_draw->rounded_rect(draw_ctx, (DcVec2){52.0f, 34.0f}, (DcVec2){104.0f, 52.0f}, 10.0f, (DcStroke){ .color = (DcVec4){ .r = 1.0f, .g = 0.84f, .b = 0.56f, .a = 0.85f }, .width = 1.0f });
    dc_draw->circle(draw_ctx, (DcVec2){142.0f, 60.0f}, 42.0f, (DcStroke){ .color = (DcVec4){ .r = 1.0f, .g = 0.84f, .b = 0.56f, .a = 0.85f }, .width = 1.0f });
}

// 32: Mouse hit registration is separate from drawing. The ID ties the hit
// target to later event queries; multiple shapes can intentionally share an ID.
static void draw_example_32_mouse_rect(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const char *id = "drawfunction2_mouse_rect";
    dc_mouse->rect(draw_ctx, id, (DcVec2){48.0f, 34.0f}, (DcVec2){124.0f, 52.0f});
    DcVec4 fill = dc_mouse->hovered(draw_ctx, id) ? (DcVec4){ .r = 0.72f, .g = 0.38f, .b = 0.24f, .a = 0.94f } : (DcVec4){ .r = 0.34f, .g = 0.22f, .b = 0.16f, .a = 0.88f };
    dc_draw->rounded_rect_filled(draw_ctx, (DcVec2){48.0f, 34.0f}, (DcVec2){124.0f, 52.0f}, 8.0f, fill);
    dc_draw->text(draw_ctx, (DcVec2){12.0f, 18.0f}, "hover target", (DcTextStyle){ .size = 8.5f, .color = (DcVec4){ .r = 0.54f, .g = 0.64f, .b = 0.70f, .a = 1.0f } });
}

// 33: The mouse API has shape helpers that match common draw primitives. This
// registers a circular hit target and uses hovered() to change the fill.
static void draw_example_33_mouse_circle(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const char *id = "drawfunction2_mouse_circle";
    dc_mouse->circle(draw_ctx, id, (DcVec2){110.0f, 60.0f}, 34.0f);
    DcVec4 fill = dc_mouse->hovered(draw_ctx, id) ? (DcVec4){ .r = 0.72f, .g = 0.38f, .b = 0.24f, .a = 0.94f } : (DcVec4){ .r = 0.34f, .g = 0.22f, .b = 0.16f, .a = 0.88f };
    dc_draw->circle_filled(draw_ctx, (DcVec2){110.0f, 60.0f}, 34.0f, fill);
    dc_draw->text(draw_ctx, (DcVec2){12.0f, 18.0f}, "circle hit", (DcTextStyle){ .size = 8.5f, .color = (DcVec4){ .r = 0.54f, .g = 0.64f, .b = 0.70f, .a = 1.0f } });
}

// 34: Polygon hit targets use the same point data as polygon drawing. The same
// point array is used for both hit registration and rendering in this example.
static void draw_example_34_mouse_polygon(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const char *id = "drawfunction2_mouse_polygon";
    DcVec2 points[] = {
        {76.0f, 30.0f},
        {166.0f, 42.0f},
        {136.0f, 88.0f},
        {52.0f, 72.0f},
    };
    dc_mouse->polygon(draw_ctx, id, points, 4, (DcVec2){0.0f, 0.0f});
    DcVec4 fill = dc_mouse->hovered(draw_ctx, id) ? (DcVec4){ .r = 0.72f, .g = 0.38f, .b = 0.24f, .a = 0.94f } : (DcVec4){ .r = 0.34f, .g = 0.22f, .b = 0.16f, .a = 0.88f };
    dc_draw->polygon_filled(draw_ctx, points, 4, fill);
    dc_draw->text(draw_ctx, (DcVec2){12.0f, 18.0f}, "polygon hit", (DcTextStyle){ .size = 8.5f, .color = (DcVec4){ .r = 0.54f, .g = 0.64f, .b = 0.70f, .a = 1.0f } });
}

// 35: Event queries are ID based. Registration happens as the frame is drawn;
// event queries report the last resolved mouse state for that ID.
static void draw_example_35_mouse_events(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    const char *id = "drawfunction2_mouse_events";
    dc_mouse->rect(draw_ctx, id, (DcVec2){48.0f, 34.0f}, (DcVec2){124.0f, 52.0f});

    bool hovered = dc_mouse->hovered(draw_ctx, id);
    bool pressed = dc_mouse->pressed(draw_ctx, id);
    bool released = dc_mouse->released(draw_ctx, id);
    bool active = dc_mouse->active(draw_ctx, id);
    bool clicked = dc_mouse->clicked(draw_ctx, id);

    DcVec4 fill = (DcVec4){ .r = 0.34f, .g = 0.22f, .b = 0.16f, .a = 0.88f };
    const char *state = "hover/click/hold";
    if (released) {
        fill = (DcVec4){ .r = 0.92f, .g = 0.70f, .b = 0.30f, .a = 1.0f };
        state = "released";
    } else if (pressed) {
        fill = (DcVec4){ .r = 1.0f, .g = 0.86f, .b = 0.42f, .a = 1.0f };
        state = "pressed";
    } else if (clicked) {
        fill = (DcVec4){ .r = 0.96f, .g = 0.72f, .b = 0.34f, .a = 1.0f };
        state = "clicked";
    } else if (active) {
        fill = (DcVec4){ .r = 0.98f, .g = 0.62f, .b = 0.32f, .a = 1.0f };
        state = "active";
    } else if (hovered) {
        fill = (DcVec4){ .r = 0.72f, .g = 0.38f, .b = 0.24f, .a = 0.94f };
        state = "hovered";
    }

    dc_draw->rounded_rect_filled(draw_ctx, (DcVec2){48.0f, 34.0f}, (DcVec2){124.0f, 52.0f}, 8.0f, fill);
    dc_draw->text(draw_ctx, (DcVec2){12.0f, 18.0f}, state, (DcTextStyle){ .size = 8.5f, .color = (DcVec4){ .r = 0.54f, .g = 0.64f, .b = 0.70f, .a = 1.0f } });
}

// 36: DrawFunction args and global mouse state are also available. PHASE comes
// from XML, while get_state() exposes the current mouse snapshot.
static void draw_example_36_args_and_state(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    float phase = phase_degrees(args);
    float radius = 20.0f + 8.0f * (0.5f + 0.5f * sinf(phase_radians(args)));
    const DcMouse *mouse = dc_mouse->get_state(draw_ctx);
    dc_draw->circle_filled(draw_ctx, (DcVec2){70.0f, 60.0f}, radius, (DcVec4){ .r = 0.40f, .g = 0.62f, .b = 0.90f, .a = 0.90f });
    dc_draw->text(draw_ctx, (DcVec2){112.0f, 66.0f}, "Arg PHASE", (DcTextStyle){ .size = 9.0f, .color = (DcVec4){ .r = 0.88f, .g = 0.96f, .b = 1.0f, .a = 1.0f } });
    dc_draw->text(draw_ctx, (DcVec2){112.0f, 46.0f}, mouse && mouse->position_valid ? "mouse valid" : "mouse invalid", (DcTextStyle){ .size = 8.5f, .color = (DcVec4){ .r = 0.58f, .g = 0.68f, .b = 0.76f, .a = 1.0f } });
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

void display_init(DcAppContext *app_ctx) {
    if (!dc_texture) {
        g_image_status = "texture api unavailable";
        return;
    }

    g_image_texture = dc_load_image(app_ctx, "../../assets/nasa.png", &g_image_size);
    if (!g_image_texture) {
        g_image_status = "dc_load_image failed";
        return;
    }

    if (!dc_get_texture_size(app_ctx, g_image_texture, &g_image_size)) {
        g_image_status = "texture size unavailable";
    }
}

// PHASE is an XML variable passed to draw_reference_grid(). Incrementing it in
// display_draw() gives the C examples a tiny bit of animation without adding a
// separate timing API to the sample.
void display_draw(DcAppContext *app_ctx) {
    (void)app_ctx;
    if (!PHASE) return;
    *PHASE += 1.0;
}

void display_close(DcAppContext *app_ctx) {
    (void)app_ctx;
}

// The grid itself is also drawn from C. Each cell is a pushed container with a
// 220x120 virtual coordinate system, so every draw_example_## can use the same
// local coordinates no matter where the cell appears on screen.
void draw_reference_grid(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
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

        if (!dc_draw->container_push(draw_ctx, (DcVec2){x, y}, (DcVec2){cell_w, cell_h}, (DcVec2){220.0f, 120.0f})) {
            continue;
        }

        // Draw the card background and label before handing control to the
        // example. The example only needs to care about its own feature.
        float band = (float)row / 5.0f;
        dc_draw->rounded_rect_filled(draw_ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){220.0f, 120.0f}, 7.0f, (DcVec4){ .r = 0.075f + band * 0.025f, .g = 0.088f, .b = 0.105f + band * 0.020f, .a = 1.0f });
        dc_draw->rounded_rect(draw_ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){220.0f, 120.0f}, 7.0f, (DcStroke){ .color = (DcVec4){ .r = 0.24f, .g = 0.32f, .b = 0.40f, .a = 1.0f }, .width = 1.0f });
        dc_draw->text(draw_ctx, (DcVec2){10.0f, 100.0f}, examples[i].label, (DcTextStyle){ .size = 9.0f, .color = (DcVec4){ .r = 0.76f, .g = 0.88f, .b = 0.96f, .a = 1.0f } });
        examples[i].draw(draw_ctx, args);
        dc_draw->container_pop(draw_ctx);
    }
}
