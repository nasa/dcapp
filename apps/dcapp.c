// PL includes
#define PL_EXPERIMENTAL
#include "../pilotlight/src/pl.h"
#define PL_MATH_INCLUDE_FUNCTIONS
#include "../pilotlight/libs/pl_math.h"

// PL extension includes
#include "../pilotlight/extensions/pl_camera_ext.h"
#include "../pilotlight/extensions/pl_draw_ext.h"
#include "../pilotlight/extensions/pl_profile_ext.h"
#include "../pilotlight/extensions/pl_starter_ext.h"
#include "../pilotlight/extensions/pl_graphics_ext.h"
#include "../pilotlight/extensions/pl_vfs_ext.h"
#include "../pilotlight/extensions/pl_shader_ext.h"
#include "../pilotlight/extensions/pl_draw_backend_ext.h"
#include "../extensions/pl_terrain_ext.h"

// general includes
#include <ctype.h>
#include <string.h>

// PL macros
#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_REALLOC(x, y) _ext_memory->tracked_realloc((x), (y), __FILE__, __LINE__)
#define PL_FREE(x) _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

// PL extensions
const plWindowI      *_ext_windows      = NULL;
const plDrawI        *_ext_draw         = NULL;
const plDrawBackendI *_ext_draw_backend = NULL;
const plStarterI     *_ext_starter      = NULL;
const plProfileI     *_ext_profile      = NULL;
const plMemoryI      *_ext_memory       = NULL;
const plLibraryI     *_ext_library      = NULL;
const plIOI          *_ext_ioi          = NULL;
const plGraphicsI    *_ext_gfx          = NULL;
const plTerrainI     *_ext_terrain      = NULL;
const plVfsI         *_ext_vfs          = NULL;
const plShaderI      *_ext_shader       = NULL;
const plCameraI      *_ext_camera       = NULL;

// PL app data
typedef struct __PlAppData {

    plWindow      *window;
    plDrawLayer2D *layer;
    plDrawList2D  *draw_list;
    plFont        *cousine_sdf_font;

} _PlAppData;

// PL functions
PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, _PlAppData *pl_app_data);
PL_EXPORT void  pl_app_shutdown(_PlAppData *pl_app_data);
PL_EXPORT void  pl_app_resize(_PlAppData *pl_app_data);
PL_EXPORT void  pl_app_update(_PlAppData *pl_app_data);

// dcapp includes
#include "../src/utils/env.h"
#include "../src/utils/stb_sb.h"
#include "../src/utils/file.h"
#include "../src/utils/string.h"
#include "../src/app/elem.h"
#include "../src/app/enums.h"
#include "../src/app/lookup.h"
#include "../src/app/config.h"
#include "../src/trick.h"
#include <libxml/parser.h>

// dcapp node structs
typedef struct __ValIndex2 {
    union {
        DcAppValIndex x, r, lat, roll;
    };
    union {
        DcAppValIndex y, g, lon, pitch;
    };
} _ValIndex2;

typedef struct __ValIndex3 {
    union {
        DcAppValIndex x, r, lat, roll;
    };
    union {
        DcAppValIndex y, g, lon, pitch;
    };
    union {
        DcAppValIndex z, b, ele, yaw;
    };
} _ValIndex3;

typedef struct __ValIndex4 {
    union {
        DcAppValIndex x, r;
    };
    union {
        DcAppValIndex y, g;
    };
    union {
        DcAppValIndex z, b;
    };
    union {
        DcAppValIndex w, a;
    };
} _ValIndex4;

typedef enum __NodeType {
    NODE_TYPE_UNDEFINED,
    NODE_TYPE_CONTAINER,
    NODE_TYPE_CONDITIONAL,
    NODE_TYPE_PANEL,
    NODE_TYPE_POLYGON,
    NODE_TYPE_SET,
    NODE_TYPE_TERRAIN,
    NODE_TYPE_TEXT,
    NODE_TYPE_WINDOW,
    NODE_TYPE__COUNT,
    NODE_TYPE__MAX = NODE_TYPE__COUNT - 1,
} _NodeType;

typedef uint32_t _NodeIndex;
const _NodeIndex NODE_INDEX_UNDEFINED = -1;

typedef struct __NodeConditional {
    DcAppValIndex type;
    DcAppValIndex value1;
    DcAppValIndex value2;
    _NodeIndex    child_true;
    _NodeIndex    child_false;
} _NodeConditional;

typedef struct __NodeContainer {
    _ValIndex2    position;
    _ValIndex2    dimension;
    _ValIndex2    virtual_dimension;
    _ValIndex2    pivot_local_align;
    _ValIndex2    pivot_position;
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    DcAppValIndex rotation;
    _NodeIndex    child;
} _NodeContainer;

typedef struct __NodeMap {
    _ValIndex2    position;
    _ValIndex2    origin;
    _ValIndex2    dimension;
    _ValIndex2    virtual_dimension;
    _ValIndex2    pivot_align;
    _ValIndex2    pivot_position;
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    _ValIndex3    lle;
    DcAppValIndex yaw;
    DcAppValIndex rotation;
    // DcDemManager *demManager;
} _NodeMap;

typedef struct __NodePanel {
    _ValIndex2    parent_dimension;
    _ValIndex2    virtual_dimension;
    DcAppValIndex index;
    _NodeIndex    child;
} _NodePanel;

typedef struct __NodePolygon {
    _ValIndex4    fill_color;
    _ValIndex4    line_color;
    DcAppValIndex line_width;

    _ValIndex2 *sb_points;
    bool        fill_enabled;
    bool        line_enabled;

    _ValIndex2    origin;
    _ValIndex2    pivot_position;
    DcAppValIndex rotation;
} _NodePolygon;

typedef struct __NodeSet {
    DcAppVarIndex var_index;
    DcAppValIndex operation; // because operator was taken :(
    DcAppValIndex operand;
} _NodeSet;

typedef struct __NodeText {
    _ValIndex2    position;
    _ValIndex2    origin;
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    _ValIndex2    pivot_align;
    _ValIndex2    pivot_position;
    DcAppValIndex rotation;
    DcAppValIndex size;
    _ValIndex4    fill_color;
    _ValIndex4    line_color;
    bool          fill_enabled;
    bool          line_enabled;
    // DcAppValIndex  font;

    // stretchy buffers contains values and formats
    // TODO move this to a context
    DcAppValIndex *sb_vals;
    char          *sb_fillers;
    uint8_t       *sb_filler_indices;
    char          *sb_formats;
    uint8_t       *sb_format_indices;
    DcValueType   *sb_format_types;
} _NodeText;

typedef struct __NodeTerrain {

    // general positioning of display
    _ValIndex2    dimension;
    _ValIndex2    position;
    _ValIndex2    origin;
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    _ValIndex2    pivot_align;
    _ValIndex2    pivot_position;
    DcAppValIndex rotation;

    // terrain specific
    _ValIndex3 lle;
    _ValIndex3 rpy;
    uint8_t    terrain_index;

} _NodeTerrain;

typedef struct __NodeWindow {
    _ValIndex2 position;
    _ValIndex2 dimension;
    _ValIndex2 virtual_dimension;
    _NodeIndex child;
    char      *title;
} _NodeWindow;

typedef struct __Node {
    _NodeType  type;
    _NodeIndex parent;
    _NodeIndex next;
    union {
        _NodeConditional conditional;
        _NodeContainer   container;
        _NodeMap         map;
        _NodePanel       panel;
        _NodePolygon     polygon;
        _NodeSet         set;
        _NodeTerrain     terrain;
        _NodeText        text;
        _NodeWindow      window;
    };
} _Node;

// dcapp trick structs
typedef struct __TrickTxVarContext {
    DcAppVarIndex   dcapp_var_index;
    DcTrickVarIndex trick_var_index;
    DcValue         prev_value;
} _TrickTxVarContext;

typedef struct __TrickRxVarContext {
    DcAppVarIndex   dcapp_var_index;
    DcTrickVarIndex trick_var_index;
} _TrickRxVarContext;

typedef struct __TrickContext {
    DcTrick            *trick;
    _TrickTxVarContext *sb_tx_var_contexts;
    _TrickRxVarContext *sb_rx_var_contexts;
} _TrickContext;

// dcapp app data
typedef struct __DcAppData {

    // config + lookup
    DcAppLookup *lookup;
    DcAppConfig *config;

    // nodes
    _Node     *sb_nodes;
    _NodeIndex window;

    // logic
    plSharedLibrary *logic_lib;
    void (*logic_pre_init)();
    void (*logic_init)();
    void (*logic_draw)();
    void (*logic_close)();

    // trick
    _TrickContext *sb_tricks;

} _DcAppData;
static _DcAppData data;

// styles (index 0 reserved for defaults)
char *sb_node_style_names;
int  *sb_node_style_name_offsets;
static _Node (*_sb_node_styles)[NODE_TYPE__COUNT];

// node utils
static const char *_node_type_to_string(_NodeType type);
static _Node      *_index_to_node(_NodeIndex index);
static _NodeIndex  _register_node(_Node *node);

// draw utils
static bool       _load_color_from_string(xmlNodePtr xml_node, const char *attr_name, _ValIndex4 *color_out);
static _NodeIndex _process_node_children(xmlNodePtr xml_node, _NodeIndex node_index, DcAppElemType elem_type, _ValIndex2 dimensions, const char *directory);
static _NodeIndex _process_node(xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, _ValIndex2 parent_dimensions, const char *directory);
static void       _draw_node_list(_PlAppData *pl_app_data, _NodeIndex node_index, plVec2 *parent_dimensions, plMat4 *node_transform);
static void       _draw_node(_PlAppData *pl_app_data, _NodeIndex node_index, plVec2 *parent_dimensions, plMat4 *parent_transform);

PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, _PlAppData *pl_app_data) {

    if (pl_app_data) {
        _ext_windows      = pl_get_api_latest(api_registry, plWindowI);
        _ext_draw         = pl_get_api_latest(api_registry, plDrawI);
        _ext_draw_backend = pl_get_api_latest(api_registry, plDrawBackendI);
        _ext_starter      = pl_get_api_latest(api_registry, plStarterI);
        _ext_profile      = pl_get_api_latest(api_registry, plProfileI);
        _ext_memory       = pl_get_api_latest(api_registry, plMemoryI);
        _ext_library      = pl_get_api_latest(api_registry, plLibraryI);
        _ext_ioi          = pl_get_api_latest(api_registry, plIOI);
        _ext_gfx          = pl_get_api_latest(api_registry, plGraphicsI);
        _ext_vfs          = pl_get_api_latest(api_registry, plVfsI);
        _ext_shader       = pl_get_api_latest(api_registry, plShaderI);
        _ext_terrain      = pl_get_api_latest(api_registry, plTerrainI);
        _ext_camera       = pl_get_api_latest(api_registry, plCameraI);
        return pl_app_data;
    }

    // retrieve extension registry
    const plExtensionRegistryI *extension_registry = pl_get_api_latest(api_registry, plExtensionRegistryI);

    // load required extensions
    extension_registry->load("pl_unity_ext", NULL, NULL, true);
    extension_registry->load("pl_platform_ext", NULL, NULL, false);
    extension_registry->load("pl_terrain_ext", NULL, NULL, true);

    // load extensions
    _ext_windows      = pl_get_api_latest(api_registry, plWindowI);
    _ext_draw         = pl_get_api_latest(api_registry, plDrawI);
    _ext_draw_backend = pl_get_api_latest(api_registry, plDrawBackendI);
    _ext_starter      = pl_get_api_latest(api_registry, plStarterI);
    _ext_profile      = pl_get_api_latest(api_registry, plProfileI);
    _ext_memory       = pl_get_api_latest(api_registry, plMemoryI);
    _ext_library      = pl_get_api_latest(api_registry, plLibraryI);
    _ext_ioi          = pl_get_api_latest(api_registry, plIOI);
    _ext_gfx          = pl_get_api_latest(api_registry, plGraphicsI);
    _ext_vfs          = pl_get_api_latest(api_registry, plVfsI);
    _ext_shader       = pl_get_api_latest(api_registry, plShaderI);
    _ext_terrain      = pl_get_api_latest(api_registry, plTerrainI);
    _ext_camera       = pl_get_api_latest(api_registry, plCameraI);

    // allocate app memory
    pl_app_data = (_PlAppData *)PL_ALLOC(sizeof(_PlAppData));
    memset(pl_app_data, 0, sizeof(_PlAppData));

    // parse input arguments
    plIO *_ext_io = _ext_ioi->get_io();
    if (_ext_io->iArgc < 4) {
        fprintf(stderr, "DCApp pl_app_load(): missing dcapp config file\n");
    }

    // TODO process input arguments (constant setting)
    // std::vector<std::string> args(_ext_io->apArgv + 3, _ext_io->apArgv + _ext_io->iArgc);

    // create config
    const char *config_filepath = _ext_io->apArgv[3];
    data.config                 = dc_app_config_create(config_filepath);

    // create lookup
    data.lookup = dc_app_lookup_create();

    // set environment (used for dcapp XMLs)
    dc_utils_set_env("dcappDisplayHome", data.config->config_dir_path, 1);

    // clean XML file
    dc_app_config_clean_xml(data.config, data.lookup);

    // save to file
    char log_file[256];
    dc_utils_join_paths(data.config->cache_dir_path, "xml.log", log_file, sizeof(log_file));
    printf("%s\n", log_file);
    dc_app_config_save_to_file(data.config, log_file);

    // build dcapp node tree
    xmlNodePtr root_node = xmlDocGetRootElement(data.config->xml_doc);
    _process_node(root_node, NODE_INDEX_UNDEFINED, DC_APP_ELEM_TYPE_UNDEFINED, (_ValIndex2){0, 0}, data.config->config_dir_path);

    // mount VFS dirs
    _ext_vfs->mount_directory("/shaders-terrain", "../../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/assets", "../../data", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/cache", "cache", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/shaders", "../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/shader-temp", "../shader-temp", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/tiles", "../../data", PL_VFS_MOUNT_FLAGS_NONE);

    // set initial window params
    _Node       *window_node = _index_to_node(data.window);
    plWindowDesc window_desc = {};
    window_desc.pcTitle      = window_node->window.title;
    window_desc.uWidth       = (uint32_t)(dc_app_lookup_get_value(data.lookup, window_node->window.dimension.x)->value_integer);
    window_desc.uHeight      = (uint32_t)(dc_app_lookup_get_value(data.lookup, window_node->window.dimension.y)->value_integer);
    window_desc.iXPos        = dc_app_lookup_get_value(data.lookup, window_node->window.position.x)->value_integer;
    window_desc.iYPos        = dc_app_lookup_get_value(data.lookup, window_node->window.position.y)->value_integer;
    _ext_windows->create(window_desc, &(pl_app_data->window));
    _ext_windows->show(pl_app_data->window);

    // initialize the starter API (handles alot of boilerplate)
    plStarterInit tStarterInit = {
        .tFlags   = PL_STARTER_FLAGS_ALL_EXTENSIONS & (~PL_STARTER_FLAGS_DRAW_EXT) & (~PL_STARTER_FLAGS_SHADER_EXT) | PL_STARTER_FLAGS_MSAA,
        .ptWindow = pl_app_data->window};
    _ext_starter->initialize(tStarterInit);

    // init draw extension
    _ext_draw->initialize(NULL);

    // init draw backend
    plDevice *device = _ext_starter->get_device();
    _ext_draw_backend->initialize(device);

    // create font atlas
    plFontAtlas *pt_atlas = _ext_draw->create_font_atlas();
    _ext_draw->set_font_atlas(pt_atlas);

    // typical font range (you can also add individual characters)
    const plFontRange font_range = {
        .iFirstCodePoint = 0x0020,
        .uCharCount      = 0x00FF - 0x0020};

    // adding previous font but as a signed distance field (SDF)
    plFontConfig font_config   = {};
    font_config.bSdf           = true; // only works with ttf
    font_config.fSize          = 25.0f;
    font_config.uHOverSampling = 1;
    font_config.uVOverSampling = 1;
    font_config.ucOnEdgeValue  = 180;
    font_config.iSdfPadding    = 1;
    font_config.uRangeCount    = 1;
    font_config.ptRanges       = &font_range;

    pl_app_data->cousine_sdf_font = _ext_draw->add_font_from_file_ttf(_ext_draw->get_current_font_atlas(), font_config, "../data/pilotlight-assets-master/fonts/Cousine-Regular.ttf");

    // register our app drawlist
    pl_app_data->draw_list = _ext_draw->request_2d_drawlist();

    // request layers (allows drawing out of order)
    pl_app_data->layer = _ext_draw->request_2d_layer(pl_app_data->draw_list);

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

    // init terrain backend
    // _ext_starter->get_device();
    // _ext_terrain->initialize(_ext_starter->get_device());
    // plCommandBuffer *temp_cmd_buffer = _ext_starter->get_temporary_command_buffer();
    // _ext_terrain->load_mesh(temp_cmd_buffer, "/assets/terrain.bin", 7, 128);

    // plTerrainInit tTerrainInit = {};
    // pl_app_data->terrain          = _ext_terrain->create_terrain_from_file(temp_cmd_buffer, "/assets/moon_terrain.json");
    // _ext_starter->submit_temporary_command_buffer(temp_cmd_buffer);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~font atlas texture~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // draw backend handles creating the font atlas texture and
    // uploading to the GPU but it requires a command buffer (in an non recording state).
    // Later examples will go into command buffers without using the starter ext

    plCommandBuffer *raw_cmd_buffer = _ext_starter->get_raw_command_buffer(); // not recording

    // actually record, submit, & wait
    _ext_draw_backend->build_font_atlas(raw_cmd_buffer, _ext_draw->get_current_font_atlas());

    // return back to the pool
    _ext_starter->return_raw_command_buffer(raw_cmd_buffer);

    // return app memory
    return pl_app_data;
}

PL_EXPORT void pl_app_shutdown(_PlAppData *pl_app_data) {

    // ensure device is done with resources
    plDevice *device = _ext_starter->get_device();
    _ext_gfx->flush_device(device); // waits for the GPU to be done with all work

    // cleans up texture and other resources
    _ext_draw_backend->cleanup_font_atlas(_ext_draw->get_current_font_atlas());

    _ext_starter->cleanup();
    _ext_windows->destroy(pl_app_data->window);
    PL_FREE(pl_app_data);
}

PL_EXPORT void pl_app_resize(_PlAppData *pl_app_data) {
    _ext_starter->resize();
}

static long long _frame_count;
PL_EXPORT void   pl_app_update(_PlAppData *pl_app_data) {
    // this needs to be the first call when using the starter
    // extension. You must return if it returns false (usually a swapchain recreation).
    if (!_ext_starter->begin_frame()) {
        return;
    }

    _frame_count++;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~drawing & profile API~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _ext_draw_backend->new_frame();

    // send trick data
    for (int ii = 0; ii < sbcount(data.sb_tricks); ii++) {

        _TrickContext *trick_context = &(data.sb_tricks[ii]);
        DcTrick       *trick         = trick_context->trick;

        // add tx commands to buffer
        if (trick->is_connected) {
            for (int jj = 0; jj < sbcount(trick_context->sb_tx_var_contexts); jj++) {

                _TrickTxVarContext *tx_var_context = &(trick_context->sb_tx_var_contexts[jj]);
                DcAppLookupVar     *dc_app_var     = dc_app_lookup_get_var(data.lookup, tx_var_context->dcapp_var_index);
                DcValue            *curr_value     = dc_app_lookup_get_value(data.lookup, dc_app_var->value_index);
                DcValue            *prev_value     = &tx_var_context->prev_value;

                // send if new value is different
                if (!dc_value_is_equal(curr_value, prev_value)) {
                    dc_trick_set_tx_var(trick, tx_var_context->trick_var_index, curr_value->value_string);
                    dc_value_copy(prev_value, curr_value);
                }
            }
        }

        // send the updated buffer, receive the new data, update the connection status
        dc_trick_update(trick);

        // receive the new data
        if (trick->has_new_data && trick->is_connected) {
            char rx_buffer[256];
            for (int jj = 0; jj < sbcount(trick_context->sb_rx_var_contexts); jj++) {

                _TrickRxVarContext *rx_var_context = &(trick_context->sb_rx_var_contexts[jj]);
                DcAppLookupVar     *dc_app_var     = dc_app_lookup_get_var(data.lookup, rx_var_context->dcapp_var_index);
                DcValue            *value          = dc_app_lookup_get_value(data.lookup, dc_app_var->value_index);

                dc_trick_get_rx_var_value(trick, rx_var_context->trick_var_index, rx_buffer);
                dc_app_lookup_set_var_to_string(data.lookup, rx_var_context->dcapp_var_index, rx_buffer);
            }
        }
    }

    // process logic, update vars from extern_data
    if (data.logic_draw) {
        data.logic_draw();
        for (int var_index = 0; var_index < dc_app_lookup_get_var_count(data.lookup); var_index++) {
            dc_app_lookup_refresh_var_from_extern(data.lookup, var_index);
        }
    }
    // double lat   = dc_utils_degrees_to_radians(dc_app_lookup_get_value(data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["LATITUDE_deg"]])->value_index)->value_double);
    // double lon   = dc_utils_degrees_to_radians(dc_app_lookup_get_value(data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["LONGITUDE_deg"]])->value_index)->value_double);
    // double alt   = dc_app_lookup_get_value(data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["ALTITUDE_m"]])->value_index)->value_double;
    // double roll  = dc_utils_degrees_to_radians(dc_app_lookup_get_value(data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["ROLL_deg"]])->value_index)->value_double);
    // double pitch = dc_utils_degrees_to_radians(dc_app_lookup_get_value(data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["PITCH_deg"]])->value_index)->value_double);
    // double yaw   = dc_utils_degrees_to_radians(dc_app_lookup_get_value(data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["YAW_deg"]])->value_index)->value_double);

    // // calculate x, y, z
    // //  r = 2 * R * tan(pi/4 - lat/2)
    // //  theta = lon
    // //  x = r * sin(theta)
    // //  y = altitude
    // //  z = r * cos(theta)
    // double r = 2 * 1737400 * tan(M_PI_4 - fabs(lat) / 2);
    // double x = r * sin(lon);
    // double y = alt;
    // double z = r * cos(lon);

    // // set rotation
    // pitch = 270.0;
    // roll = 0.0;
    // yaw = 0.0;

    // // process camera
    // // ext_terrain->set_camera_pos(app_data->terrain, x, y, z);
    // ext_terrain->set_camera_orientation(app_data->terrain, pitch, yaw, roll);

    // start terrain rendering
    // plCommandBuffer *cmd_buffer = ext_starter->get_command_buffer();
    // ext_terrain->render(app_data->terrain, cmd_buffer);
    // ext_starter->submit_command_buffer(cmd_buffer);
    // ext_draw->add_image(app_data->layer, ext_terrain->get_terrain_texture(app_data->terrain).uIndex, {0.0f, 0.0f}, {1000.0f, 1000.0f});

    // draw node
    _draw_node(pl_app_data, data.window, NULL, NULL);

    // submit draw layer
    _ext_draw->submit_2d_layer(pl_app_data->layer);

    // start main pass & return the encoder being used
    plRenderEncoder *encoder = _ext_starter->begin_main_pass();

    // submit our 2d drawlist
    plIO *ptIO = _ext_ioi->get_io();
    _ext_draw_backend->submit_2d_drawlist(pl_app_data->draw_list, encoder, ptIO->tMainViewportSize.x, ptIO->tMainViewportSize.y, _ext_gfx->get_swapchain_info(_ext_starter->get_swapchain()).tSampleCount);

    _ext_starter->end_main_pass();

    // must be the last function called when using the starter extension
    _ext_starter->end_frame();
}

static const char *_node_type_to_string(_NodeType type) {
    switch (type) {
        case NODE_TYPE_CONTAINER:
            return "Container";
        case NODE_TYPE_CONDITIONAL:
            return "Conditional";
        case NODE_TYPE_PANEL:
            return "Panel";
        case NODE_TYPE_POLYGON:
            return "Polygon";
        case NODE_TYPE_SET:
            return "Set";
        case NODE_TYPE_TERRAIN:
            return "Terrain";
        case NODE_TYPE_TEXT:
            return "Text";
        case NODE_TYPE_WINDOW:
            return "Window";
        default:
            fprintf(stderr, "DCAPP _node_type_to_string(): Unknown node type %d\n", type);
            return "";
    }
}

static _Node *_index_to_node(_NodeIndex index) {
    if (index == NODE_INDEX_UNDEFINED) {
        return NULL;
    }
    return &(data.sb_nodes[index]);
}

static _NodeIndex _register_node(_Node *node) {
    sbpush(data.sb_nodes, *node);
    return sbcount(data.sb_nodes) - 1;
}

static bool _load_color_from_string(xmlNodePtr xml_node, const char *attr_name, _ValIndex4 *color_out) {

    xmlChar *raw_color = xmlGetProp(xml_node, BAD_CAST attr_name);
    if (raw_color) {

        // clean raw string
        char cleaned_color[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned_color, (const char *)(const char *)raw_color, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_color);

        // split by whitespace
        const size_t index_buffer_max = 20;
        size_t       index_buffer[index_buffer_max];
        size_t       index_count;
        dc_utils_split_string_inplace(cleaned_color, dc_utils_whitespace, index_buffer, index_buffer_max, &index_count);

        // if empty, assume no color
        if (index_count == 0) {
            return false;
        }

        // process each color
        if (index_count > 0) {
            color_out->r = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[0]]));
        } else {
            DcValue value = dc_value_create_value_double(0.0);
            color_out->r  = dc_app_lookup_register_value(data.lookup, &value);
        }
        if (index_count > 1) {
            color_out->g = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[1]]));
        } else {
            DcValue value = dc_value_create_value_double(0.0);
            color_out->g  = dc_app_lookup_register_value(data.lookup, &value);
        }
        if (index_count > 2) {
            color_out->b = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[2]]));
        } else {
            DcValue value = dc_value_create_value_double(0.0);
            color_out->b  = dc_app_lookup_register_value(data.lookup, &value);
        }
        if (index_count > 3) {
            color_out->a = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[3]]));
        } else {
            DcValue value = dc_value_create_value_double(1.0);
            color_out->a  = dc_app_lookup_register_value(data.lookup, &value);
        }

        return true;
    } else {
        return false;
    }
}

