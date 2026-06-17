#include "dcapp.h"
#include <stdio.h>
#include <stdlib.h>

#define BAILLY_LAT -66.82
#define BAILLY_LON -68.90
#define BAILLY_RADIUS 150500.0f
#define CLAVIUS_LAT -58.62
#define CLAVIUS_LON -14.73
#define CLAVIUS_RADIUS 115385.0f
#define MAGINUS_LAT -50.00
#define MAGINUS_LON -6.20
#define MAGINUS_RADIUS 97000.0f
#define SCHILLER_LAT -51.72
#define SCHILLER_LON -39.78
#define SCHILLER_RADIUS 89500.0f
#define HAUSEN_LAT -65.11
#define HAUSEN_LON -88.49
#define HAUSEN_RADIUS 81500.0f
#define SHACKLETON_LAT -89.67
#define SHACKLETON_LON 129.78
#define SHACKLETON_RADIUS 10460.0f

static DcPlanetHandle logic_planet;
static DcPlanetViewHandle logic_planet_view;
static DcPlanetBreadcrumbsHandle logic_orbit_breadcrumbs;
static int logic_texture_refresh = -1;
static int logic_active_shader = -1;

static void update_logic_shader(void) {
    if (!logic_planet_view || !RightShader) return;

    int desired = *RightShader;
    if (desired == logic_active_shader) return;

    bool updated = false;
    if (desired == 1) {
        updated = dc_planet->set_view_shaders(logic_planet_view, NULL, "shaders/planet_elevation.frag");
    } else if (desired == 2) {
        updated = dc_planet->set_view_shaders(logic_planet_view, NULL, "shaders/planet_slope.frag");
    } else if (desired == 3) {
        updated = dc_planet->set_view_shaders(logic_planet_view, "shaders/planet_flat.vert", "shaders/planet_elevation.frag");
    } else {
        updated = dc_planet->set_view_shaders(logic_planet_view, NULL, NULL);
    }

    if (updated) {
        logic_active_shader = desired;
    }
}

void display_init(DcAppContext *app_ctx, void **user_data) {
    (void)user_data;
    const char *display_home = getenv("dcappDisplayHome");
    if (!display_home || display_home[0] == '\0') return;

    char data_path[4096] = {0};
    snprintf(data_path, sizeof(data_path), "%s/../../data/LDEM_45S_400M.planet.json", display_home);

    logic_planet = dc_planet->create_planet_with_id(app_ctx, "LogicMoon", (DcPlanetCreateInfo){
        .data_path = data_path,
        .mesh_cache_size = 128u * 1024u * 1024u,
    });
    if (logic_planet) {
        dc_planet->set_texture_geodetic(app_ctx, logic_planet, "../../assets/nasa-worm.png", -90.0, 180.0, TexMpp ? (float)*TexMpp : 2000.0f);
        logic_texture_refresh = TextureRefresh ? *TextureRefresh : -1;
        logic_planet_view = dc_planet->create_geodetic_view(app_ctx, logic_planet, 1024, 1024);
        update_logic_shader();
        logic_orbit_breadcrumbs = dc_planet->create_breadcrumbs(app_ctx, DC_PLANET_CRS_GEODETIC, 512, 25000.0f);
    }
}

void display_draw(DcAppContext *app_ctx, void *user_data) {
    (void)user_data;
    static double orbit_lon = 315.0;
    orbit_lon += 0.25;
    if (orbit_lon >= 360.0) {
        orbit_lon -= 360.0;
    }

    *OrbitLat = -65.0;
    *OrbitLon = orbit_lon;

    if (logic_planet && logic_orbit_breadcrumbs) {
        dc_planet->update_breadcrumbs_geodetic(logic_orbit_breadcrumbs, logic_planet,
                                              (DcVec3){.x = (float)*OrbitLat, .y = (float)*OrbitLon, .z = 50000.0f});
    }

    update_logic_shader();

    if (!logic_planet || !TextureRefresh || !TexMpp) return;
    if (*TextureRefresh != logic_texture_refresh) {
        dc_planet->set_texture_geodetic(app_ctx, logic_planet, "../../assets/nasa-worm.png", -90.0, 180.0, (float)*TexMpp);
        logic_texture_refresh = *TextureRefresh;
    }
}

void display_close(DcAppContext *app_ctx, void *user_data) {
    (void)user_data;
    (void)app_ctx;
}

