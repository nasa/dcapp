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
#define LOGIC_PATH_CAPACITY 4096

static DcPlanetHandle logic_planet;
static DcPlanetViewHandle logic_planet_view;
static DcPlanetBreadcrumbsHandle logic_orbit_breadcrumbs;
static DcPlanetGeojsonHandle logic_geojson;
static DcTextureId logic_image;
static int logic_texture_refresh = -1;
static int logic_texture_enabled[DC_PLANET_TEXTURE_SLOT_COUNT] = {-1, -1, -1, -1, -1};
static int logic_active_shader = -1;

static const char *logic_texture_files[DC_PLANET_TEXTURE_SLOT_COUNT] = {
    "assets/circle.png",
    "assets/square.png",
    "assets/triangle.png",
    "assets/ring.png",
    "assets/cross.png",
};
static char logic_texture_paths[DC_PLANET_TEXTURE_SLOT_COUNT][LOGIC_PATH_CAPACITY];

// Authored once in a small local grid; the pushed frame supplies scale, rotation, and position.
static const DcVec2 logic_doghouse[] = {
    {.x = -40.0f, .y = -30.0f},
    {.x = 40.0f, .y = -30.0f},
    {.x = 40.0f, .y = 10.0f},
    {.x = 0.0f, .y = 50.0f},
    {.x = -40.0f, .y = 10.0f},
};

static float texture_mpp_for_refresh(int refresh);
static bool build_dcapp_path(char *path, size_t path_capacity, const char *dcapp_home, const char *relative_path);
static int *texture_enabled_variable(uint32_t slot);
static int texture_enabled(uint32_t slot);
static void update_planet_texture_slot(DcAppContext *app_ctx, uint32_t slot, float mpp);
static void update_planet_textures(DcAppContext *app_ctx, int refresh);
static void update_logic_shader(void);

void display_init(DcAppContext *app_ctx, void **user_data) {
    (void)user_data;
    const char *dcapp_home = getenv("DCAPP_HOME");
    if (!dcapp_home || dcapp_home[0] == '\0') return;

    char data_path[LOGIC_PATH_CAPACITY] = {0};
    if (!build_dcapp_path(data_path, sizeof(data_path), dcapp_home,
                          "data/LDEM_45S_400M.planet.json")) return;
    char image_path[LOGIC_PATH_CAPACITY] = {0};
    if (!build_dcapp_path(image_path, sizeof(image_path), dcapp_home,
                          "assets/triangle.png")) return;
    for (uint32_t slot = 0; slot < DC_PLANET_TEXTURE_SLOT_COUNT; slot++) {
        if (!build_dcapp_path(logic_texture_paths[slot], sizeof(logic_texture_paths[slot]),
                              dcapp_home, logic_texture_files[slot])) return;
    }

    logic_planet = dc_planet->create_planet_with_id(app_ctx, "LogicMoon", (DcPlanetCreateInfo){
                                                                              .data_path = data_path,
                                                                              .mesh_cache_size_mb = 128u,
                                                                          });
    logic_geojson = dc_planet->load_geojson(app_ctx, "assets/geojson_test.geojson");
    logic_image = dc_texture->load_image(app_ctx, image_path, NULL);
    update_planet_textures(app_ctx, TextureRefresh ? *TextureRefresh : 0);
    logic_texture_refresh = TextureRefresh ? *TextureRefresh : -1;
    if (logic_planet) {
        logic_planet_view = dc_planet->create_geodetic_view(app_ctx, logic_planet, 1024, 1024);
        update_logic_shader();
        logic_orbit_breadcrumbs = dc_planet->create_breadcrumbs(app_ctx, DC_PLANET_CRS_GEODETIC, 512, 25000.0f);
    }
}

