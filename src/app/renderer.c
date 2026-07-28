#define _USE_MATH_DEFINES
#define PL_MATH_INCLUDE_FUNCTIONS
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "renderer.h"

#include "app/lookup.h"
#include "draw.h"
#include "draw_internal.h"
#include "font.h"
#include "logic_runtime.h"
#include "node.h"
#include "planet.h"
#include "pixelstream.h"
#include "scene.h"
#include "texture.h"
#include "value.h"
#include "utils/log.h"
#include "utils/math.h"
#include "utils/stb_sb.h"
#include "utils/string.h"
#include "utils/time.h"
#include "geo.h"
#include "pl_graphics_ext.h"
#include "pl_planet_ext.h"
#include "pl_starter_ext.h"
#include "pl_vfs_ext.h"

#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_FREE(x) _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

#ifndef DCAPP_DRAW_FUNCTION_ARG_MAX
#define DCAPP_DRAW_FUNCTION_ARG_MAX 4096
#endif

static const plMemoryI  *_ext_memory  = NULL;
static const plPlanetI  *_ext_planet  = NULL;
static const plStarterI *_ext_starter = NULL;
static const plVfsI     *_ext_vfs     = NULL;

typedef struct _DcAppDeferredSetOp {
    DcAppVarIndex var_index;
    DcAppSetType operation;
    DcValue value;
} _DcAppDeferredSetOp;

struct DcAppRenderer {
    DcAppContext            *callback_context;
    DcAppSceneContext       *scene;
    DcAppFontContext        *fonts;
    DcAppTextureContext     *textures;
    DcAppPlanetContext      *planets;
    DcAppPixelstreamContext *pixelstreams;
    DcAppLogicContext       *logic;
    DcAppLookup             *lookup;
    _DcAppDeferredSetOp     *sb_deferred_sets;
    DcAppDrawFuncArg        *sb_draw_function_args;
    DcAppVec3d              *sb_planet_points;
    char                    *sb_planet_text;
    char                    *sb_render_text;
    // Persistent storage avoids a large stack allocation that can cause issues
    // with stack guard pages during rapid redraws (e.g., window resize).
    DcAppVec2                ellipse_triangle_points[DC_APP_NODE_ELLIPSE_MAX_SEGMENTS * 3];
};

typedef struct _DcAppRenderFrame {
    plVec2 position;
    plVec2 dimensions;
    plMat4 transform;
} _DcAppRenderFrame;

static void _render_node_list(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index);
static void _render_node(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index);
static bool _build_planet_texture(
    DcAppRenderer *renderer,
    DcAppPlanetDefinition *def,
    DcAppPlanetTextureEntry *entry,
    plPlanetTexture *out);

