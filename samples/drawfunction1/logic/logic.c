#include "dcapp.h"
#include <math.h>

// DrawFunction 1 is a small feature tour. XML owns the cards and labels; each
// DrawFunction owns one focused piece of C drawing.

static float phase_radians(const DcDrawFuncArgs *args) {
    double phase = 0.0;
    if (args && args->count > 0) {
        phase = args->values[0].value_double;
    }
    return (float)(phase * 3.14159265358979323846 / 180.0);
}

static float demo_radius(float t) {
    return 50.0f + 4.0f * sinf(t);
}

void display_init(void) {
    // PHASE is declared in XML, so the logic file has no setup work.
}

void display_draw(void) {
    if (!PHASE) return;
    *PHASE += 1.2;
}

void display_close(void) {
    // No persistent resources to release.
}

void draw_simple_circle(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    if (!dc_draw) return;

    // Plain primitive drawing: fill first, outline second.
    float t = phase_radians(args);
    float radius = demo_radius(t);

    dc_draw->circle_filled(ctx, (DcVec2){196.0f, 76.0f}, radius, (DcVec4){
        .r = 0.20f,
        .g = 0.58f,
        .b = 0.94f,
        .a = 0.92f,
    });
    dc_draw->circle(ctx, (DcVec2){196.0f, 76.0f}, radius, (DcStroke){
        .color = {
            .r = 0.82f,
            .g = 0.94f,
            .b = 1.0f,
            .a = 1.0f,
        },
        .width = 2.0f,
    });
}

void draw_labeled_circle(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    if (!dc_draw) return;

    // Logic-side text: the circle returns an area, then text is centered inside
    // that area without needing a separate XML Text node.
    float t = phase_radians(args);
    float radius = demo_radius(t);
    DcDrawResult circle_result = {0};

    dc_draw->circle_filled_ex(ctx, (DcVec2){196.0f, 74.0f}, radius, (DcVec4){
        .r = 0.46f,
        .g = 0.22f,
        .b = 0.62f,
        .a = 0.94f,
    }, (DcPlacement){0}, &circle_result);
    dc_draw->circle(ctx, (DcVec2){196.0f, 74.0f}, radius, (DcStroke){
        .color = {
            .r = 0.98f,
            .g = 0.78f,
            .b = 1.0f,
            .a = 1.0f,
        },
        .width = 2.0f,
    });

    if (dc_draw->container_push_area(ctx, &circle_result.area)) {
        dc_draw->text_ex(ctx, (DcVec2){0.0f, 0.0f}, "C Text", (DcTextStyle){
            .color = {
                .r = 0.98f,
                .g = 0.92f,
                .b = 1.0f,
                .a = 1.0f,
            },
            .size = 18.0f,
        }, dc_place_center(), NULL);
        dc_draw->container_pop(ctx);
    }
}

void draw_mouse_circle(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    if (!dc_draw || !dc_mouse) return;

    // Mouse registration is separate from drawing, so only the primitives that
    // care about input pay the hit-test cost.
    float t = phase_radians(args);
    float radius = demo_radius(t);
    const char *id = "drawfunction_sample_circle";

    dc_mouse->circle(ctx, id, (DcVec2){196.0f, 88.0f}, radius, (DcPlacement){0});

    if (dc_mouse->down(ctx, id)) {
        dc_draw->circle_filled(ctx, (DcVec2){196.0f, 88.0f}, radius, (DcVec4){
            .r = 0.22f,
            .g = 0.80f,
            .b = 0.44f,
            .a = 0.96f,
        });
    } else if (dc_mouse->hovered(ctx, id)) {
        dc_draw->circle_filled(ctx, (DcVec2){196.0f, 88.0f}, radius, (DcVec4){
            .r = 0.42f,
            .g = 0.78f,
            .b = 0.42f,
            .a = 0.92f,
        });
    } else {
        dc_draw->circle_filled(ctx, (DcVec2){196.0f, 88.0f}, radius, (DcVec4){
            .r = 0.20f,
            .g = 0.50f,
            .b = 0.28f,
            .a = 0.90f,
        });
    }

    dc_draw->circle(ctx, (DcVec2){196.0f, 88.0f}, radius, (DcStroke){
        .color = {
            .r = 0.72f,
            .g = 1.0f,
            .b = 0.76f,
            .a = 0.90f,
        },
        .width = 2.0f,
    });
}

