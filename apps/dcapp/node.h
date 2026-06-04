#ifndef _DCAPP_NODE_H_
#define _DCAPP_NODE_H_

#include "pl.h"
#include "pl_math.h"
#include "pl_graphics_ext.h"
#include "pl_planet_ext.h"
#include "app/lookup.h"
#include "app/enums.h"
#include "pixelstream/mjpeg.h"
#include "pixelstream/shmem.h"
#include "dc_draw_ext.h"

typedef struct _DcAppDrawContext DcAppDrawContext;
typedef struct _DcAppDrawFuncArgs DcAppDrawFuncArgs;

// value index types
typedef DcAppValIndex _ValIndex;

typedef struct __ValIndex2 {
    union {
        _ValIndex x, r, lat, roll;
    };
    union {
        _ValIndex y, g, lon, pitch;
    };
} _ValIndex2;

typedef struct __ValIndex3 {
    union {
        _ValIndex x, r, lat, roll;
    };
    union {
        _ValIndex y, g, lon, pitch;
    };
    union {
        _ValIndex z, b, ele, yaw;
    };
} _ValIndex3;

typedef struct __ValIndex4 {
    union {
        _ValIndex x, r;
    };
    union {
        _ValIndex y, g;
    };
    union {
        _ValIndex z, b;
    };
    union {
        _ValIndex w, a;
    };
} _ValIndex4;

// Vertex data for Line and Polygon elements
typedef struct __VertexData {
    _ValIndex2 position;
    _ValIndex2 parent_align;
    _ValIndex  negate_x;
    _ValIndex  negate_y;
} _VertexData;

typedef enum __NodeType {
    NODE_TYPE_UNDEFINED,
    NODE_TYPE_ARC,
    NODE_TYPE_BLINK,
    NODE_TYPE_BUTTON,
    NODE_TYPE_CONTAINER,
    NODE_TYPE_ELLIPSE,
    NODE_TYPE_CONDITIONAL,
    NODE_TYPE_DRAW_FUNCTION,
    NODE_TYPE_FUNCTION,
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
    NODE_TYPE_PLANET_ELLIPSE,
    NODE_TYPE_PLANET_LINE,
    NODE_TYPE_PLANET_POLYGON,
    NODE_TYPE_PLANET_SPHERE,
    NODE_TYPE_PLANET_TEXT,
    NODE_TYPE_PLANET_VIEW,
    NODE_TYPE_TEXT,
    NODE_TYPE_WINDOW,

    // state-conditional container nodes (check parent's state_flags)
    NODE_TYPE_STATE_BUTTON_ENABLED,
    NODE_TYPE_STATE_BUTTON_DISABLED,
    NODE_TYPE_STATE_BUTTON_INDICATOR_ON,
    NODE_TYPE_STATE_BUTTON_INDICATOR_OFF,
    NODE_TYPE_STATE_BUTTON_TRANSITION,
    NODE_TYPE_STATE_MOUSE_PRESSED,
    NODE_TYPE_STATE_MOUSE_RELEASED,
    NODE_TYPE_STATE_MOUSE_ACTIVE,
    NODE_TYPE_STATE_MOUSE_INACTIVE,
    NODE_TYPE_STATE_MOUSE_HOVERED,
    NODE_TYPE_STATE_IF_TRUE,
    NODE_TYPE_STATE_IF_FALSE,

    NODE_TYPE__COUNT,
    NODE_TYPE__MAX = NODE_TYPE__COUNT - 1,
} _NodeType;

// State flags that parents set for conditional child nodes to check
typedef enum __NodeStateFlags {
    NODE_STATE_FLAG_NONE          = 0,
    NODE_STATE_FLAG_ENABLED       = 1 << 0,
    NODE_STATE_FLAG_INDICATOR_ON  = 1 << 1,
    NODE_STATE_FLAG_TRANSITIONING = 1 << 2,
    NODE_STATE_FLAG_PRESSED       = 1 << 3,
    NODE_STATE_FLAG_ACTIVE        = 1 << 4,
    NODE_STATE_FLAG_HOVERED       = 1 << 5,
    NODE_STATE_FLAG_RELEASED      = 1 << 6,
    NODE_STATE_FLAG_TRUE          = 1 << 7,
    NODE_STATE_FLAG_FALSE         = 1 << 8,
} _NodeStateFlags;