// Local node draw helpers
static void _render_blink(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_button(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_button_enabled(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_button_disabled(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_button_indicator_on(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_button_indicator_off(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_button_transition(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_mouse_pressed(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_mouse_released(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_mouse_active(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_mouse_inactive(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_mouse_hovered(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_if_true(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_state_if_false(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_arc(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_ellipse(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_container(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_conditional(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _execute_function(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _execute_draw_function(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_image(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_line(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_panel(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_pixelstream(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_polygon(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_rectangle(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _process_mouse_motion(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _execute_set(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_sphere(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_stencil(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_planet_breadcrumbs(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view);
static void _render_planet_container(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view);
static void _render_planet_ellipse(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view);
static void _render_planet_line(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view);
static void _render_planet_line_local(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node);
static void _render_planet_image(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view);
static void _render_planet_polygon(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view);
static void _render_planet_polygon_local(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node);
static void _render_planet_sphere(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view);
static void _render_planet_text(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view);
static void _render_planet_view(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_text(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static void _render_window(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node);
static bool _apply_set_operation(DcAppRenderer *renderer, DcAppVarIndex var_index, DcValue *var_value, DcValue *op_value, DcAppSetType operation);

void dc_app_renderer_init(plApiRegistryI *api_registry) {
    _ext_memory  = pl_get_api_latest(api_registry, plMemoryI);
    _ext_planet  = pl_get_api_latest(api_registry, plPlanetI);
    _ext_starter = pl_get_api_latest(api_registry, plStarterI);
    _ext_vfs     = pl_get_api_latest(api_registry, plVfsI);
}

DcAppRenderer *dc_app_renderer_create(
    DcAppContext *callback_context,
    DcAppSceneContext *scene,
    DcAppFontContext *fonts,
    DcAppTextureContext *textures,
    DcAppPlanetContext *planets,
    DcAppPixelstreamContext *pixelstreams,
    DcAppLogicContext *logic) {
    DcAppRenderer *renderer = PL_ALLOC(sizeof(*renderer));
    if (!renderer) return NULL;

    *renderer = (DcAppRenderer){
        .callback_context = callback_context,
        .scene            = scene,
        .fonts            = fonts,
        .textures         = textures,
        .planets          = planets,
        .pixelstreams     = pixelstreams,
        .logic            = logic,
        .lookup           = dc_app_scene_lookup(scene),
    };
    return renderer;
}

void dc_app_renderer_destroy(DcAppRenderer *renderer) {
    if (!renderer) return;
    sbfree(renderer->sb_deferred_sets);
    sbfree(renderer->sb_draw_function_args);
    sbfree(renderer->sb_planet_points);
    sbfree(renderer->sb_planet_text);
    sbfree(renderer->sb_render_text);
    PL_FREE(renderer);
}

static bool _build_planet_texture(
    DcAppRenderer *renderer,
    DcAppPlanetDefinition *def,
    DcAppPlanetTextureEntry *entry,
    plPlanetTexture *out) {
    if (!entry->source || entry->source[0] == '\0') return false;
    if (!_ext_vfs->does_file_exist(entry->source)) return false;
    memset(out, 0, sizeof(*out));
    out->pcPath = entry->source;
    if (entry->mpp != DC_APP_VAL_INDEX_UNDEFINED)
        out->fMetersPerPixel = (float)dc_app_lookup_get_value(renderer->lookup, entry->mpp)->value_double;
    if (out->fMetersPerPixel <= 0.0f) {
        DC_LOG_ERROR("PlanetTexture", "MetersPerPixel must be greater than zero for '%s'", entry->source);
        return false;
    }

    if (entry->originX != DC_APP_VAL_INDEX_UNDEFINED && entry->originY != DC_APP_VAL_INDEX_UNDEFINED) {
        out->dOriginX = dc_app_lookup_get_value(renderer->lookup, entry->originX)->value_double;
        out->dOriginY = dc_app_lookup_get_value(renderer->lookup, entry->originY)->value_double;
    } else if (entry->originX != DC_APP_VAL_INDEX_UNDEFINED || entry->originY != DC_APP_VAL_INDEX_UNDEFINED) {
        DC_LOG_ERROR("PlanetTexture", "OriginX and OriginY must be specified together for '%s'", entry->source);
        return false;
    } else if (entry->crs == DC_APP_PLANET_CRS_CARTESIAN &&
               entry->xyz.x != DC_APP_VAL_INDEX_UNDEFINED &&
               entry->xyz.y != DC_APP_VAL_INDEX_UNDEFINED &&
               entry->xyz.z != DC_APP_VAL_INDEX_UNDEFINED) {
        plVec3d cartesian_in = {
            dc_app_lookup_get_value(renderer->lookup, entry->xyz.x)->value_double,
            dc_app_lookup_get_value(renderer->lookup, entry->xyz.y)->value_double,
            dc_app_lookup_get_value(renderer->lookup, entry->xyz.z)->value_double
        };
        double r = sqrt(cartesian_in.x * cartesian_in.x +
                        cartesian_in.y * cartesian_in.y +
                        cartesian_in.z * cartesian_in.z);
        if (r <= 0.0) {
            DC_LOG_WARN("PlanetTexture", "Skipping texture with degenerate cartesian origin for '%s'", entry->source);
            return false;
        }
        plVec3d geodetic_out;
        plVec2d polar_out;
        dc_geo_cartesian_to_geodetic_d(&def->cartesian_crs, &def->geodetic_crs, &cartesian_in, &geodetic_out, 1);
        if (def->legacy_projected_origin) {
            // Old planet metadata expects the historical user-longitude projection
            // convention. New metadata uses real projected CRS meters.
            dc_geo_user_geodetic_to_polar_stereo_d(&def->geodetic_crs, &def->polar_crs, &geodetic_out, &polar_out, 1);
            polar_out.y = -polar_out.y;
        } else {
            dc_geo_geodetic_to_polar_stereo_d(&def->geodetic_crs, &def->polar_crs, &geodetic_out, &polar_out, 1);
        }
        out->dOriginX = polar_out.x;
        out->dOriginY = polar_out.y;
    } else if (entry->lle.lat != DC_APP_VAL_INDEX_UNDEFINED &&
               entry->lle.lon != DC_APP_VAL_INDEX_UNDEFINED) {
        plVec3d geodetic_in = {
            dc_app_lookup_get_value(renderer->lookup, entry->lle.lat)->value_double,
            dc_app_lookup_get_value(renderer->lookup, entry->lle.lon)->value_double,
            0.0
        };
        plVec2d polar_out;
        if (def->legacy_projected_origin) {
            // Old planet metadata expects the historical user-longitude projection
            // convention. New metadata uses real projected CRS meters.
            dc_geo_user_geodetic_to_polar_stereo_d(&def->geodetic_crs, &def->polar_crs, &geodetic_in, &polar_out, 1);
            polar_out.y = -polar_out.y;
        } else {
            dc_geo_geodetic_to_polar_stereo_d(&def->geodetic_crs, &def->polar_crs, &geodetic_in, &polar_out, 1);
        }
        out->dOriginX = polar_out.x;
        out->dOriginY = polar_out.y;
    } else {
        DC_LOG_ERROR("PlanetTexture", "Texture center must be OriginX/OriginY, Latitude/Longitude, or complete X/Y/Z for '%s'", entry->source);
        return false;
    }

    return true;
}

void dc_app_renderer_initialize_planets(DcAppRenderer *renderer) {
    if (!renderer) return;

    int def_count  = (int)dc_app_scene_get_planet_definition_count(renderer->scene);
    int view_count = (int)dc_app_scene_get_planet_view_node_count(renderer->scene);
    if (def_count == 0 && view_count == 0) return;

    DC_LOG_INFO("Planet", "Initializing %d planet def(s), %d view(s)", def_count, view_count);

    // create planets from definitions through the shared planet subsystem.
    for (int i = 0; i < def_count; i++) {
        DcAppPlanetDefinition *def = dc_app_scene_get_planet_definition(renderer->scene, i);

        int file_count = sbcount(def->sb_data_files);
        if (file_count == 0) {
            DC_LOG_WARN("Planet", "  [%d] '%s': no PlanetData file specified, skipping", i, def->name ? def->name : "?");
            continue;
        }

        const char *json_path = def->sb_data_files[0];

        DC_LOG_INFO("Planet", "  [%d] '%s' loading: %s", i, def->name, json_path);

        DcAppPlanetCreateInfo info = {
            .data_path = json_path,
            .mesh_cache_size_mb = def->mesh_cache_size_mb,
        };
        def->handle = dc_app_planet_create_planet_with_id(renderer->planets, def->name, info);
        if (!def->handle) {
            DC_LOG_ERROR("Planet", "  [%d] '%s' failed to create", i, def->name ? def->name : "?");
            continue;
        }

        def->radius                  = dc_app_planet_radius(def->handle);
        def->geodetic_crs            = *dc_app_planet_geodetic_crs(def->handle);
        def->cartesian_crs           = *dc_app_planet_cartesian_crs(def->handle);
        def->polar_crs               = *dc_app_planet_polar_crs(def->handle);
        def->legacy_projected_origin = dc_app_planet_uses_legacy_projected_origin(def->handle);
        def->index                   = dc_app_planet_index(def->handle);

        plPlanet *planet = dc_app_planet_pl(def->handle);

        // apply each initially enabled texture overlay in declaration order.
        for (int t = 0; t < sbcount(def->sb_textures); t++) {
            DcAppPlanetTextureEntry *entry = &def->sb_textures[t];
            bool enabled = true;
            if (entry->enabled != DC_APP_VAL_INDEX_UNDEFINED)
                enabled = dc_app_lookup_get_value(renderer->lookup, entry->enabled)->value_boolean;
            entry->last_enabled        = enabled;
            entry->enabled_initialized = true;

            if (entry->fire_refresh != DC_APP_VAL_INDEX_UNDEFINED) {
                entry->last_fire_refresh_value = *dc_app_lookup_get_value(renderer->lookup, entry->fire_refresh);
            }

            if (!enabled) continue;

            plPlanetTexture texture;
            if (_build_planet_texture(renderer, def, entry, &texture)) {
                DC_LOG_INFO("Planet", "  [%d] texture slot %u: %s (mpp=%.1f, originX=%.1f, originY=%.1f)",
                            i, (unsigned)entry->slot, texture.pcPath, texture.fMetersPerPixel, texture.dOriginX, texture.dOriginY);
                _ext_planet->set_texture(planet, &texture, entry->slot);
            }
        }

        DC_LOG_INFO("Planet", "  [%d] '%s' created (radius=%.0f)", i, def->name, def->radius);
    }

    // create views from planet view nodes through the shared planet subsystem.
    for (int i = 0; i < view_count; i++) {
        DcAppNodeIndex node_index = dc_app_scene_get_planet_view_node(renderer->scene, i);
        DcAppNode     *node       = dc_app_scene_get_node(renderer->scene, node_index);

        uint8_t def_idx = node->planet_view.planet_def_index;
        if (def_idx >= def_count) {
            DC_LOG_ERROR("PlanetView", "  [%d] invalid planet def index", i);
            continue;
        }

        DcAppPlanetDefinition *def = dc_app_scene_get_planet_definition(renderer->scene, def_idx);
        if (def->index == PLANET_INDEX_UNDEFINED) {
            DC_LOG_ERROR("PlanetView", "  [%d] planet '%s' not initialized", i, def->name ? def->name : "?");
            continue;
        }

        float output_width  = 1024.0f;
        float output_height = 1024.0f;

        DcAppPlanetViewHandle view_handle = node->planet_view.crs == DC_APP_PLANET_CRS_CARTESIAN
            ? dc_app_planet_create_cartesian_view(renderer->planets, def->handle, (uint32_t)output_width, (uint32_t)output_height)
            : dc_app_planet_create_geodetic_view(renderer->planets, def->handle, (uint32_t)output_width, (uint32_t)output_height);
        if (!view_handle) {
            DC_LOG_ERROR("PlanetView", "  [%d] failed to create view for '%s'", i, def->name ? def->name : "?");
            continue;
        }

        node->planet_view.handle            = view_handle;
        node->planet_view.planet_view_index = dc_app_planet_view_index(view_handle);

        // force shader mismatch so first draw applies the shader.
        if (node->planet_view.shader_index != DC_APP_VAL_INDEX_UNDEFINED) {
            int initial                           = (int)dc_app_lookup_get_value(renderer->lookup, node->planet_view.shader_index)->value_integer;
            node->planet_view.active_shader_index = initial + 1;
        }

        DC_LOG_INFO("PlanetView", "  [%d] created view for '%s' (%ux%u)", i, def->name, (uint32_t)output_width, (uint32_t)output_height);
    }
}

void dc_app_renderer_update_planets(DcAppRenderer *renderer) {
    if (!renderer) return;

    int def_count = (int)dc_app_scene_get_planet_definition_count(renderer->scene);
    for (int i = 0; i < def_count; i++) {
        DcAppPlanetDefinition *def = dc_app_scene_get_planet_definition(renderer->scene, i);
        if (def->index == PLANET_INDEX_UNDEFINED) continue;

        plPlanet *planet = dc_app_planet_pl(def->handle);
        if (!planet) continue;

        // texture enabled/refresh checks
        for (int t = 0; t < sbcount(def->sb_textures); t++) {
            DcAppPlanetTextureEntry *tex = &def->sb_textures[t];
            bool enabled = true;
            if (tex->enabled != DC_APP_VAL_INDEX_UNDEFINED)
                enabled = dc_app_lookup_get_value(renderer->lookup, tex->enabled)->value_boolean;
            bool enabled_changed = tex->enabled_initialized && enabled != tex->last_enabled;
            bool refresh_changed = false;

            if (tex->fire_refresh != DC_APP_VAL_INDEX_UNDEFINED) {
                DcValue *refresh_val = dc_app_lookup_get_value(renderer->lookup, tex->fire_refresh);
                refresh_changed = !dc_value_is_equal(refresh_val, &tex->last_fire_refresh_value);
                tex->last_fire_refresh_value = *refresh_val;
            }

            tex->last_enabled        = enabled;
            tex->enabled_initialized = true;

            if (enabled_changed && !enabled) {
                _ext_planet->set_texture(planet, NULL, tex->slot);
            } else if (enabled && (enabled_changed || refresh_changed)) {
                plPlanetTexture texture;
                if (_build_planet_texture(renderer, def, tex, &texture)) {
                    _ext_planet->set_texture(planet, &texture, tex->slot);
                } else {
                    _ext_planet->set_texture(planet, NULL, tex->slot);
                }
            }
        }

        // light direction
        if (def->light_direction.x != DC_APP_VAL_INDEX_UNDEFINED ||
            def->light_direction.y != DC_APP_VAL_INDEX_UNDEFINED ||
            def->light_direction.z != DC_APP_VAL_INDEX_UNDEFINED) {
            plPlanetRuntimeOptions opts = _ext_planet->get_runtime_options(planet);
            plVec3 prev = opts.tLightDirection;
            if (def->light_direction.x != DC_APP_VAL_INDEX_UNDEFINED)
                opts.tLightDirection.x = (float)dc_app_lookup_get_value(renderer->lookup, def->light_direction.x)->value_double;
            if (def->light_direction.y != DC_APP_VAL_INDEX_UNDEFINED)
                opts.tLightDirection.y = (float)dc_app_lookup_get_value(renderer->lookup, def->light_direction.y)->value_double;
            if (def->light_direction.z != DC_APP_VAL_INDEX_UNDEFINED)
                opts.tLightDirection.z = (float)dc_app_lookup_get_value(renderer->lookup, def->light_direction.z)->value_double;
            if (prev.x != opts.tLightDirection.x || prev.y != opts.tLightDirection.y || prev.z != opts.tLightDirection.z)
                _ext_planet->set_runtime_options(planet, opts);
        }

    }

    // prepares every shared planet once per frame.
    for (int i = 0; i < (int)dc_app_planet_count(renderer->planets); i++) {
        DcAppPlanetHandle handle = dc_app_planet_at(renderer->planets, i);
        plPlanet *planet = dc_app_planet_pl(handle);
        if (!planet) continue;

        plCommandBuffer *cmd_buf = _ext_starter->get_temporary_command_buffer();
        _ext_planet->prepare(planet, cmd_buf);
        _ext_starter->submit_temporary_command_buffer(cmd_buf);
    }
}

static _DcAppRenderFrame _get_current_frame(DcAppDrawContext *ctx) {
    const DcAppDrawArea *area = dc_app_draw_get_area(ctx);
    _DcAppRenderFrame frame = {
        .position   = {area->position[0], area->position[1]},
        .dimensions = {area->dimensions[0], area->dimensions[1]},
    };
    memcpy(frame.transform.d, area->transform, sizeof(frame.transform.d));
    return frame;
}

// Siblings stay flat while container handlers recurse into child lists in draw order.
static void _render_node_list(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index) {
    DcAppNodeIndex current_node_index = node_index;
    while (current_node_index != NODE_INDEX_UNDEFINED) {
        _render_node(ctx, renderer, current_node_index);
        current_node_index = dc_app_scene_get_node(renderer->scene, current_node_index)->next;
    }
}

static void _render_node(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index) {
    if (!ctx || !renderer || node_index == NODE_INDEX_UNDEFINED) {
        DC_LOG_ERROR("Draw", "Attempting to draw undefined node index");
        return;
    }

    DcAppNode *node = dc_app_scene_get_node(renderer->scene, node_index);
    switch (node->type) {
        case NODE_TYPE_ARC:
            _render_arc(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_BLINK:
            _render_blink(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_BUTTON:
            _render_button(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_CONDITIONAL:
            _render_conditional(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_CONTAINER:
            _render_container(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_DRAW_FUNCTION:
            _execute_draw_function(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_ELLIPSE:
            _render_ellipse(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_FUNCTION:
            _execute_function(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_IMAGE:
            _render_image(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_LINE:
            _render_line(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_PANEL:
            _render_panel(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_PIXELSTREAM:
            _render_pixelstream(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_POLYGON:
            _render_polygon(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_RECTANGLE:
            _render_rectangle(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_MOUSE_MOTION:
            _process_mouse_motion(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_SET:
            _execute_set(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_SPHERE:
            _render_sphere(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_STENCIL:
            _render_stencil(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_PLANET_VIEW:
            _render_planet_view(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_TEXT:
            _render_text(ctx, renderer, node_index, node);
            break;

        case NODE_TYPE_WINDOW:
            _render_window(ctx, renderer, node_index, node);
            break;

        // state conditional nodes (check parent's state_flags)
        case NODE_TYPE_STATE_BUTTON_ENABLED:
            _render_state_button_enabled(ctx, renderer, node_index, node);
            break;
        case NODE_TYPE_STATE_BUTTON_DISABLED:
            _render_state_button_disabled(ctx, renderer, node_index, node);
            break;
        case NODE_TYPE_STATE_BUTTON_INDICATOR_ON:
            _render_state_button_indicator_on(ctx, renderer, node_index, node);
            break;
        case NODE_TYPE_STATE_BUTTON_INDICATOR_OFF:
            _render_state_button_indicator_off(ctx, renderer, node_index, node);
            break;
        case NODE_TYPE_STATE_BUTTON_TRANSITION:
            _render_state_button_transition(ctx, renderer, node_index, node);
            break;
        case NODE_TYPE_STATE_MOUSE_PRESSED:
            _render_state_mouse_pressed(ctx, renderer, node_index, node);
            break;
        case NODE_TYPE_STATE_MOUSE_RELEASED:
            _render_state_mouse_released(ctx, renderer, node_index, node);
            break;
        case NODE_TYPE_STATE_MOUSE_ACTIVE:
            _render_state_mouse_active(ctx, renderer, node_index, node);
            break;
        case NODE_TYPE_STATE_MOUSE_INACTIVE:
            _render_state_mouse_inactive(ctx, renderer, node_index, node);
            break;
        case NODE_TYPE_STATE_MOUSE_HOVERED:
            _render_state_mouse_hovered(ctx, renderer, node_index, node);
            break;
        case NODE_TYPE_STATE_IF_TRUE:
            _render_state_if_true(ctx, renderer, node_index, node);
            break;
        case NODE_TYPE_STATE_IF_FALSE:
            _render_state_if_false(ctx, renderer, node_index, node);
            break;

        default:
            break;
    }
}

void dc_app_renderer_render(DcAppRenderer *renderer, DcAppDrawContext *ctx) {
    if (!ctx || !renderer) return;
    _render_node(ctx, renderer, dc_app_scene_get_window(renderer->scene));
}

static void _render_blink(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {

    double current_time = dc_utils_time_get();
    double frequency    = dc_app_lookup_get_value(renderer->lookup, node->blink.frequency)->value_double;
    double duty_cycle   = dc_app_lookup_get_value(renderer->lookup, node->blink.duty_cycle)->value_double;
    double duration     = dc_app_lookup_get_value(renderer->lookup, node->blink.duration)->value_double;

    // check trigger variable for any change
    if (node->blink.fire_blink != DC_APP_VAL_INDEX_UNDEFINED) {
        DcValue *val = dc_app_lookup_get_value(renderer->lookup, node->blink.fire_blink);
        if (!dc_value_is_equal(val, &node->blink.last_fire_blink_value)) {
            if (duration <= 0.0) {
                // indefinite: toggle (0 <-> 1)
                node->blink.remaining_duration = 1.0 - node->blink.remaining_duration;
            } else {
                // timed: restart
                node->blink.remaining_duration = duration;
            }
            node->blink.last_fire_blink_value = *val;
        }
    }

    // calculate blink state
    bool blink_state = true;
    if (node->blink.remaining_duration > 0.0) {
        double period  = (frequency > 0.0) ? (1.0 / frequency) : 1.0;
        double on_time = period * duty_cycle;
        blink_state    = (fmod(current_time, period) <= on_time);

        // countdown for timed blinks
        if (duration > 0.0) {
            node->blink.remaining_duration -= (current_time - node->blink.last_frame_time);
            if (node->blink.remaining_duration <= 0.0) {
                blink_state = true;
            }
        }
    }

    if (blink_state) {
        _render_node_list(ctx, renderer, node->blink.child);
    }

    node->blink.last_frame_time = current_time;
}

static void _render_button(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_position = &parent_frame.position;
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // boolean checks
    bool use_dimension[2] = {
        node->button.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->button.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_virtual_dimension[2] = {
        node->button.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->button.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation           = node->button.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->button.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->button.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->button.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->button.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->button.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->button.dimension.y)->value_double : parent_dimensions->y};

    // get virtual dimensions
    float virtual_dimension[2] = {
        use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->button.virtual_dimension.x)->value_double : dimension[0],
        use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->button.virtual_dimension.y)->value_double : dimension[1]};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(renderer->lookup, node->button.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(renderer->lookup, node->button.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->button.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->button.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->button.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->button.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->button.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2] = {0, 0};
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("Button", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                    break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("Button", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->button.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->button.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->button.local_align.x)->value_integer,
            node->button.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->button.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2] = {0, 0};
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                DC_LOG_WARN("Button", "Unknown X alignment: %d", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                DC_LOG_WARN("Button", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->button.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->button.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float          anchor[2]      = {0, 0};
        DcAppAlignType parent_align_x = node->button.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->button.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Button", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->button.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->button.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Button", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->button.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->button.position.y)->value_double : 0};

        // apply negate
        if (node->button.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->button.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->button.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->button.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->button.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->button.pivot_local_align.x)->value_integer,
                node->button.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->button.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2] = {0, 0};
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    DC_LOG_WARN("Button", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    DC_LOG_WARN("Button", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->button.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform scale
    {
        // compute matrix
        plMat4 scale_xform = pl_mat4_scale_xyz(dimension[0] / virtual_dimension[0], dimension[1] / virtual_dimension[1], 1.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &scale_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // determine enabled state
    bool is_enabled = true; // default to enabled
    if (node->button.var_enabled != DC_APP_VAR_INDEX_UNDEFINED) {
        DcValue *enabled_var_value = dc_app_lookup_get_value(renderer->lookup, dc_app_lookup_get_var_value_index(renderer->lookup, node->button.var_enabled));
        DcValue *enabled_on_value  = dc_app_lookup_get_value(renderer->lookup, node->button.val_enabled_on);
        is_enabled                 = dc_value_is_equal(enabled_var_value, enabled_on_value);
    }

    // determine indicator
    DcValue *indicator_var_value = NULL;
    bool     is_indicator_on     = false; // default to off
    if (node->button.var_indicator != DC_APP_VAR_INDEX_UNDEFINED) {
        indicator_var_value         = dc_app_lookup_get_value(renderer->lookup, dc_app_lookup_get_var_value_index(renderer->lookup, node->button.var_indicator));
        DcValue *indicator_on_value = dc_app_lookup_get_value(renderer->lookup, node->button.val_indicator_on);
        is_indicator_on             = dc_value_is_equal(indicator_var_value, indicator_on_value);
    }

    // determine transition
    DcValue *target_var_value = NULL;
    bool     is_transitioning = false; // default to not transitioning
    if (node->button.var_target != DC_APP_VAR_INDEX_UNDEFINED) {
        target_var_value = dc_app_lookup_get_value(renderer->lookup, dc_app_lookup_get_var_value_index(renderer->lookup, node->button.var_target));
        if (indicator_var_value) {
            is_transitioning = dc_value_is_not_equal(target_var_value, indicator_var_value);
        }
    }

    // process mouse event states
    bool is_pressed  = dc_app_draw_mouse_target_pressed(ctx, (DcAppDrawTargetId)node_index);
    bool is_released = dc_app_draw_mouse_target_released(ctx, (DcAppDrawTargetId)node_index);

    // process mouse events per button type (only if target variable is defined)
    if (target_var_value) {
        DcValue *target_on_value  = dc_app_lookup_get_value(renderer->lookup, node->button.val_target_on);
        DcValue *target_off_value = dc_app_lookup_get_value(renderer->lookup, node->button.val_target_off);
        switch (node->button.type) {

            // toggle: flip based on current indicator state (on release)
            case DC_APP_BUTTON_TYPE_TOGGLE:
                if (is_pressed) {
                    if (is_indicator_on) {
                        *target_var_value = *target_off_value;
                    } else {
                        *target_var_value = *target_on_value;
                    }
                }
                break;

            // momentary: flip on press/release
            case DC_APP_BUTTON_TYPE_MOMENTARY:
                if (is_pressed) {
                    *target_var_value = *target_on_value;
                } else if (is_released) {
                    *target_var_value = *target_off_value;
                }
                break;

            // standard: set to on value on release
            case DC_APP_BUTTON_TYPE_STANDARD:
                if (is_pressed) {
                    *target_var_value = *target_on_value;
                }
                break;

            default:
                break;
        }
    }

    // mouse interaction
    bool enable_mouse_events = false;
    switch (node->button.type) {
        case DC_APP_BUTTON_TYPE_MOMENTARY:
            enable_mouse_events = is_enabled;
            break;
        case DC_APP_BUTTON_TYPE_STANDARD:
        case DC_APP_BUTTON_TYPE_TOGGLE:
            enable_mouse_events = is_enabled && !is_transitioning;
            break;
        default:
            break;
    }
    if (enable_mouse_events) {
        plVec4 mouse_position = (plVec4){
            dc_app_draw_context_get_screen_mouse(ctx)->x,
            dc_app_draw_context_get_screen_mouse(ctx)->y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        bool inside = mouse_position.x > 0 && mouse_position.x < virtual_dimension[0] && mouse_position.y > 0 && mouse_position.y < virtual_dimension[1];

        // update global states (only if enabled)
        if (inside) {
            dc_app_draw_mouse_register_target(ctx, (DcAppDrawTargetId)node_index);
        }
    }

    // set state flags for conditional children to check
    node->button.state_flags = NODE_STATE_FLAG_NONE;
    if (is_enabled) {
        node->button.state_flags |= NODE_STATE_FLAG_ENABLED;
    }
    if (is_indicator_on) {
        node->button.state_flags |= NODE_STATE_FLAG_INDICATOR_ON;
    }
    if (is_transitioning) {
        node->button.state_flags |= NODE_STATE_FLAG_TRANSITIONING;
    }
    if (is_pressed) {
        node->button.state_flags |= NODE_STATE_FLAG_PRESSED;
    }
    if (dc_app_draw_mouse_target_active(ctx, (DcAppDrawTargetId)node_index)) {
        node->button.state_flags |= NODE_STATE_FLAG_ACTIVE;
    }
    if (dc_app_draw_mouse_target_hovered(ctx, (DcAppDrawTargetId)node_index)) {
        node->button.state_flags |= NODE_STATE_FLAG_HOVERED;
    }
    if (is_released) {
        node->button.state_flags |= NODE_STATE_FLAG_RELEASED;
    }

    // draw children (includes state conditional nodes that check state_flags)
    plVec2 child_position   = (plVec2){0.0f, 0.0f};
    plVec2 child_dimensions = (plVec2){virtual_dimension[0], virtual_dimension[1]};
    dc_app_draw_context_push(ctx, child_position, child_dimensions, &transform);
    _render_node_list(ctx, renderer, node->button.child);
    dc_app_draw_context_pop(ctx);
}

// Helper to get parent's state_flags (returns NODE_STATE_FLAG_NONE if parent doesn't have state_flags)
static uint32_t _get_parent_state_flags(DcAppRenderer *renderer, DcAppNode *node) {
    DcAppNode *parent_node = dc_app_scene_get_node(renderer->scene, node->parent);
    if (!parent_node) return NODE_STATE_FLAG_NONE;

    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
            return parent_node->button.state_flags;
        case NODE_TYPE_CONTAINER:
            return parent_node->container.state_flags;
        case NODE_TYPE_ELLIPSE:
            return parent_node->ellipse.state_flags;
        case NODE_TYPE_IMAGE:
            return parent_node->image.state_flags;
        case NODE_TYPE_PIXELSTREAM:
            return parent_node->pixelstream.state_flags;
        case NODE_TYPE_POLYGON:
            return parent_node->polygon.state_flags;
        case NODE_TYPE_RECTANGLE:
            return parent_node->rectangle.state_flags;
        case NODE_TYPE_CONDITIONAL:
            return parent_node->conditional.state_flags;
        default:
            return NODE_STATE_FLAG_NONE;
    }
}

static void _render_state_button_enabled(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    if (flags & NODE_STATE_FLAG_ENABLED) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

static void _render_state_button_disabled(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    if (!(flags & NODE_STATE_FLAG_ENABLED)) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

static void _render_state_button_indicator_on(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    if (!(flags & NODE_STATE_FLAG_TRANSITIONING) && (flags & NODE_STATE_FLAG_INDICATOR_ON)) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

static void _render_state_button_indicator_off(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    if (!(flags & NODE_STATE_FLAG_TRANSITIONING) && !(flags & NODE_STATE_FLAG_INDICATOR_ON)) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

static void _render_state_button_transition(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    if (flags & NODE_STATE_FLAG_TRANSITIONING) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

static void _render_state_mouse_pressed(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    if (flags & NODE_STATE_FLAG_PRESSED) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

static void _render_state_mouse_released(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    if (flags & NODE_STATE_FLAG_RELEASED) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

static void _render_state_mouse_active(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    if (flags & NODE_STATE_FLAG_ACTIVE) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

static void _render_state_mouse_inactive(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    // Inactive when not active, not hovered, not pressed
    if (!(flags & NODE_STATE_FLAG_ACTIVE) && !(flags & NODE_STATE_FLAG_HOVERED) && !(flags & NODE_STATE_FLAG_PRESSED)) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

static void _render_state_mouse_hovered(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    if (flags & NODE_STATE_FLAG_HOVERED) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

static void _render_state_if_true(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    if (flags & NODE_STATE_FLAG_TRUE) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

static void _render_state_if_false(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    uint32_t flags = _get_parent_state_flags(renderer, node);
    if (flags & NODE_STATE_FLAG_FALSE) {
        _render_node_list(ctx, renderer, node->state_event.child);
    }
}

#define _NODE_ARC_MAX_SEGMENTS 200
static void _render_arc(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_position = &parent_frame.position;
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // boolean checks
    bool use_radius             = node->arc.radius != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_rotation           = node->arc.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->arc.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->arc.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->arc.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->arc.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get radius
    float radius;
    if (use_radius) {
        radius = (float)dc_app_lookup_get_value(renderer->lookup, node->arc.radius)->value_double;
    } else {
        radius = fminf(parent_dimensions->x, parent_dimensions->y) / 2;
    }

    // get angle span and rotation
    float angle_span   = (float)dc_app_lookup_get_value(renderer->lookup, node->arc.angle)->value_double;
    float arc_rotation = use_rotation ? (float)dc_app_lookup_get_value(renderer->lookup, node->arc.rotation)->value_double : 0.0f;

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    if (use_rotation && use_pivot_position) {
        float pivot_position[2] = {
            (float)dc_app_lookup_get_value(renderer->lookup, node->arc.pivot_position.x)->value_double,
            (float)dc_app_lookup_get_value(renderer->lookup, node->arc.pivot_position.y)->value_double};
        float rotation = pl_radiansf(arc_rotation);

        plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
        plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
        plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

        transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
        transform = pl_mul_mat4t(&transform, &rotate_xform);
        transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
    } else if (use_rotation && use_pivot_parent_align) {

        DcAppAlignType parent_pivot_aligns[2] = {
            node->arc.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->arc.pivot_parent_align.x)->value_integer
                : DC_APP_ALIGN_TYPE_UNDEFINED,
            node->arc.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->arc.pivot_parent_align.y)->value_integer
                : DC_APP_ALIGN_TYPE_UNDEFINED};

        float pivot_position[2] = {0, 0};
        switch (parent_pivot_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                pivot_position[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                pivot_position[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                pivot_position[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Arc", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                break;
        }
        switch (parent_pivot_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                pivot_position[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                pivot_position[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                pivot_position[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Arc", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                break;
        }
        float rotation = pl_radiansf(arc_rotation);

        plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
        plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
        plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

        transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
        transform = pl_mul_mat4t(&transform, &rotate_xform);
        transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
    }

    // xform local alignment
    {
        float diameter = 2 * radius;

        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->arc.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->arc.local_align.x)->value_integer,
            node->arc.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->arc.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2] = {0, 0};
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * diameter / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * diameter;
                break;
            default:
                DC_LOG_WARN("Arc", "Unknown X alignment: %d", local_aligns[0]);
                trans_align_offsets[0] = -1 * diameter / 2;
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * diameter / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * diameter;
                break;
            default:
                DC_LOG_WARN("Arc", "Unknown Y alignment: %d", local_aligns[1]);
                trans_align_offsets[1] = -1 * diameter / 2;
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->arc.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->arc.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float          anchor[2]      = {0, 0};
        DcAppAlignType parent_align_x = node->arc.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->arc.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                anchor[0] = 0;
                break;
        }
        DcAppAlignType parent_align_y = node->arc.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->arc.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                anchor[1] = 0;
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->arc.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->arc.position.y)->value_double : 0};

        // apply negate
        if (node->arc.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->arc.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->arc.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->arc.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform                   = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {
        float diameter = 2 * radius;

        // get alignment
        DcAppAlignType local_pivot_aligns[2] = {
            node->arc.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->arc.pivot_local_align.x)->value_integer,
            node->arc.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->arc.pivot_local_align.y)->value_integer};

        // get pivot XY
        float pivot_position[2] = {0, 0};
        switch (local_pivot_aligns[0]) {
            case DC_APP_ALIGN_TYPE_LEFT:
                pivot_position[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_CENTER:
                pivot_position[0] = diameter / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                pivot_position[0] = diameter;
                break;
            default:
                DC_LOG_WARN("Arc", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                pivot_position[0] = diameter / 2;
                break;
        }
        switch (local_pivot_aligns[1]) {
            case DC_APP_ALIGN_TYPE_BOTTOM:
                pivot_position[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_MIDDLE:
                pivot_position[1] = diameter / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                pivot_position[1] = diameter;
                break;
            default:
                DC_LOG_WARN("Arc", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                pivot_position[1] = diameter / 2;
                break;
        }
        float rotation = pl_radiansf(arc_rotation);

        // compute matrices
        plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
        plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
        plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
        transform = pl_mul_mat4t(&transform, &rotate_xform);
        transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // calculate arc points
    // Corner-based: points range from (0,0) to (diameter, diameter), center at (radius, radius)
    // Arc starts at 3 o'clock (standard math convention), rotation handled by transform matrix
    // LocalAlign (default=CENTER) shifts by -radius to center at position
    int num_segments = node->arc.num_segments == DC_APP_VAL_INDEX_UNDEFINED ? (int)(angle_span / 3.0f) + 2 : // roughly 1 segment per 3 degrees
                           (int)dc_app_lookup_get_value(renderer->lookup, node->arc.num_segments)->value_double;
    if (num_segments < 2) num_segments = 2;
    if (num_segments > _NODE_ARC_MAX_SEGMENTS - 1) num_segments = _NODE_ARC_MAX_SEGMENTS - 1;

    // Convert to radians
    float span_rad = pl_radiansf(angle_span);

    // Generate arc points (start-based)
    // Arc starts at 3 o'clock (standard math convention), rotation handled by transform matrix
    DcAppVec2 points[_NODE_ARC_MAX_SEGMENTS];
    int    num_arc_points = num_segments + 1; // segments + 1 = number of vertices on arc

    for (int ii = 0; ii <= num_segments; ii++) {
        float t           = (float)ii / (float)num_segments; // 0 to 1
        float angle       = t * span_rad;                    // from 0 to span
        float final_angle = angle;

        // Corner-based: center at (radius, radius), points from 0 to diameter
        points[ii] = (DcAppVec2){
            radius * (1.0f + cosf(final_angle)),
            radius * (1.0f + sinf(final_angle)),
        };
    }

    // draw arc line
    float line_color[4]  = {
        node->arc.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->arc.line_color.r)->value_double,
        node->arc.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->arc.line_color.g)->value_double,
        node->arc.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->arc.line_color.b)->value_double,
        node->arc.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->arc.line_color.a)->value_double,
    };
    DcAppStroke stroke = {
        .color   = {
            .r = line_color[0],
            .g = line_color[1],
            .b = line_color[2],
            .a = line_color[3],
        },
        .width   = node->arc.line_width == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->arc.line_width)->value_double,
        .pattern = node->arc.line_pattern == DC_APP_VAL_INDEX_UNDEFINED ? 0 : (uint8_t)dc_app_lookup_get_value(renderer->lookup, node->arc.line_pattern)->value_integer,
    };
    dc_app_draw_context_push(ctx, (plVec2){0.0f, 0.0f}, (plVec2){2.0f * radius, 2.0f * radius}, &transform);
    dc_app_draw_polyline(ctx, points, (uint32_t)num_arc_points, stroke);
    dc_app_draw_context_pop(ctx);
}

static void _render_ellipse(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_position = &parent_frame.position;
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // boolean checks
    bool use_radius_x           = node->ellipse.radius_x != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_radius_y           = node->ellipse.radius_y != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_rotation           = node->ellipse.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->ellipse.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->ellipse.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->ellipse.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->ellipse.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float radius_x, radius_y, diameter_x, diameter_y;
    if (use_radius_x) {
        radius_x   = (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.radius_x)->value_double;
        diameter_x = 2 * radius_x;
    } else {
        diameter_x = parent_dimensions->x;
        radius_x   = diameter_x / 2;
    }
    if (use_radius_y) {
        radius_y   = (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.radius_y)->value_double;
        diameter_y = 2 * radius_y;
    } else {
        diameter_y = parent_dimensions->y;
        radius_y   = diameter_y / 2;
    }

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->ellipse.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->ellipse.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->ellipse.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->ellipse.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2] = {0, 0};
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("Ellipse", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                    break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("Ellipse", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->ellipse.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->ellipse.local_align.x)->value_integer,
            node->ellipse.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->ellipse.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2] = {0, 0};
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * diameter_x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * diameter_x;
                break;
            default:
                DC_LOG_WARN("Ellipse", "Unknown X alignment: %d", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * diameter_y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * diameter_y;
                break;
            default:
                DC_LOG_WARN("Ellipse", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->ellipse.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->ellipse.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float          anchor[2]      = {0, 0};
        DcAppAlignType parent_align_x = node->ellipse.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->ellipse.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Ellipse", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->ellipse.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->ellipse.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Ellipse", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.position.y)->value_double : 0};

        // apply negate
        if (node->ellipse.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->ellipse.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->ellipse.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->ellipse.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform                   = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->ellipse.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->ellipse.pivot_local_align.x)->value_integer,
                node->ellipse.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->ellipse.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2] = {0, 0};
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = diameter_x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = diameter_x;
                    break;
                default:
                    DC_LOG_WARN("Ellipse", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = diameter_y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = diameter_y;
                    break;
                default:
                    DC_LOG_WARN("Ellipse", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // check if this is a pie/wedge (angle specified and != 360)
    bool  use_angle  = node->ellipse.angle != DC_APP_VAL_INDEX_UNDEFINED;
    float angle_span = use_angle ? (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.angle)->value_double : 360.0f;
    bool  is_pie     = use_angle && !dc_utils_float_equals(angle_span, 360.0f, 0.001f);

    // get number of segments
    int num_segments = node->ellipse.num_segments == DC_APP_VAL_INDEX_UNDEFINED ? (is_pie ? (int)(angle_span / 3.0f) + 2 : 40) : dc_app_lookup_get_value(renderer->lookup, node->ellipse.num_segments)->value_integer;
    if (num_segments < 2) num_segments = 2;
    if (num_segments > DC_APP_NODE_ELLIPSE_MAX_SEGMENTS - 2) num_segments = DC_APP_NODE_ELLIPSE_MAX_SEGMENTS - 2;

    // generate points
    DcAppVec2 points[DC_APP_NODE_ELLIPSE_MAX_SEGMENTS];
    int    num_points = 0;

    if (is_pie) {
        // Pie/wedge mode: center point + arc points
        // Center point goes first for proper convex polygon winding
        plVec4 center4 = (plVec4){radius_x, radius_y, 0, 1};
        center4        = pl_mul_mat4_vec4(&transform, center4);
        points[0]      = (DcAppVec2){center4.x, center4.y};
        num_points     = 1;

        // Generate arc points (start-based)
        // Wedge starts at 3 o'clock (standard math convention), rotation handled by transform matrix
        float span_rad = pl_radiansf(angle_span);

        for (int ii = 0; ii <= num_segments; ii++) {
            float t           = (float)ii / (float)num_segments;
            float angle       = t * span_rad; // from 0 to span
            float final_angle = angle;

            plVec4 point4 = (plVec4){
                radius_x * (1.0f + cosf(final_angle)),
                radius_y * (1.0f + sinf(final_angle)),
                0, 1};
            point4               = pl_mul_mat4_vec4(&transform, point4);
            points[num_points++] = (DcAppVec2){point4.x, point4.y};
        }
    } else {
        // Full ellipse mode
        for (int ii = 0; ii < num_segments; ii++) {
            float  angle  = ii * (2.0f * (float)M_PI / num_segments);
            plVec4 point4 = (plVec4){
                radius_x * (1.0f + cosf(angle)),
                radius_y * (1.0f + sinf(angle)),
                0, 1};
            point4     = pl_mul_mat4_vec4(&transform, point4);
            points[ii] = (DcAppVec2){point4.x, point4.y};
        }
        num_points = num_segments;
    }

    // draw fill
    if (node->ellipse.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) {
        float fill_color[4] = {
            node->ellipse.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.fill_color.r)->value_double,
            node->ellipse.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.fill_color.g)->value_double,
            node->ellipse.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.fill_color.b)->value_double,
            node->ellipse.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.fill_color.a)->value_double,
        };
        if (is_pie && num_points >= 3) {
            // Use triangle fan for pie/wedge (works for any angle, including > 180 degrees)
            // points[0] = center, points[1..num_points-1] = arc points
            // Reuse context-owned scratch to keep this large fan off the stack.
            int num_triangles = num_points - 2;
            if (num_triangles > 0) {
                DcAppVec2 *triangle_points = renderer->ellipse_triangle_points;
                int        tri_idx         = 0;
                for (int ii = 0; ii < num_triangles; ii++) {
                    triangle_points[tri_idx++] = points[0];      // center
                    triangle_points[tri_idx++] = points[ii + 1]; // arc point i
                    triangle_points[tri_idx++] = points[ii + 2]; // arc point i+1
                }
                plMat4 identity = pl_identity_mat4();
                dc_app_draw_context_push(ctx, (plVec2){0.0f, 0.0f}, (plVec2){diameter_x, diameter_y}, &identity);
                dc_app_draw_triangles_filled_ex(ctx, triangle_points, (uint32_t)(num_triangles * 3), (DcAppVec4){
                    .r = fill_color[0],
                    .g = fill_color[1],
                    .b = fill_color[2],
                    .a = fill_color[3],
                }, (DcAppVec2){0.0f, 0.0f}, (DcAppPlacement){0}, NULL);
                dc_app_draw_context_pop(ctx);
            }
        } else if (!is_pie) {
            plMat4 identity = pl_identity_mat4();
            dc_app_draw_context_push(ctx, (plVec2){0.0f, 0.0f}, (plVec2){diameter_x, diameter_y}, &identity);
            dc_app_draw_convex_polygon_filled(ctx, points, (uint32_t)num_points, (DcAppVec4){
                .r = fill_color[0],
                .g = fill_color[1],
                .b = fill_color[2],
                .a = fill_color[3],
            });
            dc_app_draw_context_pop(ctx);
        }
    }

    // draw outline
    if (node->ellipse.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) {
        float line_color[4]  = {
            node->ellipse.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.line_color.r)->value_double,
            node->ellipse.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.line_color.g)->value_double,
            node->ellipse.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.line_color.b)->value_double,
            node->ellipse.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.line_color.a)->value_double,
        };
        DcAppStroke stroke = {
            .color   = {
                .r = line_color[0],
                .g = line_color[1],
                .b = line_color[2],
                .a = line_color[3],
            },
            .width   = node->ellipse.line_width == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->ellipse.line_width)->value_double,
            .pattern = node->ellipse.line_pattern == DC_APP_VAL_INDEX_UNDEFINED ? 0 : (uint8_t)dc_app_lookup_get_value(renderer->lookup, node->ellipse.line_pattern)->value_integer,
        };
        plMat4 identity = pl_identity_mat4();
        dc_app_draw_context_push(ctx, (plVec2){0.0f, 0.0f}, (plVec2){diameter_x, diameter_y}, &identity);
        if (is_pie && !(node->ellipse.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED)) {
            dc_app_draw_polyline(ctx, &points[1], (uint32_t)(num_points - 1), stroke);
        } else {
            dc_app_draw_polygon(ctx, points, (uint32_t)num_points, stroke);
        }
        dc_app_draw_context_pop(ctx);
    }

    // mouse events
    if (node->ellipse.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            dc_app_draw_context_get_screen_mouse(ctx)->x,
            dc_app_draw_context_get_screen_mouse(ctx)->y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in (ellipse equation: (x/a)^2 + (y/b)^2 <= 1)
        bool inside = false;
        if (mouse_position.x > 0 && mouse_position.x < diameter_x && mouse_position.y > 0 && mouse_position.y < diameter_y) {

            // now do the actual check using ellipse equation
            float dx              = mouse_position.x - radius_x;
            float dy              = mouse_position.y - radius_y;
            float normalized_dist = (dx * dx) / (radius_x * radius_x) + (dy * dy) / (radius_y * radius_y);
            inside                = normalized_dist <= 1.0f;
        }

        // update global states
        if (inside) {
            dc_app_draw_mouse_register_target(ctx, (DcAppDrawTargetId)node_index);
        }

        // set state flags for children to check
        node->ellipse.state_flags = NODE_STATE_FLAG_NONE;
        if (dc_app_draw_mouse_target_pressed(ctx, (DcAppDrawTargetId)node_index)) {
            node->ellipse.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (dc_app_draw_mouse_target_active(ctx, (DcAppDrawTargetId)node_index)) {
            node->ellipse.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (dc_app_draw_mouse_target_released(ctx, (DcAppDrawTargetId)node_index)) {
            node->ellipse.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (dc_app_draw_mouse_target_hovered(ctx, (DcAppDrawTargetId)node_index)) {
            node->ellipse.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 position   = (plVec2){0.0f, 0.0f};
    plVec2 dimensions = (plVec2){diameter_x, diameter_y};
    dc_app_draw_context_push(ctx, position, dimensions, &transform);
    _render_node_list(ctx, renderer, node->ellipse.child);
    dc_app_draw_context_pop(ctx);
}

static void _render_container(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_position = &parent_frame.position;
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // boolean checks
    bool use_dimension[2] = {
        node->container.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->container.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_virtual_dimension[2] = {
        node->container.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->container.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation           = node->container.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->container.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->container.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->container.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->container.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->container.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->container.dimension.y)->value_double : parent_dimensions->y};

    // get virtual dimensions
    float virtual_dimension[2] = {
        use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->container.virtual_dimension.x)->value_double : dimension[0],
        use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->container.virtual_dimension.y)->value_double : dimension[1]};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(renderer->lookup, node->container.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(renderer->lookup, node->container.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->container.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->container.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->container.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->container.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->container.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2] = {0, 0};
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("Container", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                    break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("Container", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->container.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->container.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->container.local_align.x)->value_integer,
            node->container.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->container.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2] = {0, 0};
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                DC_LOG_WARN("Text", "Unknown X alignment: %d", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                DC_LOG_WARN("Text", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->container.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->container.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float          anchor[2]      = {0, 0};
        DcAppAlignType parent_align_x = node->container.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->container.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Container", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->container.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->container.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Container", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->container.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->container.position.y)->value_double : 0};

        // apply negate
        if (node->container.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->container.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->container.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->container.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform                   = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->container.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->container.pivot_local_align.x)->value_integer,
                node->container.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->container.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2] = {0, 0};
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    DC_LOG_WARN("Container", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    DC_LOG_WARN("Container", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->container.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform scale
    {
        // compute matrix
        plMat4 scale_xform = pl_mat4_scale_xyz(dimension[0] / virtual_dimension[0], dimension[1] / virtual_dimension[1], 1.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &scale_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // mouse events
    if (node->container.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            dc_app_draw_context_get_screen_mouse(ctx)->x,
            dc_app_draw_context_get_screen_mouse(ctx)->y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        bool inside = mouse_position.x > 0 && mouse_position.x < virtual_dimension[0] && mouse_position.y > 0 && mouse_position.y < virtual_dimension[1];

        // update global states
        if (inside) {
            dc_app_draw_mouse_register_target(ctx, (DcAppDrawTargetId)node_index);
        }

        // set state flags for children to check
        node->container.state_flags = NODE_STATE_FLAG_NONE;
        if (dc_app_draw_mouse_target_pressed(ctx, (DcAppDrawTargetId)node_index)) {
            node->container.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (dc_app_draw_mouse_target_active(ctx, (DcAppDrawTargetId)node_index)) {
            node->container.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (dc_app_draw_mouse_target_released(ctx, (DcAppDrawTargetId)node_index)) {
            node->container.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (dc_app_draw_mouse_target_hovered(ctx, (DcAppDrawTargetId)node_index)) {
            node->container.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 virtual_dimensions_vec2 = (plVec2){virtual_dimension[0], virtual_dimension[1]};
    plVec2 position_vec2           = (plVec2){0.0f, 0.0f};
    dc_app_draw_context_push(ctx, position_vec2, virtual_dimensions_vec2, &transform);
    _render_node_list(ctx, renderer, node->container.child);
    dc_app_draw_context_pop(ctx);
}

static void _render_conditional(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {

    DcValue *val1 = dc_app_lookup_get_value(renderer->lookup, node->conditional.value1);
    if (!val1) {
        // DC_LOG_WARN("If", "Value1 is undefined, skipping conditional");
        return;
    }
    DcAppConditionalType type     = (node->conditional.type == DC_APP_VAL_INDEX_UNDEFINED)
                                        ? DC_APP_CONDITIONAL_TYPE_TRUE
                                        : (DcAppConditionalType)dc_app_lookup_get_value(renderer->lookup, node->conditional.type)->value_integer;
    bool                 use_val2 = node->conditional.value2 != DC_APP_VAL_INDEX_UNDEFINED;

    // evaluate
    bool result = false;
    if (use_val2) {
        DcValue *val2 = dc_app_lookup_get_value(renderer->lookup, node->conditional.value2);
        if (!val2) {
            DC_LOG_WARN("If", "Value2 is undefined, skipping conditional");
            return;
        }

        switch (type) {
            case DC_APP_CONDITIONAL_TYPE_EQ:
                result = dc_value_is_equal(val1, val2);
                break;
            case DC_APP_CONDITIONAL_TYPE_NE:
                result = dc_value_is_not_equal(val1, val2);
                break;
            case DC_APP_CONDITIONAL_TYPE_LT:
                result = dc_value_is_less(val1, val2);
                break;
            case DC_APP_CONDITIONAL_TYPE_GT:
                result = dc_value_is_greater(val1, val2);
                break;
            case DC_APP_CONDITIONAL_TYPE_LTE:
                result = dc_value_is_less_or_equal(val1, val2);
                break;
            case DC_APP_CONDITIONAL_TYPE_GTE:
                result = dc_value_is_greater_or_equal(val1, val2);
                break;
            default:
                DC_LOG_WARN("If", "Unknown conditional type %d with Val2", type);
                break;
        }
    } else {
        switch (type) {
            case DC_APP_CONDITIONAL_TYPE_TRUE:
                result = val1->value_boolean;
                break;
            case DC_APP_CONDITIONAL_TYPE_FALSE:
                result = !(val1->value_boolean);
                break;
            default:
                DC_LOG_WARN("If", "Unknown conditional type %d with Val1 only", type);
                break;
        }
    }

    // set state flags for child state event nodes to check
    node->conditional.state_flags = result ? NODE_STATE_FLAG_TRUE : NODE_STATE_FLAG_FALSE;

    // draw children (state event nodes will check our state_flags)
    _render_node_list(ctx, renderer, node->conditional.child);
}

static void _execute_function(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    if (!node->function.callback) return;

    if (node->function.fire_call != DC_APP_VAL_INDEX_UNDEFINED) {
        DcValue *val = dc_app_lookup_get_value(renderer->lookup, node->function.fire_call);
        if (!dc_value_is_equal(val, &node->function.last_fire_call_value)) {
            node->function.callback(renderer->callback_context, dc_app_logic_user_data(renderer->logic));
            node->function.last_fire_call_value = *val;
        }
    } else {
        node->function.callback(renderer->callback_context, dc_app_logic_user_data(renderer->logic));
    }
}

static void _execute_draw_function(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    if (!node->draw_function.callback) return;

    int arg_count = sbcount(node->draw_function.sb_args);
    if (arg_count > DCAPP_DRAW_FUNCTION_ARG_MAX) {
        DC_LOG_WARN("DrawFunction", "DrawFunction arg count %d exceeds max %d", arg_count, DCAPP_DRAW_FUNCTION_ARG_MAX);
        return;
    }

    sbclear(renderer->sb_draw_function_args);
    for (int ii = 0; ii < arg_count; ii++) {
        DcAppDrawFunctionArg *arg = &node->draw_function.sb_args[ii];
        DcValue *value = dc_app_lookup_get_value(renderer->lookup, arg->value);
        DcAppDrawFuncArg resolved = {.type = arg->type};
        if (value) {
            resolved.value_string = value->value_string;
            resolved.value_integer = value->value_integer;
            resolved.value_double = value->value_double;
            resolved.value_boolean = value->value_boolean;
        }
        sbpush(renderer->sb_draw_function_args, resolved);
    }

    // Contain any draw stacks opened by user code so they cannot affect later siblings.
    DcAppDrawScope scope = dc_app_draw_scope_begin(ctx);
    DcAppDrawFuncArgs args = {
        .count  = (uint32_t)arg_count,
        .values = arg_count > 0 ? renderer->sb_draw_function_args : NULL,
    };
    node->draw_function.callback(ctx, &args, dc_app_logic_user_data(renderer->logic));
    dc_app_draw_scope_end(ctx, scope);
}

static void _render_image(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_position = &parent_frame.position;
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // boolean checks
    bool use_dimension[2] = {
        node->image.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->image.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation           = node->image.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->image.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->image.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->image.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->image.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->image.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->image.dimension.y)->value_double : parent_dimensions->y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(renderer->lookup, node->image.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(renderer->lookup, node->image.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->image.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->image.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->image.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->image.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->image.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2] = {0, 0};
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("Image", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                    break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("Image", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->image.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->image.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->image.local_align.x)->value_integer,
            node->image.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->image.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2] = {0, 0};
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                DC_LOG_WARN("Text", "Unknown X alignment: %d", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                DC_LOG_WARN("Text", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->image.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->image.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float          anchor[2]      = {0, 0};
        DcAppAlignType parent_align_x = node->image.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->image.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Image", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->image.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->image.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Image", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->image.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->image.position.y)->value_double : 0};

        // apply negate
        if (node->image.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->image.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->image.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->image.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform                   = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->image.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->image.pivot_local_align.x)->value_integer,
                node->image.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->image.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2] = {0, 0};
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    DC_LOG_WARN("Image", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    DC_LOG_WARN("Image", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->image.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // PL specific fixes
    {
        // move from top-left reference to bottom-left
        plMat4 trans_pl_origin_xform = pl_mat4_translate_xyz(0, dimension[1], 0.0f);

        // flip over the y axis
        plMat4 scale_invert_y_xform = pl_mat4_scale_xyz(1.0f, -1.0f, 1.0f);

        // apply transforms
        transform = pl_mul_mat4t(&transform, &trans_pl_origin_xform);
        transform = pl_mul_mat4t(&transform, &scale_invert_y_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // draw (skip if texture failed to load)
    if (node->image.texture_index != TEXTURE_INDEX_UNDEFINED) {
        plBindGroupHandle bind_group = {0};
        if (!dc_app_texture_get_bind_group_handle(renderer->textures, (DcAppTextureId)node->image.texture_index, &bind_group)) return;
        dc_app_draw_context_push(ctx, (plVec2){0.0f, 0.0f}, (plVec2){dimension[0], dimension[1]}, &transform);
        dc_app_draw_image_quad_uv(ctx, bind_group.uData,
                                  (DcAppVec2){0.0f, 0.0f},
                                  (DcAppVec2){0.0f, dimension[1]},
                                  (DcAppVec2){dimension[0], dimension[1]},
                                  (DcAppVec2){dimension[0], 0.0f},
                                  (DcAppVec2){0.0f, 0.0f},
                                  (DcAppVec2){0.0f, 1.0f},
                                  (DcAppVec2){1.0f, 1.0f},
                                  (DcAppVec2){1.0f, 0.0f},
                                  (DcAppVec2){0.0f, 0.0f},
                                  (DcAppPlacement){0},
                                  (DcAppVec4){1.0f, 1.0f, 1.0f, 1.0f},
                                  NULL);
        dc_app_draw_context_pop(ctx);
    }

    // mouse events
    if (node->image.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            dc_app_draw_context_get_screen_mouse(ctx)->x,
            dc_app_draw_context_get_screen_mouse(ctx)->y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        bool inside = mouse_position.x > 0 && mouse_position.x < dimension[0] && mouse_position.y > 0 && mouse_position.y < dimension[1];

        // update global states
        if (inside) {
            dc_app_draw_mouse_register_target(ctx, (DcAppDrawTargetId)node_index);
        }

        // set state flags for children to check
        node->image.state_flags = NODE_STATE_FLAG_NONE;
        if (dc_app_draw_mouse_target_pressed(ctx, (DcAppDrawTargetId)node_index)) {
            node->image.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (dc_app_draw_mouse_target_active(ctx, (DcAppDrawTargetId)node_index)) {
            node->image.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (dc_app_draw_mouse_target_released(ctx, (DcAppDrawTargetId)node_index)) {
            node->image.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (dc_app_draw_mouse_target_hovered(ctx, (DcAppDrawTargetId)node_index)) {
            node->image.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 position   = (plVec2){0.0f, 0.0f};
    plVec2 dimensions = (plVec2){dimension[0], dimension[1]};
    dc_app_draw_context_push(ctx, position, dimensions, &transform);
    _render_node_list(ctx, renderer, node->image.child);
    dc_app_draw_context_pop(ctx);
}

static void _render_line(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // boolean checks
    bool use_rotation           = node->line.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->line.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->line.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->line.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->line.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(renderer->lookup, node->line.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(renderer->lookup, node->line.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->line.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->line.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->line.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->line.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->line.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2] = {0, 0};
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("Line", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                    break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("Line", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->line.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform position
    {
        // boolean check
        bool use_position[2] = {
            node->line.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->line.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        // get position
        float position[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->line.position.x)->value_double : 0.0f,
            use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->line.position.y)->value_double : 0.0f,
        };

        // apply negate
        if (node->line.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->line.negate_x)->value_boolean) {
            position[0] = -position[0];
        }
        if (node->line.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->line.negate_y)->value_boolean) {
            position[1] = -position[1];
        }

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // get points, min/max
    plVec2 min_pos    = (plVec2){FLT_MAX, FLT_MAX};
    plVec2 max_pos    = (plVec2){FLT_MIN, FLT_MIN};
    int    num_points = sbcount(node->line.sb_vertices);
    plVec2 raw_points[DC_APP_NODE_LINE_MAX_POINTS];
    DcAppVec2 points[DC_APP_NODE_LINE_MAX_POINTS];
    for (int ii = 0; ii < num_points; ii++) {

        // get raw point position
        float point_x = (float)dc_app_lookup_get_value(renderer->lookup, node->line.sb_vertices[ii].position.x)->value_double;
        float point_y = (float)dc_app_lookup_get_value(renderer->lookup, node->line.sb_vertices[ii].position.y)->value_double;

        // apply vertex negate
        if (node->line.sb_vertices[ii].negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->line.sb_vertices[ii].negate_x)->value_boolean) {
            point_x = -point_x;
        }
        if (node->line.sb_vertices[ii].negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->line.sb_vertices[ii].negate_y)->value_boolean) {
            point_y = -point_y;
        }

        // apply vertex parent_align offset
        if (node->line.sb_vertices[ii].parent_align.x != DC_APP_VAL_INDEX_UNDEFINED) {
            int parent_align_x = dc_app_lookup_get_value(renderer->lookup, node->line.sb_vertices[ii].parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_LEFT:
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    point_x += parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    point_x += parent_dimensions->x;
                    break;
                default:
                    break;
            }
        }
        if (node->line.sb_vertices[ii].parent_align.y != DC_APP_VAL_INDEX_UNDEFINED) {
            int parent_align_y = dc_app_lookup_get_value(renderer->lookup, node->line.sb_vertices[ii].parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    point_y += parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    point_y += parent_dimensions->y;
                    break;
                default:
                    break;
            }
        }

        raw_points[ii] = (plVec2){point_x, point_y};

        // update max/min
        min_pos.x = fminf(min_pos.x, raw_points[ii].x);
        min_pos.y = fminf(min_pos.y, raw_points[ii].y);
        max_pos.x = fmaxf(max_pos.x, raw_points[ii].x);
        max_pos.y = fmaxf(max_pos.y, raw_points[ii].y);

        points[ii] = (DcAppVec2){raw_points[ii].x, raw_points[ii].y};
    }

    // draw outline
    if (node->line.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) {
        float line_color[4]  = {
            node->line.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->line.line_color.r)->value_double,
            node->line.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->line.line_color.g)->value_double,
            node->line.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->line.line_color.b)->value_double,
            node->line.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->line.line_color.a)->value_double,
        };
        DcAppStroke stroke = {
            .color   = {
                .r = line_color[0],
                .g = line_color[1],
                .b = line_color[2],
                .a = line_color[3],
            },
            .width   = node->line.line_width == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->line.line_width)->value_double,
            .pattern = node->line.line_pattern == DC_APP_VAL_INDEX_UNDEFINED ? 0 : (uint8_t)dc_app_lookup_get_value(renderer->lookup, node->line.line_pattern)->value_integer,
        };
        dc_app_draw_context_push(ctx, min_pos, (plVec2){max_pos.x - min_pos.x, max_pos.y - min_pos.y}, &transform);
        dc_app_draw_polyline(ctx, points, (uint32_t)num_points, stroke);
        dc_app_draw_context_pop(ctx);
    }
}

static void _render_panel(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // DisplayIndex/ActiveDisplay check - skip panel if index doesn't match window's active display
    DcAppNode *window_node = dc_app_scene_get_node(renderer->scene, dc_app_scene_get_window(renderer->scene));
    DcAppValIndex window_active_display = window_node ? window_node->window.active_display : DC_APP_VAL_INDEX_UNDEFINED;
    DcAppValIndex panel_index           = node->panel.index;
    if (window_active_display != DC_APP_VAL_INDEX_UNDEFINED && panel_index != DC_APP_VAL_INDEX_UNDEFINED) {
        int active_display_value = dc_app_lookup_get_value(renderer->lookup, window_active_display)->value_integer;
        int panel_index_value    = dc_app_lookup_get_value(renderer->lookup, panel_index)->value_integer;
        if (active_display_value != panel_index_value) {
            return; // skip this panel - DisplayIndex doesn't match ActiveDisplay
        }
    }

    // boolean checks
    bool use_virtual_dimension[2] = {
        node->panel.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->panel.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};

    // get virtual dimensions
    float virtual_dimension[2] = {
        use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->panel.virtual_dimension.x)->value_double : parent_dimensions->x,
        use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->panel.virtual_dimension.y)->value_double : parent_dimensions->y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform scale
    {
        // compute matrix
        plMat4 scale_xform = pl_mat4_scale_xyz(parent_dimensions->x / virtual_dimension[0], parent_dimensions->y / virtual_dimension[1], 1.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &scale_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // draw background
    if (node->panel.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) {
        float bg[4] = {
            node->panel.background_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->panel.background_color.r)->value_double,
            node->panel.background_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->panel.background_color.g)->value_double,
            node->panel.background_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->panel.background_color.b)->value_double,
            node->panel.background_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->panel.background_color.a)->value_double,
        };
        dc_app_draw_context_push(ctx, (plVec2){0.0f, 0.0f}, (plVec2){virtual_dimension[0], virtual_dimension[1]}, &transform);
        dc_app_draw_rect_filled(ctx, (DcAppVec2){0.0f, 0.0f}, (DcAppVec2){virtual_dimension[0], virtual_dimension[1]}, (DcAppVec4){
            .r = bg[0],
            .g = bg[1],
            .b = bg[2],
            .a = bg[3],
        });
        dc_app_draw_context_pop(ctx);
    }

    // draw children
    plVec2 virtual_dimensions_vec2 = (plVec2){virtual_dimension[0], virtual_dimension[1]};
    plVec2 position_vec2           = (plVec2){0.0f, 0.0f};
    dc_app_draw_context_push(ctx, position_vec2, virtual_dimensions_vec2, &transform);
    _render_node_list(ctx, renderer, node->panel.child);
    dc_app_draw_context_pop(ctx);
}

static void _render_pixelstream(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_position = &parent_frame.position;
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // get source (data fetching + GPU upload already done at top of frame)
    if (node->pixelstream.source_index == DC_APP_PIXELSTREAM_SOURCE_INDEX_UNDEFINED) return;
    DcAppPixelstreamState source = {0};
    if (!dc_app_pixelstream_get_state(renderer->pixelstreams, node->pixelstream.source_index, &source)) return;

    // determine whether to use test pattern
    bool use_test_pattern = !source.connected && node->pixelstream.test_pattern_texture_index != TEXTURE_INDEX_UNDEFINED;

    // don't draw anything if no data and no test pattern
    if ((source.width <= 0 || source.height <= 0) && !use_test_pattern) return;

    // boolean checks
    bool use_dimension[2] = {
        node->pixelstream.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->pixelstream.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation           = node->pixelstream.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->pixelstream.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->pixelstream.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->pixelstream.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->pixelstream.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.dimension.y)->value_double : parent_dimensions->y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->pixelstream.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->pixelstream.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2] = {0, 0};
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("Pixelstream", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                    break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("Pixelstream", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->pixelstream.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.local_align.x)->value_integer,
            node->pixelstream.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2] = {0, 0};
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                DC_LOG_WARN("Text", "Unknown X alignment: %d", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                DC_LOG_WARN("Text", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->pixelstream.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->pixelstream.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float          anchor[2]      = {0, 0};
        DcAppAlignType parent_align_x = node->pixelstream.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->pixelstream.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("PixelStream", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->pixelstream.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->pixelstream.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("PixelStream", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.position.y)->value_double : 0};

        // apply negate
        if (node->pixelstream.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->pixelstream.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->pixelstream.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->pixelstream.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform                   = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->pixelstream.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.pivot_local_align.x)->value_integer,
                node->pixelstream.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2] = {0, 0};
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    DC_LOG_WARN("PixelStream", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    DC_LOG_WARN("PixelStream", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->pixelstream.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // PL specific fixes
    {
        // move from top-left reference to bottom-left
        plMat4 trans_pl_origin_xform = pl_mat4_translate_xyz(0, dimension[1], 0.0f);

        // flip over the y axis
        plMat4 scale_invert_y_xform = pl_mat4_scale_xyz(1.0f, -1.0f, 1.0f);

        // apply transforms
        transform = pl_mul_mat4t(&transform, &trans_pl_origin_xform);
        transform = pl_mul_mat4t(&transform, &scale_invert_y_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // compute UV coordinates
    plVec2            min_uv = {0.0f, 0.0f};
    plVec2            max_uv;
    plBindGroupHandle bind_group_handle;

    if (use_test_pattern) {
        // test pattern uses full texture
        if (!dc_app_texture_get_bind_group_handle(renderer->textures, (DcAppTextureId)node->pixelstream.test_pattern_texture_index, &bind_group_handle)) return;
        max_uv = (plVec2){1.0f, 1.0f};
    } else {
        // streaming texture
        DcAppVec2 texture_size = {0};
        if (!dc_app_texture_get_size(renderer->textures, source.texture, &texture_size) ||
            !dc_app_texture_get_bind_group_handle(renderer->textures, source.texture, &bind_group_handle)) return;
        max_uv = (plVec2){
            ((float)source.width) / texture_size.x,
            ((float)source.height) / texture_size.y};
    }

    // draw
    {
        dc_app_draw_context_push(ctx, (plVec2){0.0f, 0.0f}, (plVec2){dimension[0], dimension[1]}, &transform);
        dc_app_draw_image_quad_uv(ctx, bind_group_handle.uData,
                                  (DcAppVec2){0.0f, 0.0f},
                                  (DcAppVec2){0.0f, dimension[1]},
                                  (DcAppVec2){dimension[0], dimension[1]},
                                  (DcAppVec2){dimension[0], 0.0f},
                                  (DcAppVec2){min_uv.x, min_uv.y},
                                  (DcAppVec2){min_uv.x, max_uv.y},
                                  (DcAppVec2){max_uv.x, max_uv.y},
                                  (DcAppVec2){max_uv.x, min_uv.y},
                                  (DcAppVec2){0.0f, 0.0f},
                                  (DcAppPlacement){0}, (DcAppVec4){
                                      .r = 1.0f,
                                      .g = 1.0f,
                                      .b = 1.0f,
                                      .a = 1.0f},
                                  NULL);
        dc_app_draw_context_pop(ctx);
    }

    // mouse events
    if (node->pixelstream.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            dc_app_draw_context_get_screen_mouse(ctx)->x,
            dc_app_draw_context_get_screen_mouse(ctx)->y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        bool inside = mouse_position.x > 0 && mouse_position.x < dimension[0] && mouse_position.y > 0 && mouse_position.y < dimension[1];

        // update global states
        if (inside) {
            dc_app_draw_mouse_register_target(ctx, (DcAppDrawTargetId)node_index);
        }

        // set state flags for children to check
        node->pixelstream.state_flags = NODE_STATE_FLAG_NONE;
        if (dc_app_draw_mouse_target_pressed(ctx, (DcAppDrawTargetId)node_index)) {
            node->pixelstream.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (dc_app_draw_mouse_target_active(ctx, (DcAppDrawTargetId)node_index)) {
            node->pixelstream.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (dc_app_draw_mouse_target_released(ctx, (DcAppDrawTargetId)node_index)) {
            node->pixelstream.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (dc_app_draw_mouse_target_hovered(ctx, (DcAppDrawTargetId)node_index)) {
            node->pixelstream.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 position   = (plVec2){0.0f, 0.0f};
    plVec2 dimensions = (plVec2){dimension[0], dimension[1]};
    dc_app_draw_context_push(ctx, position, dimensions, &transform);
    _render_node_list(ctx, renderer, node->pixelstream.child);
    dc_app_draw_context_pop(ctx);
}

static void _render_polygon(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_position = &parent_frame.position;
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // boolean checks
    bool use_rotation           = node->polygon.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->polygon.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->polygon.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->polygon.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->polygon.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->polygon.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->polygon.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->polygon.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->polygon.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->polygon.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2] = {0, 0};
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("Polygon", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                    break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("Polygon", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->polygon.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform position
    {
        bool use_position[2] = {
            node->polygon.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->polygon.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float          anchor[2]      = {0, 0};
        DcAppAlignType parent_align_x = node->polygon.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->polygon.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("polygon", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->polygon.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->polygon.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("polygon", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.position.y)->value_double : 0};

        // apply negate
        if (node->polygon.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->polygon.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->polygon.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->polygon.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform                   = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // get points, min/max
    plVec2 min_pos    = (plVec2){FLT_MAX, FLT_MAX};
    plVec2 max_pos    = (plVec2){FLT_MIN, FLT_MIN};
    int    num_points = sbcount(node->polygon.sb_vertices);
    plVec2 raw_points[DC_APP_NODE_POLYGON_MAX_POINTS];
    DcAppVec2 points[DC_APP_NODE_POLYGON_MAX_POINTS];
    for (int ii = 0; ii < num_points; ii++) {

        // get raw point position
        float point_x = (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.sb_vertices[ii].position.x)->value_double;
        float point_y = (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.sb_vertices[ii].position.y)->value_double;

        // apply vertex negate
        if (node->polygon.sb_vertices[ii].negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->polygon.sb_vertices[ii].negate_x)->value_boolean) {
            point_x = -point_x;
        }
        if (node->polygon.sb_vertices[ii].negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->polygon.sb_vertices[ii].negate_y)->value_boolean) {
            point_y = -point_y;
        }

        // apply vertex parent_align offset
        if (node->polygon.sb_vertices[ii].parent_align.x != DC_APP_VAL_INDEX_UNDEFINED) {
            int parent_align_x = dc_app_lookup_get_value(renderer->lookup, node->polygon.sb_vertices[ii].parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_LEFT:
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    point_x += parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    point_x += parent_dimensions->x;
                    break;
                default:
                    break;
            }
        }
        if (node->polygon.sb_vertices[ii].parent_align.y != DC_APP_VAL_INDEX_UNDEFINED) {
            int parent_align_y = dc_app_lookup_get_value(renderer->lookup, node->polygon.sb_vertices[ii].parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    point_y += parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    point_y += parent_dimensions->y;
                    break;
                default:
                    break;
            }
        }

        raw_points[ii] = (plVec2){point_x, point_y};

        // update max/min
        min_pos.x = fminf(min_pos.x, raw_points[ii].x);
        min_pos.y = fminf(min_pos.y, raw_points[ii].y);
        max_pos.x = fmaxf(max_pos.x, raw_points[ii].x);
        max_pos.y = fmaxf(max_pos.y, raw_points[ii].y);

        points[ii] = (DcAppVec2){raw_points[ii].x, raw_points[ii].y};
    }

    // rounded check
    bool is_rounded = node->polygon.rounded != DC_APP_VAL_INDEX_UNDEFINED &&
                      dc_app_lookup_get_value(renderer->lookup, node->polygon.rounded)->value_boolean;
    float corner_radius = is_rounded ? fminf(max_pos.x - min_pos.x, max_pos.y - min_pos.y) * 0.1f : 0.0f;

        float fill_color[4] = {
            node->polygon.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.fill_color.r)->value_double,
            node->polygon.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.fill_color.g)->value_double,
            node->polygon.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.fill_color.b)->value_double,
            node->polygon.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.fill_color.a)->value_double,
        };
        float line_color[4]  = {
            node->polygon.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.line_color.r)->value_double,
            node->polygon.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.line_color.g)->value_double,
            node->polygon.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.line_color.b)->value_double,
            node->polygon.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.line_color.a)->value_double,
        };
    dc_app_draw_context_push(ctx, min_pos, (plVec2){max_pos.x - min_pos.x, max_pos.y - min_pos.y}, &transform);
    if (node->polygon.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) {
        dc_app_draw_rounded_convex_polygon_filled(ctx, points, (uint32_t)num_points, corner_radius, (DcAppVec4){
            .r = fill_color[0],
            .g = fill_color[1],
            .b = fill_color[2],
            .a = fill_color[3],
        });
    }
    if (node->polygon.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) {
        DcAppStroke stroke = {
            .color   = {
                .r = line_color[0],
                .g = line_color[1],
                .b = line_color[2],
                .a = line_color[3],
            },
            .width   = node->polygon.line_width == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->polygon.line_width)->value_double,
            .pattern = node->polygon.line_pattern == DC_APP_VAL_INDEX_UNDEFINED ? 0 : (uint8_t)dc_app_lookup_get_value(renderer->lookup, node->polygon.line_pattern)->value_integer,
        };
        dc_app_draw_rounded_polygon(ctx, points, (uint32_t)num_points, corner_radius, stroke);
    }
    dc_app_draw_context_pop(ctx);

    // mouse events
    if (node->polygon.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            dc_app_draw_context_get_screen_mouse(ctx)->x,
            dc_app_draw_context_get_screen_mouse(ctx)->y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        // first do the simple check to make sure it's even within the bounds (for performance)
        bool inside = false;
        if (mouse_position.x > min_pos.x && mouse_position.x < max_pos.x && mouse_position.y > min_pos.y && mouse_position.y < max_pos.y) {

            // now do the actual check
            for (int ii = 0, jj = num_points - 1; ii < num_points; jj = ii++) {
                double xi = raw_points[ii].x, yi = raw_points[ii].y;
                double xj = raw_points[jj].x, yj = raw_points[jj].y;

                bool intersect = ((yi > mouse_position.y) != (yj > mouse_position.y)) && (mouse_position.x < (xj - xi) * (mouse_position.y - yi) / (yj - yi + 1e-12) + xi);
                if (intersect) {
                    inside = !inside;
                }
            }
        }

        // update global states
        if (inside) {
            dc_app_draw_mouse_register_target(ctx, (DcAppDrawTargetId)node_index);
        }

        // set state flags for children to check
        node->polygon.state_flags = NODE_STATE_FLAG_NONE;
        if (dc_app_draw_mouse_target_pressed(ctx, (DcAppDrawTargetId)node_index)) {
            node->polygon.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (dc_app_draw_mouse_target_active(ctx, (DcAppDrawTargetId)node_index)) {
            node->polygon.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (dc_app_draw_mouse_target_released(ctx, (DcAppDrawTargetId)node_index)) {
            node->polygon.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (dc_app_draw_mouse_target_hovered(ctx, (DcAppDrawTargetId)node_index)) {
            node->polygon.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 position   = (plVec2){min_pos.x, min_pos.y};
    plVec2 dimensions = (plVec2){max_pos.x - min_pos.x, max_pos.y - min_pos.y};
    dc_app_draw_context_push(ctx, position, dimensions, &transform);
    _render_node_list(ctx, renderer, node->polygon.child);
    dc_app_draw_context_pop(ctx);
}

static void _render_rectangle(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_position = &parent_frame.position;
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // boolean checks
    bool use_dimension[2] = {
        node->rectangle.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->rectangle.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation           = node->rectangle.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->rectangle.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->rectangle.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->rectangle.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->rectangle.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.dimension.y)->value_double : parent_dimensions->y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->rectangle.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->rectangle.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->rectangle.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->rectangle.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2] = {0, 0};
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("Rectangle", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                    break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("Rectangle", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->rectangle.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->rectangle.local_align.x)->value_integer,
            node->rectangle.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->rectangle.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2] = {0, 0};
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                DC_LOG_WARN("Text", "Unknown X alignment: %d", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                DC_LOG_WARN("Text", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->rectangle.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->rectangle.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float          anchor[2]      = {0, 0};
        DcAppAlignType parent_align_x = node->rectangle.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->rectangle.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Rectangle", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->rectangle.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->rectangle.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Rectangle", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.position.y)->value_double : 0};

        // apply negate
        if (node->rectangle.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->rectangle.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->rectangle.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->rectangle.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform                   = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->rectangle.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->rectangle.pivot_local_align.x)->value_integer,
                node->rectangle.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->rectangle.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2] = {0, 0};
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    DC_LOG_WARN("Rectangle", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    DC_LOG_WARN("Rectangle", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // get points
    plVec2 raw_points[4] = {
        (plVec2){0.0f, 0.0f},
        (plVec2){dimension[0], 0.0f},
        (plVec2){dimension[0], dimension[1]},
        (plVec2){0.0f, dimension[1]}};

    // rounded check
    bool is_rounded = node->rectangle.rounded != DC_APP_VAL_INDEX_UNDEFINED &&
                      dc_app_lookup_get_value(renderer->lookup, node->rectangle.rounded)->value_boolean;
    float corner_radius = is_rounded ? fminf(dimension[0], dimension[1]) * 0.1f : 0.0f;

    DcAppVec2 quad[4] = {
        {raw_points[0].x, raw_points[0].y},
        {raw_points[1].x, raw_points[1].y},
        {raw_points[2].x, raw_points[2].y},
        {raw_points[3].x, raw_points[3].y},
    };
        float fill_color[4] = {
            node->rectangle.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.fill_color.r)->value_double,
            node->rectangle.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.fill_color.g)->value_double,
            node->rectangle.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.fill_color.b)->value_double,
            node->rectangle.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.fill_color.a)->value_double,
        };
        float line_color[4]  = {
            node->rectangle.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.line_color.r)->value_double,
            node->rectangle.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.line_color.g)->value_double,
            node->rectangle.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.line_color.b)->value_double,
            node->rectangle.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.line_color.a)->value_double,
    };
    dc_app_draw_context_push(ctx, (plVec2){0.0f, 0.0f}, (plVec2){dimension[0], dimension[1]}, &transform);
    if (node->rectangle.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) {
        dc_app_draw_rounded_quad_filled(ctx, quad[0],
                                       quad[1],
                                       quad[2],
                                       quad[3],
                                       corner_radius, (DcAppVec4){
            .r = fill_color[0],
            .g = fill_color[1],
            .b = fill_color[2],
            .a = fill_color[3],
        });
    }
    if (node->rectangle.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) {
        DcAppStroke stroke = {
            .color   = {
                .r = line_color[0],
                .g = line_color[1],
                .b = line_color[2],
                .a = line_color[3],
            },
            .width   = node->rectangle.line_width == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->rectangle.line_width)->value_double,
            .pattern = node->rectangle.line_pattern == DC_APP_VAL_INDEX_UNDEFINED ? 0 : (uint8_t)dc_app_lookup_get_value(renderer->lookup, node->rectangle.line_pattern)->value_integer,
        };
        dc_app_draw_rounded_quad(ctx, quad[0],
                                quad[1],
                                quad[2],
                                quad[3],
                                corner_radius, stroke);
    }
    dc_app_draw_context_pop(ctx);

    // mouse events
    if (node->rectangle.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            dc_app_draw_context_get_screen_mouse(ctx)->x,
            dc_app_draw_context_get_screen_mouse(ctx)->y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        bool inside = mouse_position.x > 0 && mouse_position.x < dimension[0] && mouse_position.y > 0 && mouse_position.y < dimension[1];

        // update global states
        if (inside) {
            dc_app_draw_mouse_register_target(ctx, (DcAppDrawTargetId)node_index);
        }

        // set state flags for children to check
        node->rectangle.state_flags = NODE_STATE_FLAG_NONE;
        if (dc_app_draw_mouse_target_pressed(ctx, (DcAppDrawTargetId)node_index)) {
            node->rectangle.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (dc_app_draw_mouse_target_active(ctx, (DcAppDrawTargetId)node_index)) {
            node->rectangle.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (dc_app_draw_mouse_target_released(ctx, (DcAppDrawTargetId)node_index)) {
            node->rectangle.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (dc_app_draw_mouse_target_hovered(ctx, (DcAppDrawTargetId)node_index)) {
            node->rectangle.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 position   = (plVec2){0.0f, 0.0f};
    plVec2 dimensions = (plVec2){dimension[0], dimension[1]};
    dc_app_draw_context_push(ctx, position, dimensions, &transform);
    _render_node_list(ctx, renderer, node->rectangle.child);
    dc_app_draw_context_pop(ctx);
}

static void _process_mouse_motion(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plMat4 *parent_transform = &parent_frame.transform;
    // Only update if mouse is down (being dragged)
    if (!dc_app_draw_context_get_screen_mouse(ctx)->down) {
        return;
    }

    // Get mouse position in window coordinates
    const DcAppMouse *mouse = dc_app_draw_context_get_screen_mouse(ctx);
    plVec2 mouse_pos = {mouse->x, mouse->y};

    // Transform mouse position from screen space to parent's virtual coordinate space
    plMat4 inv_transform = pl_mat4t_invert(parent_transform);
    plVec4 mouse_screen  = {mouse_pos.x, mouse_pos.y, 0.0f, 1.0f};
    plVec4 mouse_local   = pl_mul_mat4_vec4(&inv_transform, mouse_screen);

    // Set VariableX if defined
    if (node->mouse_motion.var_x != DC_APP_VAR_INDEX_UNDEFINED) {
        DcValue *var_x = dc_app_lookup_get_value(renderer->lookup, dc_app_lookup_get_var_value_index(renderer->lookup, node->mouse_motion.var_x));
        if (var_x->type == DC_VALUE_TYPE_DOUBLE) {
            var_x->value_double = mouse_local.x;
        } else if (var_x->type == DC_VALUE_TYPE_INTEGER) {
            var_x->value_integer = (int)mouse_local.x;
        }
    }

    // Set VariableY if defined
    if (node->mouse_motion.var_y != DC_APP_VAR_INDEX_UNDEFINED) {
        DcValue *var_y = dc_app_lookup_get_value(renderer->lookup, dc_app_lookup_get_var_value_index(renderer->lookup, node->mouse_motion.var_y));
        if (var_y->type == DC_VALUE_TYPE_DOUBLE) {
            var_y->value_double = mouse_local.y;
        } else if (var_y->type == DC_VALUE_TYPE_INTEGER) {
            var_y->value_integer = (int)mouse_local.y;
        }
    }
}

static bool _apply_set_operation(DcAppRenderer *renderer, DcAppVarIndex var_index, DcValue *var_value, DcValue *op_value, DcAppSetType operation) {
    switch (operation) {
        case DC_APP_SET_TYPE_UNDEFINED:
        case DC_APP_SET_TYPE_EQUAL:
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    strncpy(var_value->value_string, op_value->value_string, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    var_value->value_string[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean = op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_ADD:
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING: {
                    int num_chars = DC_VALUE_STRING_BUFFER_SIZE - (int)strlen(var_value->value_string) - 1;
                    strncat(var_value->value_string, op_value->value_string, num_chars);
                    break;
                }
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer += op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double += op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean += op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_SUBTRACT:
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer -= op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double -= op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean -= op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_MULTIPLY:
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer *= op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double *= op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean *= op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_DIVIDE:
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    if (op_value->value_integer == 0) {
                        DC_LOG_ERROR("Set", "Divide by zero (int)");
                        return false;
                    }
                    var_value->value_integer /= op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (op_value->value_double == 0.0) {
                        DC_LOG_ERROR("Set", "Divide by zero (double)");
                        return false;
                    }
                    var_value->value_double /= op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    if (op_value->value_boolean == 0) {
                        DC_LOG_ERROR("Set", "Divide by zero (bool)");
                        return false;
                    }
                    var_value->value_boolean /= op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_MIN:
            // min(var, operand) - caps value at operand (upper bound)
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer > op_value->value_integer)
                        var_value->value_integer = op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (var_value->value_double > op_value->value_double)
                        var_value->value_double = op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_MAX:
            // max(var, operand) - floors value at operand (lower bound)
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer < op_value->value_integer)
                        var_value->value_integer = op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (var_value->value_double < op_value->value_double)
                        var_value->value_double = op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_PUSH:
            dc_app_lookup_var_push(renderer->lookup, var_index);
            return false; // don't refresh - we're just saving state

        case DC_APP_SET_TYPE_POP:
            dc_app_lookup_var_pop(renderer->lookup, var_index);
            break;

        case DC_APP_SET_TYPE_NEGATE:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = -var_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = -var_value->value_double;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_RECIPROCAL:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer == 0) {
                        DC_LOG_ERROR("Set", "Reciprocal of zero (int)");
                        break;
                    }
                    var_value->value_integer = 1 / var_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (var_value->value_double == 0.0) {
                        DC_LOG_ERROR("Set", "Reciprocal of zero (double)");
                        break;
                    }
                    var_value->value_double = 1.0 / var_value->value_double;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_ABSOLUTE:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer < 0)
                        var_value->value_integer = -var_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = fabs(var_value->value_double);
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_SQUARE:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = var_value->value_integer * var_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = var_value->value_double * var_value->value_double;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_SQRT:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer < 0) {
                        DC_LOG_ERROR("Set", "Sqrt of negative (int)");
                        break;
                    }
                    var_value->value_integer = (int)floor(sqrt((double)var_value->value_integer));
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (var_value->value_double < 0.0) {
                        DC_LOG_ERROR("Set", "Sqrt of negative (double)");
                        break;
                    }
                    var_value->value_double = sqrt(var_value->value_double);
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_MODULO:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    if (op_value->value_integer == 0) {
                        DC_LOG_ERROR("Set", "Modulo by zero (int)");
                        break;
                    }
                    var_value->value_integer %= op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (op_value->value_double == 0.0) {
                        DC_LOG_ERROR("Set", "Modulo by zero (double)");
                        break;
                    }
                    var_value->value_double = fmod(var_value->value_double, op_value->value_double);
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    if (op_value->value_boolean == 0) {
                        DC_LOG_ERROR("Set", "Modulo by zero (bool)");
                        break;
                    }
                    var_value->value_boolean %= op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_POWER:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = (int)pow((double)var_value->value_integer, (double)op_value->value_integer);
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = pow(var_value->value_double, op_value->value_double);
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean = (int)pow((double)var_value->value_boolean, (double)op_value->value_boolean);
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_LOG:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer <= 0) {
                        DC_LOG_ERROR("Set", "Log domain error (int)");
                        break;
                    }
                    var_value->value_integer = (int)log((double)var_value->value_integer); // natural log
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (var_value->value_double <= 0.0) {
                        DC_LOG_ERROR("Set", "Log domain error (double)");
                        break;
                    }
                    var_value->value_double = log(var_value->value_double); // natural log
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_EXP:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = (int)exp((double)var_value->value_integer);
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = exp(var_value->value_double);
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_ROUND:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    // already an integer; no-op
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = round(var_value->value_double);
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_SIGN:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = (var_value->value_integer > 0) - (var_value->value_integer < 0);
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = (var_value->value_double > 0.0) - (var_value->value_double < 0.0);
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean = var_value->value_boolean ? 1 : 0;
                    break;
                default:
                    break;
            }
            break;

        default:
            DC_LOG_ERROR("Set", "Invalid operator: %d", operation);
            return false;
    }

    return true; // refresh needed
}

static void _execute_set(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {

    // skip if variable or operand is undefined
    if (node->set.var_index == DC_APP_VAR_INDEX_UNDEFINED || node->set.operand == DC_APP_VAL_INDEX_UNDEFINED) {
        return;
    }

    DcValue *op_value = dc_app_lookup_get_value(renderer->lookup, node->set.operand);

    // get operation
    DcAppSetType operation = node->set.operation == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_SET_TYPE_UNDEFINED : (DcAppSetType)(dc_app_lookup_get_value(renderer->lookup, node->set.operation)->value_integer);

    // if deferred, snapshot the value and defer execution
    if (node->set.deferred != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->set.deferred)->value_boolean) {
        _DcAppDeferredSetOp qop;
        qop.var_index = node->set.var_index;
        qop.operation = operation;
        qop.value     = *op_value; // struct copy snapshot
        sbpush(renderer->sb_deferred_sets, qop);
        return;
    }

    // immediate execution
    DcValue *var_value = dc_app_lookup_get_value(renderer->lookup, dc_app_lookup_get_var_value_index(renderer->lookup, node->set.var_index));
    if (_apply_set_operation(renderer, node->set.var_index, var_value, op_value, operation)) {
        // re-fetch var_value after POP (value_index may have changed)
        if (operation == DC_APP_SET_TYPE_POP) {
            var_value = dc_app_lookup_get_value(renderer->lookup, dc_app_lookup_get_var_value_index(renderer->lookup, node->set.var_index));
        }
        dc_value_refresh(var_value);
    }
}

void dc_app_renderer_flush_deferred_sets(DcAppRenderer *renderer) {
    int count = sbcount(renderer->sb_deferred_sets);
    for (int i = 0; i < count; i++) {
        _DcAppDeferredSetOp *qop = &renderer->sb_deferred_sets[i];
        DcValue        *var_value = dc_app_lookup_get_value(
            renderer->lookup,
            dc_app_lookup_get_var_value_index(renderer->lookup, qop->var_index));
        if (_apply_set_operation(renderer, qop->var_index, var_value, &qop->value, qop->operation)) {
            // re-fetch var_value after POP (value_index may have changed)
            if (qop->operation == DC_APP_SET_TYPE_POP) {
                var_value = dc_app_lookup_get_value(
                    renderer->lookup,
                    dc_app_lookup_get_var_value_index(renderer->lookup, qop->var_index));
            }
            dc_value_refresh(var_value);
        }
    }
    sbclear(renderer->sb_deferred_sets);
}

static void _render_sphere(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_position = &parent_frame.position;
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // boolean checks
    bool use_radius             = node->sphere.radius != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_rotation           = node->sphere.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->sphere.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->sphere.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->sphere.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->sphere.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get radius
    float radius, diameter;
    if (use_radius) {
        radius   = (float)dc_app_lookup_get_value(renderer->lookup, node->sphere.radius)->value_double;
        diameter = 2 * radius;
    } else {
        diameter = fminf(parent_dimensions->x, parent_dimensions->y);
        radius   = diameter / 2;
    }

    // 2D transform (for positioning in orthographic view)
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(renderer->lookup, node->sphere.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(renderer->lookup, node->sphere.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->sphere.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->sphere.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->sphere.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->sphere.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->sphere.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2] = {0, 0};
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("Sphere", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                    break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("Sphere", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->sphere.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    // note: sphere is built centered at origin, so alignment works differently than rect-based elements
    {
        DcAppAlignType local_aligns[2] = {
            node->sphere.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->sphere.local_align.x)->value_integer,
            node->sphere.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->sphere.local_align.y)->value_integer};

        float trans_align_offsets[2] = {0, 0};
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = radius; // move right so left edge aligns
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = 0; // sphere is already centered
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -radius; // move left so right edge aligns
                break;
            default:
                DC_LOG_WARN("Sphere", "Unknown X alignment: %d", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = radius; // move down so bottom edge aligns
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = 0; // sphere is already centered
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -radius; // move up so top edge aligns
                break;
            default:
                DC_LOG_WARN("Sphere", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);
        transform                      = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->sphere.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->sphere.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float          anchor[2]      = {0, 0};
        DcAppAlignType parent_align_x = node->sphere.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->sphere.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Sphere", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->sphere.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->sphere.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Sphere", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->sphere.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->sphere.position.y)->value_double : 0};

        // apply negate
        if (node->sphere.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->sphere.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->sphere.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->sphere.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform                   = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation (around local pivot)
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {
            DcAppAlignType local_pivot_aligns[2] = {
                node->sphere.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->sphere.pivot_local_align.x)->value_integer,
                node->sphere.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->sphere.pivot_local_align.y)->value_integer};

            float pivot_position[2] = {0, 0};
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = diameter / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = diameter;
                    break;
                default:
                    DC_LOG_WARN("Sphere", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = diameter / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = diameter;
                    break;
                default:
                    DC_LOG_WARN("Sphere", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->sphere.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // build sphere transform: first internal rotation, then 2D positioning/scaling
    // start with internal rotation (roll, pitch, yaw) - applied first (rightmost in multiplication)
    plMat4 sphere_transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    {
        float roll  = node->sphere.rpy.roll == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->sphere.rpy.roll)->value_double);
        float pitch = node->sphere.rpy.pitch == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->sphere.rpy.pitch)->value_double);
        float yaw   = node->sphere.rpy.yaw == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->sphere.rpy.yaw)->value_double);

        // offset yaw by -90 degrees to match legacy ADI ball orientation
        yaw -= (float)M_PI_2;

        // apply rotations in order: yaw (Y), pitch (X), roll (Z) for globe-like rotation
        if (yaw != 0.0f) {
            plMat4 yaw_xform = pl_mat4_rotate_vec3(yaw, (plVec3){0.0f, 1.0f, 0.0f});
            sphere_transform = pl_mul_mat4t(&sphere_transform, &yaw_xform);
        }
        if (pitch != 0.0f) {
            plMat4 pitch_xform = pl_mat4_rotate_vec3(pitch, (plVec3){1.0f, 0.0f, 0.0f});
            sphere_transform   = pl_mul_mat4t(&sphere_transform, &pitch_xform);
        }
        if (roll != 0.0f) {
            plMat4 roll_xform = pl_mat4_rotate_vec3(roll, (plVec3){0.0f, 0.0f, 1.0f});
            sphere_transform  = pl_mul_mat4t(&sphere_transform, &roll_xform);
        }
    }

    // then apply the 2D transform (scale + position) - applied after rotation
    sphere_transform = pl_mul_mat4t(&transform, &sphere_transform);

    // get fill color
    float fill_color[4] = {
        node->sphere.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->sphere.fill_color.r)->value_double,
        node->sphere.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->sphere.fill_color.g)->value_double,
        node->sphere.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->sphere.fill_color.b)->value_double,
        node->sphere.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->sphere.fill_color.a)->value_double,
    };
    uint32_t pl_fill_color = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);

    // create sphere geometry definition - sphere is built at origin, transform handles positioning
    plSphere sphere_def = {
        .tCenter = (plVec3){0.0f, 0.0f, 0.0f},
        .fRadius = radius};

    // draw textured or solid sphere
    if (node->sphere.texture_index != TEXTURE_INDEX_UNDEFINED) {
        // textured sphere
        uint32_t texture_id = 0;
        if (!dc_app_texture_get_bind_group(renderer->textures, (DcAppTextureId)node->sphere.texture_index, &texture_id)) return;
        dc_app_draw_3d_sphere_textured(ctx, texture_id, sphere_def, &sphere_transform, pl_fill_color);
    } else {
        // solid sphere
        dc_app_draw_3d_sphere_filled(ctx, sphere_def, pl_fill_color);
    }
}

static void _render_stencil(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    int num_children = sbcount(node->stencil.sb_children);

    if (!dc_app_draw_stencil_begin(ctx)) return;

    for (int i = 0; i < num_children; i++) {
        DcAppStencilChild *stencil_child = &node->stencil.sb_children[i];

        switch (stencil_child->type) {
            case STENCIL_CHILD_TYPE_ADD:
                dc_app_draw_stencil_add(ctx);
                break;
            case STENCIL_CHILD_TYPE_REMOVE:
                dc_app_draw_stencil_remove(ctx);
                break;
            case STENCIL_CHILD_TYPE_DRAW:
                dc_app_draw_stencil_draw(ctx);
                break;
            default:
                continue;
        }

        _render_node_list(ctx, renderer, stencil_child->child);
    }

    dc_app_draw_stencil_end(ctx);
}

static void _render_planet_breadcrumbs(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view) {
    DcAppNodePlanetBreadcrumbs *breadcrumbs = &node->planet_breadcrumbs;
    DcAppPlanetDefinition            *def         = dc_app_scene_get_planet_definition(renderer->scene, breadcrumbs->planet_def_index);

    if (breadcrumbs->clear != DC_APP_VAL_INDEX_UNDEFINED) {
        DcValue *clear_value = dc_app_lookup_get_value(renderer->lookup, breadcrumbs->clear);
        if (breadcrumbs->clear_value_initialized && !dc_value_is_equal(clear_value, &breadcrumbs->last_clear_value)) {
            sbclear(breadcrumbs->sb_points);
        }
        breadcrumbs->last_clear_value        = *clear_value;
        breadcrumbs->clear_value_initialized = true;
    }

    bool enabled = true;
    if (breadcrumbs->enabled != DC_APP_VAL_INDEX_UNDEFINED) {
        enabled = dc_app_lookup_get_value(renderer->lookup, breadcrumbs->enabled)->value_boolean;
    }
    if (!enabled) return;

    plVec3d point = {0};
    bool   have_point = false;
    if (breadcrumbs->crs == DC_APP_PLANET_CRS_CARTESIAN) {
        if (breadcrumbs->xyz.x != DC_APP_VAL_INDEX_UNDEFINED &&
            breadcrumbs->xyz.y != DC_APP_VAL_INDEX_UNDEFINED &&
            breadcrumbs->xyz.z != DC_APP_VAL_INDEX_UNDEFINED) {
            point = (plVec3d){
                dc_app_lookup_get_value(renderer->lookup, breadcrumbs->xyz.x)->value_double,
                dc_app_lookup_get_value(renderer->lookup, breadcrumbs->xyz.y)->value_double,
                dc_app_lookup_get_value(renderer->lookup, breadcrumbs->xyz.z)->value_double
            };
            have_point = true;
        }
    } else {
        if (breadcrumbs->lat != DC_APP_VAL_INDEX_UNDEFINED &&
            breadcrumbs->lon != DC_APP_VAL_INDEX_UNDEFINED) {
            double alt = 0.0;
            if (breadcrumbs->alt != DC_APP_VAL_INDEX_UNDEFINED) {
                alt = dc_app_lookup_get_value(renderer->lookup, breadcrumbs->alt)->value_double;
            } else if (breadcrumbs->height_above_terrain != DC_APP_VAL_INDEX_UNDEFINED) {
                alt = dc_app_lookup_get_value(renderer->lookup, breadcrumbs->height_above_terrain)->value_double;
            }
            plVec3d geodetic = {
                dc_app_lookup_get_value(renderer->lookup, breadcrumbs->lat)->value_double,
                dc_app_lookup_get_value(renderer->lookup, breadcrumbs->lon)->value_double,
                alt
            };
            dc_geo_geodetic_to_cartesian_d(&def->geodetic_crs, &def->cartesian_crs, &geodetic, &point, 1);
            have_point = true;
        }
    }
    if (!have_point || !isfinite(point.x) || !isfinite(point.y) || !isfinite(point.z)) return;

    float point_spacing = breadcrumbs->point_spacing != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, breadcrumbs->point_spacing)->value_double : 1.0f;
    if (point_spacing < 0.0f) point_spacing = 0.0f;

    int point_count = sbcount(breadcrumbs->sb_points);
    if (point_count == 0) {
        sbpush(breadcrumbs->sb_points, point);
    } else {
        plVec3d last = breadcrumbs->sb_points[point_count - 1];
        double  dx   = point.x - last.x;
        double  dy   = point.y - last.y;
        double  dz   = point.z - last.z;
        double  dist = sqrt(dx * dx + dy * dy + dz * dz);
        if ((point_spacing == 0.0f && dist > 0.0) || dist >= (double)point_spacing) {
            sbpush(breadcrumbs->sb_points, point);
        }
    }

    int max_points = breadcrumbs->max_points != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, breadcrumbs->max_points)->value_integer : 4096;
    if (max_points < 2) max_points = 2;
    point_count = sbcount(breadcrumbs->sb_points);
    if (point_count > max_points) {
        sbshiftn(breadcrumbs->sb_points, point_count - max_points);
    }

    point_count = sbcount(breadcrumbs->sb_points);
    if (point_count < 2) return;

    float line_width = breadcrumbs->line_width != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, breadcrumbs->line_width)->value_double : 1.0f;

    float lc[4] = {1.0f, 0.0f, 0.0f, 0.5f};
    if (breadcrumbs->config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) {
        lc[0] = breadcrumbs->line_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, breadcrumbs->line_color.r)->value_double : 1.0f;
        lc[1] = breadcrumbs->line_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, breadcrumbs->line_color.g)->value_double : 1.0f;
        lc[2] = breadcrumbs->line_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, breadcrumbs->line_color.b)->value_double : 1.0f;
        lc[3] = breadcrumbs->line_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, breadcrumbs->line_color.a)->value_double : 1.0f;
    }
    sbclear(renderer->sb_planet_points);
    for (int ii = 0; ii < point_count; ii++) {
        plVec3d point = breadcrumbs->sb_points[ii];
        sbpush(renderer->sb_planet_points, ((DcAppVec3d){point.x, point.y, point.z}));
    }
    dc_app_draw_planet_line_cartesian(
        ctx,
        draw_view,
        renderer->sb_planet_points,
        (uint32_t)point_count,
        (DcAppStroke){
            .color = {lc[0], lc[1], lc[2], lc[3]},
            .width = line_width,
            .pattern = breadcrumbs->line_pattern == DC_APP_VAL_INDEX_UNDEFINED
                ? 0 : (uint8_t)dc_app_lookup_get_value(renderer->lookup, breadcrumbs->line_pattern)->value_integer,
        });
}

static void _render_planet_container(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view) {
    DcAppNodePlanetContainer *container = &node->planet_container;

    double lat = container->lat != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, container->lat)->value_double : 0.0;
    double lon = container->lon != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, container->lon)->value_double : 0.0;
    double height = container->height_above_terrain != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, container->height_above_terrain)->value_double : 0.0;

    DcAppPlanetLocalTransform transform = {0};
    transform.scale = container->scale != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, container->scale)->value_double : 1.0f;
    transform.rotation_degrees = container->rotation != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, container->rotation)->value_double : 0.0f;

    if (!dc_app_draw_planet_container_push_geodetic(ctx, draw_view, lat, lon, height, transform)) return;

    DcAppNodeIndex child_index = container->child;
    while (child_index != NODE_INDEX_UNDEFINED) {
        DcAppNode *child = dc_app_scene_get_node(renderer->scene, child_index);
        if (child->type == NODE_TYPE_PLANET_LINE)
            _render_planet_line_local(ctx, renderer, child);
        else if (child->type == NODE_TYPE_PLANET_POLYGON)
            _render_planet_polygon_local(ctx, renderer, child);
        child_index = child->next;
    }

    dc_app_draw_planet_container_pop(ctx);
}

static void _render_planet_ellipse(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view) {
    (void)ctx;
    DcAppPlanetDefinition *def = dc_app_scene_get_planet_definition(renderer->scene, node->planet_ellipse.planet_def_index);

    // resolve values
    double lat = node->planet_ellipse.lat != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.lat)->value_double : 0.0;
    double lon = node->planet_ellipse.lon != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.lon)->value_double : 0.0;
    float radius_x = node->planet_ellipse.radius_x != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.radius_x)->value_double : 0.0f;
    float radius_y = node->planet_ellipse.radius_y != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.radius_y)->value_double : 0.0f;
    float rotation = node->planet_ellipse.rotation != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.rotation)->value_double : 0.0f;
    double height = node->planet_ellipse.height_above_terrain != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.height_above_terrain)->value_double : 0.0;
    int segments = node->planet_ellipse.segments != DC_APP_VAL_INDEX_UNDEFINED
        ? (int)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.segments)->value_double : 64;
    float line_width = node->planet_ellipse.line_width != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.line_width)->value_double : 1.0f;

    if (radius_x <= 0.0f || radius_y <= 0.0f) return;
    if (segments < 3) segments = 3;
    if (segments > DC_APP_NODE_ELLIPSE_MAX_SEGMENTS) segments = DC_APP_NODE_ELLIPSE_MAX_SEGMENTS;

    plVec3d center;
    if (node->planet_ellipse.crs == DC_APP_PLANET_CRS_CARTESIAN) {
        // cartesian centers are already in renderer-native planet space
        center = (plVec3d){
            node->planet_ellipse.xyz.x != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.xyz.x)->value_double : 0.0,
            node->planet_ellipse.xyz.y != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.xyz.y)->value_double : 0.0,
            node->planet_ellipse.xyz.z != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.xyz.z)->value_double : 0.0
        };
    } else {
        plVec3d geodetic = {lat, lon, height};
        dc_geo_geodetic_to_cartesian_d(&def->geodetic_crs, &def->cartesian_crs, &geodetic, &center, 1);
    }

    bool fill_enabled = (node->planet_ellipse.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) != 0;
    DcAppVec4 fill_color = {0};
    if (fill_enabled) {
        float fc[4] = {
            node->planet_ellipse.fill_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.fill_color.r)->value_double : 1.0f,
            node->planet_ellipse.fill_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.fill_color.g)->value_double : 1.0f,
            node->planet_ellipse.fill_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.fill_color.b)->value_double : 1.0f,
            node->planet_ellipse.fill_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.fill_color.a)->value_double : 1.0f
        };
        fill_color = (DcAppVec4){fc[0], fc[1], fc[2], fc[3]};
    }

    bool line_enabled = (node->planet_ellipse.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) != 0;
    DcAppVec4 line_color = {0};
    if (line_enabled) {
        float lc[4] = {
            node->planet_ellipse.line_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.line_color.r)->value_double : 1.0f,
            node->planet_ellipse.line_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.line_color.g)->value_double : 1.0f,
            node->planet_ellipse.line_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.line_color.b)->value_double : 1.0f,
            node->planet_ellipse.line_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_ellipse.line_color.a)->value_double : 1.0f
        };
        line_color = (DcAppVec4){lc[0], lc[1], lc[2], lc[3]};
    }

    DcAppVec3d draw_center = {center.x, center.y, center.z};
    DcAppVec2 draw_radius = {radius_x, radius_y};
    dc_app_draw_planet_ellipse_cartesian_enabled(
        draw_view,
        &draw_center,
        &draw_radius,
        rotation,
        (uint32_t)segments,
        line_width,
        PL_COLOR_32_RGBA(line_color.r, line_color.g, line_color.b, line_color.a),
        line_enabled,
        PL_COLOR_32_RGBA(fill_color.r, fill_color.g, fill_color.b, fill_color.a),
        fill_enabled);
}

static void _render_planet_line(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view) {
    DcAppPlanetDefinition *def = dc_app_scene_get_planet_definition(renderer->scene, node->planet_line.planet_def_index);

    // determine point count
    uint32_t count = node->planet_line.is_dynamic
        ? (uint32_t)sbcount(node->planet_line.sb_points_dynamic)
        : (uint32_t)sbcount(node->planet_line.sb_points_static);
    if (count < 2) return;

    double height = node->planet_line.height_above_terrain != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_line.height_above_terrain)->value_double : 0.0;
    float line_width = node->planet_line.line_width != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_width)->value_double : 1.0f;

    // resolve line color
    float lc[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    if (node->planet_line.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) {
        lc[0] = node->planet_line.line_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_color.r)->value_double : 1.0f;
        lc[1] = node->planet_line.line_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_color.g)->value_double : 1.0f;
        lc[2] = node->planet_line.line_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_color.b)->value_double : 1.0f;
        lc[3] = node->planet_line.line_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_color.a)->value_double : 1.0f;
    }
    // convert to 3D
    DcAppVec3d *pts3d = (DcAppVec3d *)malloc(sizeof(DcAppVec3d) * count);
    if (!pts3d) return;

    if (node->planet_line.is_dynamic) {
        for (uint32_t p = 0; p < count; p++) {
            DcAppPlanetVertexDynamic *v = &node->planet_line.sb_points_dynamic[p];
            double lat = v->lat != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->lat)->value_double : 0.0;
            double lon = v->lon != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->lon)->value_double : 0.0;
            double alt = v->alt != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->alt)->value_double : height;
            if (node->planet_line.crs == DC_APP_PLANET_CRS_CARTESIAN) {
                // cartesian vertices are already in renderer-native planet space
                pts3d[p] = (DcAppVec3d){
                    v->xyz.x != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->xyz.x)->value_double : 0.0,
                    v->xyz.y != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->xyz.y)->value_double : 0.0,
                    v->xyz.z != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->xyz.z)->value_double : 0.0
                };
            } else {
                plVec3d in = {lat, lon, alt};
                plVec3d out;
                dc_geo_geodetic_to_cartesian_d(&def->geodetic_crs, &def->cartesian_crs, &in, &out, 1);
                pts3d[p] = (DcAppVec3d){out.x, out.y, out.z};
            }
        }
    } else {
        for (uint32_t p = 0; p < count; p++) {
            DcAppPlanetVertexStatic *pt = &node->planet_line.sb_points_static[p];
            double pt_height = pt->has_alt ? pt->alt : height;
            plVec3d in = {pt->lat, pt->lon, pt_height};
            plVec3d out;
            dc_geo_geodetic_to_cartesian_d(&def->geodetic_crs, &def->cartesian_crs, &in, &out, 1);
            pts3d[p] = (DcAppVec3d){out.x, out.y, out.z};
        }
    }

    dc_app_draw_planet_line_cartesian(
        ctx,
        draw_view,
        pts3d,
        count,
        (DcAppStroke){
            .color = {lc[0], lc[1], lc[2], lc[3]},
            .width = line_width,
            .pattern = node->planet_line.line_pattern == DC_APP_VAL_INDEX_UNDEFINED ? 0 : (uint8_t)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_pattern)->value_integer,
        });
    free(pts3d);
}

static void _render_planet_line_local(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node) {
    uint32_t count = (uint32_t)sbcount(node->planet_line.sb_points_dynamic);
    if (count < 2) return;

    DcAppVec2 *points = (DcAppVec2 *)malloc(sizeof(*points) * count);
    if (!points) return;

    for (uint32_t i = 0; i < count; i++) {
        DcAppPlanetVertexDynamic *vertex = &node->planet_line.sb_points_dynamic[i];
        points[i].x = vertex->xyz.x != DC_APP_VAL_INDEX_UNDEFINED
            ? (float)dc_app_lookup_get_value(renderer->lookup, vertex->xyz.x)->value_double : 0.0f;
        points[i].y = vertex->xyz.y != DC_APP_VAL_INDEX_UNDEFINED
            ? (float)dc_app_lookup_get_value(renderer->lookup, vertex->xyz.y)->value_double : 0.0f;
    }

    float line_width = node->planet_line.line_width != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_width)->value_double : 1.0f;
    DcAppVec4 line_color = {1.0f, 1.0f, 1.0f, 1.0f};
    if (node->planet_line.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) {
        line_color.r = node->planet_line.line_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_color.r)->value_double : 1.0f;
        line_color.g = node->planet_line.line_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_color.g)->value_double : 1.0f;
        line_color.b = node->planet_line.line_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_color.b)->value_double : 1.0f;
        line_color.a = node->planet_line.line_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_color.a)->value_double : 1.0f;
    }

    dc_app_draw_planet_line_local(
        ctx,
        points,
        count,
        (DcAppStroke){
            .color = line_color,
            .width = line_width,
            .pattern = node->planet_line.line_pattern == DC_APP_VAL_INDEX_UNDEFINED ? 0 : (uint8_t)dc_app_lookup_get_value(renderer->lookup, node->planet_line.line_pattern)->value_integer,
        });
    free(points);
}

static void _render_planet_polygon(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view) {
    (void)ctx;
    DcAppPlanetDefinition *def = dc_app_scene_get_planet_definition(renderer->scene, node->planet_polygon.planet_def_index);

    // determine point count
    uint32_t count = node->planet_polygon.is_dynamic
        ? (uint32_t)sbcount(node->planet_polygon.sb_points_dynamic)
        : (uint32_t)sbcount(node->planet_polygon.sb_points_static);
    if (count < 3) return;

    double height = node->planet_polygon.height_above_terrain != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.height_above_terrain)->value_double : 0.0;
    float line_width = node->planet_polygon.line_width != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_width)->value_double : 1.0f;

    // convert to 3D
    DcAppVec3d *pts3d = (DcAppVec3d *)malloc(sizeof(DcAppVec3d) * count);
    if (!pts3d) return;

    if (node->planet_polygon.is_dynamic) {
        for (uint32_t p = 0; p < count; p++) {
            DcAppPlanetVertexDynamic *v = &node->planet_polygon.sb_points_dynamic[p];
            double lat = v->lat != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->lat)->value_double : 0.0;
            double lon = v->lon != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->lon)->value_double : 0.0;
            double alt = v->alt != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->alt)->value_double : height;
            if (node->planet_polygon.crs == DC_APP_PLANET_CRS_CARTESIAN) {
                // cartesian vertices are already in renderer-native planet space
                pts3d[p] = (DcAppVec3d){
                    v->xyz.x != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->xyz.x)->value_double : 0.0,
                    v->xyz.y != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->xyz.y)->value_double : 0.0,
                    v->xyz.z != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, v->xyz.z)->value_double : 0.0
                };
            } else {
                plVec3d in = {lat, lon, alt};
                plVec3d out;
                dc_geo_geodetic_to_cartesian_d(&def->geodetic_crs, &def->cartesian_crs, &in, &out, 1);
                pts3d[p] = (DcAppVec3d){out.x, out.y, out.z};
            }
        }
    } else {
        for (uint32_t p = 0; p < count; p++) {
            DcAppPlanetVertexStatic *pt = &node->planet_polygon.sb_points_static[p];
            double pt_height = pt->has_alt ? pt->alt : height;
            plVec3d in = {pt->lat, pt->lon, pt_height};
            plVec3d out;
            dc_geo_geodetic_to_cartesian_d(&def->geodetic_crs, &def->cartesian_crs, &in, &out, 1);
            pts3d[p] = (DcAppVec3d){out.x, out.y, out.z};
        }
    }

    bool fill_enabled = (node->planet_polygon.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) != 0;
    DcAppVec4 fill_color = {0};
    if (fill_enabled) {
        float fc[4] = {
            node->planet_polygon.fill_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.fill_color.r)->value_double : 1.0f,
            node->planet_polygon.fill_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.fill_color.g)->value_double : 1.0f,
            node->planet_polygon.fill_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.fill_color.b)->value_double : 1.0f,
            node->planet_polygon.fill_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.fill_color.a)->value_double : 1.0f};
        fill_color = (DcAppVec4){fc[0], fc[1], fc[2], fc[3]};
    }

    bool line_enabled = (node->planet_polygon.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) != 0;
    DcAppVec4 line_color = {0};
    if (line_enabled) {
        float lc[4] = {
            node->planet_polygon.line_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_color.r)->value_double : 1.0f,
            node->planet_polygon.line_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_color.g)->value_double : 1.0f,
            node->planet_polygon.line_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_color.b)->value_double : 1.0f,
            node->planet_polygon.line_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_color.a)->value_double : 1.0f};
        line_color = (DcAppVec4){lc[0], lc[1], lc[2], lc[3]};
    }

    dc_app_draw_planet_polygon_cartesian_enabled(
        draw_view,
        pts3d,
        count,
        line_width,
        PL_COLOR_32_RGBA(line_color.r, line_color.g, line_color.b, line_color.a),
        node->planet_polygon.line_pattern == DC_APP_VAL_INDEX_UNDEFINED ? 0 : (uint8_t)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_pattern)->value_integer,
        line_enabled,
        PL_COLOR_32_RGBA(fill_color.r, fill_color.g, fill_color.b, fill_color.a),
        fill_enabled);
    free(pts3d);
}