void draw_logic_planet_view(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args, void *user_data) {
    (void)user_data;
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
    DcPlanetViewOptions options = dc_planet_view_options_default();
    if (Tau) options.tau = (float)*Tau;

    DcDrawPlanetViewHandle view = dc_draw->planet_view_geodetic(
        draw_ctx,
        logic_planet_view,
        *Latitude,
        *Longitude,
        *Elevation,
        (DcVec3){.roll = 0.0f, .pitch = 0.0f, .yaw = (float)*Heading},
        60.0f,
        use_ortho,
        options,
        (DcVec2){0.0f, 0.0f},
        size,
        dc_place_top_left(),
        0);

    if (!view) return;

    const DcVec3 geojson_polygon[] = {
        {.x = -66.5f, .y = 300.0f, .z = 1500.0f},
        {.x = -66.5f, .y = 345.0f, .z = 1500.0f},
        {.x = -63.5f, .y = 345.0f, .z = 1500.0f},
        {.x = -63.5f, .y = 300.0f, .z = 1500.0f},
    };

    dc_draw->planet_polygon_geodetic(draw_ctx, view, geojson_polygon, 4, 4400.0f,
                                     (DcVec4){.r = 0.25f, .g = 0.70f, .b = 1.0f, .a = 0.90f},
                                     (DcVec4){.r = 0.25f, .g = 0.70f, .b = 1.0f, .a = 0.10f});

    DcPlanetBreadcrumbsPoints orbit_trail = dc_planet->get_breadcrumbs_points(logic_orbit_breadcrumbs);
    if (orbit_trail.count >= 2 && orbit_trail.crs == DC_PLANET_CRS_GEODETIC) {
        dc_draw->planet_line_geodetic(draw_ctx, view, orbit_trail.points, orbit_trail.count, 7000.0f,
                                      (DcVec4){.r = 1.0f, .g = 0.75f, .b = 0.18f, .a = 0.85f});
    }
    dc_draw->planet_sphere_geodetic(draw_ctx, view, *OrbitLat, *OrbitLon, 50000.0, 18000.0,
                                    (DcVec4){.r = 1.0f, .g = 0.75f, .b = 0.18f, .a = 1.0f});

    dc_draw->planet_sphere_geodetic(draw_ctx, view, BAILLY_LAT, BAILLY_LON, 1000.0, BAILLY_RADIUS, (DcVec4){.r = 1.0f, .g = 0.78f, .b = 0.18f, .a = 0.18f});
    dc_draw->planet_sphere_geodetic(draw_ctx, view, CLAVIUS_LAT, CLAVIUS_LON, 1000.0, CLAVIUS_RADIUS, (DcVec4){.r = 0.0f, .g = 0.95f, .b = 1.0f, .a = 0.20f});
    dc_draw->planet_sphere_geodetic(draw_ctx, view, MAGINUS_LAT, MAGINUS_LON, 1000.0, MAGINUS_RADIUS, (DcVec4){.r = 0.45f, .g = 1.0f, .b = 0.25f, .a = 0.18f});
    dc_draw->planet_sphere_geodetic(draw_ctx, view, SCHILLER_LAT, SCHILLER_LON, 1000.0, SCHILLER_RADIUS, (DcVec4){.r = 1.0f, .g = 0.42f, .b = 0.12f, .a = 0.18f});
    dc_draw->planet_sphere_geodetic(draw_ctx, view, HAUSEN_LAT, HAUSEN_LON, 1000.0, HAUSEN_RADIUS, (DcVec4){.r = 0.35f, .g = 0.55f, .b = 1.0f, .a = 0.19f});
    dc_draw->planet_sphere_geodetic(draw_ctx, view, SHACKLETON_LAT, SHACKLETON_LON, 1000.0, SHACKLETON_RADIUS, (DcVec4){.r = 1.0f, .g = 0.25f, .b = 0.95f, .a = 0.23f});
    dc_draw->planet_text_geodetic(draw_ctx, view, BAILLY_LAT, BAILLY_LON, 85000.0, "Bailly 301 km", 30000.0f, (DcVec4){.r = 1.0f, .g = 0.92f, .b = 0.62f, .a = 1.0f});
    dc_draw->planet_text_geodetic(draw_ctx, view, CLAVIUS_LAT, CLAVIUS_LON, 85000.0, "Clavius 231 km", 30000.0f, (DcVec4){.r = 1.0f, .g = 0.92f, .b = 0.62f, .a = 1.0f});
    dc_draw->planet_text_geodetic(draw_ctx, view, MAGINUS_LAT, MAGINUS_LON, 85000.0, "Maginus 194 km", 28000.0f, (DcVec4){.r = 1.0f, .g = 0.92f, .b = 0.62f, .a = 1.0f});
    dc_draw->planet_text_geodetic(draw_ctx, view, SCHILLER_LAT, SCHILLER_LON, 85000.0, "Schiller 179 km", 28000.0f, (DcVec4){.r = 1.0f, .g = 0.92f, .b = 0.62f, .a = 1.0f});
    dc_draw->planet_text_geodetic(draw_ctx, view, HAUSEN_LAT, HAUSEN_LON, 85000.0, "Hausen 163 km", 28000.0f, (DcVec4){.r = 1.0f, .g = 0.92f, .b = 0.62f, .a = 1.0f});
    dc_draw->planet_text_geodetic(draw_ctx, view, SHACKLETON_LAT, SHACKLETON_LON, 50000.0, "Shackleton 21 km", 30000.0f, (DcVec4){.r = 1.0f, .g = 0.4f, .b = 1.0f, .a = 1.0f});
}
