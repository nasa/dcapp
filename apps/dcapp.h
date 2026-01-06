#ifndef _DCAPP_H_
#define _DCAPP_H_

// PL includes
#define PL_EXPERIMENTAL
#include "../pilotlight/src/pl.h"
#define PL_MATH_INCLUDE_FUNCTIONS
#include "../pilotlight/libs/pl_math.h"

// PL extension includes
#include "../pilotlight/extensions/pl_camera_ext.h"
#include "../pilotlight/extensions/pl_profile_ext.h"
#include "../pilotlight/extensions/pl_starter_ext.h"
#include "../pilotlight/extensions/pl_graphics_ext.h"
#include "../pilotlight/extensions/pl_vfs_ext.h"
#include "../pilotlight/extensions/pl_shader_ext.h"
#include "../pilotlight/extensions/pl_image_ext.h"

// dcapp extension includes
#include "../extensions/dc_draw_ext.h"
#include "../extensions/dc_draw_backend_ext.h"
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

// dcapp includes
#include "../src/utils/stb_sb.h"
#include "../src/app/elem.h"
#include "../src/app/enums.h"
#include "../src/app/lookup.h"
#include "../src/app/config.h"
#include "../src/pixelstream/mjpeg.h"
#include "../src/trick.h"
#include <libxml/parser.h>

// dcapp node structs
typedef DcAppValIndex _ValIndex1;

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
    NODE_TYPE_BLINK,
    NODE_TYPE_BUTTON,
    NODE_TYPE_CIRCLE,
    NODE_TYPE_CONTAINER,
    NODE_TYPE_CONDITIONAL,
    NODE_TYPE_IMAGE,
    NODE_TYPE_LINE,
    NODE_TYPE_MOUSE_MOTION,
    NODE_TYPE_PANEL,
    NODE_TYPE_PIXELSTREAM,
    NODE_TYPE_POLYGON,
    NODE_TYPE_RECTANGLE,
    NODE_TYPE_SET,
    NODE_TYPE_SPHERE,
    NODE_TYPE_STENCIL,
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

typedef struct __NodeBlink {
    DcAppValIndex frequency;
    DcAppValIndex duty_cycle;
    DcAppValIndex duration;
    DcAppVarIndex var;
    _NodeIndex    child;

    // runtime state
    double remaining_duration;
    double last_frame_time;
    int    last_trigger_value;
} _NodeBlink;

typedef struct __NodeButton {

    // standard transforms
    _ValIndex2    position;
    _ValIndex2    dimension;
    _ValIndex2    virtual_dimension;
    _ValIndex2    pivot_local_align;
    _ValIndex2    pivot_position;
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    DcAppValIndex rotation;

    // node indices drawn for a given state
    _NodeIndex child_enabled;
    _NodeIndex child_disabled;
    _NodeIndex child_indicator_on;
    _NodeIndex child_indicator_off;
    _NodeIndex child_pressed;
    _NodeIndex child_released;
    _NodeIndex child_transition;
    _NodeIndex child; // default children, not attached to any events

    // comparison values for each state
    DcAppValIndex val_enabled_on;
    DcAppValIndex val_target_on;
    DcAppValIndex val_target_off;
    DcAppValIndex val_indicator_on;

    // variable indices to be set for each state
    DcAppVarIndex var_enabled;
    DcAppVarIndex var_target;
    DcAppVarIndex var_indicator;

    // manage transitions (off -> on, on -> off, stable)
    // (don't think I need this)
    // * can easily check if var_target.val != var_indicator.val,
    // * and if var_target.val == val_target_on, then it's transitioning to ON
    // * opposite for transitioning to OFF
    // int transition_state;

    // type for button
    DcAppButtonType type;
} _NodeButton;

#define _NODE_CIRCLE_MAX_SEGMENTS 1000
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

typedef int _TextureIndex;
#define TEXTURE_INDEX_UNDEFINED -1
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

    _TextureIndex texture_index;

    _MouseEventChildren mouse_events;
} _NodeImage;

#define _NODE_LINE_MAX_POINTS 1000
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

typedef struct __NodeMouseMotion {
    DcAppVarIndex var_x;
    DcAppVarIndex var_y;
} _NodeMouseMotion;

typedef struct __NodePanel {
    _ValIndex2    parent_dimension;
    _ValIndex2    virtual_dimension;
    DcAppValIndex index;
    _NodeIndex    child;
} _NodePanel;

#define _NODE_PIXELSTREAM_MAX_WIDTH 3840
#define _NODE_PIXELSTREAM_MAX_HEIGHT 2160
typedef struct __PixelstreamMjpegData {
    DcPsMjpegHandle handle;
    unsigned char  *raw_jpeg;
    size_t          raw_jpeg_size;
} _PixelstreamMjpegData;

typedef struct __NodePixelstream {
    _ValIndex2    position;
    _ValIndex2    dimension;
    _ValIndex2    pivot_local_align;
    _ValIndex2    pivot_position;
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    DcAppValIndex rotation;

    _TextureIndex       texture_index;
    _MouseEventChildren mouse_events;

    DcAppPixelstreamType type;
    unsigned char       *frame;
    int                  frame_width;
    int                  frame_height;
    union {
        _PixelstreamMjpegData mjpeg;
    };
} _NodePixelstream;