// Config flags set at parse time (never change at runtime)
typedef enum __NodeConfigFlags {
    NODE_CONFIG_FLAG_NONE               = 0,
    NODE_CONFIG_FLAG_FILL_ENABLED       = 1 << 0,
    NODE_CONFIG_FLAG_LINE_ENABLED       = 1 << 1,
    NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS = 1 << 2,
    NODE_CONFIG_FLAG_BACKGROUND_ENABLED = 1 << 3,
} _NodeConfigFlags;

typedef int      _NodeIndex;
const _NodeIndex NODE_INDEX_UNDEFINED = 0;
const _NodeIndex NODE_FIRST_INDEX     = 1;

typedef struct __NodeArc {
    _ValIndex2 position;
    _ValIndex2 pivot_local_align;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_position;
    _ValIndex2 local_align;
    _ValIndex2 parent_align;
    _ValIndex  rotation; // where the arc starts (0 = right, 90 = top)
    _ValIndex  radius;
    _ValIndex  angle; // span of the arc in degrees
    _ValIndex  num_segments;
    _ValIndex4 line_color;
    _ValIndex  line_width;
    _ValIndex  line_pattern;
    _ValIndex  negate_x;
    _ValIndex  negate_y;
} _NodeArc;

typedef struct __NodeBlink {
    _ValIndex  frequency;
    _ValIndex  duty_cycle;
    _ValIndex  duration;
    _ValIndex  fire_blink;
    _NodeIndex child;

    // runtime state
    double  remaining_duration;
    double  last_frame_time;
    DcValue last_fire_blink_value;
} _NodeBlink;

// State event node (children drawn when parent state matches)
typedef struct __NodeStateEvent {
    _NodeIndex child;
} _NodeStateEvent;

typedef struct __NodeButton {

    // standard transforms
    _ValIndex2 position;
    _ValIndex2 dimension;
    _ValIndex2 virtual_dimension;
    _ValIndex2 pivot_local_align;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_position;
    _ValIndex2 local_align;
    _ValIndex2 parent_align;
    _ValIndex  rotation;
    _ValIndex  negate_x;
    _ValIndex  negate_y;

    // children (regular child nodes, including state-conditional nodes)
    _NodeIndex child;

    // state flags for conditional children to check
    uint32_t state_flags;

    // comparison values for each state
    _ValIndex val_enabled_on;
    _ValIndex val_target_on;
    _ValIndex val_target_off;
    _ValIndex val_indicator_on;

    // variable indices to be set for each state
    DcAppVarIndex var_enabled;
    DcAppVarIndex var_target;
    DcAppVarIndex var_indicator;

    // type for button
    DcAppButtonType type;
} _NodeButton;

#define _NODE_ELLIPSE_MAX_SEGMENTS 1000
typedef struct __NodeEllipse {
    _ValIndex2 position;
    _ValIndex2 pivot_local_align;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_position;
    _ValIndex2 local_align;
    _ValIndex2 parent_align;
    _ValIndex  rotation; // where the wedge starts (0 = right, 90 = top)
    _ValIndex  angle;    // span of the wedge in degrees (360 = full ellipse)
    _ValIndex  radius_x;
    _ValIndex  radius_y;
    _ValIndex  num_segments;
    _ValIndex4 fill_color;
    _ValIndex4 line_color;
    _ValIndex  line_width;
    _ValIndex  line_pattern;

    _NodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;

    _ValIndex negate_x;
    _ValIndex negate_y;
} _NodeEllipse;

typedef struct __NodeConditional {
    _ValIndex  type;
    _ValIndex  value1;
    _ValIndex  value2;
    _NodeIndex child;
    uint32_t   state_flags;
} _NodeConditional;

typedef struct __NodeContainer {
    _ValIndex2 position;
    _ValIndex2 dimension;
    _ValIndex2 virtual_dimension;
    _ValIndex2 pivot_local_align;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_position;
    _ValIndex2 local_align;
    _ValIndex2 parent_align;
    _ValIndex  rotation;
    _ValIndex  negate_x;
    _ValIndex  negate_y;
    _NodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;
} _NodeContainer;

typedef int _TextureIndex;
#define TEXTURE_INDEX_UNDEFINED 0
#define TEXTURE_FIRST_INDEX 1
typedef struct __Texture {
    plTextureHandle   texture_handle;
    plBindGroupHandle bind_group_handle;
} _Texture;
typedef struct __NodeImage {
    _ValIndex2 position;
    _ValIndex2 dimension;
    _ValIndex2 pivot_local_align;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_position;
    _ValIndex2 local_align;
    _ValIndex2 parent_align;
    _ValIndex  rotation;
    _ValIndex  negate_x;
    _ValIndex  negate_y;

    _TextureIndex texture_index;

    _NodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;
} _NodeImage;