void display_draw(DcAppContext *app_ctx, void *user_data) {
    (void)user_data;
    static double orbit_lon = 315.0;
    static double local_rotation = 0.0;
    orbit_lon += 0.25;
    if (orbit_lon >= 360.0) {
        orbit_lon -= 360.0;
    }
    local_rotation += 0.5;
    if (local_rotation >= 360.0) {
        local_rotation -= 360.0;
    }

    *OrbitLat = -65.0;
    *OrbitLon = orbit_lon;
    *LocalRotation = local_rotation;

    if (logic_planet && logic_orbit_breadcrumbs) {
        dc_planet->update_breadcrumbs_geodetic(logic_orbit_breadcrumbs, logic_planet,
                                               (DcVec3d){.x = *OrbitLat, .y = *OrbitLon, .z = 50000.0});
    }

    if (logic_planet && LightY) {
        dc_planet->set_light_direction(logic_planet, (DcVec3){.x = -1.0f, .y = (float)*LightY, .z = -1.0f});
    }

    update_logic_shader();

    int refresh = TextureRefresh ? *TextureRefresh : 0;
    if (refresh != logic_texture_refresh) {
        update_planet_textures(app_ctx, refresh);
        logic_texture_refresh = refresh;
        return;
    }

    float mpp = texture_mpp_for_refresh(refresh);
    for (uint32_t slot = 0; slot < DC_PLANET_TEXTURE_SLOT_COUNT; slot++) {
        if (texture_enabled(slot) != logic_texture_enabled[slot]) {
            update_planet_texture_slot(app_ctx, slot, mpp);
        }
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
        snprintf(*LogicReadout, sizeof(*LogicReadout),
                 "lat/lon/ele: %.2f %.2f %.0f    rpy: 0.00 0.00 %.2f    local rotation: %.1f",
                 *Latitude, *Longitude, *Elevation, *Heading, *LocalRotation);
    }

    const DcDrawArea *area = dc_draw->get_area(draw_ctx);
    DcVec2 size = {450.0f, 450.0f};
    if (area) {
        size.x = area->dimensions[0];
        size.y = area->dimensions[1];
    }

    if (!logic_planet_view) return;

    bool use_ortho = *UseOrtho != 0;
    DcPlanetViewOptions options = {0};
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
        (DcPlacement){
            .parent_align_x = DC_ALIGN_LEFT,
            .parent_align_y = DC_ALIGN_TOP,
            .local_align_x = DC_ALIGN_LEFT,
            .local_align_y = DC_ALIGN_TOP,
        },
        0);

    if (!view) return;

    if (logic_geojson) {
        DcPlanetGeojsonStyle geojson_style = {0};
        geojson_style.flags = DC_PLANET_GEOJSON_STYLE_FLAGS_LINE_COLOR |
                              DC_PLANET_GEOJSON_STYLE_FLAGS_FILL_COLOR |
                              DC_PLANET_GEOJSON_STYLE_FLAGS_LINE_WIDTH;
        geojson_style.height_above_terrain = 1500.0;
        geojson_style.line_width = 2.2f;
        geojson_style.line_color = (DcVec4){.r = 0.25f, .g = 0.70f, .b = 1.0f, .a = 0.90f};
        geojson_style.fill_color = (DcVec4){.r = 0.25f, .g = 0.70f, .b = 1.0f, .a = 0.10f};
        dc_draw->planet_geojson(draw_ctx, view, logic_geojson, geojson_style);
    }

    if (logic_image) {
        dc_draw->planet_image_geodetic(
            draw_ctx, view, CLAVIUS_LAT, 345.27, 120000.0, logic_image,
            (DcVec2){120000.0f, 120000.0f}, 15.0f, (float)*LocalRotation, true,
            (DcVec4){.r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f});
    }

    dc_draw->planet_ellipse_filled_geodetic(
        draw_ctx, view, -55.0, 335.0, 1500.0,
        (DcVec2){80000.0f, 40000.0f}, 25.0f, 64,
        (DcVec4){.r = 1.0f, .g = 0.20f, .b = 0.70f, .a = 0.12f});
    dc_draw->planet_ellipse_geodetic(
        draw_ctx, view, -55.0, 335.0, 1500.0,
        (DcVec2){80000.0f, 40000.0f}, 25.0f, 64, 2.5f,
        (DcVec4){.r = 1.0f, .g = 0.35f, .b = 0.80f, .a = 0.90f});

    DcPlanetBreadcrumbsPoints orbit_trail = dc_planet->get_breadcrumbs_points(logic_orbit_breadcrumbs);
    if (orbit_trail.count >= 2 && orbit_trail.crs == DC_PLANET_CRS_GEODETIC) {
        dc_draw->planet_line_geodetic(
            draw_ctx, view, orbit_trail.points, orbit_trail.count,
            (DcStroke){
                .color = {.r = 1.0f, .g = 0.75f, .b = 0.18f, .a = 0.85f},
                .width = 3.5f,
                .pattern = 0xF0,
            });
    }
    dc_draw->planet_sphere_geodetic(draw_ctx, view, *OrbitLat, *OrbitLon, 50000.0, 18000.0,
                                    (DcVec4){.r = 1.0f, .g = 0.75f, .b = 0.18f, .a = 1.0f});

    DcPlanetLocalTransform doghouse_transform = {
        .scale = 2000.0f,
        .rotation_degrees = (float)*LocalRotation,
    };
    if (dc_draw->planet_container_push_geodetic(
            draw_ctx, view, *OrbitLat, *OrbitLon, 60000.0, doghouse_transform)) {
        dc_draw->planet_convex_polygon_filled_local(
            draw_ctx, logic_doghouse,
            (uint32_t)(sizeof(logic_doghouse) / sizeof(logic_doghouse[0])),
            (DcVec4){.r = 1.0f, .g = 0.45f, .b = 0.10f, .a = 0.22f});
        dc_draw->planet_polygon_local(
            draw_ctx, logic_doghouse,
            (uint32_t)(sizeof(logic_doghouse) / sizeof(logic_doghouse[0])),
            (DcStroke){
                .color = {.r = 1.0f, .g = 0.75f, .b = 0.18f, .a = 1.0f},
                .width = 2.1f,
                .pattern = 0xAA,
            });
        dc_draw->planet_container_pop(draw_ctx);
    }

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