// returns the first child (if any)
static _NodeIndex _process_node_children(xmlNodePtr xml_node, _NodeIndex node_index, DcAppElemType elem_type, _ValIndex2 dimensions, const char *directory) {
    xmlNodePtr xml_child_node = xml_node->children;

    _NodeIndex first_child_index         = NODE_INDEX_UNDEFINED;
    _NodeIndex previous_child_node_index = NODE_INDEX_UNDEFINED;
    while (xml_child_node) {

        _NodeIndex child_node_index = _process_node(xml_child_node, node_index, elem_type, dimensions, directory);

        if (child_node_index != NODE_INDEX_UNDEFINED) {

            // get node addresses here since the address could change per node process
            _Node *node                = _index_to_node(node_index);
            _Node *child_node          = _index_to_node(child_node_index);
            _Node *previous_child_node = _index_to_node(previous_child_node_index);

            // if the current node and child exists
            if (node && child_node) {

                // set nodes's first child if this is the first child
                if (previous_child_node_index == NODE_INDEX_UNDEFINED) {
                    first_child_index = child_node_index;
                }
            }

            // if there is a previous node
            if (previous_child_node) {

                // set the next node of the previous node
                previous_child_node->next = child_node_index;
            }

            // set previous child node, accounting for cases where the
            // child node is actually a node list
            _NodeIndex last_child_node_index = child_node_index;
            _Node     *last_child_node       = _index_to_node(last_child_node_index);
            while (last_child_node->next != NODE_INDEX_UNDEFINED) {
                last_child_node_index = last_child_node->next;
                last_child_node       = _index_to_node(last_child_node_index);
            }
            previous_child_node_index = last_child_node_index;
        }

        // increment pointer
        xml_child_node = xml_child_node->next;
    }

    return first_child_index;
}