void draw_aligned_area(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    if (!dc_draw) return;

    // The circle returns a resolved area. Pushing it lets the following shapes
    // align to the circle's local parent dimensions.
    float t = phase_radians(args);
    float radius = demo_radius(t);
    DcDrawResult circle_result = {0};

    dc_draw->circle_filled_ex(ctx, (DcVec2){196.0f, 78.0f}, radius, (DcVec4){
        .r = 0.18f,
        .g = 0.30f,
        .b = 0.42f,
        .a = 0.92f,
    }, (DcPlacement){0}, &circle_result);
    dc_draw->circle(ctx, (DcVec2){196.0f, 78.0f}, radius, (DcStroke){
        .color = {
            .r = 0.78f,
            .g = 0.92f,
            .b = 1.0f,
            .a = 0.95f,
        },
        .width = 2.0f,
    });

    if (dc_draw->container_push_area(ctx, &circle_result.area)) {
        const DcDrawArea *area = dc_draw->get_area(ctx);
        float w = area ? area->dimensions[0] : radius * 2.0f;
        float h = area ? area->dimensions[1] : radius * 2.0f;

        DcVec2 horizontal_start = {
            .x = 0.0f,
            .y = h * 0.5f,
        };
        DcVec2 horizontal_end = {
            .x = w,
            .y = h * 0.5f,
        };
        DcVec2 vertical_start = {
            .x = w * 0.5f,
            .y = 0.0f,
        };
        DcVec2 vertical_end = {
            .x = w * 0.5f,
            .y = h,
        };
        DcStroke axis_stroke = {
            .color = {
                .r = 0.54f,
                .g = 0.70f,
                .b = 0.82f,
                .a = 0.70f,
            },
            .width = 1.0f,
        };

        dc_draw->line(ctx, horizontal_start, horizontal_end, axis_stroke);
        dc_draw->line(ctx, vertical_start, vertical_end, axis_stroke);

        DcPlacement spin = dc_place_center();
        spin.pivot_align_x = DC_ALIGN_CENTER;
        spin.pivot_align_y = DC_ALIGN_MIDDLE;
        spin.rotation      = t * 25.0f;
        dc_draw->rect_filled_ex(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){22.0f, 22.0f}, (DcVec4){
            .r = 0.96f,
            .g = 0.84f,
            .b = 0.34f,
            .a = 0.95f,
        }, spin, NULL);

        DcPlacement left = dc_place_left();
        dc_draw->rect_filled_ex(ctx, (DcVec2){18.0f, 0.0f}, (DcVec2){12.0f, 12.0f}, (DcVec4){
            .r = 0.70f,
            .g = 0.90f,
            .b = 1.0f,
            .a = 0.90f,
        }, left, NULL);

        DcPlacement right = dc_place_right();
        dc_draw->rect_filled_ex(ctx, (DcVec2){-18.0f, 0.0f}, (DcVec2){12.0f, 12.0f}, (DcVec4){
            .r = 0.70f,
            .g = 0.90f,
            .b = 1.0f,
            .a = 0.90f,
        }, right, NULL);

        DcPlacement top = dc_place_top();
        dc_draw->rect_filled_ex(ctx, (DcVec2){0.0f, -18.0f}, (DcVec2){12.0f, 12.0f}, (DcVec4){
            .r = 0.70f,
            .g = 0.90f,
            .b = 1.0f,
            .a = 0.90f,
        }, top, NULL);

        DcPlacement bottom = dc_place_bottom();
        dc_draw->rect_filled_ex(ctx, (DcVec2){0.0f, 18.0f}, (DcVec2){12.0f, 12.0f}, (DcVec4){
            .r = 0.70f,
            .g = 0.90f,
            .b = 1.0f,
            .a = 0.90f,
        }, bottom, NULL);
        dc_draw->container_pop(ctx);
    }
}
