#include "dcapp.h"
#include <math.h>

#define POINT_COUNT 240
#define PI 3.14159265358979323846f

static float g_phase;

void display_init(DcAppContext *app_ctx) {
    (void)app_ctx;
}

void display_draw(DcAppContext *app_ctx) {
    (void)app_ctx;
    g_phase += 0.025f;
}

void display_close(DcAppContext *app_ctx) {
    (void)app_ctx;
}

void draw_lissajous(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    if (!dc_draw) return;

    DcVec2 points[POINT_COUNT];
    for (int i = 0; i < POINT_COUNT; i++) {
        float t = ((float)i / (float)(POINT_COUNT - 1)) * PI * 2.0f;
        points[i].x = 400.0f + sinf(t * 3.0f + g_phase) * 300.0f;
        points[i].y = 380.0f + sinf(t * 4.0f) * 300.0f;
    }

    dc_draw->line(draw_ctx, (DcVec2){100.0f, 380.0f}, (DcVec2){700.0f, 380.0f}, (DcStroke){ .color = (DcVec4){ .r = 0.18f, .g = 0.28f, .b = 0.34f, .a = 1.0f }, .width = 1.0f });
    dc_draw->line(draw_ctx, (DcVec2){400.0f, 80.0f}, (DcVec2){400.0f, 680.0f}, (DcStroke){ .color = (DcVec4){ .r = 0.18f, .g = 0.28f, .b = 0.34f, .a = 1.0f }, .width = 1.0f });
    dc_draw->polyline(draw_ctx, points, POINT_COUNT, (DcStroke){ .color = (DcVec4){ .r = 0.30f, .g = 0.80f, .b = 1.0f, .a = 1.0f }, .width = 2.0f });
}
