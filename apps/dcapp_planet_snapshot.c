#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "pl.h"

#define PL_MATH_INCLUDE_FUNCTIONS
#include "pl_math.h"

#include "pl_camera_ext.h"
#include "pl_graphics_ext.h"
#include "pl_planet_ext.h"
#include "pl_planet_processor_ext.h"
#include "pl_resource_ext.h"
#include "pl_shader_ext.h"
#include "pl_starter_ext.h"
#include "pl_vfs_ext.h"

#include "../src/utils/file.h"
#include "../src/utils/string.h"
#include "../src/utils/math.h"
#include "../src/geo.h"

#define PL_JSON_IMPLEMENTATION
#include "pl_json.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define SNAPSHOT_SETTLE_IDLE_FRAMES 3
#define SNAPSHOT_TAU 0.05f

#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_FREE(x) _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

typedef enum _SnapshotCrs {
    SNAPSHOT_CRS_UNDEFINED,
    SNAPSHOT_CRS_GEODETIC,
    SNAPSHOT_CRS_CARTESIAN,
} _SnapshotCrs;

typedef enum _SnapshotAttitudeFrame {
    SNAPSHOT_ATTITUDE_FRAME_UNDEFINED,
    SNAPSHOT_ATTITUDE_FRAME_LOCAL_NED,
    SNAPSHOT_ATTITUDE_FRAME_CARTESIAN_RPY,
} _SnapshotAttitudeFrame;

typedef struct AppData {
    // CLI inputs
    const char *planet_data;
    const char *output;
    const char *vertex_shader;
    const char *fragment_shader;

    // Camera interpretation
    _SnapshotCrs crs;
    _SnapshotAttitudeFrame attitude_frame;

    // Camera position and attitude
    double lat, lon, elevation;
    double x, y, z;
    float roll, pitch, yaw;
    float fov;

    // Output image
    uint32_t width;
    uint32_t height;

    // Window and renderer state
    plWindow *window;
    plPlanet *planet;
    plPlanetView *view;
    plPlanetProcessInfo process_info;

    // GPU readback target
    plBufferHandle readback_buffer;
    size_t readback_size;

    // Capture state
    uint32_t frame;
    uint32_t idle_frame_count;
    bool done;

    // Cleanup guards
    bool starter_initialized;
    bool planet_initialized;
    bool resource_initialized;
    bool shader_initialized;
} AppData;

static const plIOI *_ext_ioi = NULL;
static const plWindowI *_ext_windows = NULL;
static const plGraphicsI *_ext_gfx = NULL;
static const plStarterI *_ext_starter = NULL;
static const plPlanetI *_ext_planet = NULL;
static const plResourceI *_ext_resource = NULL;
static const plCameraI *_ext_camera = NULL;
static const plShaderI *_ext_shader = NULL;
static const plVfsI *_ext_vfs = NULL;
static const plMemoryI *_ext_memory = NULL;

PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, AppData *app);
PL_EXPORT void pl_app_shutdown(AppData *app);
PL_EXPORT void pl_app_resize(plWindow *window, AppData *app);
PL_EXPORT void pl_app_update(AppData *app);

// Init and setup
static void _show_help(void);
static bool _load_planet_data(AppData *app);

// Argument parsing
static bool _parse_args(int argc, char **argv, AppData *app);

PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, AppData *app) {
    // Hot reload: refresh API pointers, keep existing app data.
    if (app) {
        _ext_ioi = pl_get_api_latest(api_registry, plIOI);
        _ext_windows = pl_get_api_latest(api_registry, plWindowI);
        _ext_gfx = pl_get_api_latest(api_registry, plGraphicsI);
        _ext_starter = pl_get_api_latest(api_registry, plStarterI);
        _ext_planet = pl_get_api_latest(api_registry, plPlanetI);
        _ext_resource = pl_get_api_latest(api_registry, plResourceI);
        _ext_camera = pl_get_api_latest(api_registry, plCameraI);
        _ext_shader = pl_get_api_latest(api_registry, plShaderI);
        _ext_vfs = pl_get_api_latest(api_registry, plVfsI);
        _ext_memory = pl_get_api_latest(api_registry, plMemoryI);
        return app;
    }

    // Load only the extensions this utility needs.
    const plExtensionRegistryI *registry = pl_get_api_latest(api_registry, plExtensionRegistryI);
    registry->load("pl_unity_ext", NULL, NULL, true);
    registry->load("pl_platform_ext", "pl_load_platform_ext", "pl_unload_platform_ext", false);
    registry->load("pl_planet_processor_ext", NULL, NULL, true);
    registry->load("pl_planet_ext", NULL, NULL, true);

    // Cache extension APIs.
    _ext_ioi = pl_get_api_latest(api_registry, plIOI);
    _ext_windows = pl_get_api_latest(api_registry, plWindowI);
    _ext_gfx = pl_get_api_latest(api_registry, plGraphicsI);
    _ext_starter = pl_get_api_latest(api_registry, plStarterI);
    _ext_planet = pl_get_api_latest(api_registry, plPlanetI);
    _ext_resource = pl_get_api_latest(api_registry, plResourceI);
    _ext_camera = pl_get_api_latest(api_registry, plCameraI);
    _ext_shader = pl_get_api_latest(api_registry, plShaderI);
    _ext_vfs = pl_get_api_latest(api_registry, plVfsI);
    _ext_memory = pl_get_api_latest(api_registry, plMemoryI);

    app = (AppData *)PL_ALLOC(sizeof(AppData));
    if (!app)
        return NULL;
    memset(app, 0, sizeof(AppData));

    // Parse all snapshot settings before creating GPU resources.
    plIO *io = _ext_ioi->get_io();
    if (!_parse_args(io->iArgc - 3, io->apArgv + 3, app)) {
        io->bRunning = false;
        return app;
    }

    // Mount shader search paths used by planet shaders.
    _ext_vfs->mount_directory("/shaders-terrain", "../../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/shaders", "../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/shader-temp", "../shader-temp", PL_VFS_MOUNT_FLAGS_NONE);

    // Create the render window at the requested snapshot size.
    plWindowDesc window_desc = {
        .pcTitle = "dcapp planet snapshot",
        .iXPos = 100,
        .iYPos = 100,
        .uWidth = app->width,
        .uHeight = app->height,
    };
    _ext_windows->create(window_desc, &app->window);
    _ext_windows->show(app->window);

    // Initialize Pilotlight graphics, then override shader options.
    plStarterInit starter_init = {
        .tFlags = (PL_STARTER_FLAGS_ALL_EXTENSIONS & ~PL_STARTER_FLAGS_SHADER_EXT) | PL_STARTER_FLAGS_VSYNC_OFF,
        .ptWindow = app->window,
    };
    _ext_starter->initialize(starter_init);
    app->starter_initialized = true;

    plShaderOptions shader_opts = {0};
    shader_opts.apcIncludeDirectories[0] = "/shaders/";
    shader_opts.apcIncludeDirectories[1] = "/shaders-terrain/";
    shader_opts.apcDirectories[0] = "/shaders/";
    shader_opts.apcDirectories[1] = "/shaders-terrain/";
    shader_opts.pcCacheOutputDirectory = "/shader-temp/";
    shader_opts.tFlags = PL_SHADER_FLAGS_AUTO_OUTPUT | PL_SHADER_FLAGS_INCLUDE_DEBUG | PL_SHADER_FLAGS_ALWAYS_COMPILE;
    _ext_shader->initialize(&shader_opts);
    app->shader_initialized = true;

    _ext_starter->finalize();

    // Planet rendering depends on resource and planet extension state.
    plDevice *device = _ext_starter->get_device();
    plResourceManagerInit resource_init = {.ptDevice = device};
    _ext_resource->initialize(resource_init);
    app->resource_initialized = true;
    plPlanetExtInit planet_init = {.ptDevice = device};
    _ext_planet->initialize(planet_init);
    app->planet_initialized = true;

    if (!_load_planet_data(app)) {
        io->bRunning = false;
        return app;
    }

    // Create the planet, its offscreen view, and the CPU readback buffer.
    plPlanetInit snapshot_planet_init = {
        .dRadius = app->process_info.tGeodeticModel.sphere.dRadius,
        .tLoadFlags = PL_PLANET_LOAD_FLAGS_NONE,
    };

    plCommandBuffer *cmd = _ext_starter->get_temporary_command_buffer();
    app->planet = _ext_planet->create_planet(cmd, snapshot_planet_init, &app->process_info);
    _ext_starter->submit_temporary_command_buffer(cmd);

    plPlanetViewInit view_init = {
        .uOutputWidth = app->width,
        .uOutputHeight = app->height,
        .pcVertexShader = app->vertex_shader,
        .pcFragmentShader = app->fragment_shader,
    };

    cmd = _ext_starter->get_temporary_command_buffer();
    app->view = _ext_planet->create_view(app->planet, cmd, view_init);
    _ext_starter->submit_temporary_command_buffer(cmd);

    if (!app->planet || !app->view) {
        io->bRunning = false;
        return app;
    }

    plPlanetViewRuntimeOptions view_opts = _ext_planet->get_view_runtime_options(app->view);
    view_opts.fTau = SNAPSHOT_TAU;
    _ext_planet->set_view_runtime_options(app->view, view_opts);

    app->readback_size = (size_t)app->width * (size_t)app->height * 4;
    const plBufferDesc readback_desc = {
        .tUsage = PL_BUFFER_USAGE_STAGING,
        .szByteSize = app->readback_size,
        .pcDebugName = "planet snapshot readback",
    };
    app->readback_buffer = _ext_gfx->create_buffer(device, &readback_desc, NULL);
    plBuffer *readback_buffer = _ext_gfx->get_buffer(device, app->readback_buffer);
    if (!readback_buffer) {
        io->bRunning = false;
        return app;
    }
    const plDeviceMemoryAllocation readback_allocation = _ext_gfx->allocate_memory(
        device, readback_buffer->tMemoryRequirements.ulSize,
        PL_MEMORY_FLAGS_HOST_VISIBLE | PL_MEMORY_FLAGS_HOST_COHERENT,
        readback_buffer->tMemoryRequirements.uMemoryTypeBits, "planet snapshot readback memory");
    _ext_gfx->bind_buffer_to_memory(device, app->readback_buffer, &readback_allocation);

    return app;
}