static _NodeIndex _process_node(xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, _ValIndex2 parent_dimensions, const char *directory) {

    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);
    switch (elem_type) {

        case DC_APP_ELEM_TYPE_NONELEM: {
            // ignore non-element nodes
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_CONSTANT: {
            // ignore at this point
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_CONTAINER: {
            _Node dc_node;
            dc_node.type   = NODE_TYPE_CONTAINER;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.container.position.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                dc_node.container.position.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.container.position.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                dc_node.container.position.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // x dimension
            xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
            if (!raw_x_dimension) {
                raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
            }
            if (raw_x_dimension) {
                char cleaned_x_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_dimension, (const char *)raw_x_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_dimension);
                dc_node.container.dimension.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_dimension);
            } else {
                dc_node.container.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y dimension
            xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
            if (!raw_y_dimension) {
                raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
            }
            if (raw_y_dimension) {
                char cleaned_y_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_dimension, (const char *)raw_y_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_dimension);
                dc_node.container.dimension.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_dimension);
            } else {
                dc_node.container.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // virtual x dimension
            xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
            if (!raw_x_virtual_dimension) {
                raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
            }
            if (raw_x_virtual_dimension) {
                char cleaned_x_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_virtual_dimension, (const char *)raw_x_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_virtual_dimension);
                dc_node.container.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_virtual_dimension);
            } else {
                dc_node.container.virtual_dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // virtual y virtual_dimension
            xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
            if (!raw_y_virtual_dimension) {
                raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
            }
            if (raw_y_virtual_dimension) {
                char cleaned_y_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_virtual_dimension, (const char *)raw_y_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_virtual_dimension);
                dc_node.container.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_virtual_dimension);
            } else {
                dc_node.container.virtual_dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local x align
            xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
            if (!raw_x_align) {
                raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
            }
            if (raw_x_align) {
                char cleaned_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_align, (const char *)raw_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_align);
                dc_node.container.local_align.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_align);
            } else {
                dc_node.container.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local y align
            xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
            if (!raw_y_align) {
                raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
            }
            if (raw_y_align) {
                char cleaned_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_align, (const char *)raw_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_align);
                dc_node.container.local_align.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_align);
            } else {
                dc_node.container.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent x align
            xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
            if (raw_parent_x_align) {
                char cleaned_parent_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_x_align, (const char *)raw_parent_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_x_align);
                dc_node.container.parent_align.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_x_align);
            } else {
                dc_node.container.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent y align
            xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
            if (raw_parent_y_align) {
                char cleaned_parent_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_y_align, (const char *)raw_parent_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_y_align);
                dc_node.container.parent_align.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_y_align);
            } else {
                dc_node.container.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.container.rotation = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                dc_node.container.rotation = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.container.pivot_position.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.container.pivot_position.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

                dc_node.container.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.container.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

            } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

                xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotAlignX");
                if (raw_pivot_align_x) {
                    char cleaned_pivot_align_x[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_x, (const char *)raw_pivot_align_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_x);
                    dc_node.container.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_x);
                } else {
                    dc_node.container.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                }

                xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotAlignY");
                if (raw_pivot_align_y) {
                    char cleaned_pivot_align_y[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_y, (const char *)raw_pivot_align_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_y);
                    dc_node.container.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_y);
                } else {
                    dc_node.container.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
                }

                dc_node.container.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.container.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
            } else {
                fprintf(stderr, "DCAPP _process_node(): Container: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _NodeIndex first_child_index = _process_node_children(xml_node, node_index, elem_type, dc_node.container.virtual_dimension, directory);

            // update child index
            _Node *node           = _index_to_node(node_index);
            node->container.child = first_child_index;

            // return
            return node_index;
        }

        // really just the root element, left in for legacy reasons
        case DC_APP_ELEM_TYPE_DCAPP: {
            _process_node_children(xml_node, NODE_INDEX_UNDEFINED, elem_type, parent_dimensions, directory);
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_DEFAULT: {
            // ignore at this point
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_FALSE: {
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_IF: {

                    // process children
                    _NodeIndex first_child_index = _process_node_children(xml_node, parent_node_index, elem_type, parent_dimensions, directory);

                    // update child false node
                    _Node *parent_node                   = _index_to_node(parent_node_index);
                    parent_node->conditional.child_false = first_child_index;
                    break;
                }
                default:
                    fprintf(stderr, "DCAPP _process_node(): Invalid elem parent of type %s for <False>\n", dc_app_elem_type_to_string(parent_elem_type));
            }
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_IF: {

            _Node dc_node                   = {};
            dc_node.type                    = NODE_TYPE_CONDITIONAL;
            dc_node.parent                  = parent_node_index;
            dc_node.next                    = NODE_INDEX_UNDEFINED;
            dc_node.conditional.value1      = DC_APP_VAL_INDEX_UNDEFINED;
            dc_node.conditional.value2      = DC_APP_VAL_INDEX_UNDEFINED;
            dc_node.conditional.child_true  = NODE_INDEX_UNDEFINED;
            dc_node.conditional.child_false = NODE_INDEX_UNDEFINED;

            // conditional type
            xmlChar *raw_type = xmlGetProp(xml_node, BAD_CAST "Operation");
            if (raw_type) {
                char cleaned_type[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_type, (const char *)raw_type, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_type);
                dc_node.conditional.type = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_type);
            } else {
                DcValue value            = dc_value_create_value_integer(DC_APP_CONDITIONAL_TYPE_TRUE);
                dc_node.conditional.type = dc_app_lookup_register_value(data.lookup, &value);
            }

            // value1
            xmlChar *raw_value1 = xmlGetProp(xml_node, BAD_CAST "Value");
            if (!raw_value1) {
                raw_value1 = xmlGetProp(xml_node, BAD_CAST "Value1");
            }
            if (raw_value1) {
                char cleaned_value1[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_value1, (const char *)raw_value1, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_value1);
                dc_node.conditional.value1 = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_STRING, cleaned_value1);
            } else {
                fprintf(stderr, "DCAPP _process_node: Conditional: no value specified\n");
            }

            // value2
            xmlChar *raw_value2 = xmlGetProp(xml_node, BAD_CAST "Value2");
            if (raw_value2) {
                char cleaned_value2[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_value2, (const char *)raw_value2, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_value2);
                dc_node.conditional.value2 = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_STRING, cleaned_value2);
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children (assigning to true/false handled in separate cases, e.g. DC_APP_ELEM_TYPE_TRUE)
            _NodeIndex first_child_index = _process_node_children(xml_node, node_index, elem_type, parent_dimensions, directory);

            // handle implicit <True> elements
            if (first_child_index != NODE_INDEX_UNDEFINED) {

                // ignore if True element already exists
                _Node *node = _index_to_node(node_index);
                if (node->conditional.child_true == NODE_INDEX_UNDEFINED) {
                    node->conditional.child_true = first_child_index;
                } else {
                    printf("DCApp _process_node: Conditional: <If> element has <True> explicit and implicit elements. Ignoring the implicit definitions\n");
                }
            }

            // return
            return node_index;
        }

        // at this point, just used to set the directory path
        case DC_APP_ELEM_TYPE_INCLUDE: {

            xmlChar *file = xmlGetProp(xml_node, BAD_CAST "File");
            char     directory[DC_UTILS_FILEPATH_BUFFER_SIZE];
            dc_utils_get_directory((const char *)file, directory, sizeof(directory));
            return _process_node_children(xml_node, parent_node_index, parent_elem_type, parent_dimensions, directory);
        }

        case DC_APP_ELEM_TYPE_LOGIC: {

            if (data.logic_lib) {
                fprintf(stderr, "DCApp _process_node(): duplicate <Logic> definitions\n");
            }

            if (dc_app_lookup_get_var_count(data.lookup)) {
                printf("DCApp _process_node(): Warning: Declaring <Logic> after <Variables> have been defined. Ensure this behavior is intended.\n");
            }

            xmlChar *raw_filepath = xmlGetProp(xml_node, BAD_CAST "File");
            if (raw_filepath) {

                // clean filepath
                char cleaned_filepath[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_filepath, (const char *)raw_filepath, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_filepath);

                // convert to absolute path
                char abs_filepath[DC_VALUE_STRING_BUFFER_SIZE];
                if (dc_utils_is_relative_path(cleaned_filepath)) {
                    dc_utils_join_paths(directory, cleaned_filepath, abs_filepath, sizeof(abs_filepath));
                } else {
                    strcpy(abs_filepath, cleaned_filepath);
                }

                // open .so file
                const plLibraryDesc logic_so_desc = {
                    .tFlags = PL_LIBRARY_FLAGS_NONE,
                    .pcName = abs_filepath,
                };
                if (_ext_library->load(logic_so_desc, &data.logic_lib) != PL_LIBRARY_RESULT_SUCCESS) {
                    fprintf(stderr, "DCApp _process_node(): Failed to load logic .so file (%s)\n", abs_filepath);
                }

                // load functions
                data.logic_pre_init = (void (*)(void))_ext_library->load_function(data.logic_lib, "DisplayPreInit");
                data.logic_init     = (void (*)(void))_ext_library->load_function(data.logic_lib, "DisplayInit");
                data.logic_draw     = (void (*)(void))_ext_library->load_function(data.logic_lib, "DisplayDraw");
                data.logic_close    = (void (*)(void))_ext_library->load_function(data.logic_lib, "DisplayClose");
            } else {
                fprintf(stderr, "DCAPP _process_node(): <Logic> has no File element\n");
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_PANEL: {
            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_PANEL;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // parent dimensions
            dc_node.panel.parent_dimension = parent_dimensions;

            // virtual x dimension
            xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
            if (!raw_x_virtual_dimension) {
                raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
            }
            if (raw_x_virtual_dimension) {
                char cleaned_x_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_virtual_dimension, (const char *)raw_x_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_virtual_dimension);
                dc_node.panel.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_virtual_dimension);
            } else {
                dc_node.panel.virtual_dimension.x = parent_dimensions.x;
            }

            // virtual y virtual_dimension
            xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
            if (!raw_y_virtual_dimension) {
                raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
            }
            if (raw_y_virtual_dimension) {
                char cleaned_y_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_virtual_dimension, (const char *)raw_y_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_virtual_dimension);
                dc_node.panel.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_virtual_dimension);
            } else {
                dc_node.panel.virtual_dimension.y = parent_dimensions.y;
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _NodeIndex first_child_index = _process_node_children(xml_node, node_index, elem_type, dc_node.panel.virtual_dimension, directory);

            // update child index
            _Node *node       = _index_to_node(node_index);
            node->panel.child = first_child_index;

            // return
            return node_index;
        }

        case DC_APP_ELEM_TYPE_POLYGON: {
            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_POLYGON;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            dc_node.polygon.fill_enabled = _load_color_from_string(xml_node, "FillColor", &(dc_node.polygon.fill_color));
            dc_node.polygon.line_enabled = _load_color_from_string(xml_node, "LineColor", &(dc_node.polygon.line_color));

            // line width
            xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
            if (raw_line_width) {
                char cleaned_line_width[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_line_width, (const char *)raw_line_width, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_line_width);
                dc_node.polygon.line_width = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_line_width);
            } else {
                DcValue value              = dc_value_create_value_double(1.0);
                dc_node.polygon.line_width = dc_app_lookup_register_value(data.lookup, &value);
            }

            // x origin
            xmlChar *raw_x_origin = xmlGetProp(xml_node, BAD_CAST "OriginX");
            if (raw_x_origin) {
                char cleaned_x_origin[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_origin, (const char *)raw_x_origin, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_origin);
                dc_node.polygon.origin.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_origin);
            } else {
                DcValue value            = dc_value_create_value_double(0.0);
                dc_node.polygon.origin.x = dc_app_lookup_register_value(data.lookup, &value);
            }

            // y origin
            xmlChar *raw_y_origin = xmlGetProp(xml_node, BAD_CAST "OriginY");
            if (raw_y_origin) {
                char cleaned_y_origin[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_origin, (const char *)raw_y_origin, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_origin);
                dc_node.polygon.origin.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_origin);
            } else {
                DcValue value            = dc_value_create_value_double(0.0);
                dc_node.polygon.origin.y = dc_app_lookup_register_value(data.lookup, &value);
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.polygon.rotation = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                DcValue value            = dc_value_create_value_double(0.0);
                dc_node.polygon.rotation = dc_app_lookup_register_value(data.lookup, &value);
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.polygon.pivot_position.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.polygon.pivot_position.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

            } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

                DcValue value                    = dc_value_create_value_double(0.0);
                dc_node.polygon.pivot_position.x = dc_app_lookup_register_value(data.lookup, &value);
                dc_node.polygon.pivot_position.y = dc_app_lookup_register_value(data.lookup, &value);

            } else {
                fprintf(stderr, "DCAPP _process_node(): Polygon: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _process_node_children(xml_node, node_index, elem_type, parent_dimensions, directory);

            // return
            return node_index;
        }

        case DC_APP_ELEM_TYPE_SET: {

            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_SET;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // variable
            xmlChar *raw_variable = xmlGetProp(xml_node, BAD_CAST "Variable");
            if (raw_variable) {
                char cleaned_variable[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_variable, (const char *)raw_variable, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_variable);
                dc_node.set.var_index = dc_app_lookup_get_var_index(data.lookup, cleaned_variable);
            } else {
                fprintf(stderr, "DCAPP _process_node: Missing attribute 'Variable' in <Set> element\n");
            }

            // operand
            xmlChar *raw_operand = xmlNodeGetContent(xml_node);
            if (raw_operand) {
                char cleaned_operand[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_operand, (const char *)raw_operand, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_operand);
                dc_node.set.operand = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_operand);
            } else {
                fprintf(stderr, "DCAPP _process_node: Missing Node content in <Set> element\n");
            }

            // operator
            xmlChar *raw_operator = xmlGetProp(xml_node, BAD_CAST "Operator");
            if (raw_operator) {
                char cleaned_operator[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_operator, (const char *)raw_operator, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_operator);
                dc_node.set.operation = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_operator);
            } else {
                DcValue value         = dc_value_create_value_integer(DC_APP_SET_TYPE_EQUAL);
                dc_node.set.operation = dc_app_lookup_register_value(data.lookup, &value);
            }

            // register node
            return _register_node(&dc_node);
        }

        case DC_APP_ELEM_TYPE_STYLE: {
            // ignore at this point
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TERRAIN: {
            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_TERRAIN;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.terrain.position.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                DcValue value              = dc_value_create_value_double(0.0);
                dc_node.terrain.position.x = dc_app_lookup_register_value(data.lookup, &value);
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.terrain.position.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                DcValue value              = dc_value_create_value_double(0.0);
                dc_node.terrain.position.y = dc_app_lookup_register_value(data.lookup, &value);
            }

            // x origin
            xmlChar *raw_x_origin = xmlGetProp(xml_node, BAD_CAST "OriginX");
            if (raw_x_origin) {
                char cleaned_x_origin[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_origin, (const char *)raw_x_origin, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_origin);
                dc_node.terrain.origin.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_origin);
            } else {
                DcValue value            = dc_value_create_value_double(0.0);
                dc_node.terrain.origin.x = dc_app_lookup_register_value(data.lookup, &value);
            }

            // y origin
            xmlChar *raw_y_origin = xmlGetProp(xml_node, BAD_CAST "OriginY");
            if (raw_y_origin) {
                char cleaned_y_origin[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_origin, (const char *)raw_y_origin, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_origin);
                dc_node.terrain.origin.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_origin);
            } else {
                DcValue value            = dc_value_create_value_double(0.0);
                dc_node.terrain.origin.y = dc_app_lookup_register_value(data.lookup, &value);
            }

            // x dimension
            xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
            if (!raw_x_dimension) {
                raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
            }
            if (raw_x_dimension) {
                char cleaned_x_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_dimension, (const char *)raw_x_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_dimension);
                dc_node.terrain.dimension.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_dimension);
            } else {
                dc_node.terrain.dimension.x = parent_dimensions.x;
            }

            // y dimension
            xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
            if (!raw_y_dimension) {
                raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
            }
            if (raw_y_dimension) {
                char cleaned_y_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_dimension, (const char *)raw_y_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_dimension);
                dc_node.terrain.dimension.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_dimension);
            } else {
                dc_node.terrain.dimension.y = parent_dimensions.y;
            }

            // x align
            xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
            if (!raw_x_align) {
                raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
            }
            if (raw_x_align) {
                char cleaned_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_align, (const char *)raw_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_align);
                dc_node.terrain.local_align.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_align);
            } else {
                DcValue value                 = dc_value_create_value_integer(DC_APP_ALIGN_TYPE_LEFT);
                dc_node.terrain.local_align.x = dc_app_lookup_register_value(data.lookup, &value);
            }

            // y align
            xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
            if (!raw_y_align) {
                raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
            }
            if (raw_y_align) {
                char cleaned_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_align, (const char *)raw_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_align);
                dc_node.terrain.local_align.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_align);
            } else {
                DcValue value                 = dc_value_create_value_integer(DC_APP_ALIGN_TYPE_BOTTOM);
                dc_node.terrain.local_align.y = dc_app_lookup_register_value(data.lookup, &value);
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.terrain.rotation = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                DcValue value            = dc_value_create_value_double(0.0);
                dc_node.terrain.rotation = dc_app_lookup_register_value(data.lookup, &value);
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.terrain.pivot_position.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.terrain.pivot_position.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

                DcValue value                 = dc_value_create_value_integer(DC_APP_ALIGN_TYPE_UNDEFINED);
                dc_node.terrain.pivot_align.x = dc_app_lookup_register_value(data.lookup, &value);
                dc_node.terrain.pivot_align.y = dc_app_lookup_register_value(data.lookup, &value);

            } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

                xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotAlignX");
                if (raw_pivot_align_x) {
                    char cleaned_pivot_align_x[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_x, (const char *)raw_pivot_align_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_x);
                    dc_node.terrain.pivot_align.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_x);
                } else {
                    DcValue value                 = dc_value_create_value_double(DC_APP_ALIGN_TYPE_LEFT);
                    dc_node.terrain.pivot_align.x = dc_app_lookup_register_value(data.lookup, &value);
                }

                xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotAlignX");
                if (raw_pivot_align_y) {
                    char cleaned_pivot_align_y[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_y, (const char *)raw_pivot_align_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_y);
                    dc_node.terrain.pivot_align.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_y);
                } else {
                    DcValue value                 = dc_value_create_value_double(DC_APP_ALIGN_TYPE_BOTTOM);
                    dc_node.terrain.pivot_align.y = dc_app_lookup_register_value(data.lookup, &value);
                }
            } else {
                fprintf(stderr, "DCAPP _process_node(): Terrain: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // lat
            xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "Latitude");
            if (raw_lat) {
                char cleaned_lat[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_lat, (const char *)raw_lat, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_lat);
                dc_node.terrain.lle.lat = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_lat);
            } else {
                fprintf(stderr, "DCApp _process_node(): Must supply attribute Latitude with Terrain primitive");
            }

            // lon
            xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
            if (raw_lon) {
                char cleaned_lon[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_lon, (const char *)raw_lon, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_lon);
                dc_node.terrain.lle.lon = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_lon);
            } else {
                fprintf(stderr, "DCApp _process_node(): Must supply attribute Longitude with Terrain primitive");
            }

            // ele
            xmlChar *raw_ele = xmlGetProp(xml_node, BAD_CAST "Elevation");
            if (raw_ele) {
                char cleaned_ele[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_ele, (const char *)raw_ele, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_ele);
                dc_node.terrain.lle.ele = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_ele);
            } else {
                fprintf(stderr, "DCApp _process_node(): Must supply attribute Elevation with Terrain primitive");
            }

            // roll
            xmlChar *raw_roll = xmlGetProp(xml_node, BAD_CAST "Roll");
            if (raw_roll) {
                char cleaned_roll[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_roll, (const char *)raw_roll, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_roll);
                dc_node.terrain.rpy.roll = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_roll);
            } else {
                fprintf(stderr, "DCApp _process_node(): Must supply attribute Roll with Terrain primitive");
            }

            // pitch
            xmlChar *raw_pitch = xmlGetProp(xml_node, BAD_CAST "Pitch");
            if (raw_pitch) {
                char cleaned_pitch[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pitch, (const char *)raw_pitch, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pitch);
                dc_node.terrain.rpy.pitch = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pitch);
            } else {
                fprintf(stderr, "DCApp _process_node(): Must supply attribute Pitch with Terrain primitive");
            }

            // yaw
            xmlChar *raw_yaw = xmlGetProp(xml_node, BAD_CAST "Yaw");
            if (raw_yaw) {
                char cleaned_yaw[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_yaw, (const char *)raw_yaw, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_yaw);
                dc_node.terrain.rpy.yaw = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_yaw);
            } else {
                fprintf(stderr, "DCApp _process_node(): Must supply attribute Yaw with Terrain primitive");
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _process_node_children(xml_node, node_index, elem_type, parent_dimensions, directory);

            // return
            return node_index;
        }

        case DC_APP_ELEM_TYPE_TERRAIN_DEM: {

            _Node *parent_node = _index_to_node(parent_node_index);
            switch (parent_node->type) {
                case NODE_TYPE_TERRAIN: {

                    // file
                    xmlChar *raw_filepath = xmlGetProp(xml_node, BAD_CAST "File");
                    if (raw_filepath) {

                        // clean filepath
                        char cleaned_filepath[DC_VALUE_STRING_BUFFER_SIZE];
                        strncpy(cleaned_filepath, (const char *)raw_filepath, DC_VALUE_STRING_BUFFER_SIZE - 1);
                        xmlFree(raw_filepath);

                        // convert to absolute path
                        char abs_filepath[DC_VALUE_STRING_BUFFER_SIZE];
                        if (dc_utils_is_relative_path(cleaned_filepath)) {
                            dc_utils_join_paths(directory, cleaned_filepath, abs_filepath, sizeof(abs_filepath));
                        } else {
                            strcpy(abs_filepath, cleaned_filepath);
                        }

                        // TODO open file
                        // print for now
                        printf("TerrainDEM file: %s\n", abs_filepath);
                    } else {
                        fprintf(stderr, "DCAPP _process_node(): <Logic> has no File element\n");
                    }

                    break;
                }
                default:
                    fprintf(stderr, "DCApp _process_node(): Invalid parent of type %s for TerrainDEM\n", _node_type_to_string(parent_node->type));
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TEXT: {
            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_TEXT;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // text
            xmlChar *raw_text = xmlNodeGetContent(xml_node);
            if (raw_text) {
                char cleaned_text[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_text, (const char *)raw_text, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_text);

                static char *sb_curr_filler = NULL;
                for (size_t ii = 0; ii < strlen(cleaned_text);) {
                    if (cleaned_text[ii] == '\\') {
                        // Escape character: skip and add next character to result
                        if (ii + 1 < strlen(cleaned_text)) {
                            sbpush(sb_curr_filler, cleaned_text[ii + 1]);
                            ii += 2;
                        } else {
                            sbpush(sb_curr_filler, cleaned_text[ii++]);
                        }
                        continue;
                    }

                    if (cleaned_text[ii] == '@') {
                        size_t start = ii;
                        ii++;

                        char var[DC_VALUE_STRING_BUFFER_SIZE];
                        bool is_braced = false;

                        if (ii < strlen(cleaned_text) && cleaned_text[ii] == '{') {
                            is_braced = true;
                            ii++;
                            size_t end = dc_utils_str_find_first(&(cleaned_text[ii]), '}');
                            if (end == -1) {
                                // No closing brace, treat as normal text
                                sbpushn(sb_curr_filler, &(cleaned_text[start]), ii - start);
                                continue;
                            }
                            strncpy(var, &(cleaned_text[ii]), end - ii);
                            ii = end + 1;
                        } else {
                            size_t start_var = ii;
                            while (ii < strlen(cleaned_text) && !isspace(cleaned_text[ii]) && cleaned_text[ii] != '(') {
                                ii++;
                            }
                            strncpy(var, &(cleaned_text[start_var]), ii - start_var);
                        }

                        DcAppValIndex   var_index = dc_app_lookup_get_var_index(data.lookup, var);
                        DcAppLookupVar *var_var   = dc_app_lookup_get_var(data.lookup, var_index);
                        sbpush(dc_node.text.sb_vals, var_var->value_index);

                        // Check for format specifier
                        char format_spec[DC_VALUE_STRING_BUFFER_SIZE];
                        if (ii < strlen(cleaned_text) && cleaned_text[ii] == '(') {
                            size_t close = dc_utils_str_find_first(&(cleaned_text[ii]), ')');
                            if (close != -1 && (ii == 0 || cleaned_text[ii - 1] != '\\')) {
                                strncpy(format_spec, &(cleaned_text[ii + 1]), close - ii - 1);
                                ii = close + 1;

                                // get format + type
                                if (dc_utils_is_format_specifier_int(format_spec)) {
                                    sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_INTEGER);
                                    sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                    sbpushn(dc_node.text.sb_formats, format_spec, strlen(format_spec) + 1);
                                } else if (dc_utils_is_format_specifier_double(format_spec)) {
                                    sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_DOUBLE);
                                    sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                    sbpushn(dc_node.text.sb_formats, format_spec, strlen(format_spec) + 1);
                                } else if (dc_utils_is_format_specifier_string(format_spec)) {
                                    sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                                    sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                    sbpushn(dc_node.text.sb_formats, format_spec, strlen(format_spec) + 1);
                                } else if (dc_utils_is_format_specifier_bool(format_spec)) {
                                    sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_BOOLEAN);
                                    sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                    sbpushn(dc_node.text.sb_formats, format_spec, strlen(format_spec) + 1);
                                } else {
                                    fprintf(stderr, "DCApp _process_node(): Unknown format specifier in Text element\n");
                                }
                            } else {
                                sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                                sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                sbpushn(dc_node.text.sb_formats, "%s", 3);
                            }
                        } else {
                            sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                            sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                            sbpushn(dc_node.text.sb_formats, "%s", 3);
                        }

                        // add the current filler to list of fillers
                        sbpush(dc_node.text.sb_filler_indices, sbcount(dc_node.text.sb_fillers));
                        sbpushn(dc_node.text.sb_fillers, sb_curr_filler, sbcount(sb_curr_filler));
                        sbpush(dc_node.text.sb_fillers, '\0');
                        sbclear(sb_curr_filler);

                        continue;
                    }

                    // Default: append character to result
                    sbpush(sb_curr_filler, cleaned_text[ii++]);
                }

                // append the remaining filler
                sbpush(dc_node.text.sb_filler_indices, sbcount(dc_node.text.sb_fillers));
                sbpushn(dc_node.text.sb_fillers, sb_curr_filler, sbcount(sb_curr_filler));
                sbpush(dc_node.text.sb_fillers, '\0');

                // clear temp buffer
                sbclear(sb_curr_filler);
            } else {
                fprintf(stderr, "DCApp _process_node(): Missing node content for text\n");
            }

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.text.position.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                DcValue value           = dc_value_create_value_double(0.0);
                dc_node.text.position.x = dc_app_lookup_register_value(data.lookup, &value);
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.text.position.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                DcValue value           = dc_value_create_value_double(0.0);
                dc_node.text.position.y = dc_app_lookup_register_value(data.lookup, &value);
            }

            // x origin
            xmlChar *raw_x_origin = xmlGetProp(xml_node, BAD_CAST "OriginX");
            if (raw_x_origin) {
                char cleaned_x_origin[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_origin, (const char *)raw_x_origin, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_origin);
                dc_node.text.origin.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_origin);
            } else {
                DcValue value         = dc_value_create_value_double(0.0);
                dc_node.text.origin.x = dc_app_lookup_register_value(data.lookup, &value);
            }

            // y origin
            xmlChar *raw_y_origin = xmlGetProp(xml_node, BAD_CAST "OriginY");
            if (raw_y_origin) {
                char cleaned_y_origin[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_origin, (const char *)raw_y_origin, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_origin);
                dc_node.text.origin.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_origin);
            } else {
                DcValue value         = dc_value_create_value_double(0.0);
                dc_node.text.origin.y = dc_app_lookup_register_value(data.lookup, &value);
            }

            // x align
            xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
            if (!raw_x_align) {
                raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
            }
            if (raw_x_align) {
                char cleaned_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_align, (const char *)raw_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_align);
                dc_node.text.local_align.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_align);
            } else {
                DcValue value              = dc_value_create_value_integer(DC_APP_ALIGN_TYPE_LEFT);
                dc_node.text.local_align.x = dc_app_lookup_register_value(data.lookup, &value);
            }

            // y align
            xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
            if (!raw_y_align) {
                raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
            }
            if (raw_y_align) {
                char cleaned_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_align, (const char *)raw_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_align);
                dc_node.text.local_align.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_align);
            } else {
                DcValue value              = dc_value_create_value_integer(DC_APP_ALIGN_TYPE_BOTTOM);
                dc_node.text.local_align.y = dc_app_lookup_register_value(data.lookup, &value);
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.text.rotation = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                DcValue value         = dc_value_create_value_double(0.0);
                dc_node.text.rotation = dc_app_lookup_register_value(data.lookup, &value);
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.text.pivot_position.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.text.pivot_position.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

                DcValue value              = dc_value_create_value_integer(DC_APP_ALIGN_TYPE_UNDEFINED);
                dc_node.text.pivot_align.x = dc_app_lookup_register_value(data.lookup, &value);
                dc_node.text.pivot_align.y = dc_app_lookup_register_value(data.lookup, &value);

            } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

                xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotAlignX");
                if (raw_pivot_align_x) {
                    char cleaned_pivot_align_x[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_x, (const char *)raw_pivot_align_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_x);
                    dc_node.text.pivot_align.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_x);
                } else {
                    DcValue value              = dc_value_create_value_double(DC_APP_ALIGN_TYPE_LEFT);
                    dc_node.text.pivot_align.x = dc_app_lookup_register_value(data.lookup, &value);
                }

                xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotAlignY");
                if (raw_pivot_align_y) {
                    char cleaned_pivot_align_y[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_y, (const char *)raw_pivot_align_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_y);
                    dc_node.text.pivot_align.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_y);
                } else {
                    DcValue value              = dc_value_create_value_double(DC_APP_ALIGN_TYPE_BOTTOM);
                    dc_node.text.pivot_align.y = dc_app_lookup_register_value(data.lookup, &value);
                }
            } else {
                fprintf(stderr, "DCAPP _process_node(): Text: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // size
            xmlChar *raw_size = xmlGetProp(xml_node, BAD_CAST "Size");
            if (raw_size) {
                char cleaned_size[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_size, (const char *)raw_size, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_size);
                dc_node.text.size = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_size);
            } else {
                DcValue value     = dc_value_create_value_double(1.0);
                dc_node.text.size = dc_app_lookup_register_value(data.lookup, &value);
            }

            dc_node.text.fill_enabled = _load_color_from_string(xml_node, "FillColor", &(dc_node.text.fill_color));
            dc_node.text.line_enabled = _load_color_from_string(xml_node, "LineColor", &(dc_node.text.line_color));

            // register node
            return _register_node(&dc_node);
        }

        case DC_APP_ELEM_TYPE_TRICK_FROM: {
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_IO: {
                    _process_node_children(xml_node, NODE_INDEX_UNDEFINED, elem_type, parent_dimensions, directory);
                    break;
                }
                default: {
                    fprintf(stderr, "DCApp _process_node(): Invalid elem parent of type %s for FromTrick\n", dc_app_elem_type_to_string(parent_elem_type));
                    break;
                }
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TRICK_IO: {

            // host
            xmlChar *raw_host = xmlGetProp(xml_node, BAD_CAST "Host");
            char     host[DC_VALUE_STRING_BUFFER_SIZE];
            if (raw_host) {
                strncpy(host, (const char *)raw_host, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_host);
            } else {
                fprintf(stderr, "DCApp _process_node: Missing Port for TrickIO\n");
            }

            // port
            xmlChar *raw_port = xmlGetProp(xml_node, BAD_CAST "Port");
            int      port;
            if (raw_port) {
                char cleaned_port[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_port, (const char *)raw_port, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_port);
                port = dc_utils_string_to_double(cleaned_port);
            } else {
                fprintf(stderr, "DCApp _process_node: Missing Port for TrickIO\n");
            }

            // data rate
            xmlChar *raw_data_rate = xmlGetProp(xml_node, BAD_CAST "Rotation");
            double   data_rate     = 0.1;
            if (raw_data_rate) {
                char cleaned_data_rate[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_data_rate, (const char *)raw_data_rate, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_data_rate);
                data_rate = dc_utils_string_to_double(cleaned_data_rate);
            }

            // create trick instance
            _TrickContext trick_context = {};
            trick_context.trick         = dc_trick_create(host, port, data_rate, 1);
            sbpush(data.sb_tricks, trick_context);

            // process children
            _process_node_children(xml_node, NODE_INDEX_UNDEFINED, elem_type, parent_dimensions, directory);

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TRICK_TO: {
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_IO: {
                    _process_node_children(xml_node, NODE_INDEX_UNDEFINED, elem_type, parent_dimensions, directory);
                    break;
                }
                default: {
                    fprintf(stderr, "DCApp _process_node(): Invalid elem parent of type %s for ToTrick\n", dc_app_elem_type_to_string(parent_elem_type));
                    break;
                }
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TRICK_VARIABLE: {

            // check for invalid elem type
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_FROM:
                case DC_APP_ELEM_TYPE_TRICK_TO:
                    break;
                default:
                    fprintf(stderr, "DCApp _process_node(): Invalid elem parent of type %s for TrickVariable\n", dc_app_elem_type_to_string(parent_elem_type));
            }

            // var path
            xmlChar *raw_trick_path = xmlGetProp(xml_node, BAD_CAST "Name");
            char     trick_path[DC_VALUE_STRING_BUFFER_SIZE];
            if (raw_trick_path) {
                strncpy(trick_path, (const char *)raw_trick_path, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_trick_path);
            } else {
                fprintf(stderr, "DCApp _process_node: Missing trick Var path for TrickVariable\n");
            }

            // dcapp var
            xmlChar *raw_dcapp_var = xmlNodeGetContent(xml_node);
            char     dcapp_var[DC_VALUE_STRING_BUFFER_SIZE];
            if (raw_dcapp_var) {
                strncpy(dcapp_var, (const char *)raw_dcapp_var, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_dcapp_var);
            } else {
                fprintf(stderr, "DCApp _process_node: Missing dcapp Var path for TrickVariable\n");
            }

            // units
            xmlChar *raw_units = xmlGetProp(xml_node, BAD_CAST "Name");
            char     cleaned_units[DC_VALUE_STRING_BUFFER_SIZE];
            char    *units = NULL;
            if (raw_units) {
                strncpy(cleaned_units, (const char *)raw_units, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_units);
                units = cleaned_units;
            }

            // get current trick context
            _TrickContext *trick_context = &(data.sb_tricks[sbcount(data.sb_tricks) - 1]);

            // handle depending on parent
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_FROM: {

                    // create + add rx var
                    _TrickRxVarContext var = {};
                    var.trick_var_index = dc_trick_add_rx_var(trick_context->trick, trick_path, units);
                    var.dcapp_var_index = dc_app_lookup_get_var_index(data.lookup, dcapp_var);
                    sbpush(trick_context->sb_rx_var_contexts, var);
                    break;
                }
                case DC_APP_ELEM_TYPE_TRICK_TO: {

                    // create + add tx var
                    _TrickTxVarContext var = {};
                    var.dcapp_var_index   = dc_app_lookup_get_var_index(data.lookup, dcapp_var);
                    DcValue *dc_var_value = dc_app_lookup_get_value(data.lookup, dc_app_lookup_get_var(data.lookup, var.dcapp_var_index)->value_index);
                    var.trick_var_index   = dc_trick_add_tx_var(trick_context->trick, trick_path, units, dc_var_value->type == DC_VALUE_TYPE_STRING);
                    dc_value_copy(&var.prev_value, dc_var_value);
                    sbpush(trick_context->sb_tx_var_contexts, var);
                    break;
                }
                default:
                    // should never reach here
                    fprintf(stderr, "DCApp _process_node(): Invalid parent for TrickVariable\n");
                    break;
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TRUE: {
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_IF: {

                    // process children
                    _NodeIndex first_child_index = _process_node_children(xml_node, parent_node_index, elem_type, parent_dimensions, directory);

                    // update child true node
                    _Node *parent_node                  = _index_to_node(parent_node_index);
                    parent_node->conditional.child_true = first_child_index;
                    break;
                }
                default:
                    fprintf(stderr, "DCApp _process_node(): Invalid elem parent of type %s for <True>.\n", dc_app_elem_type_to_string(parent_elem_type));
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_VARIABLE: {

            // name
            xmlChar *raw_name = xmlNodeGetContent(xml_node);
            char     name[DC_VALUE_STRING_BUFFER_SIZE];
            if (raw_name) {
                strncpy(name, (const char *)raw_name, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_name);
            } else {
                fprintf(stderr, "DCApp _process_node: Non-existent node content in <Variable> definition\n");
            }

            xmlChar    *raw_type = xmlGetProp(xml_node, BAD_CAST "Type");
            DcValueType type     = DC_VALUE_TYPE_STRING;
            if (raw_type) {
                char cleaned_type[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_type, (const char *)raw_type, DC_VALUE_STRING_BUFFER_SIZE - 1);
                type = dc_app_value_type_from_string(cleaned_type);
                xmlFree(raw_type);
            }

            xmlChar *raw_initial_value = xmlGetProp(xml_node, BAD_CAST "InitialValue");
            DcValue  initial_value;
            if (raw_initial_value) {
                char cleaned_initial_value[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_initial_value, (const char *)raw_initial_value, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_initial_value);
                initial_value = dc_value_create_value_string(cleaned_initial_value);
            } else {
                initial_value = dc_value_create_value_string("");
            }
            initial_value.type = type;

            // register var, link to extern
            DcAppLookupVar var = {};
            var.value_index    = dc_app_lookup_register_value(data.lookup, &initial_value);
            if (data.logic_lib) {
                var.extern_data = _ext_library->load_function(data.logic_lib, name);
            }
            DcAppVarIndex var_index = dc_app_lookup_register_var(data.lookup, name, &var);

            // set the extern data to the initial value
            if (data.logic_lib) {
                dc_app_lookup_refresh_var_from_value(data.lookup, var_index);
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_VERTEX: {
            _Node *parent_node = _index_to_node(parent_node_index);
            switch (parent_node->type) {
                case NODE_TYPE_POLYGON: {

                    // position
                    _ValIndex2 position;

                    // x position
                    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
                    if (!raw_x_position) {
                        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
                    }

                    if (raw_x_position) {
                        char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                        strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                        xmlFree(raw_x_position);
                        position.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
                    } else {
                        fprintf(stderr, "DCAPP _process_node: Invalid Vertex: No X attribute\n");
                    }

                    // y position
                    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
                    if (!raw_y_position) {
                        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
                    }
                    if (raw_y_position) {
                        char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                        strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                        xmlFree(raw_y_position);
                        position.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
                    } else {
                        fprintf(stderr, "DCAPP _process_node: Invalid Vertex: No Y attribute\n");
                    }

                    // add to parent
                    sbpush(parent_node->polygon.sb_points, position);
                    break;
                }
                default:
                    fprintf(stderr, "DCApp _process_node(): Invalid parent of type %s for vertex\n", _node_type_to_string(parent_node->type));
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_WINDOW: {
            _Node dc_node;
            dc_node.type   = NODE_TYPE_WINDOW;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // title
            xmlChar *raw_title = xmlGetProp(xml_node, BAD_CAST "Title");
            if (raw_title) {
                char cleaned_title[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_title, (const char *)raw_title, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_title);
                dc_node.window.title = strdup(cleaned_title);
            } else {
                dc_node.window.title = strdup("dcapp");
            }

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.window.position.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                DcValue value             = dc_value_create_value_double(0.0);
                dc_node.window.position.x = dc_app_lookup_register_value(data.lookup, &value);
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.window.position.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                DcValue value             = dc_value_create_value_double(0.0);
                dc_node.window.position.y = dc_app_lookup_register_value(data.lookup, &value);
            }

            // x dimension
            xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
            if (!raw_x_dimension) {
                raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
            }
            if (raw_x_dimension) {
                char cleaned_x_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_dimension, (const char *)raw_x_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_dimension);
                dc_node.window.dimension.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_dimension);
            } else {
                fprintf(stderr, "DCApp _process_node: missing x dimension for Window\n");
            }

            // y dimension
            xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
            if (!raw_y_dimension) {
                raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
            }
            if (raw_y_dimension) {
                char cleaned_y_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_dimension, (const char *)raw_y_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_dimension);
                dc_node.window.dimension.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_dimension);
            } else {
                fprintf(stderr, "DCApp _process_node: missing y dimension for Window\n");
            }

            // virtual x dimension
            xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
            if (!raw_x_virtual_dimension) {
                raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
            }
            if (raw_x_virtual_dimension) {
                char cleaned_x_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_virtual_dimension, (const char *)raw_x_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_virtual_dimension);
                dc_node.window.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_virtual_dimension);
            } else {
                dc_node.window.virtual_dimension.x = dc_node.window.dimension.x;
            }

            // virtual y virtual_dimension
            xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
            if (!raw_y_virtual_dimension) {
                raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
            }
            if (raw_y_virtual_dimension) {
                char cleaned_y_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_virtual_dimension, (const char *)raw_y_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_virtual_dimension);
                dc_node.window.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_virtual_dimension);
            } else {
                dc_node.window.virtual_dimension.y = dc_node.window.dimension.y;
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _NodeIndex first_child_index = _process_node_children(xml_node, node_index, elem_type, dc_node.window.virtual_dimension, directory);

            // update child index
            _Node *node        = _index_to_node(node_index);
            node->window.child = first_child_index;

            // set global window
            data.window = node_index;

            // return
            return node_index;
        }

        default:
            fprintf(stderr, "DCApp _process_node(): Invalid node type found\n");
            return NODE_INDEX_UNDEFINED;
    }

    fprintf(stderr, "DCApp _process_node(): Unhandled switch statement\n");
    return NODE_INDEX_UNDEFINED;
}

