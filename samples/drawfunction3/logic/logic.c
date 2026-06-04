#include "dcapp.h"

#define MAX_STARS 180
#define FIELD_WIDTH 900.0f
#define FIELD_BOTTOM 92.0f
#define FIELD_TOP 600.0f
#define FIELD_HEIGHT (FIELD_TOP - FIELD_BOTTOM)

// DrawFunction 3 shows the hybrid pattern at scale. XML owns the stable count
// controls. C owns a generated star list and draws as many stars as requested.

typedef struct Star {
    float x;
    float y;
    float speed;
    float size;
    DcVec4 color;
} Star;

static Star g_stars[MAX_STARS];
static uint32_t g_rng = 0x31415926u;

static float rand_unit(void) {
    g_rng = g_rng * 1664525u + 1013904223u;
    return (float)((g_rng >> 8) & 0x00FFFFFFu) / 16777215.0f;
}

static float rand_range(float min_value, float max_value) {
    return min_value + (max_value - min_value) * rand_unit();
}

static int clamp_count(int count) {
    if (count < 8) return 8;
    if (count > MAX_STARS) return MAX_STARS;
    return count;
}

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

static DcTextStyle text_style(float size, DcVec4 color) {
    DcTextStyle out = {0};
    out.size = size;
    out.color = color;
    return out;
}

static DcPlacement local_center(void) {
    DcPlacement placement = {0};
    placement.local_align_x = DC_ALIGN_CENTER;
    placement.local_align_y = DC_ALIGN_MIDDLE;
    return placement;
}

static void init_star(int index, bool at_top) {
    Star *star = &g_stars[index];
    float depth = rand_unit();
    float warm = rand_unit();

    star->x = rand_range(0.0f, FIELD_WIDTH);
    star->y = at_top ? FIELD_TOP + rand_range(0.0f, 30.0f) : rand_range(FIELD_BOTTOM, FIELD_TOP);
    star->speed = rand_range(30.0f, 150.0f);
    star->size = 1.0f + depth * 2.4f;

    if (warm < 0.16f) {
        star->color = rgba(1.0f, 0.86f, 0.58f, 0.44f + depth * 0.44f);
    } else if (warm > 0.86f) {
        star->color = rgba(0.70f, 0.84f, 1.0f, 0.42f + depth * 0.42f);
    } else {
        star->color = rgba(0.90f, 0.94f, 1.0f, 0.38f + depth * 0.44f);
    }
}

static void sync_star_count(void) {
    if (!StarCount) return;

    int target_count = clamp_count(*StarCount);
    if (*StarCount != target_count) {
        *StarCount = target_count;
    }

    for (int i = 0; i < target_count; i++) {
        if (g_stars[i].size <= 0.0f) {
            init_star(i, false);
        }
    }
}

void display_init(void) {
    sync_star_count();
}

void display_draw(void) {
    sync_star_count();

    const float dt = 1.0f / 60.0f;
    int count = StarCount ? clamp_count(*StarCount) : 0;
    for (int i = 0; i < count; i++) {
        Star *star = &g_stars[i];
        star->y -= star->speed * dt;
        star->x += star->speed * 0.06f * dt;

        if (star->y < FIELD_BOTTOM) {
            init_star(i, true);
        } else if (star->x > FIELD_WIDTH) {
            star->x -= FIELD_WIDTH;
        }
    }
}

void display_close(void) {
}

void draw_procedural(DcDrawContext *ctx, const DcDrawFuncArgs *args) {
    (void)args;
    if (!dc_draw) return;

    dc_draw->rect_filled(ctx, (DcVec2){0.0f, FIELD_BOTTOM}, (DcVec2){FIELD_WIDTH, FIELD_HEIGHT}, rgba(0.018f, 0.022f, 0.032f, 1.0f));

    int count = StarCount ? clamp_count(*StarCount) : 0;
    for (int i = 0; i < count; i++) {
        Star *star = &g_stars[i];

        dc_draw->circle_filled(ctx, (DcVec2){star->x, star->y}, star->size, star->color);
        if ((i % 9) == 0) {
            float sparkle = star->size * 2.4f;
            dc_draw->line(ctx, (DcVec2){star->x - sparkle, star->y}, (DcVec2){star->x + sparkle, star->y}, stroke(star->color, 0.8f));
            dc_draw->line(ctx, (DcVec2){star->x, star->y - sparkle}, (DcVec2){star->x, star->y + sparkle}, stroke(star->color, 0.8f));
        }
    }

    dc_draw->rounded_rect_filled(ctx, (DcVec2){22.0f, 520.0f}, (DcVec2){420.0f, 58.0f}, 8.0f, rgba(0.10f, 0.11f, 0.13f, 0.62f));
    dc_draw->rounded_rect(ctx, (DcVec2){22.0f, 520.0f}, (DcVec2){420.0f, 58.0f}, 8.0f, stroke(rgba(0.44f, 0.50f, 0.58f, 0.28f), 1.0f));
    dc_draw->text_ex(ctx, (DcVec2){232.0f, 557.0f}, "C-generated starfield", text_style(18.0f, rgba(0.90f, 0.94f, 1.0f, 1.0f)), local_center(), NULL);
    dc_draw->text_ex(ctx, (DcVec2){232.0f, 535.0f}, "Each star is just position, speed, size, and color.", text_style(10.0f, rgba(0.68f, 0.76f, 0.84f, 1.0f)), local_center(), NULL);
}