PL_EXPORT void pl_app_update(AppData *app) {
    if (!app || app->done)
        return;

    if (!_ext_starter->begin_frame())
        return;

    plCamera camera = {0};
    double radius = app->process_info.tGeodeticModel.sphere.dRadius;

    // Build the snapshot camera from the requested CRS and attitude frame.
    camera.tType = PL_CAMERA_TYPE_PERSPECTIVE_REVERSE_Z;
    camera.fFieldOfView = pl_radiansf(app->fov);
    camera.fAspectRatio = (app->height > 0) ? (float)app->width / (float)app->height : 1.0f;
    camera.fNearZ = 1.0f;
    camera.fFarZ = 100000000.0f;
    camera.fWidth = (float)app->width;
    camera.fHeight = (float)app->height;

    if (app->crs == SNAPSHOT_CRS_CARTESIAN) {
        _ext_camera->set_pos(&camera, app->x, app->y, app->z);
        _ext_camera->set_pitch_yaw(&camera, pl_radiansf(app->pitch), pl_radiansf(app->yaw));
        camera.fRoll = pl_radiansf(app->roll);
    } else {
        // Local-NED: yaw about down, pitch about right, roll about boresight.
        double lat_rad = dc_utils_degrees_to_radians(app->lat);
        double lon_rad = dc_utils_degrees_to_radians(app->lon);
        DcGeoCrsGeodetic geodetic_crs = dc_geo_create_crs_geodetic(radius);
        DcGeoCrsCartesian cartesian_crs = dc_geo_create_crs_cartesian(radius);
        plVec3d geodetic_in = {app->lat, app->lon, app->elevation};
        plVec3d eye;
        dc_geo_geodetic_to_cartesian_d(&geodetic_crs, &cartesian_crs, &geodetic_in, &eye, 1);
        plVec3 north, east, down, up;
        dc_geo_get_local_ned_basis(lat_rad, lon_rad, &north, &east, &down, &up);
        float yaw = pl_radiansf(app->yaw);
        float pitch = pl_radiansf(app->pitch);
        float roll = pl_radiansf(app->roll);
        plVec3 forward = down;
        plVec3 right = dc_geo_rotate_vector_around_axis(east, down, yaw);
        plVec3 desired_up = dc_geo_rotate_vector_around_axis(north, down, yaw);
        forward = dc_geo_rotate_vector_around_axis(forward, right, pitch);
        desired_up = dc_geo_rotate_vector_around_axis(desired_up, right, pitch);
        desired_up = dc_geo_rotate_vector_around_axis(desired_up, forward, roll);
        plVec3d target = {
            eye.x + (double)forward.x,
            eye.y + (double)forward.y,
            eye.z + (double)forward.z
        };
        _ext_camera->look_at(&camera, eye, target);
        camera.fRoll = 0.0f;
        _ext_camera->update(&camera);
        camera.fRoll = dc_geo_signed_angle_around_axis(camera._tUpVec, desired_up, forward);
    }

    // Camera update applies attitude and builds the perspective projection.
    _ext_camera->update(&camera);

    // Render one frame and watch tile streaming settle.
    plCommandBuffer *cmd = _ext_starter->get_command_buffer();
    _ext_planet->prepare(app->planet, cmd);
    _ext_planet->render_view(app->view, &camera, cmd);

    plPlanetStreamStats stream_stats = _ext_planet->get_stream_stats(app->planet);
    if (stream_stats.uPendingRequests == 0)
        app->idle_frame_count++;
    else
        app->idle_frame_count = 0;

    _ext_starter->submit_command_buffer(cmd);
    _ext_starter->end_frame();

    // Capture after the visible view has stopped requesting new chunks.
    if (app->idle_frame_count >= SNAPSHOT_SETTLE_IDLE_FRAMES) {
        _ext_gfx->flush_device(_ext_starter->get_device());
        plCommandBuffer *copy_cmd = _ext_starter->get_temporary_command_buffer();
        plBlitEncoder *blit = _ext_gfx->begin_blit_pass(copy_cmd);
        plBufferImageCopy copy = {
            .uImageWidth = app->width,
            .uImageHeight = app->height,
            .uImageDepth = 1,
            .uLayerCount = 1,
            .tCurrentImageUsage = PL_TEXTURE_USAGE_SAMPLED,
        };
        _ext_gfx->copy_texture_to_buffer(blit, _ext_planet->get_view_output_texture(app->view), app->readback_buffer, 1, &copy);
        _ext_gfx->end_blit_pass(blit);
        _ext_starter->submit_temporary_command_buffer(copy_cmd);
        _ext_gfx->flush_device(_ext_starter->get_device());

        plBuffer *buffer = _ext_gfx->get_buffer(_ext_starter->get_device(), app->readback_buffer);
        int stride = (int)app->width * 4;
        if (!buffer || !buffer->tMemoryAllocation.pHostMapped ||
            !stbi_write_png(app->output, (int)app->width, (int)app->height, 4, buffer->tMemoryAllocation.pHostMapped, stride)) {
            fprintf(stderr, "Error: failed to write snapshot: %s\n", app->output);
        } else {
            printf("Wrote snapshot: %s (%u frame%s, %u/%u chunks resident)\n",
                   app->output,
                   app->frame + 1,
                   app->frame == 0 ? "" : "s",
                   stream_stats.uResidentChunks,
                   stream_stats.uTotalChunks);
        }

        app->done = true;
        _ext_ioi->get_io()->bRunning = false;
    }

    app->frame++;
}

