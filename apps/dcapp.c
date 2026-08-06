#define PL_EXPERIMENTAL
#include "pl.h"
#define PL_MATH_INCLUDE_FUNCTIONS
#include "pl_math.h"

#include "pl_draw_ext.h"
#include "pl_gpu_allocators_ext.h"
#include "pl_graphics_ext.h"
#include "pl_resource_ext.h"
#include "pl_screen_log_ext.h"
#include "pl_shader_ext.h"
#include "pl_starter_ext.h"
#include "pl_vfs_ext.h"

#include "dc_draw_backend_ext.h"
#include "dc_draw_ext.h"

#include "app/config.h"
#include "app/data_link.h"
#include "app/draw.h"
#include "app/draw_api.h"
#include "app/renderer.h"
#include "app/font.h"
#include "app/logic_api.h"
#include "app/logic_runtime.h"
#include "app/lookup.h"
#include "app/node.h"
#include "app/pixelstream.h"
#include "app/planet.h"
#include "app/planet_api.h"
#include "app/scene.h"
#include "app/texture.h"
#include "app/texture_api.h"
#include "app/xml.h"
#include "app/value.h"

#define PL_JSON_IMPLEMENTATION
#include "pl_json.h"

#include "utils/log.h"

#include <libxml/parser.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Private composition root. Subsystems own the state behind these contexts.
struct DcAppContext {
    DcAppConfig *config;
    plWindow    *pl_window;

    DcAppSceneContext       *scene;
    DcAppFontContext        *fonts;
    DcAppTextureContext     *textures;
    DcAppPixelstreamContext *pixelstreams;
    DcAppPlanetContext      *planets;
    DcAppLogicContext       *logic;
    DcAppDataLinkContext    *data_link;
    DcAppDrawContext        *draw;
    DcAppRenderer           *renderer;
};

typedef struct DcAppContext _AppData;

static const plMemoryI        *_ext_memory          = NULL;
static const plWindowI        *_ext_windows         = NULL;
static const plStarterI       *_ext_starter         = NULL;
static const plIOI            *_ext_ioi             = NULL;
static const plGraphicsI      *_ext_gfx             = NULL;
static const plGPUAllocatorsI *_ext_gpu_allocators  = NULL;
static const plVfsI           *_ext_vfs             = NULL;
static const plShaderI        *_ext_shader          = NULL;
static const plResourceI      *_ext_resource        = NULL;
static const plScreenLogI     *_ext_screen_log      = NULL;
static const dcDrawI          *_ext_dc_draw          = NULL;
static const dcDrawBackendI   *_ext_dc_draw_backend  = NULL;

#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_FREE(x)  _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

// declarations
PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, _AppData *app_data);
PL_EXPORT void  pl_app_shutdown(_AppData *app_data);
PL_EXPORT void  pl_app_resize(plWindow *window, _AppData *app_data);
PL_EXPORT void  pl_app_update(_AppData *app_data);

static void     _load_apis(plApiRegistryI *api_registry);
static void     _pre_init_logic(_AppData *app_data);
static void     _bootstrap_runtime(DcAppContext *app_context, DcAppXmlContext *xml_ctx, DcAppNode *window_node);
static void    *_get_variable(DcAppContext *app_ctx, const char *name);
static double   _get_update_rate(_AppData *app_data);
static void     _refresh_values(DcAppLookup *lookup);

static DcAppTextureId _texture_load_image(DcAppContext *app_context, const char *path, DcAppVec2 *out_size);
static bool _texture_get_size(DcAppContext *app_context, DcAppTextureId texture, DcAppVec2 *out_size);