static float texture_mpp_for_refresh(int refresh) {
    return refresh ? 4000.0f : 2000.0f;
}

static bool build_dcapp_path(char *path, size_t path_capacity, const char *dcapp_home, const char *relative_path) {
    int length = snprintf(path, path_capacity, "%s/%s", dcapp_home, relative_path);
    return length >= 0 && (size_t)length < path_capacity;
}

static int *texture_enabled_variable(uint32_t slot) {
    switch (slot) {
        case 0:
            return HazardMap0Enabled;
        case 1:
            return HazardMap1Enabled;
        case 2:
            return HazardMap2Enabled;
        case 3:
            return HazardMap3Enabled;
        case 4:
            return HazardMap4Enabled;
        default:
            return NULL;
    }
}

static int texture_enabled(uint32_t slot) {
    int *enabled = texture_enabled_variable(slot);
    return enabled && *enabled != 0;
}

static void update_planet_texture_slot(DcAppContext *app_ctx, uint32_t slot, float mpp) {
    int enabled = texture_enabled(slot);
    if (logic_planet) {
        if (enabled) {
            dc_planet->set_texture_geodetic_slot(app_ctx, logic_planet, slot, logic_texture_paths[slot], -58.62, 345.27, mpp);
        } else {
            dc_planet->clear_texture(logic_planet, slot);
        }
    }
    logic_texture_enabled[slot] = enabled;
}

static void update_planet_textures(DcAppContext *app_ctx, int refresh) {
    float mpp = texture_mpp_for_refresh(refresh);
    if (TexMpp) {
        *TexMpp = (double)mpp;
    }

    for (uint32_t slot = 0; slot < DC_PLANET_TEXTURE_SLOT_COUNT; slot++) {
        int enabled = texture_enabled(slot);
        if (enabled || logic_texture_enabled[slot] > 0) {
            update_planet_texture_slot(app_ctx, slot, mpp);
        } else {
            logic_texture_enabled[slot] = enabled;
        }
    }
}

static void update_logic_shader(void) {
    if (!logic_planet_view || !RightShader) return;

    int desired = *RightShader;
    if (desired == logic_active_shader) return;

    bool updated = false;
    if (desired == 1) {
        updated = dc_planet->set_view_shaders(logic_planet_view, NULL, "shaders/planet_elevation.frag");
    } else if (desired == 2) {
        updated = dc_planet->set_view_shaders(logic_planet_view, NULL, "shaders/planet_slope.frag");
    } else {
        updated = dc_planet->set_view_shaders(logic_planet_view, NULL, NULL);
    }

    if (updated) {
        logic_active_shader = desired;
    }
}
