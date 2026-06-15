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

static int clamp_count(int count) {
    if (count < 8) return 8;
    if (count > MAX_STARS) return MAX_STARS;
    return count;
}

static void init_star(int index, bool at_top) {
    Star *star = &g_stars[index];
    float depth = rand_unit();
    float warm = rand_unit();

    star->x = FIELD_WIDTH * rand_unit();
    star->y = at_top ? FIELD_TOP + 30.0f * rand_unit() : FIELD_BOTTOM + FIELD_HEIGHT * rand_unit();
    star->speed = 30.0f + 120.0f * rand_unit();
    star->size = 1.0f + depth * 2.4f;

    if (warm < 0.16f) {
        star->color = (DcVec4){ .r = 1.0f, .g = 0.86f, .b = 0.58f, .a = 0.44f + depth * 0.44f };
    } else if (warm > 0.86f) {
        star->color = (DcVec4){ .r = 0.70f, .g = 0.84f, .b = 1.0f, .a = 0.42f + depth * 0.42f };
    } else {
        star->color = (DcVec4){ .r = 0.90f, .g = 0.94f, .b = 1.0f, .a = 0.38f + depth * 0.44f };
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

void display_init(DcAppContext *app_ctx) {
    (void)app_ctx;
    sync_star_count();
}

void display_draw(DcAppContext *app_ctx) {
    (void)app_ctx;
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

void display_close(DcAppContext *app_ctx) {
    (void)app_ctx;
}

void draw_procedural(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;
    if (!dc_draw) return;

    dc_draw->rect_filled(draw_ctx, (DcVec2){0.0f, FIELD_BOTTOM}, (DcVec2){FIELD_WIDTH, FIELD_HEIGHT}, (DcVec4){ .r = 0.018f, .g = 0.022f, .b = 0.032f, .a = 1.0f });

    int count = StarCount ? clamp_count(*StarCount) : 0;
    for (int i = 0; i < count; i++) {
        Star *star = &g_stars[i];

        dc_draw->circle_filled(draw_ctx, (DcVec2){star->x, star->y}, star->size, star->color);
        if ((i % 9) == 0) {
            float sparkle = star->size * 2.4f;
            dc_draw->line(draw_ctx, (DcVec2){star->x - sparkle, star->y}, (DcVec2){star->x + sparkle, star->y}, (DcStroke){ .color = star->color, .width = 0.8f });
            dc_draw->line(draw_ctx, (DcVec2){star->x, star->y - sparkle}, (DcVec2){star->x, star->y + sparkle}, (DcStroke){ .color = star->color, .width = 0.8f });
        }
    }

    dc_draw->rounded_rect_filled(draw_ctx, (DcVec2){22.0f, 520.0f}, (DcVec2){420.0f, 58.0f}, 8.0f, (DcVec4){ .r = 0.10f, .g = 0.11f, .b = 0.13f, .a = 0.62f });
    dc_draw->rounded_rect(draw_ctx, (DcVec2){22.0f, 520.0f}, (DcVec2){420.0f, 58.0f}, 8.0f, (DcStroke){ .color = (DcVec4){ .r = 0.44f, .g = 0.50f, .b = 0.58f, .a = 0.28f }, .width = 1.0f });
    dc_draw->text_ex(draw_ctx, (DcVec2){232.0f, 557.0f}, "C-generated starfield", (DcTextStyle){ .size = 18.0f, .color = (DcVec4){ .r = 0.90f, .g = 0.94f, .b = 1.0f, .a = 1.0f } }, (DcPlacement){ .local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE }, NULL);
    dc_draw->text_ex(draw_ctx, (DcVec2){232.0f, 535.0f}, "Each star is just position, speed, size, and color.", (DcTextStyle){ .size = 10.0f, .color = (DcVec4){ .r = 0.68f, .g = 0.76f, .b = 0.84f, .a = 1.0f } }, (DcPlacement){ .local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE }, NULL);
}