static DcAppPlanetHandle _planet_get_by_id(DcAppContext *app_context, const char *id);
static DcAppPlanetHandle _planet_create(DcAppContext *app_context, DcAppPlanetCreateInfo info);
static DcAppPlanetHandle _planet_create_with_id(DcAppContext *app_context, const char *id, DcAppPlanetCreateInfo info);
static bool _planet_set_texture_geodetic(DcAppContext *app_context, DcAppPlanetHandle planet, const char *path, double lat, double lon, float meters_per_pixel);
static bool _planet_set_texture_cartesian(DcAppContext *app_context, DcAppPlanetHandle planet, const char *path, DcAppVec3d position, float meters_per_pixel);
static bool _planet_set_texture_projected(DcAppContext *app_context, DcAppPlanetHandle planet, const char *path, double origin_x, double origin_y, float meters_per_pixel);
static bool _planet_set_texture_geodetic_slot(DcAppContext *app_context, DcAppPlanetHandle planet, uint32_t slot, const char *path, double lat, double lon, float meters_per_pixel);
static bool _planet_set_texture_cartesian_slot(DcAppContext *app_context, DcAppPlanetHandle planet, uint32_t slot, const char *path, DcAppVec3d position, float meters_per_pixel);
static bool _planet_set_texture_projected_slot(DcAppContext *app_context, DcAppPlanetHandle planet, uint32_t slot, const char *path, double origin_x, double origin_y, float meters_per_pixel);
static DcAppPlanetViewHandle _planet_create_geodetic_view(DcAppContext *app_context, DcAppPlanetHandle planet, uint32_t width, uint32_t height);
static DcAppPlanetViewHandle _planet_create_cartesian_view(DcAppContext *app_context, DcAppPlanetHandle planet, uint32_t width, uint32_t height);
static DcAppPlanetGeojsonHandle _planet_load_geojson(DcAppContext *app_context, const char *path);
static DcAppPlanetBreadcrumbsHandle _planet_create_breadcrumbs(DcAppContext *app_context, DcAppPlanetCrs crs, uint32_t max_points, float point_spacing);

static const DcAppApi _app_api = {
    .get_variable = _get_variable,
};

static const DcAppTextureApi _texture_api = {
    .load_image = _texture_load_image,
    .get_size   = _texture_get_size,
};

static const DcAppPlanetApi _planet_api = {
    .get_planet_by_id             = _planet_get_by_id,
    .create_planet                = _planet_create,
    .create_planet_with_id        = _planet_create_with_id,
    .set_texture_geodetic         = _planet_set_texture_geodetic,
    .set_texture_cartesian        = _planet_set_texture_cartesian,
    .set_texture_projected        = _planet_set_texture_projected,
    .set_texture_geodetic_slot    = _planet_set_texture_geodetic_slot,
    .set_texture_cartesian_slot   = _planet_set_texture_cartesian_slot,
    .set_texture_projected_slot   = _planet_set_texture_projected_slot,
    .clear_texture                = dc_app_planet_clear_texture,
    .set_light_direction          = dc_app_planet_set_light_direction,
    .create_geodetic_view         = _planet_create_geodetic_view,
    .create_cartesian_view        = _planet_create_cartesian_view,
    .set_view_shaders             = dc_app_planet_set_view_shaders,
    .load_geojson                 = _planet_load_geojson,
    .create_breadcrumbs           = _planet_create_breadcrumbs,
    .update_breadcrumbs_geodetic  = dc_app_planet_update_breadcrumbs_geodetic,
    .update_breadcrumbs_cartesian = dc_app_planet_update_breadcrumbs_cartesian,
    .clear_breadcrumbs            = dc_app_planet_clear_breadcrumbs,
    .get_breadcrumbs_points       = dc_app_planet_get_breadcrumbs_points,
};

static void _pre_init_logic(_AppData *app_data) {
    // exposes only dcapp-owned api tables to logic.
    const DcAppInit init = {
        .app_ctx = (DcAppContext *)app_data,
        .app     = &_app_api,
        .draw    = dc_app_draw_api(),
        .mouse   = dc_app_mouse_api(),
        .texture = &_texture_api,
        .planet  = &_planet_api,
    };
    dc_app_logic_pre_init(app_data->logic, &init);
}

PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, _AppData *app_data) {

    if (app_data) {
        // Hot reload keeps app state but refreshes extension pointers and logic-facing API tables.
        _load_apis(api_registry);
        _pre_init_logic(app_data);
        return app_data;
    }

    // retrieve extension registry
    const plExtensionRegistryI *extension_registry = pl_get_api_latest(api_registry, plExtensionRegistryI);

    // load required extensions
    extension_registry->load("pl_unity_ext", NULL, NULL, true);
    extension_registry->load("pl_platform_ext", "pl_load_platform_ext", "pl_unload_platform_ext", false);

    // load dcapp extensions (separate from pilotlight's draw extensions)
    extension_registry->load("dc_draw_ext", NULL, NULL, true);
    extension_registry->load("dc_draw_backend_ext", NULL, NULL, true);
    extension_registry->load("pl_planet_processor_ext", NULL, NULL, true);
    extension_registry->load("pl_planet_ext", NULL, NULL, true);
    _load_apis(api_registry);

    // allocate app memory
    app_data = (_AppData *)PL_ALLOC(sizeof(_AppData));
    memset(app_data, 0, sizeof(_AppData));

    // parse input arguments
    plIO *_ext_io = _ext_ioi->get_io();
    if (_ext_io->iArgc < 4) {
        DC_LOG_ERROR("App", "Missing dcapp config file");
        PL_FREE(app_data);
        return NULL;
    }

    // parse --preprocessed flag (before constants)
    const char *preprocessed_output = NULL;
    int         const_count         = 0;
    char      **const_args          = NULL;

    for (int ii = 4; ii < _ext_io->iArgc; ii++) {
        if (strcmp(_ext_io->apArgv[ii], "--preprocessed") == 0 && ii + 1 < _ext_io->iArgc) {
            preprocessed_output = _ext_io->apArgv[++ii];
        }
    }

    // collect constant args (skip --preprocessed and its value)
    if (_ext_io->iArgc > 4) {
        const_args = (char **)malloc(sizeof(char *) * (_ext_io->iArgc - 4));
        for (int ii = 4; ii < _ext_io->iArgc; ii++) {
            if (strcmp(_ext_io->apArgv[ii], "--preprocessed") == 0 && ii + 1 < _ext_io->iArgc) {
                ii++; // skip value
                continue;
            }
            const_args[const_count++] = _ext_io->apArgv[ii];
        }
    }

    // create config
    const char *config_filepath = _ext_io->apArgv[3];
    if (const_count > 0) {
        app_data->config = dc_app_config_create(config_filepath, const_args, const_count);
    } else {
        app_data->config = dc_app_config_create(config_filepath, NULL, 0);
    }
    free(const_args);

    // initialize subsystem contexts
    app_data->scene     = dc_app_scene_create();
    app_data->planets   = dc_app_planet_context_create(dc_app_config_directory(app_data->config));
    app_data->logic     = dc_app_logic_context_create();
    app_data->data_link = dc_app_data_link_context_create();

    // Export application and display roots before preprocessing XML paths.
    dc_app_config_export_environment(app_data->config);

    // create lookup
    DcAppLookup *lookup = dc_app_scene_lookup(app_data->scene);

    // preprocess XML file
    dc_app_config_preprocess(app_data->config);
    dc_app_lookup_set_suppress_missing_variable(
        lookup,
        dc_app_config_suppresses_missing_variable(app_data->config));

    // dump preprocessed XML for debugging
    dc_app_config_save_preprocessed(app_data->config, preprocessed_output);

    DcAppXmlContext *xml_ctx = dc_app_xml_context_create(
        (DcAppContext *)app_data,
        app_data->scene,
        app_data->logic,
        app_data->data_link,
        dc_app_config_root_directory(app_data->config),
        _bootstrap_runtime);

    // build dcapp node tree
    xmlNodePtr root = dc_app_config_root(app_data->config);
    dc_app_process_xml_node(
        xml_ctx,
        root,
        NODE_INDEX_UNDEFINED,
        DC_APP_ELEM_TYPE_UNDEFINED,
        dc_app_config_directory(app_data->config));
    // Freeze value storage before generated logic publishes pointers into it.
    dc_app_lookup_seal(lookup);
    dc_app_xml_context_destroy(xml_ctx);

    // initialize resource manager (needed by planet texture loading)
    plResourceManagerInit resource_init = {0};
    resource_init.ptDevice              = _ext_starter->get_device();
    _ext_resource->initialize(resource_init);

    // initialize planet rendering instances
    dc_app_renderer_initialize_planets(app_data->renderer);

    // initialize logic (link values)
    _pre_init_logic(app_data);

    // call logic init
    dc_app_logic_initialize(app_data->logic, (DcAppContext *)app_data);

    // return app memory
    return app_data;
}

