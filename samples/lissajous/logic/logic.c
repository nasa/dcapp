#include "dcapp.h"
#include <math.h>

#define POINT_COUNT 240
#define PI 3.14159265358979323846f

static float g_phase;

static DcVec4 rgba(float r, float g, float b, float a) {
    DcVec4 out = {0};
    out.r = r;
    out.g = g;
    out.b = b;
    out.a = a;
    return out;
}

static DcStroke stroke(DcVec4 color, float width) {
    DcStroke out = {0};
    out.color = color;
    out.width = width;
    return out;
}

void display_init(void) {
}

void display_draw(void) {
    g_phase += 0.025f;
}

void display_close(void) {
}

void draw_lissajous(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    if (!dc_draw) return;

    DcVec2 points[POINT_COUNT];
    for (int i = 0; i < POINT_COUNT; i++) {
        float t = ((float)i / (float)(POINT_COUNT - 1)) * PI * 2.0f;
        points[i].x = 400.0f + sinf(t * 3.0f + g_phase) * 300.0f;
        points[i].y = 380.0f + sinf(t * 4.0f) * 300.0f;
    }

    dc_draw->line(ctx, (DcVec2){100.0f, 380.0f}, (DcVec2){700.0f, 380.0f}, stroke(rgba(0.18f, 0.28f, 0.34f, 1.0f), 1.0f));
    dc_draw->line(ctx, (DcVec2){400.0f, 80.0f}, (DcVec2){400.0f, 680.0f}, stroke(rgba(0.18f, 0.28f, 0.34f, 1.0f), 1.0f));
    dc_draw->polyline(ctx, points, POINT_COUNT, stroke(rgba(0.30f, 0.80f, 1.0f, 1.0f), 2.0f));
}
