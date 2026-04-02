#ifndef _DCAPP_H_
#define _DCAPP_H_

// PL includes
#define PL_EXPERIMENTAL
#include "pl.h"
#define PL_MATH_INCLUDE_FUNCTIONS
#include "pl_math.h"

// PL extension includes
#include "pl_camera_ext.h"
#include "pl_profile_ext.h"
#include "pl_starter_ext.h"
#include "pl_graphics_ext.h"
#include "pl_gpu_allocators_ext.h"
#include "pl_vfs_ext.h"
#include "pl_shader_ext.h"
#include "pl_image_ext.h"
#include "pl_resource_ext.h"
#include "pl_draw_ext.h"
#include "pl_screen_log_ext.h"

// dcapp extension includes
#include "dc_draw_ext.h"
#include "dc_draw_backend_ext.h"
#include "pl_planet_ext.h"
#include "pl_planet_processor_ext.h"

// general includes
#include <float.h>
#include <string.h>

// PL macros
#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_REALLOC(x, y) _ext_memory->tracked_realloc((x), (y), __FILE__, __LINE__)
#define PL_FREE(x) _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

// PL extensions
const plWindowI          *_ext_windows          = NULL;
const plStarterI         *_ext_starter          = NULL;
const plProfileI         *_ext_profile          = NULL;
const plMemoryI          *_ext_memory           = NULL;
const plLibraryI         *_ext_library          = NULL;
const plIOI              *_ext_ioi              = NULL;
const plGraphicsI        *_ext_gfx              = NULL;
const plGPUAllocatorsI   *_ext_gpu_allocators   = NULL;
const plPlanetI          *_ext_planet           = NULL;
const plPlanetProcessorI *_ext_planet_processor = NULL;
const plVfsI             *_ext_vfs              = NULL;
const plShaderI          *_ext_shader           = NULL;
const plCameraI          *_ext_camera           = NULL;
const plImageI           *_ext_image            = NULL;
const plResourceI        *_ext_resource         = NULL;
const plDrawI            *_ext_draw             = NULL;
const dcDrawI            *_ext_dc_draw          = NULL;
const dcDrawBackendI     *_ext_dc_draw_backend  = NULL;
const plScreenLogI       *_ext_screen_log       = NULL;

// dcapp includes
#include "utils/stb_sb.h"
#include "app/elem.h"
#include "app/enums.h"
#include "app/lookup.h"
#include "app/config.h"
#include "pixelstream/mjpeg.h"
#include "pixelstream/shmem.h"
#include "trick.h"
#include "edge.h"
#include <libxml/parser.h>

// dcapp node structs
#include "node.h"

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
    DcTrickHandle       trick;
    _TrickTxVarContext *sb_tx_var_contexts;
    _TrickRxVarContext *sb_rx_var_contexts;
    DcAppVarIndex       connected_var_index; // variable updated with connection status
    bool                was_connected;       // previous connection state for init-on-connect
} _TrickContext;

// dcapp edge structs
typedef struct __EdgeTxVarContext {
    DcAppVarIndex  dcapp_var_index;
    DcEdgeVarIndex edge_var_index;
    DcValue        prev_value;
} _EdgeTxVarContext;

typedef struct __EdgeRxVarContext {
    DcAppVarIndex  dcapp_var_index;
    DcEdgeVarIndex edge_var_index;
} _EdgeRxVarContext;

typedef struct __EdgeContext {
    DcEdgeHandle       edge;
    _EdgeTxVarContext *sb_tx_var_contexts;
    _EdgeRxVarContext *sb_rx_var_contexts;
    DcAppVarIndex      connected_var_index; // variable updated with connection status
    bool               was_connected;       // previous connection state for disconnect detection
} _EdgeContext;

// callback used for logic file DLL loading
typedef void *(*_GetVariableValueAddr)(const char *name);

// draw batch types
typedef enum __DrawBatchType {
    DRAW_BATCH_TYPE_UNDEFINED,
    DRAW_BATCH_TYPE_2D,
    DRAW_BATCH_TYPE_3D,
} _DrawBatchType;

typedef struct __DrawList2D {
    dcDrawList2D  *draw_list;
    dcDrawLayer2D *layer;
} _DrawList2D;

typedef struct __DrawBatch {
    _DrawBatchType type;
    union {
        _DrawList2D   draw_list_2d;
        dcDrawList3D *draw_list_3d;
    };
} _DrawBatch;

// frame data
typedef struct __FrameData {

    // frame count
    unsigned long long count;

    // triggered once
    bool is_mouse_pressed;
    bool is_mouse_released;

    // continuous
    bool is_mouse_down;

    // raw position in window
    bool   is_mouse_position_valid;
    plVec2 mouse_position;

    // node clicks
    _NodeIndex pressed_node;
    _NodeIndex next_pressed_node;
    _NodeIndex hovered_node;
    _NodeIndex next_hovered_node;
    _NodeIndex released_node;
    _NodeIndex active_node;

} _FrameData;

// multi-resolution SDF font levels
#define FONT_LEVEL_COUNT 3
#define FONT_LEVEL_SMALL  0
#define FONT_LEVEL_MEDIUM 1
#define FONT_LEVEL_LARGE  2
static const float FONT_LEVEL_SIZES[FONT_LEVEL_COUNT] = {13.0f, 25.0f, 50.0f};

typedef struct __FontLevels {
    dcFont *levels[FONT_LEVEL_COUNT];
} _FontLevels;