static void _draw_node_list(_PlAppData *pl_app_data, _NodeIndex node_index, plVec2 *parent_dimensions, plMat4 *node_transform) {
    _NodeIndex current_node_index = node_index;
    while (current_node_index != NODE_INDEX_UNDEFINED) {
        _draw_node(pl_app_data, current_node_index, parent_dimensions, node_transform);
        current_node_index = _index_to_node(current_node_index)->next;
    }
}

static void _draw_node(_PlAppData *pl_app_data, _NodeIndex node_index, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    if (node_index == NODE_INDEX_UNDEFINED) {
        fprintf(stderr, "DCAPP _draw_node(): attempting to draw undefined node index\n");
    }

    _Node *node = _index_to_node(node_index);
    switch (node->type) {
        case NODE_TYPE_CONTAINER: {

            // boolean checks
            bool use_dimension[2] = {
                node->container.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
                node->container.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
            bool use_virtual_dimension[2] = {
                node->container.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
                node->container.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
            bool use_rotation       = node->container.rotation != DC_APP_VAL_INDEX_UNDEFINED;
            bool use_pivot_position = (node->container.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->container.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

            // get dimensions
            float dimension[2] = {
                use_dimension[0] ? (float)dc_app_lookup_get_value(data.lookup, node->container.dimension.x)->value_double : parent_dimensions->x,
                use_dimension[1] ? (float)dc_app_lookup_get_value(data.lookup, node->container.dimension.y)->value_double : parent_dimensions->y};

            // get virtual dimensions
            float virtual_dimension[2] = {
                use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(data.lookup, node->container.virtual_dimension.x)->value_double : dimension[0],
                use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(data.lookup, node->container.virtual_dimension.y)->value_double : dimension[1]};

            // transform
            plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

            // xform rotation (around a point)
            {
                if (use_rotation && use_pivot_position) {

                    // get pivot XY, rotation
                    float pivot_position[2] = {
                        (float)dc_app_lookup_get_value(data.lookup, node->container.pivot_position.x)->value_double,
                        (float)dc_app_lookup_get_value(data.lookup, node->container.pivot_position.y)->value_double};
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(data.lookup, node->container.rotation)->value_double);

                    // compute matrices
                    plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);
                    plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
                    plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);

                    // apply transform
                    transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
                    transform = pl_mul_mat4t(&transform, &rotate_xform);
                    transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
                }
            }

            // xform local alignment
            {
                // get alignment
                DcAppAlignType local_aligns[2] = {
                    node->container.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->container.local_align.x)->value_integer,
                    node->container.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->container.local_align.y)->value_integer};

                // compute offsets
                float trans_align_offsets[2];
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
                        fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[0]);
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
                        fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[1]);
                        break;
                }

                // compute matrix
                plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

                // apply transform
                transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
            }

            // xform position
            {
                // boolean check
                bool use_position[2] = {
                    node->container.position.x != DC_APP_VAL_INDEX_UNDEFINED,
                    node->container.position.y != DC_APP_VAL_INDEX_UNDEFINED};

                // get position
                float position[2];
                if (use_position[0]) {
                    position[0] = (float)dc_app_lookup_get_value(data.lookup, node->container.position.x)->value_double;
                } else {
                    DcAppAlignType parent_align_x = node->container.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(data.lookup, node->container.parent_align.x)->value_integer;
                    switch (parent_align_x) {
                        case DC_APP_ALIGN_TYPE_UNDEFINED:
                        case DC_APP_ALIGN_TYPE_LEFT:
                            position[0] = 0;
                            break;
                        case DC_APP_ALIGN_TYPE_CENTER:
                            position[0] = parent_dimensions->x / 2;
                            break;
                        case DC_APP_ALIGN_TYPE_RIGHT:
                            position[0] = parent_dimensions->x;
                            break;
                        default:
                            fprintf(stderr, "DCAPP _draw_node() container: Invalid parent_align_x value %d\n", parent_align_x);
                            break;
                    }
                }
                if (use_position[1]) {
                    position[1] = (float)dc_app_lookup_get_value(data.lookup, node->container.position.y)->value_double;
                } else {
                    DcAppAlignType parent_align_y = node->container.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(data.lookup, node->container.parent_align.y)->value_integer;
                    switch (parent_align_y) {
                        case DC_APP_ALIGN_TYPE_UNDEFINED:
                        case DC_APP_ALIGN_TYPE_BOTTOM:
                            position[1] = 0;
                            break;
                        case DC_APP_ALIGN_TYPE_MIDDLE:
                            position[1] = parent_dimensions->y / 2;
                            break;
                        case DC_APP_ALIGN_TYPE_TOP:
                            position[1] = parent_dimensions->y;
                            break;
                        default:
                            fprintf(stderr, "DCAPP _draw_node() container: Invalid parent_align_y value %d\n", parent_align_y);
                            break;
                    }
                }

                // compute matrix
                plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

                // apply transform
                transform = pl_mul_mat4t(&transform, &trans_position_xform);
            }

            // xform local rotation
            {
                if (use_rotation && !use_pivot_position) {

                    // get alignment
                    DcAppAlignType local_pivot_aligns[2] = {
                        node->container.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->container.pivot_local_align.x)->value_integer,
                        node->container.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->container.pivot_local_align.y)->value_integer};

                    // get pivot XY, rotation
                    float pivot_position[2];
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
                            fprintf(stderr, "Unknown pivot alignment in <container> draw call: %d\n", local_pivot_aligns[0]);
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
                            fprintf(stderr, "Unknown pivot alignment in <container> draw call: %d\n", local_pivot_aligns[1]);
                            break;
                    }
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(data.lookup, node->container.rotation)->value_double);

                    // compute matrices
                    plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);
                    plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
                    plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);

                    // apply transform
                    transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
                    transform = pl_mul_mat4t(&transform, &rotate_xform);
                    transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
                }
            }

            // xform scale
            {
                // get scale factors
                float scale_factors[2] = {
                    use_virtual_dimension[0] ? dimension[0] / (float)dc_app_lookup_get_value(data.lookup, node->container.virtual_dimension.x)->value_double : 1,
                    use_virtual_dimension[1] ? dimension[1] / (float)dc_app_lookup_get_value(data.lookup, node->container.virtual_dimension.y)->value_double : 1};

                // compute matrix
                plMat4 scale_xform = pl_mat4_scale_xyz(dimension[0] / virtual_dimension[0], dimension[1] / virtual_dimension[1], 1.0f);

                // apply transform
                transform = pl_mul_mat4t(&transform, &scale_xform);
            }

            // parent transform
            transform = pl_mul_mat4t(parent_transform, &transform);

            // draw children
            plVec2 virtual_dimensions_vec2 = (plVec2){virtual_dimension[0], virtual_dimension[1]};
            _draw_node_list(pl_app_data, node->container.child, &virtual_dimensions_vec2, &transform);
            break;
        }

        case NODE_TYPE_CONDITIONAL: {
            DcValue             *val1 = dc_app_lookup_get_value(data.lookup, node->conditional.value1);
            DcValue             *val2 = dc_app_lookup_get_value(data.lookup, node->conditional.value2);
            DcAppConditionalType type = (DcAppConditionalType)dc_app_lookup_get_value(data.lookup, node->conditional.type)->value_integer;

            // evaluate
            bool result;
            switch (type) {
                case DC_APP_CONDITIONAL_TYPE_TRUE:
                    result = val1->value_boolean;
                    break;
                case DC_APP_CONDITIONAL_TYPE_FALSE:
                    result = !(val1->value_boolean);
                    break;
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
                default:
                    fprintf(stderr, "DCApp _draw_node(): unknown conditional_type on evaluation %d\n", type);
                    break;
            }

            // process children
            if (result) {
                _draw_node_list(pl_app_data, node->conditional.child_true, parent_dimensions, parent_transform);
            } else {
                _draw_node_list(pl_app_data, node->conditional.child_false, parent_dimensions, parent_transform);
            }
            break;
        }

        case NODE_TYPE_PANEL: {

            // all transform parameters
            float parent_size[2]  = {(float)dc_app_lookup_get_value(data.lookup, node->panel.parent_dimension.x)->value_double, (float)dc_app_lookup_get_value(data.lookup, node->panel.parent_dimension.y)->value_double};
            float virtual_size[2] = {(float)dc_app_lookup_get_value(data.lookup, node->panel.virtual_dimension.x)->value_double, (float)dc_app_lookup_get_value(data.lookup, node->panel.virtual_dimension.y)->value_double};

            // compute transforms
            // scale from virtual to real dimensions
            plMat4 scale_matrix = pl_mat4_scale_xyz(
                parent_size[0] / virtual_size[0],
                parent_size[1] / virtual_size[1],
                1.0f);

            plMat4 transform;
            transform = pl_mul_mat4t(parent_transform, &scale_matrix);

            plVec2 virtual_dimensions_vec2 = (plVec2){virtual_size[0], virtual_size[1]};
            _draw_node_list(pl_app_data, node->panel.child, &virtual_dimensions_vec2, &transform);
        }

        case NODE_TYPE_POLYGON: {

            // all transform parameters
            float origin[2]         = {(float)dc_app_lookup_get_value(data.lookup, node->polygon.origin.x)->value_double, (float)dc_app_lookup_get_value(data.lookup, node->polygon.origin.y)->value_double};
            float pivot_position[2] = {(float)dc_app_lookup_get_value(data.lookup, node->polygon.pivot_position.x)->value_double, (float)dc_app_lookup_get_value(data.lookup, node->polygon.pivot_position.y)->value_double};
            float rotation          = dc_app_lookup_get_value(data.lookup, node->polygon.rotation)->value_double;

            // move origin
            plMat4 trans_origin_xform = pl_mat4_translate_xyz(
                origin[0],
                origin[1],
                0.0f);

            // move to pivot
            plMat4 trans_to_pivot_xform = pl_mat4_translate_xyz(
                -1.0 * pivot_position[0],
                -1.0 * pivot_position[1],
                0.0f);

            // rotate
            plMat4 rotate_xform = pl_mat4_rotate_vec3(
                pl_radiansf(rotation),
                (plVec3){0.0f, 0.0f, 1.0f});

            // reverse pivot move
            plMat4 trans_from_pivot_xform = pl_mat4_translate_xyz(
                pivot_position[0],
                pivot_position[1],
                0.0f);

            // compute transform
            plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            transform        = pl_mul_mat4t(&transform, &trans_from_pivot_xform);
            transform        = pl_mul_mat4t(&transform, &rotate_xform);
            transform        = pl_mul_mat4t(&transform, &trans_to_pivot_xform);
            transform        = pl_mul_mat4t(&transform, &trans_origin_xform);
            transform        = pl_mul_mat4t(parent_transform, &transform);

            // get points
            int    num_points = sbcount(node->polygon.sb_points);
            plVec2 points[num_points];
            for (int ii = 0; ii < num_points; ii++) {
                plVec4 point4 = (plVec4){
                    (float)dc_app_lookup_get_value(data.lookup, node->polygon.sb_points[ii].x)->value_double,
                    (float)dc_app_lookup_get_value(data.lookup, node->polygon.sb_points[ii].y)->value_double,
                    0, 1};
                point4     = pl_mul_mat4_vec4(&transform, point4);
                points[ii] = (plVec2){point4.x, point4.y};
            }

            // draw fill
            if (node->polygon.fill_enabled) {
                uint32_t fill_color = PL_COLOR_32_RGBA(
                    dc_app_lookup_get_value(data.lookup, node->polygon.fill_color.r)->value_double,
                    dc_app_lookup_get_value(data.lookup, node->polygon.fill_color.g)->value_double,
                    dc_app_lookup_get_value(data.lookup, node->polygon.fill_color.b)->value_double,
                    dc_app_lookup_get_value(data.lookup, node->polygon.fill_color.a)->value_double);
                _ext_draw->add_convex_polygon_filled(pl_app_data->layer, points, num_points, (plDrawSolidOptions){.uColor = fill_color});
            }

            // draw outline
            if (node->polygon.line_enabled) {
                float    lineThickness = (float)dc_app_lookup_get_value(data.lookup, node->polygon.line_width)->value_double;
                uint32_t line_color    = PL_COLOR_32_RGBA(
                    dc_app_lookup_get_value(data.lookup, node->polygon.line_color.r)->value_double,
                    dc_app_lookup_get_value(data.lookup, node->polygon.line_color.g)->value_double,
                    dc_app_lookup_get_value(data.lookup, node->polygon.line_color.b)->value_double,
                    dc_app_lookup_get_value(data.lookup, node->polygon.line_color.a)->value_double);
                _ext_draw->add_polygon(pl_app_data->layer, points, num_points, (plDrawLineOptions){.uColor = line_color, .fThickness = lineThickness});
            }
            break;
        }

        case NODE_TYPE_SET: {

            DcAppLookupVar *var       = dc_app_lookup_get_var(data.lookup, node->set.var_index);
            DcValue        *var_value = dc_app_lookup_get_value(data.lookup, var->value_index);
            DcValue        *op_value  = dc_app_lookup_get_value(data.lookup, node->set.operand);
            DcAppSetType    operation = (DcAppSetType)(dc_app_lookup_get_value(data.lookup, node->set.operation)->value_integer);

            // apply operation
            switch (operation) {
                case DC_APP_SET_TYPE_EQUAL:
                    switch (var_value->type) {
                        case DC_VALUE_TYPE_STRING:
                            var_value->value_string = op_value->value_string;
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
                            int num_chars = DC_VALUE_STRING_BUFFER_SIZE - strlen(var_value->value_string) - 1;
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
                            var_value->value_integer /= op_value->value_integer;
                            break;
                        case DC_VALUE_TYPE_DOUBLE:
                            var_value->value_double /= op_value->value_double;
                            break;
                        case DC_VALUE_TYPE_BOOLEAN:
                            var_value->value_boolean /= op_value->value_boolean;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    fprintf(stderr, "Invalid <Set> Operator value of enum %d\n", operation);
                    break;
            }

            // refresh variable
            dc_value_refresh(var_value);
            dc_app_lookup_refresh_var_from_value(data.lookup, node->set.var_index);
            break;
        }

        case NODE_TYPE_TERRAIN: {

            // all transform parameters
            float          position[2]    = {(float)dc_app_lookup_get_value(data.lookup, node->terrain.position.x)->value_double, (float)dc_app_lookup_get_value(data.lookup, node->terrain.position.y)->value_double};
            float          origin[2]      = {(float)dc_app_lookup_get_value(data.lookup, node->terrain.origin.x)->value_double, (float)dc_app_lookup_get_value(data.lookup, node->terrain.origin.y)->value_double};
            DcAppAlignType alignment[2]   = {(DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->terrain.local_align.x)->value_integer, (DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->terrain.local_align.y)->value_integer};
            DcAppAlignType pivot_align[2] = {(DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->terrain.pivot_align.x)->value_integer, (DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->terrain.pivot_align.y)->value_integer};
            float          rotation       = dc_app_lookup_get_value(data.lookup, node->terrain.rotation)->value_double;
            float          size[2]        = {(float)dc_app_lookup_get_value(data.lookup, node->terrain.dimension.x)->value_double, (float)dc_app_lookup_get_value(data.lookup, node->terrain.dimension.y)->value_double};

            // move position
            plMat4 trans_position_xform = pl_mat4_translate_xyz(
                position[0],
                position[1],
                0.0f);

            // move origin
            plMat4 trans_origin_xform = pl_mat4_translate_xyz(
                origin[0],
                origin[1],
                0.0f);

            // move alignment
            float trans_align_vec[2];
            switch (alignment[0]) {
                break;
                case DC_APP_ALIGN_TYPE_LEFT:
                    trans_align_vec[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    trans_align_vec[0] = -1 * size[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    trans_align_vec[0] = -1 * size[0];
                    break;
                default:
                    fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", alignment[0]);
                    break;
            }
            switch (alignment[1]) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    trans_align_vec[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    trans_align_vec[1] = -1 * size[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    trans_align_vec[1] = -1 * size[1];
                    break;
                default:
                    fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", alignment[1]);
                    break;
            }
            plMat4 trans_align_xform = pl_mat4_translate_xyz(
                trans_align_vec[0],
                trans_align_vec[1],
                0.0f);

            // move to pivot
            // either both pivot points need to be set, or neither
            bool  use_local_pivot = pivot_align[0] != DC_APP_ALIGN_TYPE_UNDEFINED;
            float trans_pivot_vec[2];
            if (use_local_pivot) {
                switch (pivot_align[0]) {
                    case DC_APP_ALIGN_TYPE_LEFT:
                        trans_pivot_vec[0] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_CENTER:
                        trans_pivot_vec[0] = -1 * size[0] / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_RIGHT:
                        trans_pivot_vec[0] = -1 * size[0];
                        break;
                    default:
                        fprintf(stderr, "Unknown pivot alignment in <Terrain> draw call: %d\n", pivot_align[0]);
                        break;
                }
                switch (pivot_align[1]) {
                    case DC_APP_ALIGN_TYPE_BOTTOM:
                        trans_pivot_vec[1] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_MIDDLE:
                        trans_pivot_vec[1] = -1 * size[1] / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_TOP:
                        trans_pivot_vec[1] = -1 * size[1];
                        break;
                    default:
                        fprintf(stderr, "Unknown pivot alignment in <Terrain> draw call: %d\n", pivot_align[1]);
                        break;
                }
            } else {
                float pivot_position_x = dc_app_lookup_get_value(data.lookup, node->terrain.pivot_position.x)->value_double;
                trans_pivot_vec[0]     = -1 * pivot_position_x;
                float pivot_position_y = dc_app_lookup_get_value(data.lookup, node->terrain.pivot_position.y)->value_double;
                trans_pivot_vec[1]     = -1 * pivot_position_y;
            }
            plMat4 trans_to_pivot_xform = pl_mat4_translate_xyz(
                trans_pivot_vec[0],
                trans_pivot_vec[1],
                0.0f);

            // rotate
            plMat4 rotate_xform = pl_mat4_rotate_vec3(
                pl_radiansf(rotation),
                (plVec3){0.0f, 0.0f, 1.0f});

            // reverse pivot move
            plMat4 trans_from_pivot_xform = pl_mat4_translate_xyz(
                -1 * trans_pivot_vec[0],
                -1 * trans_pivot_vec[1],
                0.0f);

            // compute transform
            plMat4 transform4 = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            if (!use_local_pivot) {
                transform4 = pl_mul_mat4t(&transform4, &trans_from_pivot_xform);
                transform4 = pl_mul_mat4t(&transform4, &rotate_xform);
                transform4 = pl_mul_mat4t(&transform4, &trans_to_pivot_xform);
            }
            transform4 = pl_mul_mat4t(&transform4, &trans_align_xform);
            transform4 = pl_mul_mat4t(&transform4, &trans_position_xform);
            transform4 = pl_mul_mat4t(&transform4, &trans_origin_xform);
            if (use_local_pivot) {
                transform4 = pl_mul_mat4t(&transform4, &trans_from_pivot_xform);
                transform4 = pl_mul_mat4t(&transform4, &rotate_xform);
                transform4 = pl_mul_mat4t(&transform4, &trans_to_pivot_xform);
            }
            transform4 = pl_mul_mat4t(parent_transform, &transform4);

            // convert to 3D matrix
            plMat3 transform3 = (plMat3){0};
            transform3.x11    = transform4.x11;
            transform3.x12    = transform4.x12;
            transform3.x13    = transform4.x14;
            transform3.x21    = transform4.x21;
            transform3.x22    = transform4.x22;
            transform3.x23    = transform4.x24;
            transform3.x31    = transform4.x31;
            transform3.x32    = transform4.x32;
            transform3.x33    = transform4.x33;

            // update text options
            // text_options.tTransform = transform3;

            // draw
            // ext_draw->add_text(app_data->layer, (plVec2){0, 0}, sb_text, text_options);
            break;
        }

        case NODE_TYPE_TEXT: {

            // expand text
            static char *sb_text = NULL;
            sbclear(sb_text);
            for (int ii = 0; ii < sbcount(node->text.sb_vals); ii++) {

                // filler
                char *filler = &(node->text.sb_fillers[node->text.sb_filler_indices[ii]]);
                sbpushn(sb_text, filler, strlen(filler));

                // value
                DcValueType format_type = node->text.sb_format_types[ii];
                char       *format      = &(node->text.sb_formats[node->text.sb_format_indices[ii]]);
                DcValue    *val         = dc_app_lookup_get_value(data.lookup, node->text.sb_vals[ii]);
                static char val_str[256]; // assume text won't be that long..
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
                        fprintf(stderr, "Uknown value type for text: %d\n", format_type);
                }
                sbpushn(sb_text, val_str, strlen(val_str));
            }

            // ending filler
            char *filler = &(node->text.sb_fillers[node->text.sb_filler_indices[sbcount(node->text.sb_vals)]]);
            sbpushn(sb_text, filler, strlen(filler));
            sbpush(sb_text, '\0');

            // get text dimensions
            plDrawTextOptions text_options = {0};
            text_options.ptFont            = pl_app_data->cousine_sdf_font;
            text_options.uColor            = PL_COLOR_32_RGBA(
                dc_app_lookup_get_value(data.lookup, node->text.fill_color.r)->value_double,
                dc_app_lookup_get_value(data.lookup, node->text.fill_color.g)->value_double,
                dc_app_lookup_get_value(data.lookup, node->text.fill_color.b)->value_double,
                dc_app_lookup_get_value(data.lookup, node->text.fill_color.a)->value_double);
            text_options.fSize = (float)dc_app_lookup_get_value(data.lookup, node->text.size)->value_double;
            plVec2 pl_size     = _ext_draw->calculate_text_size(sb_text, text_options);

            // all transform parameters
            float          position[2]    = {(float)(dc_app_lookup_get_value(data.lookup, node->text.position.x)->value_double), (float)(dc_app_lookup_get_value(data.lookup, node->text.position.y)->value_double)};
            float          origin[2]      = {(float)dc_app_lookup_get_value(data.lookup, node->text.origin.x)->value_double, (float)(dc_app_lookup_get_value(data.lookup, node->text.origin.y)->value_double)};
            DcAppAlignType alignment[2]   = {(DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->text.local_align.x)->value_integer, (DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->text.local_align.y)->value_integer};
            DcAppAlignType pivot_align[2] = {(DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->text.pivot_align.x)->value_integer, (DcAppAlignType)dc_app_lookup_get_value(data.lookup, node->text.pivot_align.y)->value_integer};
            float          rotation       = dc_app_lookup_get_value(data.lookup, node->text.rotation)->value_double;
            float          size[2]        = {pl_size.x, text_options.fSize};

            // local flip over x axis
            plMat4 scale_invert_y_xform = pl_mat4_scale_xyz(
                1.0f,
                -1.0f,
                1.0f);

            // move position
            plMat4 trans_position_xform = pl_mat4_translate_xyz(
                position[0],
                position[1],
                0.0f);

            // move from top-left reference to bottom-left
            plMat4 trans_pl_origin_xform = pl_mat4_translate_xyz(
                0,
                size[1],
                0.0f);

            // move origin
            plMat4 trans_origin_xform = pl_mat4_translate_xyz(
                origin[0],
                origin[1],
                0.0f);

            // move alignment
            float trans_align_vec[2];
            switch (alignment[0]) {
                break;
                case DC_APP_ALIGN_TYPE_LEFT:
                    trans_align_vec[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    trans_align_vec[0] = -1 * size[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    trans_align_vec[0] = -1 * size[0];
                    break;
                default:
                    fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", alignment[0]);
                    break;
            }
            switch (alignment[1]) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    trans_align_vec[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    trans_align_vec[1] = -1 * size[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    trans_align_vec[1] = -1 * size[1];
                    break;
                default:
                    fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", alignment[1]);
                    break;
            }
            plMat4 trans_align_xform = pl_mat4_translate_xyz(
                trans_align_vec[0],
                trans_align_vec[1],
                0.0f);

            // move to pivot
            // either both pivot points need to be set, or neither
            bool  use_local_pivot = pivot_align[0] != DC_APP_ALIGN_TYPE_UNDEFINED;
            float trans_pivot_vec[2];
            if (use_local_pivot) {
                switch (pivot_align[0]) {
                    case DC_APP_ALIGN_TYPE_LEFT:
                        trans_pivot_vec[0] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_CENTER:
                        trans_pivot_vec[0] = -1 * size[0] / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_RIGHT:
                        trans_pivot_vec[0] = -1 * size[0];
                        break;
                    default:
                        fprintf(stderr, "Unknown pivot alignment in <Text> draw call: %d\n", pivot_align[0]);
                        break;
                }
                switch (pivot_align[1]) {
                    case DC_APP_ALIGN_TYPE_BOTTOM:
                        trans_pivot_vec[1] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_MIDDLE:
                        trans_pivot_vec[1] = -1 * size[1] / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_TOP:
                        trans_pivot_vec[1] = -1 * size[1];
                        break;
                    default:
                        fprintf(stderr, "Unknown pivot alignment in <Text> draw call: %d\n", pivot_align[1]);
                        break;
                }
            } else {
                float pivot_position_x = (float)dc_app_lookup_get_value(data.lookup, node->text.pivot_position.x)->value_double;
                trans_pivot_vec[0]     = -1 * pivot_position_x;
                float pivot_position_y = (float)dc_app_lookup_get_value(data.lookup, node->text.pivot_position.y)->value_double;
                trans_pivot_vec[1]     = -1 * pivot_position_y;
            }
            plMat4 trans_to_pivot_xform = pl_mat4_translate_xyz(
                trans_pivot_vec[0],
                trans_pivot_vec[1],
                0.0f);

            // rotate
            plMat4 rotate_xform = pl_mat4_rotate_vec3(
                pl_radiansf(rotation),
                (plVec3){0.0f, 0.0f, 1.0f});

            // reverse pivot move
            plMat4 trans_from_pivot_xform = pl_mat4_translate_xyz(
                -1 * trans_pivot_vec[0],
                -1 * trans_pivot_vec[1],
                0.0f);

            // compute transform
            plMat4 transform4 = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            if (!use_local_pivot) {
                transform4 = pl_mul_mat4t(&transform4, &trans_from_pivot_xform);
                transform4 = pl_mul_mat4t(&transform4, &rotate_xform);
                transform4 = pl_mul_mat4t(&transform4, &trans_to_pivot_xform);
            }
            transform4 = pl_mul_mat4t(&transform4, &trans_align_xform);
            transform4 = pl_mul_mat4t(&transform4, &trans_position_xform);
            transform4 = pl_mul_mat4t(&transform4, &trans_origin_xform);
            if (use_local_pivot) {
                transform4 = pl_mul_mat4t(&transform4, &trans_from_pivot_xform);
                transform4 = pl_mul_mat4t(&transform4, &rotate_xform);
                transform4 = pl_mul_mat4t(&transform4, &trans_to_pivot_xform);
            }
            transform4 = pl_mul_mat4t(&transform4, &trans_pl_origin_xform);
            transform4 = pl_mul_mat4t(&transform4, &scale_invert_y_xform);
            transform4 = pl_mul_mat4t(parent_transform, &transform4);

            // convert to 3D matrix
            plMat3 transform3 = (plMat3){0};
            transform3.x11    = transform4.x11;
            transform3.x12    = transform4.x12;
            transform3.x13    = transform4.x14;
            transform3.x21    = transform4.x21;
            transform3.x22    = transform4.x22;
            transform3.x23    = transform4.x24;
            transform3.x31    = transform4.x31;
            transform3.x32    = transform4.x32;
            transform3.x33    = transform4.x33;

            // update text options
            text_options.tTransform = transform3;

            // draw
            _ext_draw->add_text(pl_app_data->layer, (plVec2){0, 0}, sb_text, text_options);

            break;
        }

        case NODE_TYPE_WINDOW: {
            // TODO move this code to only the resize() function

            // current dimensions
            uint32_t dimensionX, dimensionY;
            _ext_windows->get_size(pl_app_data->window, &dimensionX, &dimensionY);
            DcValue *dimension_value_x       = dc_app_lookup_get_value(data.lookup, node->window.dimension.x);
            DcValue *dimension_value_y       = dc_app_lookup_get_value(data.lookup, node->window.dimension.y);
            dimension_value_x->value_integer = (int)dimensionX;
            dimension_value_y->value_integer = (int)dimensionY;
            dc_value_refresh(dimension_value_x);
            dc_value_refresh(dimension_value_y);

            // all transform parameters
            float size[2]         = {(float)dc_app_lookup_get_value(data.lookup, node->window.dimension.x)->value_double, (float)dc_app_lookup_get_value(data.lookup, node->window.dimension.y)->value_double};
            float virtual_size[2] = {(float)dc_app_lookup_get_value(data.lookup, node->window.virtual_dimension.x)->value_double, (float)dc_app_lookup_get_value(data.lookup, node->window.virtual_dimension.y)->value_double};

            // compute transforms
            // translate from negative to positive range
            plMat4 trans_matrix = pl_mat4_translate_xyz(
                0.0f,
                size[1],
                0.0f);

            // scale from virtual to real dimensions, flip y axis
            plMat4 scale_matrix = pl_mat4_scale_xyz(
                size[0] / virtual_size[0],
                size[1] / virtual_size[1] * -1.0f,
                1.0f);

            plMat4 transform;
            transform = pl_mul_mat4t(&trans_matrix, &scale_matrix);

            plVec2 virtual_dimensions_vec2 = (plVec2){virtual_size[0], virtual_size[1]};
            _draw_node_list(pl_app_data, node->window.child, &virtual_dimensions_vec2, &transform);

            break;
        }

        default:
            break;
    }
}