PL_EXPORT void pl_app_shutdown(AppData *app) {
    if (!app)
        return;

    plDevice *device = (app->starter_initialized && _ext_starter) ? _ext_starter->get_device() : NULL;
    if (device && _ext_gfx)
        _ext_gfx->flush_device(device);

    if (app->view && _ext_planet)
        _ext_planet->cleanup_view(app->view);
    if (app->planet && _ext_planet)
        _ext_planet->cleanup_planet(app->planet);
    if (app->readback_buffer.uData != 0 && device && _ext_gfx) {
        _ext_gfx->destroy_buffer(device, app->readback_buffer);
        app->readback_buffer = (plBufferHandle){0};
    }
    if (app->process_info.atTiles) {
        PL_FREE(app->process_info.atTiles);
        app->process_info.atTiles = NULL;
    }

    if (app->planet_initialized && _ext_planet)
        _ext_planet->cleanup();
    if (app->resource_initialized && _ext_resource)
        _ext_resource->cleanup();
    if (app->shader_initialized && _ext_shader)
        _ext_shader->cleanup();

    if (app->starter_initialized && _ext_starter)
        _ext_starter->cleanup();
    if (app->window && _ext_windows)
        _ext_windows->destroy(app->window);

    PL_FREE(app);
}

PL_EXPORT void pl_app_resize(plWindow *window, AppData *app) {
    (void)window;
    (void)app;
    if (_ext_starter)
        _ext_starter->resize();
}

