#include "dcapp.h"
#include <stdio.h>
#include <stdlib.h>

#define CLAVIUS_LAT -58.62
#define CLAVIUS_LON -14.73
#define CLAVIUS_RADIUS 115385.0f
#define SHACKLETON_LAT -89.67
#define SHACKLETON_LON 129.78
#define SHACKLETON_RADIUS 10460.0f

static DcPlanetHandle logic_planet;
static DcPlanetViewHandle logic_planet_view;
static int logic_texture_refresh = -1;

void display_init(DcAppContext *app_ctx) {
    const char *display_home = getenv("dcappDisplayHome");
    if (!display_home || display_home[0] == '\0') return;

    char data_path[4096] = {0};
    snprintf(data_path, sizeof(data_path), "%s/../../data/LDEM_45S_100M.planet.json", display_home);

    logic_planet = dc_planet->create_planet_with_id(app_ctx, "LogicMoon", (DcPlanetCreateInfo){
        .data_path = data_path,
        .mesh_cache_size = 128u * 1024u * 1024u,
    });
    if (logic_planet) {
        dc_planet->set_texture_geodetic(app_ctx, logic_planet, "../../assets/nasa-worm.png", -90.0, 180.0, TexMpp ? (float)*TexMpp : 2000.0f);
        logic_texture_refresh = TextureRefresh ? *TextureRefresh : -1;
        logic_planet_view = dc_planet->create_geodetic_view(app_ctx, logic_planet, 1024, 1024);
    }
}

void display_draw(DcAppContext *app_ctx) {
    if (!logic_planet || !TextureRefresh || !TexMpp) return;
    if (*TextureRefresh != logic_texture_refresh) {
        dc_planet->set_texture_geodetic(app_ctx, logic_planet, "../../assets/nasa-worm.png", -90.0, 180.0, (float)*TexMpp);
        logic_texture_refresh = *TextureRefresh;
    }
}

void display_close(DcAppContext *app_ctx) {
    (void)app_ctx;
}

void draw_logic_planet_view(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args) {
    (void)args;

    if (LogicReadout) {
        snprintf(*LogicReadout, sizeof(*LogicReadout), "lat/lon/ele: %.2f %.2f %.0f    rpy: 0.00 0.00 %.2f", *Latitude, *Longitude, *Elevation, *Heading);
    }

    const DcDrawArea *area = dc_draw->get_area(draw_ctx);
    DcVec2 size = {450.0f, 450.0f};
    if (area) {
        size.x = area->dimensions[0];
        size.y = area->dimensions[1];
    }

    if (!logic_planet_view) return;

    bool use_ortho = *UseOrtho != 0;

    DcDrawPlanetViewHandle view = dc_draw->planet_view_geodetic(
        draw_ctx,
        logic_planet_view,
        *Latitude,
        *Longitude,
        *Elevation,
        (DcVec3){.roll = 0.0f, .pitch = 0.0f, .yaw = (float)*Heading},
        60.0f,
        use_ortho,
        (DcVec2){0.0f, 0.0f},
        size,
        dc_place_top_left(),
        0);

    if (!view) return;

    dc_draw->planet_sphere_geodetic(draw_ctx, view, CLAVIUS_LAT, CLAVIUS_LON, 1000.0, CLAVIUS_RADIUS, (DcVec4){0.0f, 1.0f, 1.0f, 0.18f});
    dc_draw->planet_sphere_geodetic(draw_ctx, view, SHACKLETON_LAT, SHACKLETON_LON, 1000.0, SHACKLETON_RADIUS, (DcVec4){1.0f, 0.4f, 1.0f, 0.22f});
    dc_draw->planet_text_geodetic(draw_ctx, view, CLAVIUS_LAT, CLAVIUS_LON, 50000.0, "Clavius crater 230.77 km", 60000.0f, (DcVec4){0.0f, 1.0f, 1.0f, 1.0f});
    dc_draw->planet_text_geodetic(draw_ctx, view, SHACKLETON_LAT, SHACKLETON_LON, 50000.0, "Shackleton crater 20.92 km", 30000.0f, (DcVec4){1.0f, 0.4f, 1.0f, 1.0f});
}