#define _NODE_LINE_MAX_POINTS 1000
typedef struct __NodeLine {
    _ValIndex2 position;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_position;
    _ValIndex  rotation;
    _ValIndex  negate_x;
    _ValIndex  negate_y;
    _ValIndex4 line_color;
    _ValIndex  line_width;
    _ValIndex  line_pattern;

    _VertexData *sb_vertices;
    uint8_t      config_flags;
} _NodeLine;

typedef struct __NodeMouseMotion {
    DcAppVarIndex var_x;
    DcAppVarIndex var_y;
} _NodeMouseMotion;

typedef struct __NodePanel {
    _ValIndex2 parent_dimension;
    _ValIndex2 virtual_dimension;
    _ValIndex4 background_color;
    _ValIndex  index;
    uint8_t    config_flags;
    _NodeIndex child;
} _NodePanel;

#define _NODE_PIXELSTREAM_MAX_WIDTH 3840
#define _NODE_PIXELSTREAM_MAX_HEIGHT 2160

// unique pixelstream source (shared across nodes with the same key)
typedef int _PixelstreamSourceIndex;
#define _PIXELSTREAM_SOURCE_INDEX_UNDEFINED -1

typedef struct __PixelstreamSource {
    DcAppPixelstreamType type;
    _TextureIndex        texture_index;
    bool                 is_connected;

    // frame data (fetched once per frame by the source, not per-node)
    unsigned char *frame;
    int            frame_width;
    int            frame_height;

    union {
        struct {
            DcPsMjpegHandle handle;
            unsigned char  *raw_jpeg;
            size_t          raw_jpeg_size;
        } mjpeg;
        struct {
            DcPsShmemHandle handle;
        } shmem;
    };
} _PixelstreamSource;

typedef struct __NodePixelstream {
    _ValIndex2 position;
    _ValIndex2 dimension;
    _ValIndex2 pivot_local_align;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_position;
    _ValIndex2 local_align;
    _ValIndex2 parent_align;
    _ValIndex  rotation;
    _ValIndex  negate_x;
    _ValIndex  negate_y;

    _TextureIndex test_pattern_texture_index;

    _NodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;

    _PixelstreamSourceIndex source_index;
} _NodePixelstream;

#define _NODE_POLYGON_MAX_POINTS 1000
typedef struct __NodePolygon {
    _ValIndex2 position;
    _ValIndex2 parent_align;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_position;
    _ValIndex  rotation;
    _ValIndex  negate_x;
    _ValIndex  negate_y;
    _ValIndex4 fill_color;
    _ValIndex4 line_color;
    _ValIndex  line_width;
    _ValIndex  line_pattern;

    _NodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;

    _VertexData *sb_vertices;
    _ValIndex    rounded;
} _NodePolygon;

typedef struct __NodeRectangle {
    _ValIndex2 position;
    _ValIndex2 dimension;
    _ValIndex2 pivot_local_align;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_position;
    _ValIndex2 local_align;
    _ValIndex2 parent_align;
    _ValIndex  rotation;
    _ValIndex  negate_x;
    _ValIndex  negate_y;
    _ValIndex4 fill_color;
    _ValIndex4 line_color;
    _ValIndex  line_width;
    _ValIndex  line_pattern;

    _NodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;
    _ValIndex  rounded;
} _NodeRectangle;

typedef struct __NodeSet {
    DcAppVarIndex var_index;
    _ValIndex     operation; // because operator was taken :(
    _ValIndex     operand;
    _ValIndex     deferred; // defer to end of draw pass
} _NodeSet;

typedef struct __DeferredSetOp {
    DcAppVarIndex var_index;
    DcAppSetType  operation;
    DcValue       value; // copied by value at defer time
} _DeferredSetOp;

typedef struct __NodeFunction {
    void (*callback)(void);
    _ValIndex fire_call;
    DcValue   last_fire_call_value;
} _NodeFunction;

typedef struct __DrawFunctionArg {
    DcValueType type;
    _ValIndex   value;
} _DrawFunctionArg;

typedef struct __NodeDrawFunction {
    void (*callback)(DcAppDrawContext *, const DcAppDrawFuncArgs *);
    _DrawFunctionArg *sb_args;
} _NodeDrawFunction;

