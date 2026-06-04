#include "dcapp.h"
#include <math.h>

#define RIPPLE_COUNT 8

// DrawFunction 3 is the "full procedural panel" version. Unlike drawfunction1,
// the XML only creates a Panel; this file owns the visual state,
// animation, mouse input, and generated primitive counts.

typedef struct Ripple {
    float x;
    float y;
    float age;
    bool active;
} Ripple;

static float g_time;
static int g_next_ripple;
static Ripple g_ripples[RIPPLE_COUNT];

void display_init(void) {
    // Static globals start zeroed, so there is nothing to initialize here.
}

void display_draw(void) {
    // This logic callback runs once per frame. It advances the sample's
    // animation clock and ages out old click ripples.
    g_time += 1.0f / 60.0f;
    for (int i = 0; i < RIPPLE_COUNT; i++) {
        if (!g_ripples[i].active) continue;
        g_ripples[i].age += 1.0f / 60.0f;
        if (g_ripples[i].age > 1.45f) {
            g_ripples[i].active = false;
        }
    }
}

void display_close(void) {
    // No persistent resources to release.
}

void draw_procedural(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    if (!dc_draw || !dc_mouse) return;

    // The XML for this sample only creates a Panel and calls this function.
    // The changing primitive count, animation, and click interaction all live
    // here in C, where loops and state are cheap.
    //
    // Register the whole panel as a mouse target. The event query uses dcapp's
    // resolved topmost target, so XML and DrawFunction hit regions compete in
    // the same mouse event system.
    dc_mouse->rect(ctx, "ripple_panel", (DcVec2){0.0f, 0.0f}, (DcVec2){900.0f, 600.0f}, (DcPlacement){0});
    if (dc_mouse->pressed(ctx, "ripple_panel")) {
        const DcMouse *mouse = dc_mouse->get_state(ctx);
        if (!mouse) return;

        g_ripples[g_next_ripple] = (Ripple){
            .x = mouse->x,
            .y = mouse->y,
            .age = 0.0f,
            .active = true,
        };
        g_next_ripple = (g_next_ripple + 1) % RIPPLE_COUNT;
    }

    dc_draw->rect_filled(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){900.0f, 600.0f}, (DcVec4){
        .r = 0.040f,
        .g = 0.045f,
        .b = 0.052f,
        .a = 1.0f,
    });

    // ---------------------------------------------------------------------
    // Generated Square Field
    // ---------------------------------------------------------------------
    // The grid is intentionally not XML. The cell count, colors, sizes, and
    // ripple response are all products of loops and math.
    const int cols = 40;
    const int rows = 23;
    const float left = 54.0f;
    const float bottom = 112.0f;
    const float step = 20.0f;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            float cx = left + (float)x * step;
            float cy = bottom + (float)y * step;
            float nx = (float)x / (float)(cols - 1);
            float ny = (float)y / (float)(rows - 1);
            float v = 0.5f + 0.5f * sinf(nx * 14.0f + g_time * 1.6f + sinf(ny * 9.0f - g_time));
            float hit = 0.0f;

            // Each active click ripple contributes brightness and size to
            // cells near its expanding wavefront.
            for (int i = 0; i < RIPPLE_COUNT; i++) {
                if (!g_ripples[i].active) continue;
                float dx = cx - g_ripples[i].x;
                float dy = cy - g_ripples[i].y;
                float radius = g_ripples[i].age * 260.0f;
                float wave = 1.0f - fabsf(sqrtf(dx * dx + dy * dy) - radius) / 28.0f;
                if (wave > 0.0f) {
                    hit += wave * (1.0f - g_ripples[i].age / 1.45f);
                }
            }
            if (hit > 1.0f) hit = 1.0f;

            float size = 6.0f + v * 9.0f + hit * 9.0f;
            DcPlacement placement = {
                .local_align_x = DC_ALIGN_CENTER,
                .local_align_y = DC_ALIGN_MIDDLE,
            };
            dc_draw->rect_filled_ex(ctx, (DcVec2){cx, cy}, (DcVec2){size, size}, (DcVec4){
                .r = 0.08f + v * 0.18f + hit * 0.60f,
                .g = 0.18f + v * 0.42f + hit * 0.26f,
                .b = 0.30f + (1.0f - v) * 0.44f + hit * 0.12f,
                .a = 0.88f,
            }, placement, NULL);
        }
    }

    // Draw the ripple rings after the square field so the user can see where
    // each click is propagating from.
    for (int i = 0; i < RIPPLE_COUNT; i++) {
        if (!g_ripples[i].active) continue;
        float life = 1.0f - g_ripples[i].age / 1.45f;
        DcStroke ring = {
            .color = {
                .r = 0.78f,
                .g = 0.96f,
                .b = 1.0f,
                .a = life,
            },
            .width = 2.0f,
        };
        dc_draw->circle(ctx, (DcVec2){g_ripples[i].x, g_ripples[i].y}, g_ripples[i].age * 260.0f, ring);
    }

    // ---------------------------------------------------------------------
    // Signal Analyzer Strip
    // ---------------------------------------------------------------------
    // The container moves the local origin to the strip. The stencil clips the
    // generated bars to the same rounded shape, which is exactly the kind of
    // thing that is awkward to express compactly in XML.
    if (dc_draw->container_push(ctx, (DcVec2){54.0f, 24.0f}, (DcVec2){792.0f, 70.0f}, (DcVec2){792.0f, 70.0f})) {
        dc_draw->rounded_rect_filled(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){792.0f, 70.0f}, 7.0f, (DcVec4){
            .r = 0.070f,
            .g = 0.078f,
            .b = 0.088f,
            .a = 0.96f,
        });

        if (dc_draw->stencil_begin(ctx)) {
            dc_draw->stencil_add(ctx);
            dc_draw->rounded_rect_filled(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){792.0f, 70.0f}, 7.0f, dc_stencil_color());

            dc_draw->stencil_draw(ctx);
            // Ninety-six bars are generated from a formula. This keeps the XML
            // tiny while still drawing a dense, data-display-like element.
            for (int i = 0; i < 96; i++) {
                float u = (float)i / 95.0f;
                float h = 10.0f + 36.0f * (0.5f + 0.5f * sinf(u * 28.0f + g_time * 3.0f));
                h += 12.0f * (0.5f + 0.5f * cosf(u * 73.0f - g_time * 1.7f));
                dc_draw->rect_filled(ctx, (DcVec2){22.0f + u * 748.0f, 12.0f}, (DcVec2){5.0f, h}, (DcVec4){
                    .r = 0.25f + u * 0.55f,
                    .g = 0.82f - u * 0.25f,
                    .b = 0.90f,
                    .a = 0.90f,
                });
            }
            dc_draw->stencil_end(ctx);
        }

        dc_draw->rounded_rect(ctx, (DcVec2){0.0f, 0.0f}, (DcVec2){792.0f, 70.0f}, 7.0f, (DcStroke){
            .color = {
                .r = 0.30f,
                .g = 0.36f,
                .b = 0.42f,
                .a = 1.0f,
            },
            .width = 1.0f,
        });
        dc_draw->container_pop(ctx);
    }
}