static void _render_planet_polygon_local(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node) {
    uint32_t count = (uint32_t)sbcount(node->planet_polygon.sb_points_dynamic);
    if (count < 3) return;

    DcAppVec2 *points = (DcAppVec2 *)malloc(sizeof(*points) * count);
    if (!points) return;

    for (uint32_t i = 0; i < count; i++) {
        DcAppPlanetVertexDynamic *vertex = &node->planet_polygon.sb_points_dynamic[i];
        points[i].x = vertex->xyz.x != DC_APP_VAL_INDEX_UNDEFINED
            ? (float)dc_app_lookup_get_value(renderer->lookup, vertex->xyz.x)->value_double : 0.0f;
        points[i].y = vertex->xyz.y != DC_APP_VAL_INDEX_UNDEFINED
            ? (float)dc_app_lookup_get_value(renderer->lookup, vertex->xyz.y)->value_double : 0.0f;
    }

    float line_width = node->planet_polygon.line_width != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_width)->value_double : 1.0f;
    DcAppVec4 line_color = {0};
    DcAppVec4 fill_color = {0};
    bool line_enabled = (node->planet_polygon.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) != 0;
    if (line_enabled) {
        line_color.r = node->planet_polygon.line_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_color.r)->value_double : 1.0f;
        line_color.g = node->planet_polygon.line_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_color.g)->value_double : 1.0f;
        line_color.b = node->planet_polygon.line_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_color.b)->value_double : 1.0f;
        line_color.a = node->planet_polygon.line_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_color.a)->value_double : 1.0f;
    }
    bool fill_enabled = (node->planet_polygon.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) != 0;
    if (fill_enabled) {
        fill_color.r = node->planet_polygon.fill_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.fill_color.r)->value_double : 1.0f;
        fill_color.g = node->planet_polygon.fill_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.fill_color.g)->value_double : 1.0f;
        fill_color.b = node->planet_polygon.fill_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.fill_color.b)->value_double : 1.0f;
        fill_color.a = node->planet_polygon.fill_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.fill_color.a)->value_double : 1.0f;
    }

    dc_app_draw_planet_polygon_local_enabled(
        ctx,
        points,
        count,
        line_width,
        PL_COLOR_32_RGBA(line_color.r, line_color.g, line_color.b, line_color.a),
        node->planet_polygon.line_pattern == DC_APP_VAL_INDEX_UNDEFINED ? 0 : (uint8_t)dc_app_lookup_get_value(renderer->lookup, node->planet_polygon.line_pattern)->value_integer,
        line_enabled,
        PL_COLOR_32_RGBA(fill_color.r, fill_color.g, fill_color.b, fill_color.a),
        fill_enabled);
    free(points);
}