PL_EXPORT void pl_app_shutdown(_AppData *app_data) {

    // call logic close
    // unload logic shared library
    dc_app_logic_context_destroy(app_data->logic, (DcAppContext *)app_data);
    app_data->logic = NULL;

    // get device
    plDevice *device = _ext_starter->get_device();

    // wait for GPU to finish all work before destroying resources
    _ext_gfx->flush_device(device);

    // cleanup draw batch system
    dc_app_renderer_destroy(app_data->renderer);
    dc_app_draw_context_destroy(app_data->draw);

    // cleanup pixelstream sources
    // cleanup pixelstream global contexts
    dc_app_pixelstream_context_destroy(app_data->pixelstreams);

    // cleanup fonts
    dc_app_font_context_destroy(app_data->fonts);

    // cleanup textures
    dc_app_texture_context_destroy(app_data->textures);

    // cleanup planet views, instances, and extension
    dc_app_planet_context_destroy(app_data->planets);
    _ext_resource->cleanup();

    // cleanup trick contexts
    // cleanup edge contexts
    dc_app_data_link_context_destroy(app_data->data_link);

    // cleanup lookup and config
    dc_app_scene_destroy(app_data->scene);
    dc_app_config_destroy(app_data->config);

    // cleanup draw backend (GPU buffers, bind group pool, font atlas, drawlists)
    _ext_dc_draw_backend->cleanup_font_atlas(NULL);
    _ext_dc_draw_backend->cleanup();
    _ext_dc_draw->cleanup();

    // cleanup GPU memory allocators (buddy heap, staging allocators)
    _ext_gpu_allocators->cleanup(device);
    _ext_starter->cleanup();
    _ext_windows->destroy(app_data->pl_window);
    PL_FREE(app_data);
}

PL_EXPORT void pl_app_resize(plWindow *window, _AppData *app_data) {
    (void)window;
    (void)app_data;
    _ext_starter->resize();
}

PL_EXPORT void pl_app_update(_AppData *app_data) {

    // this needs to be the first call when using the starter
    // extension. You must return if it returns false (usually a swapchain recreation).
    if (!_ext_starter->begin_frame()) {
        return;
    }

    _ext_resource->new_frame();
    DcAppLookup *lookup = dc_app_scene_lookup(app_data->scene);

    // External and logic writes happen before cached value representations are synchronized.
    dc_app_data_link_update(app_data->data_link, lookup);

    // process pixelstream data
    dc_app_pixelstream_update(app_data->pixelstreams);

    // process logic
    dc_app_logic_update(app_data->logic, (DcAppContext *)app_data, _get_update_rate(app_data));

    // refresh variables
    _refresh_values(lookup);

    // get mouse position
    plVec2 mouse_position = _ext_ioi->get_mouse_pos();

    // get mouse button status
    DcAppDrawFrameInput draw_input = {
        .mouse_position       = mouse_position,
        .mouse_position_valid = isfinite(mouse_position.x) && isfinite(mouse_position.y),
        .mouse_down           = _ext_ioi->is_mouse_down(PL_MOUSE_BUTTON_LEFT),
    };

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~drawing & profile API~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // new_frame: backend calls draw's new_frame + allocates dynamic data block
    _ext_dc_draw_backend->new_frame();

    // reset draw batch system for new frame
    dc_app_draw_context_begin(app_data->draw, draw_input);

    // update planet definitions (texture reload, prepare)
    dc_app_renderer_update_planets(app_data->renderer);

    // draw node
    dc_app_renderer_render(app_data->renderer, app_data->draw);
    dc_app_draw_context_end(app_data->draw);

    // flush deferred set operations (deferred Sets applied atomically)
    dc_app_renderer_flush_deferred_sets(app_data->renderer);

    // reset any unpoped variable stacks
    dc_app_lookup_reset_var_stacks(lookup);

    // start main pass & return the encoder being used
    plRenderEncoder *encoder = _ext_starter->begin_main_pass();

    // submit draw lists from batch system in order
    dc_app_draw_context_submit(app_data->draw, encoder);
    _ext_starter->end_main_pass();

    // must be the last function called when using the starter extension
    _ext_starter->end_frame();

    // update node states
    dc_app_draw_context_commit(app_data->draw);
}