typedef struct __NodeSphere {
    // 2D positioning (where to draw in the orthographic view)
    _ValIndex2 position;
    _ValIndex2 pivot_local_align;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_position;
    _ValIndex2 local_align;
    _ValIndex2 parent_align;
    _ValIndex  rotation; // external 2D rotation in orthographic view
    _ValIndex  negate_x;
    _ValIndex  negate_y;

    // sphere properties
    _ValIndex  radius;
    _ValIndex4 fill_color;

    // internal rotation (roll, pitch, yaw of the sphere itself)
    _ValIndex3 rpy;

    // optional texture
    _TextureIndex texture_index;
} _NodeSphere;

#define DC_STENCIL_MAX_DEPTH 8

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
    _ValIndex2 position;
    _ValIndex2 local_align;
    _ValIndex2 parent_align;
    _ValIndex2 pivot_local_align;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_position;
    _ValIndex  rotation;
    _ValIndex  negate_x;
    _ValIndex  negate_y;
    _ValIndex  size;
    _ValIndex  log;
    _ValIndex4 fill_color;
    _ValIndex4 line_color;
    _ValIndex4 background_color;
    _ValIndex  bold;
    _ValIndex  italic;
    uint8_t    config_flags;
    _ValIndex  shadow_offset;
    _ValIndex  update_rate;
    double     last_update_time;
    int        font_index; // 1-based index into sb_fonts (0 = default)

    // stretchy buffers contains values and formats
    // TODO move this to a context
    _ValIndex   *sb_vals;
    char        *sb_fillers;
    uint8_t     *sb_filler_indices;
    char        *sb_formats;
    uint8_t     *sb_format_indices;
    DcValueType *sb_format_types;
    char        *sb_cached_text;
} _NodeText;

typedef struct {
    char *vertex_path;   // heap-allocated (NULL = keep default "planet.vert")
    char *fragment_path; // heap-allocated (NULL = keep default "planet.frag")
    int   index;
} _PlanetShaderEntry;

typedef struct {
    char     *source;                  // heap-allocated absolute file path
    _ValIndex3 xyz;                    // double vars: native Cartesian center
    _ValIndex mpp;                     // double var: meters per pixel
    _ValIndex lat;                     // double var: latitude (degrees)
    _ValIndex lon;                     // double var: longitude (degrees)
    _ValIndex fire_refresh;            // var: change triggers texture reload
    DcValue   last_fire_refresh_value; // edge detection (fire on change)
    DcAppPlanetCrs crs;
} _PlanetTextureEntry;

#define PLANET_INDEX_UNDEFINED 0
#define PLANET_VIEW_INDEX_UNDEFINED 0

typedef struct __PlanetDef {
    char *name; // lookup key (from Name attr)

    // data
    char **sb_data_files; // stretchy buffer of heap-allocated file paths
    double radius;        // resolved from JSON at init

    // texture overlays
    _PlanetTextureEntry *sb_textures; // stretchy buffer

    // shader overrides (library; per-view selection via PlanetView ShaderIndex)
    _PlanetShaderEntry *sb_shaders; // stretchy buffer

    // light direction
    _ValIndex3 light_direction;

    // coordinate reference system inherited by PlanetTexture
    DcAppPlanetCrs crs;

    // VRAM
    uint32_t mesh_cache_size; // bytes, 0 = default (256 MB each)

    // runtime
    uint8_t index; // 1-based index into sb_planets
} _PlanetDef;

typedef struct __NodePlanetEllipse {
    _ValIndex  lat;
    _ValIndex  lon;
    _ValIndex3 xyz;
    _ValIndex  radius_x;
    _ValIndex  radius_y;
    _ValIndex  rotation;
    _ValIndex  height_above_terrain;
    _ValIndex4 line_color;
    _ValIndex  line_width;
    _ValIndex4 fill_color;
    _ValIndex  segments;
    uint8_t    config_flags;
    uint8_t    planet_def_index;
    DcAppPlanetCrs crs;
} _NodePlanetEllipse;

// fixed point (GeoJSON, baked at parse time)
typedef struct __PlanetVertexStatic {
    double lon;
    double lat;
    double alt; // meters above surface
    bool   has_alt;
} _PlanetVertexStatic;

// variable-bound point (XML, resolved at draw time)
typedef struct __PlanetVertexDynamic {
    _ValIndex lat;
    _ValIndex lon;
    _ValIndex alt;
    _ValIndex3 xyz;
} _PlanetVertexDynamic;

typedef struct __NodePlanetLine {
    _PlanetVertexStatic  *sb_points_static;  // fixed points (GeoJSON)
    _PlanetVertexDynamic *sb_points_dynamic; // variable-bound points (XML)
    bool                  is_dynamic;
    _ValIndex             height_above_terrain;
    _ValIndex4            line_color;
    _ValIndex             line_width;
    uint8_t               config_flags;
    uint8_t               planet_def_index;
    DcAppPlanetCrs        crs;
} _NodePlanetLine;