static void _render_planet_sphere(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view) {
    double lat = node->planet_sphere.lat != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_sphere.lat)->value_double : 0.0;
    double lon = node->planet_sphere.lon != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_sphere.lon)->value_double : 0.0;
    double height = node->planet_sphere.height_above_terrain != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_sphere.height_above_terrain)->value_double : 0.0;
    double radius = node->planet_sphere.radius != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_sphere.radius)->value_double : 1000.0;

    if (!(node->planet_sphere.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED)) return;

    float fc[4] = {
        node->planet_sphere.fill_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_sphere.fill_color.r)->value_double : 1.0f,
        node->planet_sphere.fill_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_sphere.fill_color.g)->value_double : 1.0f,
        node->planet_sphere.fill_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_sphere.fill_color.b)->value_double : 1.0f,
        node->planet_sphere.fill_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_sphere.fill_color.a)->value_double : 1.0f
    };
    if (node->planet_sphere.crs == DC_APP_PLANET_CRS_CARTESIAN) {
        DcAppVec3d pos = {
            node->planet_sphere.xyz.x != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_sphere.xyz.x)->value_double : 0.0,
            node->planet_sphere.xyz.y != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_sphere.xyz.y)->value_double : 0.0,
            node->planet_sphere.xyz.z != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_sphere.xyz.z)->value_double : 0.0
        };
        dc_app_draw_planet_sphere_cartesian(ctx, draw_view, pos, (float)radius, (DcAppVec4){fc[0], fc[1], fc[2], fc[3]});
    } else {
        dc_app_draw_planet_sphere_geodetic(ctx, draw_view, lat, lon, height, radius, (DcAppVec4){fc[0], fc[1], fc[2], fc[3]});
    }
}