static void _show_help(void) {
    printf("Usage: dcapp-planet-snapshot --planet-data FILE --crs geodetic|cartesian --output FILE [options]\n");
    printf("\n");
    printf("Required:\n");
    printf("  --planet-data FILE       Preprocessed .planet.json chunk metadata\n");
    printf("  --crs MODE               geodetic or cartesian\n");
    printf("  --output FILE            Output PNG path\n");
    printf("\n");
    printf("Geodetic camera:\n");
    printf("  --crs geodetic --attitude-frame local-ned\n");
    printf("  --lat N --lon N --elevation N [--yaw DEG --pitch DEG --roll DEG]\n");
    printf("\n");
    printf("Cartesian camera:\n");
    printf("  --crs cartesian --attitude-frame cartesian-rpy\n");
    printf("  --x N --y N --z N --roll DEG --pitch DEG --yaw DEG\n");
    printf("\n");
    printf("Options:\n");
    printf("  --attitude-frame MODE    local-ned or cartesian-rpy\n");
    printf("  --width N                Output width (default: 1024)\n");
    printf("  --height N               Output height (default: 1024)\n");
    printf("  --fov DEG                Vertical FOV (default: 60)\n");
    printf("  --vertex-shader FILE     Optional custom vertex shader\n");
    printf("  --fragment-shader FILE   Optional custom fragment shader\n");
    printf("  -h, --help               Show this help\n");
}