static void _bootstrap_runtime(DcAppContext *app_context, DcAppXmlContext *xml_ctx, DcAppNode *window_node) {
    _AppData *app_data = (_AppData *)app_context;

    // mount VFS dirs
    _ext_vfs->mount_directory("/shaders-terrain", "../../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/assets", "../../data", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/cache", "cache", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/shaders", "../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/shader-temp", "../shader-temp", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/tiles", "../../data", PL_VFS_MOUNT_FLAGS_NONE);

    // set initial window params
    plWindowDesc window_desc = {};
    window_desc.pcTitle      = window_node->window.title;
    window_desc.uWidth       = window_node->window.fullscreen && window_node->window.init_dimension.x < 1.0f
                                   ? 1280u
                                   : (uint32_t)window_node->window.init_dimension.x;
    window_desc.uHeight      = window_node->window.fullscreen && window_node->window.init_dimension.y < 1.0f
                                   ? 720u
                                   : (uint32_t)window_node->window.init_dimension.y;
    window_desc.iXPos        = (int)window_node->window.init_position.x;
    window_desc.iYPos        = (int)window_node->window.init_position.y;
    _ext_windows->create(window_desc, &(app_data->pl_window));

    if (window_node->window.fullscreen) {
        plFullScreenDesc fullscreen_desc = {};
        fullscreen_desc.tMode = PL_FULLSCREEN_MODE_BORDERLESS;
        fullscreen_desc.iMonitor = -1;
        _ext_windows->set_fullscreen(app_data->pl_window, &fullscreen_desc);
    }

    _ext_windows->show(app_data->pl_window);

    // initialize the starter API (handles alot of boilerplate)
    plStarterInit tStarterInit = {
        .tFlags   = PL_STARTER_FLAGS_ALL_EXTENSIONS & (~PL_STARTER_FLAGS_SHADER_EXT) | PL_STARTER_FLAGS_MSAA | PL_STARTER_FLAGS_DEPTH_BUFFER,
        .ptWindow = app_data->pl_window};
    _ext_starter->initialize(tStarterInit);

    #ifdef NDEBUG
    _ext_screen_log->set_flags(PL_SCREEN_LOG_FLAGS_HIDE_MESSAGES);
    #endif

    // get device
    plDevice *device = _ext_starter->get_device();

    // initialize dc_draw_ext and dc_draw_backend_ext (pl_starter doesn't do this since we use dcDrawI)
    dcDrawInit tDrawInit = {0};
    _ext_dc_draw->initialize(&tDrawInit);
    _ext_dc_draw_backend->initialize(device);

    // create default font atlas
    app_data->fonts = dc_app_font_context_create();
    dc_app_xml_set_fonts(xml_ctx, app_data->fonts);

    // initialize shader compiler
    plShaderOptions shader_options          = {};
    shader_options.apcIncludeDirectories[0] = "/shaders/";
    shader_options.apcIncludeDirectories[1] = "/shaders-terrain/";
    shader_options.apcDirectories[0]        = "/shaders/";
    shader_options.apcDirectories[1]        = "/shaders-terrain/";
    shader_options.pcCacheOutputDirectory   = "/shader-temp/";
    shader_options.tFlags                   = PL_SHADER_FLAGS_AUTO_OUTPUT | PL_SHADER_FLAGS_INCLUDE_DEBUG | PL_SHADER_FLAGS_ALWAYS_COMPILE;
    _ext_shader->initialize(&shader_options);

    // wraps up
    _ext_starter->finalize();

    app_data->textures = dc_app_texture_context_create(dc_app_config_directory(app_data->config));

    // initialize pixelstream contexts
    app_data->pixelstreams = dc_app_pixelstream_context_create(app_data->textures);
    app_data->draw = dc_app_draw_context_create(
        dc_app_font_default(app_data->fonts),
        app_data->textures);
    app_data->renderer = dc_app_renderer_create(
        app_context,
        app_data->scene,
        app_data->fonts,
        app_data->textures,
        app_data->planets,
        app_data->pixelstreams,
        app_data->logic);

    dc_app_xml_set_textures(xml_ctx, app_data->textures);
    dc_app_xml_set_pixelstreams(xml_ctx, app_data->pixelstreams);
}

// -- handlers for logic files --
// * only works once all variables are registered, as pointer
// * values could change otherwise
static void *_get_variable(DcAppContext *app_ctx, const char *name) {
    _AppData *app_data = (_AppData *)app_ctx;
    if (!app_data) return NULL;

    // get variable
    DcAppLookup   *lookup      = dc_app_scene_lookup(app_data->scene);
    DcAppValIndex  value_index = dc_app_lookup_get_var_value_index_by_name(lookup, name);
    if (value_index == DC_APP_VAL_INDEX_UNDEFINED) return NULL;

    // return value
    DcValue *value = dc_app_lookup_get_value(lookup, value_index);
    return dc_value_get_addr(value);
}

static double _get_update_rate(_AppData *app_data) {

    if (!app_data) return 0.0;

    DcAppNode *window_node = dc_app_scene_get_node(
        app_data->scene,
        dc_app_scene_get_window(app_data->scene));
    if (!window_node || window_node->type != NODE_TYPE_WINDOW ||
        window_node->window.update_rate == DC_APP_VAL_INDEX_UNDEFINED) {
        return 0.0;
    }

    DcValue *update_rate_value = dc_app_lookup_get_value(
        dc_app_scene_lookup(app_data->scene),
        window_node->window.update_rate);
    return update_rate_value ? update_rate_value->value_double : 0.0;
}

static void _refresh_values(DcAppLookup *lookup) {
    for (int ii = DC_APP_LOOKUP_FIRST_INDEX; ii < dc_app_lookup_get_var_count(lookup); ii++) {
        DcAppValIndex value_index = dc_app_lookup_get_var_value_index(lookup, ii);
        DcValue      *value       = dc_app_lookup_get_value(lookup, value_index);
        dc_value_refresh(value);
    }
}

static DcAppTextureId _texture_load_image(DcAppContext *app_context, const char *path, DcAppVec2 *out_size) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_texture_load_image(app_data ? app_data->textures : NULL, path, out_size);
}