static void _render_planet_image(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view) {
    if (!renderer || !node || !ctx || node->planet_image.texture_index == TEXTURE_INDEX_UNDEFINED) return;

    double lat = node->planet_image.lat != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_image.lat)->value_double : 0.0;
    double lon = node->planet_image.lon != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_image.lon)->value_double : 0.0;
    double height = node->planet_image.height_above_terrain != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_image.height_above_terrain)->value_double : 0.0;
    DcAppVec2 size = {
        node->planet_image.dimension.x != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_image.dimension.x)->value_double : 0.0f,
        node->planet_image.dimension.y != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_image.dimension.y)->value_double : 0.0f
    };
    DcAppVec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};
    if (node->planet_image.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) {
        tint.r = node->planet_image.tint_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_image.tint_color.r)->value_double : 1.0f;
        tint.g = node->planet_image.tint_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_image.tint_color.g)->value_double : 1.0f;
        tint.b = node->planet_image.tint_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_image.tint_color.b)->value_double : 1.0f;
        tint.a = node->planet_image.tint_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_image.tint_color.a)->value_double : 1.0f;
    }

    if (node->planet_image.crs == DC_APP_PLANET_CRS_CARTESIAN) {
        DcAppVec3d position = {
            node->planet_image.xyz.x != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_image.xyz.x)->value_double : 0.0,
            node->planet_image.xyz.y != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_image.xyz.y)->value_double : 0.0,
            node->planet_image.xyz.z != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_image.xyz.z)->value_double : 0.0
        };
        dc_app_draw_planet_image_cartesian(ctx, draw_view, position, (DcAppTextureId)node->planet_image.texture_index, size, tint);
    } else {
        dc_app_draw_planet_image_geodetic(ctx, draw_view, lat, lon, height, (DcAppTextureId)node->planet_image.texture_index, size, tint);
    }
}