#define _NODE_POLYGON_MAX_POINTS 1000
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

typedef struct __NodeSphere {
    // 2D positioning (where to draw in the orthographic view)
    _ValIndex2    position;
    _ValIndex2    pivot_local_align;
    _ValIndex2    pivot_position;
    _ValIndex2    local_align;
    _ValIndex2    parent_align;
    DcAppValIndex rotation; // external 2D rotation in orthographic view

    // sphere properties
    DcAppValIndex radius;
    _ValIndex4    fill_color;

    // internal rotation (roll, pitch, yaw of the sphere itself)
    _ValIndex3 rpy;

    // optional texture
    _TextureIndex texture_index;
} _NodeSphere;

typedef enum __StencilChildType {
    STENCIL_CHILD_TYPE_UNDEFINED,
    STENCIL_CHILD_TYPE_ADD,
    STENCIL_CHILD_TYPE_REMOVE,
    STENCIL_CHILD_TYPE_DRAW,
} _StencilChildType;

typedef struct __StencilChild {
    _NodeIndex        child;
    _StencilChildType type;
} _StencilChild;

typedef struct __NodeStencil {
    _StencilChild *sb_children;
} _NodeStencil;

#define _NODE_TEXT_MAX_LINES 256
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
        _NodeBlink       blink;
        _NodeButton      button;
        _NodeCircle      circle;
        _NodeConditional conditional;
        _NodeContainer   container;
        _NodeImage       image;
        _NodeLine        line;
        _NodeMouseMotion mouse_motion;
        _NodePanel       panel;
        _NodePixelstream pixelstream;
        _NodePolygon     polygon;
        _NodeRectangle   rectangle;
        _NodeSet         set;
        _NodeSphere      sphere;
        _NodeStencil     stencil;
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

// callback used for logic file DLL loading
typedef void *(*_GetVariableValueAddr)(const char *name);

// draw batch types
typedef enum __DrawBatchType {
    DRAW_BATCH_TYPE_UNDEFINED,
    DRAW_BATCH_TYPE_2D,
    DRAW_BATCH_TYPE_3D,
} _DrawBatchType;

typedef struct __DrawList2D {
    plDrawList2D  *draw_list;
    plDrawLayer2D *layer;
} _DrawList2D;

typedef struct __DrawBatch {
    _DrawBatchType type;
    union {
        _DrawList2D   draw_list_2d;
        plDrawList3D *draw_list_3d;
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

// app data
typedef struct __AppData {

    // pl things
    plWindow      *pl_window;
    plDrawLayer2D *pl_layer;
    plDrawList2D  *pl_draw_list;
    plFont        *pl_vera_sdf_font;

    // staging buffer
    plBufferHandle pl_staging_buffer_handle;
    size_t         pl_staging_buffer_size;

    // stencil shaders (2D)
    plShaderHandle stencil_create_2d_shader;
    plShaderHandle stencil_remove_2d_shader;
    plShaderHandle stencil_draw_2d_shader;

    // stencil shaders (SDF)
    plShaderHandle stencil_create_sdf_shader;
    plShaderHandle stencil_remove_sdf_shader;
    plShaderHandle stencil_draw_sdf_shader;

    // config + lookup
    DcAppLookup *lookup;
    DcAppConfig *config;

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

    // frame data
    _FrameData frame_data;

    // draw batch system
    _DrawBatch    *sb_draw_batches;      // batches for current frame (cleared each frame)
    _DrawList2D   *sb_draw_list_2d_pool; // pool of 2D draw list + layer pairs (from extension)
    plDrawList3D **sb_draw_list_3d_pool; // pool of 3D draw list pointers (from extension)
    int            draw_list_2d_index;   // current index into 2D pool
    int            draw_list_3d_index;   // current index into 3D pool

} _AppData;

// pl utils
static void _init_app_data(_AppData *app_data, _Node *window_node);

// node utils
static const char *_node_type_to_string(_NodeType type);
static _Node      *_get_node(_AppData *app_data, _NodeIndex index);
static _NodeIndex  _register_node(_AppData *app_data, _Node *node);

// misc. utils
static _Texture _create_texture(_AppData *app_data, uint32_t texture_width, uint32_t texture_height, const char *texture_name);

// xml processing utils
static bool       _load_color_from_string(_AppData *app_data, xmlNodePtr xml_node, const char *attr_name, _ValIndex4 *color_out);
static _NodeIndex _process_xml_node_children(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex node_index, DcAppElemType elem_type, const char *directory);
static _NodeIndex _process_xml_node(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);

// drawing utils
static void _draw_init(void);
static void _draw_batch_reset(_AppData *app_data);
static void _draw_node_list(_AppData *app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *node_transform);
static void _draw_node(_AppData *app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _init_stencil_pipelines(_AppData* app_data, plDevice* device, plRenderPassHandle render_pass);

#endif
