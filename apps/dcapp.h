#ifndef _DCAPP_H_
#define _DCAPP_H_

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
#include "../pilotlight/extensions/pl_image_ext.h"
#include "../extensions/pl_terrain_ext.h"

// general includes
#include <float.h>
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
const plImageI       *_ext_image        = NULL;

// PL app data
typedef struct __PlAppData {

    plWindow      *window;
    plDrawLayer2D *layer;
    plDrawList2D  *draw_list;
    plFont        *cousine_sdf_font;

    plBufferHandle staging_buffer_handle;
    size_t         staging_buffer_size;

} _PlAppData;

// PL functions
PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, _PlAppData *pl_app_data);
PL_EXPORT void  pl_app_shutdown(_PlAppData *pl_app_data);
PL_EXPORT void  pl_app_resize(_PlAppData *pl_app_data);
PL_EXPORT void  pl_app_update(_PlAppData *pl_app_data);

// dcapp includes
#include "../src/utils/stb_sb.h"
#include "../src/app/elem.h"
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
    NODE_TYPE_CIRCLE,
    NODE_TYPE_CONTAINER,
    NODE_TYPE_CONDITIONAL,
    NODE_TYPE_IMAGE,
    NODE_TYPE_LINE,
    NODE_TYPE_PANEL,
    NODE_TYPE_POLYGON,
    NODE_TYPE_RECTANGLE,
    NODE_TYPE_SET,
    NODE_TYPE_TERRAIN,
    NODE_TYPE_TEXT,
    NODE_TYPE_WINDOW,
    NODE_TYPE__COUNT,
    NODE_TYPE__MAX = NODE_TYPE__COUNT - 1,
} _NodeType;

typedef int      _NodeIndex;
const _NodeIndex NODE_INDEX_UNDEFINED = -1;

typedef struct __MouseEventChildren {
    _NodeIndex active;
    _NodeIndex hovered;
    _NodeIndex inactive;
    _NodeIndex pressed;
    _NodeIndex released;
    bool       enabled;
} _MouseEventChildren;

static const int _NODE_CIRCLE_MAX_SEGMENTS = 1000;
typedef struct __NodeCircle {
    _ValIndex2    position;
    _ValIndex2    pivot_local_align;
    _ValIndex2    pivot_position;
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    DcAppValIndex rotation;
    DcAppValIndex radius;
    DcAppValIndex num_segments;
    _ValIndex4    fill_color;
    _ValIndex4    line_color;
    DcAppValIndex line_width;

    _MouseEventChildren mouse_events;

    bool fill_enabled;
    bool line_enabled;
} _NodeCircle;

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

typedef int                _TextureIndex;
static const _TextureIndex TEXTURE_INDEX_UNDEFINED = -1;
typedef struct __Texture {
    plTextureHandle   texture_handle;
    plBindGroupHandle bind_group_handle;
} _Texture;
typedef struct __NodeImage {
    _ValIndex2    position;
    _ValIndex2    dimension;
    _ValIndex2    pivot_local_align;
    _ValIndex2    pivot_position;
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    DcAppValIndex rotation;

    _Texture texture;

    _MouseEventChildren mouse_events;
} _NodeImage;

static const int _NODE_LINE_MAX_POINTS = 1000;
typedef struct __NodeLine {
    _ValIndex2    position;
    _ValIndex2    pivot_position;
    DcAppValIndex rotation;
    _ValIndex4    line_color;
    DcAppValIndex line_width;

    _ValIndex2 *sb_points;
    bool        fill_enabled;
    bool        line_enabled;
} _NodeLine;

typedef struct __NodePanel {
    _ValIndex2    parent_dimension;
    _ValIndex2    virtual_dimension;
    DcAppValIndex index;
    _NodeIndex    child;
} _NodePanel;

static const int _NODE_POLYGON_MAX_POINTS = 1000;
typedef struct __NodePolygon {
    _ValIndex2    position;
    _ValIndex2    pivot_position;
    DcAppValIndex rotation;
    _ValIndex4    fill_color;
    _ValIndex4    line_color;
    DcAppValIndex line_width;

    _MouseEventChildren mouse_events;

    _ValIndex2 *sb_points;
    bool        fill_enabled;
    bool        line_enabled;
} _NodePolygon;

typedef struct __NodeRectangle {
    _ValIndex2    position;
    _ValIndex2    dimension;
    _ValIndex2    pivot_local_align;
    _ValIndex2    pivot_position;
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    DcAppValIndex rotation;
    _ValIndex4    fill_color;
    _ValIndex4    line_color;
    DcAppValIndex line_width;

    _MouseEventChildren mouse_events;

    bool fill_enabled;
    bool line_enabled;
} _NodeRectangle;

typedef struct __NodeSet {
    DcAppVarIndex var_index;
    DcAppValIndex operation; // because operator was taken :(
    DcAppValIndex operand;
} _NodeSet;

typedef struct __NodeText {
    _ValIndex2    position;
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    _ValIndex2    pivot_local_align;
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
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    _ValIndex2    pivot_position;
    _ValIndex2    pivot_local_align;
    DcAppValIndex rotation;

    // terrain specific
    _ValIndex3 lle;
    _ValIndex3 rpy;
    uint8_t    terrain_index;

} _NodeTerrain;

typedef struct __NodeWindow {
    plVec2     init_position;
    plVec2     init_dimension;
    _ValIndex2 virtual_dimension;
    _NodeIndex child;
    char      *title;
} _NodeWindow;

typedef struct __Node {
    _NodeType  type;
    _NodeIndex parent;
    _NodeIndex next;
    union {
        _NodeCircle      circle;
        _NodeConditional conditional;
        _NodeContainer   container;
        _NodeImage       image;
        _NodeLine        line;
        _NodePanel       panel;
        _NodePolygon     polygon;
        _NodeRectangle   rectangle;
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

// node utils
static const char *_node_type_to_string(_NodeType type);
static _Node      *_get_node(_NodeIndex index);
static _NodeIndex  _register_node(_Node *node);

// pl utils
static void _init_pl_app_data(_PlAppData *pl_app_data, _Node *window_node);

// draw utils
static bool       _load_color_from_string(xmlNodePtr xml_node, const char *attr_name, _ValIndex4 *color_out);
static _NodeIndex _process_xml_node_children(_PlAppData *pl_app_data, xmlNodePtr xml_node, _NodeIndex node_index, DcAppElemType elem_type, const char *directory);
static _NodeIndex _process_xml_node(_PlAppData *pl_app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static void       _draw_node_list(_PlAppData *pl_app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *node_transform);
static void       _draw_node(_PlAppData *pl_app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);

// globals
static char      *_sb_texture_names;
static char      *_sb_texture_name_offsets;
static _Texture  *_sb_textures;
static _FrameData _frame_data;
static _DcAppData _dc_data;

#endif