static void _render_planet_text(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNode *node, DcAppDrawPlanetViewHandle draw_view) {
    // resolve position
    double lat = node->planet_text.lat != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_text.lat)->value_double : 0.0;
    double lon = node->planet_text.lon != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_text.lon)->value_double : 0.0;
    double height = node->planet_text.height_above_terrain != DC_APP_VAL_INDEX_UNDEFINED
        ? dc_app_lookup_get_value(renderer->lookup, node->planet_text.height_above_terrain)->value_double : 0.0;
    float size = node->planet_text.size != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_text.size)->value_double : 14.0f;

    // expand text (same pattern as _render_text)
    char **sb_text = &renderer->sb_planet_text;
    sbclear(*sb_text);
    for (int ii = 0; ii < sbcount(node->planet_text.sb_vals); ii++) {

        // filler
        char *filler = &(node->planet_text.sb_fillers[node->planet_text.sb_filler_indices[ii]]);
        sbpushn(*sb_text, filler, (int)strlen(filler));

        // value
        DcValueType format_type = node->planet_text.sb_format_types[ii];
        char       *format      = &(node->planet_text.sb_formats[node->planet_text.sb_format_indices[ii]]);
        char val_str[256] = {0};
        if (node->planet_text.sb_vals[ii] == DC_APP_VAL_INDEX_UNDEFINED) {
            val_str[0] = '\0';
        } else {
            DcValue *val = dc_app_lookup_get_value(renderer->lookup, node->planet_text.sb_vals[ii]);
            switch (format_type) {
                case DC_VALUE_TYPE_STRING:
                    snprintf(val_str, sizeof(val_str), format, val->value_string);
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    snprintf(val_str, sizeof(val_str), format, val->value_integer);
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    snprintf(val_str, sizeof(val_str), format, val->value_double);
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    snprintf(val_str, sizeof(val_str), format, val->value_boolean);
                    break;
                default:
                    DC_LOG_WARN("PlanetText", "Unknown value type: %d", format_type);
            }
        }
        sbpushn(*sb_text, val_str, (int)strlen(val_str));
    }

    // ending filler
    char *filler = &(node->planet_text.sb_fillers[node->planet_text.sb_filler_indices[sbcount(node->planet_text.sb_vals)]]);
    sbpushn(*sb_text, filler, (int)strlen(filler));
    sbpush(*sb_text, '\0');

    DcAppVec3d pos = {0};
    if (node->planet_text.crs == DC_APP_PLANET_CRS_CARTESIAN) {
        pos = (DcAppVec3d){
            node->planet_text.xyz.x != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_text.xyz.x)->value_double : 0.0,
            node->planet_text.xyz.y != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_text.xyz.y)->value_double : 0.0,
            node->planet_text.xyz.z != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_text.xyz.z)->value_double : 0.0
        };
    }

    // resolve color
    float fc[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    if (node->planet_text.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) {
        fc[0] = node->planet_text.fill_color.r != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_text.fill_color.r)->value_double : 1.0f;
        fc[1] = node->planet_text.fill_color.g != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_text.fill_color.g)->value_double : 1.0f;
        fc[2] = node->planet_text.fill_color.b != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_text.fill_color.b)->value_double : 1.0f;
        fc[3] = node->planet_text.fill_color.a != DC_APP_VAL_INDEX_UNDEFINED ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_text.fill_color.a)->value_double : 1.0f;
    }

    if (node->planet_text.crs == DC_APP_PLANET_CRS_CARTESIAN) {
        dc_app_draw_planet_text_cartesian(ctx, draw_view, pos, *sb_text, size, (DcAppVec4){fc[0], fc[1], fc[2], fc[3]});
    } else {
        dc_app_draw_planet_text_geodetic(ctx, draw_view, lat, lon, height, *sb_text, size, (DcAppVec4){fc[0], fc[1], fc[2], fc[3]});
    }
}