static bool _parse_args(int argc, char **argv, AppData *app) {
    // Defaults
    memset(app, 0, sizeof(*app));
    app->width = 1024;
    app->height = 1024;
    app->fov = 60.0f;

    // First pass: collect raw option strings.
    const char *planet_data = NULL;
    const char *output = NULL;
    const char *vertex_shader = NULL;
    const char *fragment_shader = NULL;
    const char *crs = NULL;
    const char *attitude_frame = NULL;
    const char *lat = NULL;
    const char *lon = NULL;
    const char *elevation = NULL;
    const char *x = NULL;
    const char *y = NULL;
    const char *z = NULL;
    const char *roll = NULL;
    const char *pitch = NULL;
    const char *yaw = NULL;
    const char *width = NULL;
    const char *height = NULL;
    const char *fov = NULL;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "--snapshot-help") == 0) {
            _show_help();
            return false;
        } else if (strcmp(argv[i], "--planet-data") == 0 && i + 1 < argc)
            planet_data = argv[++i];
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            output = argv[++i];
        else if (strcmp(argv[i], "--crs") == 0 && i + 1 < argc)
            crs = argv[++i];
        else if (strcmp(argv[i], "--attitude-frame") == 0 && i + 1 < argc)
            attitude_frame = argv[++i];
        else if (strcmp(argv[i], "--lat") == 0 && i + 1 < argc)
            lat = argv[++i];
        else if (strcmp(argv[i], "--lon") == 0 && i + 1 < argc)
            lon = argv[++i];
        else if (strcmp(argv[i], "--elevation") == 0 && i + 1 < argc)
            elevation = argv[++i];
        else if (strcmp(argv[i], "--x") == 0 && i + 1 < argc)
            x = argv[++i];
        else if (strcmp(argv[i], "--y") == 0 && i + 1 < argc)
            y = argv[++i];
        else if (strcmp(argv[i], "--z") == 0 && i + 1 < argc)
            z = argv[++i];
        else if (strcmp(argv[i], "--roll") == 0 && i + 1 < argc)
            roll = argv[++i];
        else if (strcmp(argv[i], "--pitch") == 0 && i + 1 < argc)
            pitch = argv[++i];
        else if (strcmp(argv[i], "--yaw") == 0 && i + 1 < argc)
            yaw = argv[++i];
        else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc)
            width = argv[++i];
        else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc)
            height = argv[++i];
        else if (strcmp(argv[i], "--fov") == 0 && i + 1 < argc)
            fov = argv[++i];
        else if (strcmp(argv[i], "--vertex-shader") == 0 && i + 1 < argc)
            vertex_shader = argv[++i];
        else if (strcmp(argv[i], "--fragment-shader") == 0 && i + 1 < argc)
            fragment_shader = argv[++i];
        else {
            fprintf(stderr, "Error: unknown or incomplete option: %s\n", argv[i]);
            return false;
        }
    }

    // Required global inputs and enum values.
    if (!planet_data || !output || !crs || !attitude_frame) {
        fprintf(stderr, "Error: --planet-data, --crs, --attitude-frame, and --output are required\n");
        _show_help();
        return false;
    }
    if (strcmp(crs, "geodetic") == 0)
        app->crs = SNAPSHOT_CRS_GEODETIC;
    else if (strcmp(crs, "cartesian") == 0)
        app->crs = SNAPSHOT_CRS_CARTESIAN;
    else {
        fprintf(stderr, "Error: --crs must be geodetic or cartesian\n");
        return false;
    }
    if (strcmp(attitude_frame, "local-ned") == 0)
        app->attitude_frame = SNAPSHOT_ATTITUDE_FRAME_LOCAL_NED;
    else if (strcmp(attitude_frame, "cartesian-rpy") == 0)
        app->attitude_frame = SNAPSHOT_ATTITUDE_FRAME_CARTESIAN_RPY;
    else {
        fprintf(stderr, "Error: --attitude-frame must be local-ned or cartesian-rpy\n");
        return false;
    }

    // CRS-specific required and forbidden parameters.
    if (app->crs == SNAPSHOT_CRS_GEODETIC) {
        if (app->attitude_frame != SNAPSHOT_ATTITUDE_FRAME_LOCAL_NED) {
            fprintf(stderr, "Error: geodetic CRS currently requires --attitude-frame local-ned\n");
            return false;
        }
        if (!lat || !lon || !elevation) {
            fprintf(stderr, "Error: geodetic CRS requires --lat, --lon, and --elevation\n");
            return false;
        }
        if (x || y || z) {
            fprintf(stderr, "Error: geodetic CRS cannot use cartesian position arguments\n");
            return false;
        }
    } else {
        if (app->attitude_frame != SNAPSHOT_ATTITUDE_FRAME_CARTESIAN_RPY) {
            fprintf(stderr, "Error: cartesian CRS currently requires --attitude-frame cartesian-rpy\n");
            return false;
        }
        if (!x || !y || !z || !roll || !pitch || !yaw) {
            fprintf(stderr, "Error: cartesian CRS requires --x, --y, --z, --roll, --pitch, and --yaw\n");
            return false;
        }
        if (lat || lon || elevation) {
            fprintf(stderr, "Error: cartesian CRS cannot use geodetic camera arguments\n");
            return false;
        }
    }

    // Validate numeric text before converting into app data.
    if ((lat && !dc_utils_string_is_double(lat)) ||
        (lon && !dc_utils_string_is_double(lon)) ||
        (elevation && !dc_utils_string_is_double(elevation)) ||
        (x && !dc_utils_string_is_double(x)) ||
        (y && !dc_utils_string_is_double(y)) ||
        (z && !dc_utils_string_is_double(z)) ||
        (roll && !dc_utils_string_is_double(roll)) ||
        (pitch && !dc_utils_string_is_double(pitch)) ||
        (yaw && !dc_utils_string_is_double(yaw)) ||
        (fov && !dc_utils_string_is_double(fov))) {
        fprintf(stderr, "Error: camera position, attitude, and FOV values must be numeric\n");
        return false;
    }
    if ((width && !dc_utils_string_is_int(width)) || (height && !dc_utils_string_is_int(height))) {
        fprintf(stderr, "Error: --width and --height must be integer values\n");
        return false;
    }

    // Commit validated parameters to the app.
    app->planet_data = planet_data;
    app->output = output;
    app->vertex_shader = vertex_shader;
    app->fragment_shader = fragment_shader;
    if (lat) app->lat = dc_utils_string_to_double(lat);
    if (lon) app->lon = dc_utils_string_to_double(lon);
    if (elevation) app->elevation = dc_utils_string_to_double(elevation);
    if (x) app->x = dc_utils_string_to_double(x);
    if (y) app->y = dc_utils_string_to_double(y);
    if (z) app->z = dc_utils_string_to_double(z);
    if (roll) app->roll = (float)dc_utils_string_to_double(roll);
    if (pitch) app->pitch = (float)dc_utils_string_to_double(pitch);
    if (yaw) app->yaw = (float)dc_utils_string_to_double(yaw);
    if (width) {
        int value = dc_utils_string_to_integer(width);
        if (value < 0) {
            fprintf(stderr, "Error: --width must be non-negative\n");
            return false;
        }
        app->width = (uint32_t)value;
    }
    if (height) {
        int value = dc_utils_string_to_integer(height);
        if (value < 0) {
            fprintf(stderr, "Error: --height must be non-negative\n");
            return false;
        }
        app->height = (uint32_t)value;
    }
    if (fov) app->fov = (float)dc_utils_string_to_double(fov);

    // Output image sanity checks.
    if (app->width == 0 || app->height == 0) {
        fprintf(stderr, "Error: --width and --height must be greater than zero\n");
        return false;
    }
    if (app->fov <= 0.0f || app->fov >= 180.0f) {
        fprintf(stderr, "Error: --fov must be between 0 and 180 degrees\n");
        return false;
    }

    return true;
}