static bool _texture_get_size(DcAppContext *app_context, DcAppTextureId texture, DcAppVec2 *out_size) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_texture_get_size(app_data ? app_data->textures : NULL, texture, out_size);
}

static DcAppPlanetHandle _planet_get_by_id(DcAppContext *app_context, const char *id) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_get_planet_by_id(app_data ? app_data->planets : NULL, id);
}

static DcAppPlanetHandle _planet_create(DcAppContext *app_context, DcAppPlanetCreateInfo info) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_create_planet(app_data ? app_data->planets : NULL, info);
}

static DcAppPlanetHandle _planet_create_with_id(DcAppContext *app_context, const char *id, DcAppPlanetCreateInfo info) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_create_planet_with_id(app_data ? app_data->planets : NULL, id, info);
}

static bool _planet_set_texture_geodetic(DcAppContext *app_context, DcAppPlanetHandle planet, const char *path, double lat, double lon, float meters_per_pixel) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_set_texture_geodetic(app_data ? app_data->planets : NULL, planet, path, lat, lon, meters_per_pixel);
}

static bool _planet_set_texture_cartesian(DcAppContext *app_context, DcAppPlanetHandle planet, const char *path, DcAppVec3d position, float meters_per_pixel) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_set_texture_cartesian(app_data ? app_data->planets : NULL, planet, path, position, meters_per_pixel);
}