static void _render_planet_view(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_position = &parent_frame.position;
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;
    (void)node_index;

    // look up planet def
    DcAppPlanetDefinition *def = dc_app_scene_get_planet_definition(renderer->scene, node->planet_view.planet_def_index);
    if (def->index == PLANET_INDEX_UNDEFINED) return;

    plPlanet *planet = dc_app_planet_pl(def->handle);
    if (!planet) return;

    // boolean checks
    bool use_dimension[2] = {
        node->planet_view.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->planet_view.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation           = node->planet_view.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->planet_view.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->planet_view.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->planet_view.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->planet_view.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.dimension.y)->value_double : parent_dimensions->y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->planet_view.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->planet_view.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->planet_view.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->planet_view.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2] = {0, 0};
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("PlanetView", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                    break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("PlanetView", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        DcAppAlignType local_aligns[2] = {
            node->planet_view.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->planet_view.local_align.x)->value_integer,
            node->planet_view.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->planet_view.local_align.y)->value_integer};

        float trans_align_offsets[2] = {0, 0};
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                DC_LOG_WARN("PlanetView", "Unknown X alignment: %d", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                DC_LOG_WARN("PlanetView", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);
        transform                      = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->planet_view.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->planet_view.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float          anchor[2]      = {0, 0};
        DcAppAlignType parent_align_x = node->planet_view.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->planet_view.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("PlanetView", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->planet_view.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->planet_view.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("PlanetView", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.position.y)->value_double : 0};

        if (node->planet_view.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->planet_view.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->planet_view.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->planet_view.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform                   = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

            DcAppAlignType local_pivot_aligns[2] = {
                node->planet_view.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->planet_view.pivot_local_align.x)->value_integer,
                node->planet_view.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->planet_view.pivot_local_align.y)->value_integer};

            float pivot_position[2] = {0, 0};
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    DC_LOG_WARN("PlanetView", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    DC_LOG_WARN("PlanetView", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    if (!node->planet_view.handle ||
        node->planet_view.planet_view_index == PLANET_VIEW_INDEX_UNDEFINED ||
        node->planet_view.planet_view_index > dc_app_planet_view_count(renderer->planets)) {
        DC_LOG_WARN("PlanetView", "Skipping planet view because it was not initialized");
        return;
    }

    plPlanetView *view = dc_app_planet_view_pl(node->planet_view.handle);
    if (!view) {
        DC_LOG_WARN("PlanetView", "Skipping planet view because its renderer view is unavailable");
        return;
    }

    // shader swap
    if (node->planet_view.shader_index != DC_APP_VAL_INDEX_UNDEFINED && sbcount(def->sb_shaders) > 0) {
        int desired = (int)dc_app_lookup_get_value(renderer->lookup, node->planet_view.shader_index)->value_integer;
        if (desired != node->planet_view.active_shader_index) {
            DcAppPlanetShaderEntry *found = NULL;
            for (int j = 0; j < sbcount(def->sb_shaders); j++) {
                if (def->sb_shaders[j].index == desired) {
                    found = &def->sb_shaders[j];
                    break;
                }
            }
            if (dc_app_planet_set_view_shaders(
                    node->planet_view.handle,
                    found ? found->vertex_path : NULL,
                    found ? found->fragment_path : NULL)) {
                node->planet_view.active_shader_index = desired;
            }
        }
    }

    DcAppPlanetViewOptions options = {0};
    if (node->planet_view.tau != DC_APP_VAL_INDEX_UNDEFINED)
        options.tau = (float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.tau)->value_double;
    if (node->planet_view.flatten != DC_APP_VAL_INDEX_UNDEFINED &&
        dc_app_lookup_get_value(renderer->lookup, node->planet_view.flatten)->value_boolean)
        options.flags |= DC_APP_PLANET_VIEW_FLAGS_FLATTEN;

    float fov = node->planet_view.fov != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.fov)->value_double
        : 60.0f;
    DcAppVec3 rpy = {0};
    rpy.roll = node->planet_view.rpy.roll != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.rpy.roll)->value_double
        : 0.0f;
    rpy.pitch = node->planet_view.rpy.pitch != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.rpy.pitch)->value_double
        : 0.0f;
    rpy.yaw = node->planet_view.rpy.yaw != DC_APP_VAL_INDEX_UNDEFINED
        ? (float)dc_app_lookup_get_value(renderer->lookup, node->planet_view.rpy.yaw)->value_double
        : 0.0f;
    bool use_ortho = (node->planet_view.orthographic != DC_APP_VAL_INDEX_UNDEFINED &&
                      dc_app_lookup_get_value(renderer->lookup, node->planet_view.orthographic)->value_boolean);

    // Bound the view so its queued planet work finishes before the next sibling.
    DcAppDrawScope scope = dc_app_draw_scope_begin(ctx);
    dc_app_draw_context_push(ctx, (plVec2){0.0f, 0.0f}, (plVec2){dimension[0], dimension[1]}, &transform);
    DcAppDrawPlanetViewHandle draw_view = NULL;
    bool view_added = false;
    if (node->planet_view.crs == DC_APP_PLANET_CRS_GEODETIC) {
        double lat = node->planet_view.lle.lat != DC_APP_VAL_INDEX_UNDEFINED
            ? dc_app_lookup_get_value(renderer->lookup, node->planet_view.lle.lat)->value_double : 0.0;
        double lon = node->planet_view.lle.lon != DC_APP_VAL_INDEX_UNDEFINED
            ? dc_app_lookup_get_value(renderer->lookup, node->planet_view.lle.lon)->value_double : 0.0;
        double elevation = node->planet_view.lle.ele != DC_APP_VAL_INDEX_UNDEFINED
            ? dc_app_lookup_get_value(renderer->lookup, node->planet_view.lle.ele)->value_double : 0.0;
        draw_view = dc_app_draw_planet_view_geodetic(
            ctx,
            node->planet_view.handle,
            lat,
            lon,
            elevation,
            rpy,
            fov,
            use_ortho,
            options,
            (DcAppVec2){0.0f, 0.0f},
            (DcAppVec2){dimension[0], dimension[1]},
            (DcAppPlacement){0},
            NULL);
        view_added = true;
    } else if (node->planet_view.crs == DC_APP_PLANET_CRS_CARTESIAN) {
        DcAppVec3d position = {
            node->planet_view.xyz.x != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_view.xyz.x)->value_double : 0.0,
            node->planet_view.xyz.y != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_view.xyz.y)->value_double : 0.0,
            node->planet_view.xyz.z != DC_APP_VAL_INDEX_UNDEFINED ? dc_app_lookup_get_value(renderer->lookup, node->planet_view.xyz.z)->value_double : 0.0
        };
        draw_view = dc_app_draw_planet_view_cartesian(
            ctx,
            node->planet_view.handle,
            position,
            rpy,
            fov,
            use_ortho,
            options,
            (DcAppVec2){0.0f, 0.0f},
            (DcAppVec2){dimension[0], dimension[1]},
            (DcAppPlacement){0},
            NULL);
        view_added = true;
    }

    // submits xml planet overlays before the queued planet view is rendered.
    if (view_added) {
        DcAppNodeIndex child_index = node->planet_view.child;
        while (child_index != NODE_INDEX_UNDEFINED) {
            DcAppNode *child = dc_app_scene_get_node(renderer->scene, child_index);
            if (child->type == NODE_TYPE_PLANET_BREADCRUMBS)
                _render_planet_breadcrumbs(ctx, renderer, child, draw_view);
            else if (child->type == NODE_TYPE_PLANET_CONTAINER)
                _render_planet_container(ctx, renderer, child, draw_view);
            else if (child->type == NODE_TYPE_PLANET_ELLIPSE)
                _render_planet_ellipse(ctx, renderer, child, draw_view);
            else if (child->type == NODE_TYPE_PLANET_LINE)
                _render_planet_line(ctx, renderer, child, draw_view);
            else if (child->type == NODE_TYPE_PLANET_POLYGON)
                _render_planet_polygon(ctx, renderer, child, draw_view);
            else if (child->type == NODE_TYPE_PLANET_IMAGE)
                _render_planet_image(ctx, renderer, child, draw_view);
            else if (child->type == NODE_TYPE_PLANET_SPHERE)
                _render_planet_sphere(ctx, renderer, child, draw_view);
            else if (child->type == NODE_TYPE_PLANET_TEXT)
                _render_planet_text(ctx, renderer, child, draw_view);
            child_index = child->next;
        }
    }

    dc_app_draw_context_pop(ctx);
    dc_app_draw_scope_end(ctx, scope);
}