static bool _load_planet_data(AppData *app) {
    char *json_text = dc_utils_load_text_file(app->planet_data);
    if (!json_text) {
        fprintf(stderr, "Error: failed to load planet data: %s\n", app->planet_data);
        return false;
    }

    plJsonObject *json_root = NULL;
    if (!pl_load_json(json_text, &json_root)) {
        fprintf(stderr, "Error: failed to parse planet data: %s\n", app->planet_data);
        free(json_text);
        return false;
    }

    plJsonObject *root = json_root;
    double radius = pl_json_double_member(root, "radius", 0.0);
    float meters_per_pixel = pl_json_float_member(root, "meters_per_pixel", 0.0f);
    int tile_size = pl_json_int_member(root, "tile_size", 0);
    int cols = pl_json_int_member(root, "cols", 0);
    int rows = pl_json_int_member(root, "rows", 0);
    float min_height = pl_json_float_member(root, "min_height", 0.0f);
    float max_height = pl_json_float_member(root, "max_height", 0.0f);
    int tree_depth = pl_json_int_member(root, "tree_depth", 0);
    float max_base_error = pl_json_float_member(root, "max_base_error", 0.0f);

    uint32_t tile_count = 0;
    plJsonObject *tile_array = pl_json_array_member(root, "tiles", &tile_count);
    if (radius <= 0.0 || meters_per_pixel <= 0.0f || tile_size <= 0 || cols <= 0 || rows <= 0 || !tile_array || tile_count == 0) {
        fprintf(stderr, "Error: invalid planet data metadata: %s\n", app->planet_data);
        pl_unload_json(&json_root);
        free(json_text);
        return false;
    }
    uint64_t expected_tile_count = (uint64_t)cols * (uint64_t)rows;
    if (expected_tile_count > UINT32_MAX || tile_count != (uint32_t)expected_tile_count) {
        fprintf(stderr, "Error: planet tile count mismatch: cols=%d rows=%d tiles=%u\n", cols, rows, tile_count);
        pl_unload_json(&json_root);
        free(json_text);
        return false;
    }

    // Fill the extension's process-info block from the chunk metadata.
    plPlanetProcessInfo *info = &app->process_info;
    memset(info, 0, sizeof(*info));
    info->tProjection.tType = PL_PROJECTION_POLAR_STEREOGRAPHIC;
    info->tProjection.tPolarStereo.dLatitudeOfOrigin = -90.0;
    info->tProjection.tPolarStereo.dLongitudeOfOrigin = 0.0;
    info->tProjection.tPolarStereo.dScaleFactor = 1.0;
    info->tProjection.tPolarStereo.dFalseEasting = 0.0;
    info->tProjection.tPolarStereo.dFalseNorthing = 0.0;
    plJsonObject *projection_obj = pl_json_member(root, "projection");
    // Pre-projection .planet.json files omitted this block and stored tile centers
    // as legacy lat/lon values instead of explicit projected CRS meters.
    bool legacy_projected_origin = projection_obj == NULL;
    if (projection_obj) {
        char projection_type[64] = {0};
        pl_json_string_member(projection_obj, "type", projection_type, sizeof(projection_type));
        if (projection_type[0] && strcmp(projection_type, "polar_stereographic") != 0) {
            fprintf(stderr, "Error: unsupported planet projection '%s'\n", projection_type);
            pl_unload_json(&json_root);
            free(json_text);
            return false;
        }
        info->tProjection.tPolarStereo.dLatitudeOfOrigin =
            pl_json_double_member(projection_obj, "latitude_of_origin", info->tProjection.tPolarStereo.dLatitudeOfOrigin);
        info->tProjection.tPolarStereo.dLongitudeOfOrigin =
            pl_json_double_member(projection_obj, "longitude_of_origin", info->tProjection.tPolarStereo.dLongitudeOfOrigin);
        info->tProjection.tPolarStereo.dScaleFactor =
            pl_json_double_member(projection_obj, "scale_factor", info->tProjection.tPolarStereo.dScaleFactor);
        info->tProjection.tPolarStereo.dFalseEasting =
            pl_json_double_member(projection_obj, "false_easting", info->tProjection.tPolarStereo.dFalseEasting);
        info->tProjection.tPolarStereo.dFalseNorthing =
            pl_json_double_member(projection_obj, "false_northing", info->tProjection.tPolarStereo.dFalseNorthing);
    }
    if (fabs(info->tProjection.tPolarStereo.dLatitudeOfOrigin) < 45.0 ||
        info->tProjection.tPolarStereo.dScaleFactor <= 0.0) {
        fprintf(stderr, "Error: invalid polar stereographic projection in planet data\n");
        pl_unload_json(&json_root);
        free(json_text);
        return false;
    }
    info->tGeodeticModel.tDatum = PL_DATUM_SPHERE;
    info->tGeodeticModel.sphere.dRadius = radius;
    info->dMetersPerPixel = meters_per_pixel;
    info->uSize = (uint32_t)tile_size;
    info->uTileCount = tile_count;
    info->uHorizontalTiles = (uint32_t)cols;
    info->uVerticalTiles = (uint32_t)rows;
    info->atTiles = (plPlanetProcessTileInfo *)PL_ALLOC(tile_count * sizeof(plPlanetProcessTileInfo));
    if (!info->atTiles) {
        pl_unload_json(&json_root);
        free(json_text);
        return false;
    }

    // Resolve per-tile origins and chunk files.
    char json_dir[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_get_directory(app->planet_data, json_dir, sizeof(json_dir));

    for (uint32_t i = 0; i < tile_count; i++) {
        plJsonObject *tile_obj = pl_json_member_by_index(tile_array, i);
        plPlanetProcessTileInfo *tile = &info->atTiles[i];
        memset(tile, 0, sizeof(*tile));

        // resolve tile origin
        if (pl_json_member_exist(tile_obj, "originX") && pl_json_member_exist(tile_obj, "originY")) {
            tile->dOriginX = pl_json_double_member(tile_obj, "originX", 0.0);
            tile->dOriginY = pl_json_double_member(tile_obj, "originY", 0.0);
        } else if (pl_json_member_exist(tile_obj, "lat") && pl_json_member_exist(tile_obj, "lon")) {
            double lat = pl_json_double_member(tile_obj, "lat", 0.0);
            double lon = pl_json_double_member(tile_obj, "lon", 0.0);
            DcGeoCrsGeodetic geodetic_crs = dc_geo_create_crs_geodetic(radius);
            DcGeoCrsPolarStereo polar_crs = dc_geo_create_crs_polar_stereographic(
                radius,
                info->tProjection.tPolarStereo.dLatitudeOfOrigin,
                info->tProjection.tPolarStereo.dLongitudeOfOrigin);
            polar_crs.scale_factor = info->tProjection.tPolarStereo.dScaleFactor;
            polar_crs.false_easting = info->tProjection.tPolarStereo.dFalseEasting;
            polar_crs.false_northing = info->tProjection.tPolarStereo.dFalseNorthing;
            plVec3 geodetic_in = {(float)lat, (float)lon, 0.0f};
            plVec2 polar_out;
            if (legacy_projected_origin) {
                // Compatibility path for old lat/lon tile metadata. New metadata
                // should provide originX/originY directly in projected CRS meters.
                dc_geo_user_geodetic_to_polar_stereo(&geodetic_crs, &polar_crs, &geodetic_in, &polar_out, 1);
                polar_out.y = -polar_out.y;
            } else {
                dc_geo_geodetic_to_polar_stereo(&geodetic_crs, &polar_crs, &geodetic_in, &polar_out, 1);
            }
            tile->dOriginX = (double)polar_out.x;
            tile->dOriginY = (double)polar_out.y;
        }

        tile->dMaxBaseError = max_base_error;
        tile->dMaxHeight = max_height;
        tile->dMinHeight = min_height;
        tile->iTreeDepth = tree_depth;

        // resolve chunk file path
        char chunk_file[256] = {0};
        pl_json_string_member(tile_obj, "file", chunk_file, sizeof(chunk_file));
        if (!chunk_file[0]) {
            fprintf(stderr, "Error: tile %u missing chunk file\n", i);
            PL_FREE(info->atTiles);
            info->atTiles = NULL;
            pl_unload_json(&json_root);
            free(json_text);
            return false;
        }

        char abs_path[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
        if (dc_utils_is_relative_path(chunk_file)) {
            if (dc_utils_join_paths(json_dir, chunk_file, abs_path, sizeof(abs_path)) != 0) {
                PL_FREE(info->atTiles);
                info->atTiles = NULL;
                pl_unload_json(&json_root);
                free(json_text);
                return false;
            }
        } else {
            strncpy(abs_path, chunk_file, sizeof(abs_path) - 1);
        }
        strncpy(tile->acOutputFile, abs_path, sizeof(tile->acOutputFile) - 1);
    }

    pl_unload_json(&json_root);
    free(json_text);
    return true;
}