static bool _planet_set_texture_projected(DcAppContext *app_context, DcAppPlanetHandle planet, const char *path, double origin_x, double origin_y, float meters_per_pixel) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_set_texture_projected_slot(app_data ? app_data->planets : NULL, planet, 0, path, origin_x, origin_y, meters_per_pixel);
}

static bool _planet_set_texture_geodetic_slot(DcAppContext *app_context, DcAppPlanetHandle planet, uint32_t slot, const char *path, double lat, double lon, float meters_per_pixel) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_set_texture_geodetic_slot(app_data ? app_data->planets : NULL, planet, slot, path, lat, lon, meters_per_pixel);
}

static bool _planet_set_texture_cartesian_slot(DcAppContext *app_context, DcAppPlanetHandle planet, uint32_t slot, const char *path, DcAppVec3d position, float meters_per_pixel) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_set_texture_cartesian_slot(app_data ? app_data->planets : NULL, planet, slot, path, position, meters_per_pixel);
}

static bool _planet_set_texture_projected_slot(DcAppContext *app_context, DcAppPlanetHandle planet, uint32_t slot, const char *path, double origin_x, double origin_y, float meters_per_pixel) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_set_texture_projected_slot(app_data ? app_data->planets : NULL, planet, slot, path, origin_x, origin_y, meters_per_pixel);
}

static DcAppPlanetViewHandle _planet_create_geodetic_view(DcAppContext *app_context, DcAppPlanetHandle planet, uint32_t width, uint32_t height) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_create_geodetic_view(app_data ? app_data->planets : NULL, planet, width, height);
}

static DcAppPlanetViewHandle _planet_create_cartesian_view(DcAppContext *app_context, DcAppPlanetHandle planet, uint32_t width, uint32_t height) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_create_cartesian_view(app_data ? app_data->planets : NULL, planet, width, height);
}

static DcAppPlanetGeojsonHandle _planet_load_geojson(DcAppContext *app_context, const char *path) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_load_geojson(app_data ? app_data->planets : NULL, path);
}

static DcAppPlanetBreadcrumbsHandle _planet_create_breadcrumbs(DcAppContext *app_context, DcAppPlanetCrs crs, uint32_t max_points, float point_spacing) {
    _AppData *app_data = (_AppData *)app_context;
    return dc_app_planet_create_breadcrumbs(app_data ? app_data->planets : NULL, crs, max_points, point_spacing);
}

static void _load_apis(plApiRegistryI *api_registry) {
    _ext_memory          = pl_get_api_latest(api_registry, plMemoryI);
    _ext_windows         = pl_get_api_latest(api_registry, plWindowI);
    _ext_starter         = pl_get_api_latest(api_registry, plStarterI);
    _ext_ioi             = pl_get_api_latest(api_registry, plIOI);
    _ext_gfx             = pl_get_api_latest(api_registry, plGraphicsI);
    _ext_gpu_allocators  = pl_get_api_latest(api_registry, plGPUAllocatorsI);
    _ext_vfs             = pl_get_api_latest(api_registry, plVfsI);
    _ext_shader          = pl_get_api_latest(api_registry, plShaderI);
    _ext_resource        = pl_get_api_latest(api_registry, plResourceI);
    _ext_screen_log      = pl_get_api_latest(api_registry, plScreenLogI);
    _ext_dc_draw          = pl_get_api_latest(api_registry, dcDrawI);
    _ext_dc_draw_backend  = pl_get_api_latest(api_registry, dcDrawBackendI);
    dc_app_scene_init(api_registry);
    dc_app_font_init(api_registry);
    dc_app_texture_init(api_registry);
    dc_app_pixelstream_init(api_registry);
    dc_app_planet_init(api_registry);
    dc_app_logic_runtime_init(api_registry);
    dc_app_data_link_init(api_registry);
    dc_app_draw_init(api_registry);
    dc_app_renderer_init(api_registry);
    dc_app_xml_init(api_registry);
}