typedef struct __NodePlanetPolygon {
    _PlanetVertexStatic  *sb_points_static;  // fixed points (GeoJSON)
    _PlanetVertexDynamic *sb_points_dynamic; // variable-bound points (XML)
    bool                  is_dynamic;
    _ValIndex             height_above_terrain;
    _ValIndex4            line_color;
    _ValIndex             line_width;
    _ValIndex4            fill_color;
    uint8_t               config_flags;
    uint8_t               planet_def_index;
    DcAppPlanetCrs        crs;
} _NodePlanetPolygon;

typedef struct __NodePlanetSphere {
    _ValIndex  lat;
    _ValIndex  lon;
    _ValIndex3 xyz;
    _ValIndex  height_above_terrain;
    _ValIndex  radius;
    _ValIndex4 fill_color;
    uint8_t    config_flags;
    uint8_t    planet_def_index;
    DcAppPlanetCrs crs;
} _NodePlanetSphere;

typedef struct __NodePlanetText {
    _ValIndex  lat;
    _ValIndex  lon;
    _ValIndex3 xyz;
    _ValIndex  height_above_terrain;
    _ValIndex  size;
    _ValIndex4 fill_color;
    uint8_t    config_flags;
    uint8_t    planet_def_index;
    DcAppPlanetCrs crs;

    // text content (same pattern as _NodeText)
    _ValIndex   *sb_vals;
    char        *sb_fillers;
    uint8_t     *sb_filler_indices;
    char        *sb_formats;
    uint8_t     *sb_format_indices;
    DcValueType *sb_format_types;
} _NodePlanetText;

typedef struct __NodePlanetView {

    // general positioning of display
    _ValIndex2 dimension;
    _ValIndex2 position;
    _ValIndex2 local_align;
    _ValIndex2 parent_align;
    _ValIndex2 pivot_position;
    _ValIndex2 pivot_parent_align;
    _ValIndex2 pivot_local_align;
    _ValIndex  rotation;
    _ValIndex  negate_x;
    _ValIndex  negate_y;

    // camera
    _ValIndex3 lle;
    _ValIndex3 xyz;
    _ValIndex3 rpy;
    _ValIndex  heading;
    _ValIndex  orthographic;
    DcAppPlanetCrs crs;

    // shader selection (per-view; indexes into parent PlanetDef's sb_shaders)
    _ValIndex shader_index;        // variable holding active shader index
    int       active_shader_index; // last-applied index (for change detection)

    // LOD
    _ValIndex tau; // LOD error threshold (default 0.3, lower = more aggressive)

    // flattening
    _ValIndex flatten;

    // references
    uint8_t planet_def_index;  // index into sb_planet_defs (resolved at parse time)
    uint8_t planet_view_index; // 1-based index into sb_planet_views

    // children (PlanetEllipse, etc.)
    _NodeIndex child;

} _NodePlanetView;

typedef struct __NodeWindow {
    plVec2     init_position;
    plVec2     init_dimension;
    _ValIndex2 virtual_dimension;
    _NodeIndex child;
    char      *title;
    _ValIndex  active_display;
    _ValIndex  frame_rate_limit;
    bool       fullscreen;
} _NodeWindow;

typedef struct __Node {
    _NodeType  type;
    _NodeIndex parent;
    _NodeIndex next;
    union {
        _NodeArc           arc;
        _NodeBlink         blink;
        _NodeButton        button;
        _NodeConditional   conditional;
        _NodeDrawFunction  draw_function;
        _NodeEllipse       ellipse;
        _NodeContainer     container;
        _NodeFunction      function;
        _NodeImage         image;
        _NodeLine          line;
        _NodeMouseMotion   mouse_motion;
        _NodePanel         panel;
        _NodePixelstream   pixelstream;
        _NodePolygon       polygon;
        _NodeRectangle     rectangle;
        _NodeSet           set;
        _NodeSphere        sphere;
        _NodeStateEvent    state_event;
        _NodeStencil       stencil;
        _NodePlanetEllipse planet_ellipse;
        _NodePlanetLine    planet_line;
        _NodePlanetPolygon planet_polygon;
        _NodePlanetSphere  planet_sphere;
        _NodePlanetText    planet_text;
        _NodePlanetView    planet_view;
        _NodeText          text;
        _NodeWindow        window;
    };
} _Node;

#endif