static void _render_text(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {
    _DcAppRenderFrame parent_frame = _get_current_frame(ctx);
    plVec2 *parent_position = &parent_frame.position;
    plVec2 *parent_dimensions = &parent_frame.dimensions;
    plMat4 *parent_transform = &parent_frame.transform;

    // expand text, honoring legacy UpdateRate by caching variable expansion
    bool should_update_text = true;
    if (node->text.update_rate != DC_APP_VAL_INDEX_UNDEFINED) {
        double current_time = dc_utils_time_get();
        double update_rate  = dc_app_lookup_get_value(renderer->lookup, node->text.update_rate)->value_double;
        should_update_text = node->text.sb_cached_text == NULL ||
                             update_rate <= 0.0 ||
                             current_time - node->text.last_update_time > update_rate;
        if (should_update_text)
            node->text.last_update_time = current_time;
    }
    if (should_update_text) {
        sbclear(node->text.sb_cached_text);
        for (int ii = 0; ii < sbcount(node->text.sb_vals); ii++) {

            // filler
            char *filler = &(node->text.sb_fillers[node->text.sb_filler_indices[ii]]);
            sbpushn(node->text.sb_cached_text, filler, (int)strlen(filler));

            // value
            DcValueType format_type = node->text.sb_format_types[ii];
            char       *format      = &(node->text.sb_formats[node->text.sb_format_indices[ii]]);
            char val_str[256] = {0}; // assume text won't be that long..
            if (node->text.sb_vals[ii] == DC_APP_VAL_INDEX_UNDEFINED) {
                val_str[0] = '\0'; // empty string for undefined variable
            } else {
                DcValue *val = dc_app_lookup_get_value(renderer->lookup, node->text.sb_vals[ii]);
                switch (format_type) {
                    case DC_VALUE_TYPE_STRING:
                        snprintf(val_str, sizeof(val_str), format, val->value_string);
                        break;
                    case DC_VALUE_TYPE_INTEGER:
                        snprintf(val_str, sizeof(val_str), format, val->value_integer);
                        break;
                    case DC_VALUE_TYPE_DOUBLE:
                        snprintf(val_str, sizeof(val_str), format, val->value_double);
                        break;
                    case DC_VALUE_TYPE_BOOLEAN:
                        snprintf(val_str, sizeof(val_str), format, val->value_boolean);
                        break;
                    default:
                        DC_LOG_WARN("Text", "Unknown value type: %d", format_type);
                }
            }
            sbpushn(node->text.sb_cached_text, val_str, (int)strlen(val_str));
        }

        // ending filler
        char *filler = &(node->text.sb_fillers[node->text.sb_filler_indices[sbcount(node->text.sb_vals)]]);
        sbpushn(node->text.sb_cached_text, filler, (int)strlen(filler));
        sbpush(node->text.sb_cached_text, '\0');
    }
    char *sb_text = node->text.sb_cached_text ? node->text.sb_cached_text : "";

    // log
    if (node->text.log != DC_APP_VAL_INDEX_UNDEFINED) {
        const char *label = dc_app_lookup_get_value(renderer->lookup, node->text.log)->value_string;
        printf("[%s] %s\n", label, sb_text);
    }

    // split mutates delimiters, so keep the cached text intact
    sbclear(renderer->sb_render_text);
    sbpushn(renderer->sb_render_text, sb_text, (int)strlen(sb_text) + 1);
    sb_text = renderer->sb_render_text;

    // get text substrings per newline
    size_t subtext_indices[DC_APP_NODE_TEXT_MAX_LINES];
    size_t num_lines;
    dc_utils_split_string_inplace(sb_text, "\n", subtext_indices, DC_APP_NODE_TEXT_MAX_LINES, &num_lines);

    // setup text options
    dcDrawTextOptions text_options = {0};
    float fill_color[4]            = {
        node->text.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.fill_color.r)->value_double,
        node->text.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.fill_color.g)->value_double,
        node->text.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.fill_color.b)->value_double,
        node->text.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.fill_color.a)->value_double,
    };
    text_options.uColor = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
    text_options.fSize  = node->text.size == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.size)->value_double;

    // select font level based on rendered pixel size
    {
        // compute rendered pixel size: raw size * parent transform scale
        float parent_scale_y = sqrtf(parent_transform->x21 * parent_transform->x21 + parent_transform->x22 * parent_transform->x22);
        float rendered_size  = text_options.fSize * parent_scale_y;
        text_options.ptFont  = dc_app_font_resolve(renderer->fonts, node->text.font_index, rendered_size);
    }

    // get each strings size
    plVec2 dimensions[DC_APP_NODE_TEXT_MAX_LINES];
    plVec2 total_dimensions = {0.0f, 0.0f};
    for (int ii = 0; ii < num_lines; ii++) {
        dimensions[ii] = dc_app_draw_text_options_size(&sb_text[subtext_indices[ii]], text_options);

        // overwrite the y dimension with the size
        dimensions[ii].y = text_options.fSize;

        // compute totals
        total_dimensions.x = fmaxf(total_dimensions.x, dimensions[ii].x);
        total_dimensions.y += dimensions[ii].y;
    }

    // boolean checks
    bool use_rotation           = node->text.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position     = (node->text.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->text.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->text.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->text.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    bool is_italic   = node->text.italic != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->text.italic)->value_boolean;
    bool is_bold     = node->text.bold != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->text.bold)->value_boolean;
    bool is_outlined = node->text.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED;
    bool has_background = node->text.config_flags & NODE_CONFIG_FLAG_BACKGROUND_ENABLED;

    DcAppVec4 background_color = {
        .r = 0.0f,
        .g = 0.0f,
        .b = 0.0f,
        .a = 1.0f,
    };
    if (has_background) {
        float bg[4] = {
            node->text.background_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.background_color.r)->value_double,
            node->text.background_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.background_color.g)->value_double,
            node->text.background_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.background_color.b)->value_double,
            node->text.background_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.background_color.a)->value_double,
        };
        background_color = (DcAppVec4){
            .r = bg[0],
            .g = bg[1],
            .b = bg[2],
            .a = bg[3],
        };
    }

    // iterate over each string
    // TODO this has some redundant transforms.....clean this up!
    for (int ii = 0; ii < num_lines; ii++) {

        // transform
        plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

        // xform rotation (around a point)
        {
            if (use_rotation && use_pivot_position) {

                // get pivot XY, rotation
                float pivot_position[2] = {
                    (float)dc_app_lookup_get_value(renderer->lookup, node->text.pivot_position.x)->value_double,
                    (float)dc_app_lookup_get_value(renderer->lookup, node->text.pivot_position.y)->value_double};
                float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->text.rotation)->value_double);

                // compute matrices
                plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
                plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
                plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

                // apply transform
                transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
                transform = pl_mul_mat4t(&transform, &rotate_xform);
                transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
            } else if (use_rotation && use_pivot_parent_align) {

                DcAppAlignType parent_pivot_aligns[2] = {
                    node->text.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                        ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->text.pivot_parent_align.x)->value_integer
                        : DC_APP_ALIGN_TYPE_UNDEFINED,
                    node->text.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                        ? (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->text.pivot_parent_align.y)->value_integer
                        : DC_APP_ALIGN_TYPE_UNDEFINED};

                float pivot_position[2] = {0, 0};
                switch (parent_pivot_aligns[0]) {
                    case DC_APP_ALIGN_TYPE_UNDEFINED:
                    case DC_APP_ALIGN_TYPE_LEFT:
                        pivot_position[0] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_CENTER:
                        pivot_position[0] = parent_dimensions->x / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_RIGHT:
                        pivot_position[0] = parent_dimensions->x;
                        break;
                    default:
                        DC_LOG_WARN("Text", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]);
                        break;
                }
                switch (parent_pivot_aligns[1]) {
                    case DC_APP_ALIGN_TYPE_UNDEFINED:
                    case DC_APP_ALIGN_TYPE_BOTTOM:
                        pivot_position[1] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_MIDDLE:
                        pivot_position[1] = parent_dimensions->y / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_TOP:
                        pivot_position[1] = parent_dimensions->y;
                        break;
                    default:
                        DC_LOG_WARN("Text", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]);
                        break;
                }
                float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->text.rotation)->value_double);

                plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
                plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
                plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

                transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
                transform = pl_mul_mat4t(&transform, &rotate_xform);
                transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
            }
        }

        // xform local alignment
        {
            // get alignment
            DcAppAlignType local_aligns[2] = {
                node->text.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->text.local_align.x)->value_integer,
                node->text.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->text.local_align.y)->value_integer};

            // compute offsets
            float trans_align_offsets[2] = {0, 0};
            switch (local_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    trans_align_offsets[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    trans_align_offsets[0] = -1 * dimensions[ii].x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    trans_align_offsets[0] = -1 * dimensions[ii].x;
                    break;
                default:
                    DC_LOG_WARN("Text", "Unknown X alignment: %d", local_aligns[0]);
                    break;
            }
            switch (local_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    trans_align_offsets[1] = 0;
                    trans_align_offsets[1] += 0.0f * dimensions[ii].y * (num_lines - 1);
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    trans_align_offsets[1] = -1 * dimensions[ii].y / 2;
                    trans_align_offsets[1] -= 0.5f * dimensions[ii].y * (num_lines - 1);
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    trans_align_offsets[1] = -1 * dimensions[ii].y;
                    trans_align_offsets[1] -= 1.0f * dimensions[ii].y * (num_lines - 1);
                    break;
                default:
                    DC_LOG_WARN("Text", "Unknown Y alignment: %d", local_aligns[1]);
                    break;
            }

            trans_align_offsets[1] -= dimensions[ii].y * ii;

            // compute matrix
            plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
        }

        // xform position
        {
            bool use_position[2] = {
                node->text.position.x != DC_APP_VAL_INDEX_UNDEFINED,
                node->text.position.y != DC_APP_VAL_INDEX_UNDEFINED};

            float          anchor[2]      = {0, 0};
            DcAppAlignType parent_align_x = node->text.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->text.parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    anchor[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    anchor[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    anchor[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("Text", "Invalid parent_align_x: %d", parent_align_x);
                    break;
            }
            DcAppAlignType parent_align_y = node->text.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(renderer->lookup, node->text.parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    anchor[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    anchor[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    anchor[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("Text", "Invalid parent_align_y: %d", parent_align_y);
                    break;
            }

            float offset[2] = {
                use_position[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->text.position.x)->value_double : 0,
                use_position[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->text.position.y)->value_double : 0};

            // apply negate
            if (node->text.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->text.negate_x)->value_boolean) {
                offset[0] = -offset[0];
            }
            if (node->text.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(renderer->lookup, node->text.negate_y)->value_boolean) {
                offset[1] = -offset[1];
            }

            float position[2] = {
                parent_position->x + anchor[0] + offset[0],
                parent_position->y + anchor[1] + offset[1]};

            plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
            transform                   = pl_mul_mat4t(&transform, &trans_position_xform);
        }

        // xform local rotation
        {
            if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

                // get alignment
                DcAppAlignType local_pivot_aligns[2] = {
                    node->text.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->text.pivot_local_align.x)->value_integer,
                    node->text.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(renderer->lookup, node->text.pivot_local_align.y)->value_integer};

                // get pivot XY, rotation
                float pivot_position[2] = {0, 0};
                switch (local_pivot_aligns[0]) {
                    case DC_APP_ALIGN_TYPE_UNDEFINED:
                    case DC_APP_ALIGN_TYPE_LEFT:
                        pivot_position[0] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_CENTER:
                        pivot_position[0] = total_dimensions.x / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_RIGHT:
                        pivot_position[0] = total_dimensions.x;
                        break;
                    default:
                        DC_LOG_WARN("Text", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                        break;
                }
                switch (local_pivot_aligns[1]) {
                    case DC_APP_ALIGN_TYPE_UNDEFINED:
                    case DC_APP_ALIGN_TYPE_BOTTOM:
                        pivot_position[1] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_MIDDLE:
                        pivot_position[1] = total_dimensions.y / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_TOP:
                        pivot_position[1] = total_dimensions.y;
                        break;
                    default:
                        DC_LOG_WARN("Text", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                        break;
                }
                float rotation = pl_radiansf((float)dc_app_lookup_get_value(renderer->lookup, node->text.rotation)->value_double);

                // compute matrices
                plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
                plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
                plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

                // apply transform
                transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
                transform = pl_mul_mat4t(&transform, &rotate_xform);
                transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
            }
        }

        // italic shear
        if (is_italic) {
            plMat4 shear_xform = (plMat4){1, 0, 0, 0, 0.2f, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            transform = pl_mul_mat4t(&transform, &shear_xform);
        }

        // PL specific fixes
        {
            // move from top-left reference to bottom-left
            plMat4 trans_pl_origin_xform = pl_mat4_translate_xyz(0, total_dimensions.y, 0.0f);

            // flip over the y axis
            plMat4 scale_invert_y_xform = pl_mat4_scale_xyz(1.0f, -1.0f, 1.0f);

            // apply transforms
            transform = pl_mul_mat4t(&transform, &trans_pl_origin_xform);
            transform = pl_mul_mat4t(&transform, &scale_invert_y_xform);
        }

        // parent transform
        transform = pl_mul_mat4t(parent_transform, &transform);

        // convert to 3D matrix
        plMat3 transform3 = (plMat3){0};
        transform3.x11    = transform.x11;
        transform3.x12    = transform.x12;
        transform3.x13    = transform.x14;
        transform3.x21    = transform.x21;
        transform3.x22    = transform.x22;
        transform3.x23    = transform.x24;
        transform3.x31    = transform.x31;
        transform3.x32    = transform.x32;
        transform3.x33    = transform.x33;

        // update text options
        text_options.tTransform = transform3;

        // background pass
        if (has_background) {
            dc_app_draw_context_push(ctx, (plVec2){0.0f, 0.0f}, dimensions[ii], &transform);
            dc_app_draw_rect_filled(ctx, (DcAppVec2){0.0f, 0.0f}, (DcAppVec2){dimensions[ii].x, dimensions[ii].y}, background_color);
            dc_app_draw_context_pop(ctx);
        }

        // shadow pass
        if (node->text.shadow_offset != DC_APP_VAL_INDEX_UNDEFINED) {
            dcDrawTextOptions shadow_opts = text_options;
            shadow_opts.uColor            = PL_COLOR_32_RGBA(0, 0, 0, 1);

            float offset = (float)dc_app_lookup_get_value(renderer->lookup, node->text.shadow_offset)->value_double;
            shadow_opts.tTransform.x13 += offset;
            shadow_opts.tTransform.x23 += offset;

            dc_app_draw_text_options(ctx, &sb_text[subtext_indices[ii]], shadow_opts);
        }

        // draw fill
        dcDrawTextOptions fill_opts = text_options;
        if (is_bold && !is_outlined) {
            fill_opts.tFlags |= DC_DRAW_TEXT_FLAG_BOLD;
        }
        dc_app_draw_text_options(ctx, &sb_text[subtext_indices[ii]], fill_opts);

        // outline pass: draw on top of fill using outline shader
        if (is_outlined) {
            float outline_color[4] = {
                node->text.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.line_color.r)->value_double,
                node->text.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.line_color.g)->value_double,
                node->text.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.line_color.b)->value_double,
                node->text.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(renderer->lookup, node->text.line_color.a)->value_double,
            };
            dcDrawTextOptions outline_opts = text_options;
            outline_opts.uColor = PL_COLOR_32_RGBA(outline_color[0], outline_color[1], outline_color[2], outline_color[3]);
            outline_opts.tFlags |= DC_DRAW_TEXT_FLAG_OUTLINE;
            dc_app_draw_text_options(ctx, &sb_text[subtext_indices[ii]], outline_opts);
        }
    }
}

static void _render_window(DcAppDrawContext *ctx, DcAppRenderer *renderer, DcAppNodeIndex node_index, DcAppNode *node) {

    // TODO move this code to only the resize() function

    // current dimensions
    const DcAppDrawArea *area = dc_app_draw_get_area(ctx);

    // boolean checks
    bool use_virtual_dimension[2] = {
        node->window.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->window.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};

    // all transform parameters
    float dimension[2]         = {area->dimensions[0], area->dimensions[1]};
    float virtual_dimension[2] = {
        use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(renderer->lookup, node->window.virtual_dimension.x)->value_double : node->window.init_dimension.x,
        use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(renderer->lookup, node->window.virtual_dimension.y)->value_double : node->window.init_dimension.y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // PL xform translate Y from negative to positive range
    {
        // compute matrix
        plMat4 trans_pl_matrix = pl_mat4_translate_xyz(0.0f, dimension[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_pl_matrix);
    }

    // PL xform flip y axis
    {
        // compute matrix
        plMat4 scale_pl_matrix = pl_mat4_scale_xyz(1.0f, -1.0f, 1.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &scale_pl_matrix);
    }

    // xform scale from virtual to real dimensions,
    {
        // compute matrix
        plMat4 scale_matrix = pl_mat4_scale_xyz(dimension[0] / virtual_dimension[0], dimension[1] / virtual_dimension[1], 1.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &scale_matrix);
    }

    // draw children
    plVec2 position_vec2           = (plVec2){0.0f, 0.0f};
    plVec2 virtual_dimensions_vec2 = (plVec2){virtual_dimension[0], virtual_dimension[1]};
    dc_app_draw_context_push(ctx, position_vec2, virtual_dimensions_vec2, &transform);
    _render_node_list(ctx, renderer, node->window.child);
    dc_app_draw_context_pop(ctx);
}