// app data
typedef struct __AppData {

    // pl things
    plWindow      *pl_window;
    dcDrawLayer2D *pl_layer;
    dcDrawList2D  *pl_draw_list;
    dcFont        *pl_vera_sdf_font;

    // GPU memory allocators
    plDeviceMemoryAllocatorI *gpu_local_dedicated_allocator;
    plDeviceMemoryAllocatorI *gpu_local_buddy_allocator;
    plDeviceMemoryAllocatorI *gpu_staging_uncached_allocator;

    // staging buffer
    plBufferHandle pl_staging_buffer_handle;
    size_t         pl_staging_buffer_size;

    // stencil shaders (2D)
    plShaderHandle stencil_create_2d_shader;
    plShaderHandle stencil_remove_2d_shader;
    plShaderHandle stencil_draw_2d_shader[DC_STENCIL_MAX_DEPTH];
    plShaderHandle stencil_cleanup_2d_shader;

    // stencil shaders (SDF)
    plShaderHandle stencil_create_sdf_shader;
    plShaderHandle stencil_remove_sdf_shader;
    plShaderHandle stencil_draw_sdf_shader[DC_STENCIL_MAX_DEPTH];
    plShaderHandle stencil_cleanup_sdf_shader;

    // stencil shaders (3D solid)
    plShaderHandle stencil_create_3d_solid_shader;
    plShaderHandle stencil_remove_3d_solid_shader;
    plShaderHandle stencil_draw_3d_solid_shader[DC_STENCIL_MAX_DEPTH];
    plShaderHandle stencil_cleanup_3d_solid_shader;

    // stencil shaders (3D textured)
    plShaderHandle stencil_create_3d_textured_shader;
    plShaderHandle stencil_remove_3d_textured_shader;
    plShaderHandle stencil_draw_3d_textured_shader[DC_STENCIL_MAX_DEPTH];
    plShaderHandle stencil_cleanup_3d_textured_shader;

    // active stencil shader overrides (injected by _draw_batch_get_*)
    plShaderHandle *active_2d_shader_override;
    plShaderHandle *active_sdf_shader_override;
    plShaderHandle *active_3d_solid_shader_override;
    plShaderHandle *active_3d_textured_shader_override;
    bool            stencil_2d_dirty;
    bool            stencil_3d_dirty;

    // stencil state
    int stencil_depth;

    // config + lookup
    DcAppLookup *lookup;
    DcAppConfig *config;

    // fonts (custom TTF fonts, collected during XML parse, loaded during init)
    char    *sb_font_paths;        // stretchy buffer of null-terminated path strings
    int     *sb_font_path_offsets; // offset into sb_font_paths for each font
    dcFont **sb_fonts;             // loaded font pointers (populated during init)

    _FontLevels *sb_font_levels; // multi-res SDF levels, parallel to sb_fonts

    // textures
    char     *sb_texture_names;
    int      *sb_texture_name_offsets;
    _Texture *sb_textures;

    // nodes
    _Node     *sb_nodes;
    _NodeIndex window;

    // logic
    plSharedLibrary *logic_lib;
    void (*logic_pre_init)(_GetVariableValueAddr);
    void (*logic_init)();
    void (*logic_draw)();
    void (*logic_close)();

    // trick
    _TrickContext *sb_tricks;

    // edge
    _EdgeContext *sb_edges;

    // frame data
    _FrameData frame_data;

    // deferred set operations (flushed after draw pass)
    _DeferredSetOp *sb_deferred_sets;

    // draw batch system
    _DrawBatch    *sb_draw_batches;      // batches for current frame (cleared each frame)
    _DrawList2D   *sb_draw_list_2d_pool; // pool of 2D draw list + layer pairs (from extension)
    dcDrawList3D **sb_draw_list_3d_pool; // pool of 3D draw list pointers (from extension)
    int            draw_list_2d_index;   // current index into 2D pool
    int            draw_list_3d_index;   // current index into 3D pool

    // pixelstream sources (unique, deduplicated by source key during XML parse)
    _PixelstreamSource *sb_ps_sources;
    char               *sb_ps_source_keys;        // stretchy buffer of null-terminated key strings
    int                *sb_ps_source_key_offsets;  // offset into sb_ps_source_keys for each source

    // planet instances
    _PlanetDef    *sb_planet_defs;              // collected during XML parse (top-level Planet definitions)
    _NodeIndex    *sb_planet_view_node_indices; // collected PlanetView nodes for init
    plPlanet     **sb_planets;                  // created planet instances (one per PlanetDef)
    plPlanetView **sb_planet_views;             // created view instances (one per PlanetView element)
    bool           planet_ext_initialized;      // true after _ext_planet->initialize() called

} _AppData;

// shared node utils
static _Node *_get_node(_AppData *app_data, _NodeIndex index) {
    if (index == NODE_INDEX_UNDEFINED) {
        return NULL;
    }
    return &(app_data->sb_nodes[index]);
}

// xml processing utils
static bool       _load_color_from_string(_AppData *app_data, xmlNodePtr xml_node, const char *attr_name, _ValIndex4 *color_out);
static _NodeIndex _process_xml_node_children(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex node_index, DcAppElemType elem_type, const char *directory);
static _NodeIndex _process_xml_node(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);

// drawing utils
static void _draw_batch_reset(_AppData *app_data);
static void _draw_node_list(_AppData *app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *node_transform);
static void _draw_node(_AppData *app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);

#endif
