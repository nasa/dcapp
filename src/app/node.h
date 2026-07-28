#ifndef DC_APP_NODE_H
#define DC_APP_NODE_H

#include "app/draw_types.h"
#include "app/logic_callbacks.h"
#include "app/lookup_types.h"
#include "app/node_types.h"
#include "app/pixelstream_types.h"
#include "app/planet_types.h"
#include "app/texture_types.h"
#include "geo.h"
#include "pl_math.h"
#include "value.h"

struct DcAppContext;
struct DcAppDrawContext;
struct DcAppDrawFuncArgs;
struct DcAppPlanet;
struct DcAppPlanetView;

// value index types
typedef struct DcAppValIndex2 {
    union {
        DcAppValIndex x, r, lat, roll;
    };
    union {
        DcAppValIndex y, g, lon, pitch;
    };
} DcAppValIndex2;

typedef struct DcAppValIndex3 {
    union {
        DcAppValIndex x, r, lat, roll;
    };
    union {
        DcAppValIndex y, g, lon, pitch;
    };
    union {
        DcAppValIndex z, b, ele, yaw;
    };
} DcAppValIndex3;

typedef struct DcAppValIndex4 {
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
} DcAppValIndex4;

// Vertex data for Line and Polygon elements
typedef struct DcAppVertexData {
    DcAppValIndex2 position;
    DcAppValIndex2 parent_align;
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;
} DcAppVertexData;

typedef struct DcAppNodeArc {
    DcAppValIndex2 position;
    DcAppValIndex2 pivot_local_align;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex2 local_align;
    DcAppValIndex2 parent_align;
    DcAppValIndex  rotation; // where the arc starts (0 = right, 90 = top)
    DcAppValIndex  radius;
    DcAppValIndex  angle; // span of the arc in degrees
    DcAppValIndex  num_segments;
    DcAppValIndex4 line_color;
    DcAppValIndex  line_width;
    DcAppValIndex  line_pattern;
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;
} DcAppNodeArc;

typedef struct DcAppNodeBlink {
    DcAppValIndex  frequency;
    DcAppValIndex  duty_cycle;
    DcAppValIndex  duration;
    DcAppValIndex  fire_blink;
    DcAppNodeIndex child;

    // runtime state
    double  remaining_duration;
    double  last_frame_time;
    DcValue last_fire_blink_value;
} DcAppNodeBlink;

// State event node (children drawn when parent state matches)
typedef struct DcAppNodeStateEvent {
    DcAppNodeIndex child;
} DcAppNodeStateEvent;

typedef struct DcAppNodeButton {

    // standard transforms
    DcAppValIndex2 position;
    DcAppValIndex2 dimension;
    DcAppValIndex2 virtual_dimension;
    DcAppValIndex2 pivot_local_align;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex2 local_align;
    DcAppValIndex2 parent_align;
    DcAppValIndex  rotation;
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;

    // children (regular child nodes, including state-conditional nodes)
    DcAppNodeIndex child;

    // state flags for conditional children to check
    uint32_t state_flags;

    // comparison values for each state
    DcAppValIndex val_enabled_on;
    DcAppValIndex val_target_on;
    DcAppValIndex val_target_off;
    DcAppValIndex val_indicator_on;

    // variable indices to be set for each state
    DcAppVarIndex var_enabled;
    DcAppVarIndex var_target;
    DcAppVarIndex var_indicator;

    // type for button
    DcAppButtonType type;
} DcAppNodeButton;

#define DC_APP_NODE_ELLIPSE_MAX_SEGMENTS 1000
typedef struct DcAppNodeEllipse {
    DcAppValIndex2 position;
    DcAppValIndex2 pivot_local_align;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex2 local_align;
    DcAppValIndex2 parent_align;
    DcAppValIndex  rotation; // where the wedge starts (0 = right, 90 = top)
    DcAppValIndex  angle;    // span of the wedge in degrees (360 = full ellipse)
    DcAppValIndex  radius_x;
    DcAppValIndex  radius_y;
    DcAppValIndex  num_segments;
    DcAppValIndex4 fill_color;
    DcAppValIndex4 line_color;
    DcAppValIndex  line_width;
    DcAppValIndex  line_pattern;

    DcAppNodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;

    DcAppValIndex negate_x;
    DcAppValIndex negate_y;
} DcAppNodeEllipse;

typedef struct DcAppNodeConditional {
    DcAppValIndex  type;
    DcAppValIndex  value1;
    DcAppValIndex  value2;
    DcAppNodeIndex child;
    uint32_t   state_flags;
} DcAppNodeConditional;

typedef struct DcAppNodeContainer {
    DcAppValIndex2 position;
    DcAppValIndex2 dimension;
    DcAppValIndex2 virtual_dimension;
    DcAppValIndex2 pivot_local_align;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex2 local_align;
    DcAppValIndex2 parent_align;
    DcAppValIndex  rotation;
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;
    DcAppNodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;
} DcAppNodeContainer;

typedef struct DcAppNodeImage {
    DcAppValIndex2 position;
    DcAppValIndex2 dimension;
    DcAppValIndex2 pivot_local_align;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex2 local_align;
    DcAppValIndex2 parent_align;
    DcAppValIndex  rotation;
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;

    DcAppTextureIndex texture_index;

    DcAppNodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;
} DcAppNodeImage;

#define DC_APP_NODE_LINE_MAX_POINTS 1000
typedef struct DcAppNodeLine {
    DcAppValIndex2 position;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex  rotation;
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;
    DcAppValIndex4 line_color;
    DcAppValIndex  line_width;
    DcAppValIndex  line_pattern;

    DcAppVertexData *sb_vertices;
    uint8_t      config_flags;
} DcAppNodeLine;

typedef struct DcAppNodeMouseMotion {
    DcAppVarIndex var_x;
    DcAppVarIndex var_y;
} DcAppNodeMouseMotion;

typedef struct DcAppNodePanel {
    DcAppValIndex2 parent_dimension;
    DcAppValIndex2 virtual_dimension;
    DcAppValIndex4 background_color;
    DcAppValIndex  index;
    uint8_t    config_flags;
    DcAppNodeIndex child;
} DcAppNodePanel;

typedef struct DcAppNodePixelstream {
    DcAppValIndex2 position;
    DcAppValIndex2 dimension;
    DcAppValIndex2 pivot_local_align;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex2 local_align;
    DcAppValIndex2 parent_align;
    DcAppValIndex  rotation;
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;

    DcAppTextureIndex test_pattern_texture_index;

    DcAppNodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;

    DcAppPixelstreamSourceIndex source_index;
} DcAppNodePixelstream;

#define DC_APP_NODE_POLYGON_MAX_POINTS 1000
typedef struct DcAppNodePolygon {
    DcAppValIndex2 position;
    DcAppValIndex2 parent_align;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex  rotation;
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;
    DcAppValIndex4 fill_color;
    DcAppValIndex4 line_color;
    DcAppValIndex  line_width;
    DcAppValIndex  line_pattern;

    DcAppNodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;

    DcAppVertexData *sb_vertices;
    DcAppValIndex    rounded;
} DcAppNodePolygon;

typedef struct DcAppNodeRectangle {
    DcAppValIndex2 position;
    DcAppValIndex2 dimension;
    DcAppValIndex2 pivot_local_align;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex2 local_align;
    DcAppValIndex2 parent_align;
    DcAppValIndex  rotation;
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;
    DcAppValIndex4 fill_color;
    DcAppValIndex4 line_color;
    DcAppValIndex  line_width;
    DcAppValIndex  line_pattern;

    DcAppNodeIndex child;
    uint32_t   state_flags;
    uint8_t    config_flags;
    DcAppValIndex  rounded;
} DcAppNodeRectangle;

typedef struct DcAppNodeSet {
    DcAppVarIndex var_index;
    DcAppValIndex     operation; // because operator was taken :(
    DcAppValIndex     operand;
    DcAppValIndex     deferred; // defer to end of draw pass
} DcAppNodeSet;

typedef struct DcAppNodeFunction {
    DcAppLogicFunctionFn callback;
    DcAppValIndex fire_call;
    DcValue   last_fire_call_value;
} DcAppNodeFunction;

typedef struct DcAppDrawFunctionArg {
    DcValueType type;
    DcAppValIndex   value;
} DcAppDrawFunctionArg;

typedef struct DcAppNodeDrawFunction {
    DcAppLogicDrawFunctionFn callback;
    DcAppDrawFunctionArg *sb_args;
} DcAppNodeDrawFunction;

typedef struct DcAppNodeSphere {
    // 2D positioning (where to draw in the orthographic view)
    DcAppValIndex2 position;
    DcAppValIndex2 pivot_local_align;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex2 local_align;
    DcAppValIndex2 parent_align;
    DcAppValIndex  rotation; // external 2D rotation in orthographic view
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;

    // sphere properties
    DcAppValIndex  radius;
    DcAppValIndex4 fill_color;

    // internal rotation (roll, pitch, yaw of the sphere itself)
    DcAppValIndex3 rpy;

    // optional texture
    DcAppTextureIndex texture_index;
} DcAppNodeSphere;

typedef struct DcAppStencilChild {
    DcAppNodeIndex        child;
    DcAppStencilChildType type;
} DcAppStencilChild;

typedef struct DcAppNodeStencil {
    DcAppStencilChild *sb_children;
} DcAppNodeStencil;

#define DC_APP_NODE_TEXT_MAX_LINES 256
typedef struct DcAppNodeText {
    DcAppValIndex2 position;
    DcAppValIndex2 local_align;
    DcAppValIndex2 parent_align;
    DcAppValIndex2 pivot_local_align;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex  rotation;
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;
    DcAppValIndex  size;
    DcAppValIndex  log;
    DcAppValIndex4 fill_color;
    DcAppValIndex4 line_color;
    DcAppValIndex4 background_color;
    DcAppValIndex  bold;
    DcAppValIndex  italic;
    uint8_t    config_flags;
    DcAppValIndex  shadow_offset;
    DcAppValIndex  update_rate;
    double     last_update_time;
    int        font_index; // 1-based index into sb_fonts (0 = default)

    // stretchy buffers contains values and formats
    // TODO move this to a context
    DcAppValIndex   *sb_vals;
    char        *sb_fillers;
    uint8_t     *sb_filler_indices;
    char        *sb_formats;
    uint8_t     *sb_format_indices;
    DcValueType *sb_format_types;
    char        *sb_cached_text;
} DcAppNodeText;

typedef struct DcAppPlanetShaderEntry {
    char *vertex_path;   // heap-allocated (NULL = keep default "planet.vert")
    char *fragment_path; // heap-allocated (NULL = keep default "planet.frag")
    int   index;
} DcAppPlanetShaderEntry;

typedef struct DcAppPlanetTextureEntry {
    char     *source;                  // heap-allocated absolute file path
    DcAppValIndex2 lle;                    // double vars: geodetic center in degrees
    DcAppValIndex3 xyz;                    // double vars: native Cartesian center
    DcAppValIndex mpp;                     // double var: meters per pixel
    DcAppValIndex originX;                 // double var: meters in projected CRS
    DcAppValIndex originY;                 // double var: meters in projected CRS
    DcAppValIndex enabled;                 // boolean var: load/remove this texture slot
    DcAppValIndex fire_refresh;            // var: change triggers texture reload
    DcValue   last_fire_refresh_value; // edge detection (fire on change)
    DcAppPlanetCrs crs;
    uint8_t slot;                      // assigned by XML declaration order
    bool    last_enabled;
    bool    enabled_initialized;
} DcAppPlanetTextureEntry;

#define PLANET_INDEX_UNDEFINED 0
#define PLANET_VIEW_INDEX_UNDEFINED 0

typedef struct DcAppPlanetDefinition {
    char *name; // lookup key (from Name attr)

    // data
    char **sb_data_files; // stretchy buffer of heap-allocated file paths
    double radius;        // resolved from JSON at init
    DcGeoCrsGeodetic geodetic_crs;
    DcGeoCrsCartesian cartesian_crs;
    DcGeoCrsPolarStereo polar_crs;
    bool legacy_projected_origin;

    // texture overlays
    DcAppPlanetTextureEntry *sb_textures; // stretchy buffer

    // shader overrides (library; per-view selection via PlanetView ShaderIndex)
    DcAppPlanetShaderEntry *sb_shaders; // stretchy buffer

    // light direction
    DcAppValIndex3 light_direction;

    // coordinate reference system inherited by PlanetTexture
    DcAppPlanetCrs crs;

    // VRAM
    uint32_t mesh_cache_size_mb; // combined cache size in MiB, 0 = renderer default

    // runtime
    uint8_t index; // 1-based index into sb_planets
    struct DcAppPlanet *handle; // dcapp handle for xml and logic interop
} DcAppPlanetDefinition;

typedef struct DcAppNodePlanetContainer {
    DcAppValIndex  lat;
    DcAppValIndex  lon;
    DcAppValIndex  height_above_terrain;
    DcAppValIndex  scale;
    DcAppValIndex  rotation;
    uint8_t    planet_def_index;
    DcAppNodeIndex child;
} DcAppNodePlanetContainer;

typedef struct DcAppNodePlanetEllipse {
    DcAppValIndex  lat;
    DcAppValIndex  lon;
    DcAppValIndex3 xyz;
    DcAppValIndex  radius_x;
    DcAppValIndex  radius_y;
    DcAppValIndex  rotation;
    DcAppValIndex  height_above_terrain;
    DcAppValIndex4 line_color;
    DcAppValIndex  line_width;
    DcAppValIndex4 fill_color;
    DcAppValIndex  segments;
    uint8_t    config_flags;
    uint8_t    planet_def_index;
    DcAppPlanetCrs crs;
} DcAppNodePlanetEllipse;

// fixed point (GeoJSON, baked at parse time)
typedef struct DcAppPlanetVertexStatic {
    double lon;
    double lat;
    double alt; // meters above surface
    bool   has_alt;
} DcAppPlanetVertexStatic;

// variable-bound point (XML, resolved at draw time)
typedef struct DcAppPlanetVertexDynamic {
    DcAppValIndex lat;
    DcAppValIndex lon;
    DcAppValIndex alt;
    DcAppValIndex3 xyz;
} DcAppPlanetVertexDynamic;

typedef struct DcAppNodePlanetLine {
    DcAppPlanetVertexStatic  *sb_points_static;  // fixed points (GeoJSON)
    DcAppPlanetVertexDynamic *sb_points_dynamic; // variable-bound points (XML)
    bool                  is_dynamic;
    DcAppValIndex             height_above_terrain;
    DcAppValIndex4            line_color;
    DcAppValIndex             line_width;
    DcAppValIndex             line_pattern;
    uint8_t               config_flags;
    uint8_t               planet_def_index;
    DcAppPlanetCrs        crs;
} DcAppNodePlanetLine;

typedef struct DcAppNodePlanetImage {
    DcAppValIndex  lat;
    DcAppValIndex  lon;
    DcAppValIndex3 xyz;
    DcAppValIndex  height_above_terrain;
    DcAppValIndex2 dimension;
    DcAppValIndex4 tint_color;
    DcAppTextureIndex texture_index;
    uint8_t    config_flags;
    uint8_t    planet_def_index;
    DcAppPlanetCrs crs;
} DcAppNodePlanetImage;

typedef struct DcAppNodePlanetBreadcrumbs {
    DcAppValIndex       lat;
    DcAppValIndex       lon;
    DcAppValIndex       alt;
    DcAppValIndex3      xyz;
    DcAppValIndex       height_above_terrain;
    DcAppValIndex       point_spacing;
    DcAppValIndex       max_points;
    DcAppValIndex       clear;
    DcValue         last_clear_value;
    bool            clear_value_initialized;
    DcAppValIndex       enabled;
    DcAppValIndex4      line_color;
    DcAppValIndex       line_width;
    DcAppValIndex       line_pattern;
    plVec3d        *sb_points;
    uint8_t         config_flags;
    uint8_t         planet_def_index;
    DcAppPlanetCrs  crs;
} DcAppNodePlanetBreadcrumbs;

typedef struct DcAppNodePlanetPolygon {
    DcAppPlanetVertexStatic  *sb_points_static;  // fixed points (GeoJSON)
    DcAppPlanetVertexDynamic *sb_points_dynamic; // variable-bound points (XML)
    bool                  is_dynamic;
    DcAppValIndex             height_above_terrain;
    DcAppValIndex4            line_color;
    DcAppValIndex             line_width;
    DcAppValIndex             line_pattern;
    DcAppValIndex4            fill_color;
    uint8_t               config_flags;
    uint8_t               planet_def_index;
    DcAppPlanetCrs        crs;
} DcAppNodePlanetPolygon;

typedef struct DcAppNodePlanetSphere {
    DcAppValIndex  lat;
    DcAppValIndex  lon;
    DcAppValIndex3 xyz;
    DcAppValIndex  height_above_terrain;
    DcAppValIndex  radius;
    DcAppValIndex4 fill_color;
    uint8_t    config_flags;
    uint8_t    planet_def_index;
    DcAppPlanetCrs crs;
} DcAppNodePlanetSphere;

typedef struct DcAppNodePlanetText {
    DcAppValIndex  lat;
    DcAppValIndex  lon;
    DcAppValIndex3 xyz;
    DcAppValIndex  height_above_terrain;
    DcAppValIndex  size;
    DcAppValIndex4 fill_color;
    uint8_t    config_flags;
    uint8_t    planet_def_index;
    DcAppPlanetCrs crs;

    // text content (same pattern as DcAppNodeText)
    DcAppValIndex   *sb_vals;
    char        *sb_fillers;
    uint8_t     *sb_filler_indices;
    char        *sb_formats;
    uint8_t     *sb_format_indices;
    DcValueType *sb_format_types;
} DcAppNodePlanetText;

typedef struct DcAppNodePlanetView {

    // general positioning of display
    DcAppValIndex2 dimension;
    DcAppValIndex2 position;
    DcAppValIndex2 local_align;
    DcAppValIndex2 parent_align;
    DcAppValIndex2 pivot_position;
    DcAppValIndex2 pivot_parent_align;
    DcAppValIndex2 pivot_local_align;
    DcAppValIndex  rotation;
    DcAppValIndex  negate_x;
    DcAppValIndex  negate_y;

    // camera
    DcAppValIndex3 lle;
    DcAppValIndex3 xyz;
    DcAppValIndex3 rpy;
    DcAppValIndex  fov;
    DcAppValIndex  orthographic;
    DcAppPlanetCrs crs;
    DcAppPlanetAttitudeFrame attitude_frame;

    // shader selection (per-view; indexes into parent PlanetDef's sb_shaders)
    DcAppValIndex shader_index;        // variable holding active shader index
    int       active_shader_index; // last-applied index (for change detection)

    // LOD
    DcAppValIndex tau; // LOD error threshold (default 0.3, lower = more aggressive)

    // flattening
    DcAppValIndex flatten;

    // references
    uint8_t planet_def_index;  // index into sb_planet_defs (resolved at parse time)
    uint8_t planet_view_index; // 1-based index into sb_planet_views
    struct DcAppPlanetView *handle; // dcapp handle for xml and logic interop

    // children (PlanetEllipse, etc.)
    DcAppNodeIndex child;

} DcAppNodePlanetView;

typedef struct DcAppNodeWindow {
    plVec2     init_position;
    plVec2     init_dimension;
    DcAppValIndex2 virtual_dimension;
    DcAppNodeIndex child;
    char      *title;
    DcAppValIndex  active_display;
    DcAppValIndex  update_rate;
    bool       fullscreen;
} DcAppNodeWindow;

// Nodes use index-linked child and sibling lists because scene storage may move.
typedef struct DcAppNode {
    DcAppNodeType  type;
    DcAppNodeIndex parent;
    DcAppNodeIndex next;
    union {
        DcAppNodeArc           arc;
        DcAppNodeBlink         blink;
        DcAppNodeButton        button;
        DcAppNodeConditional   conditional;
        DcAppNodeDrawFunction  draw_function;
        DcAppNodeEllipse       ellipse;
        DcAppNodeContainer     container;
        DcAppNodeFunction      function;
        DcAppNodeImage         image;
        DcAppNodeLine          line;
        DcAppNodeMouseMotion   mouse_motion;
        DcAppNodePanel         panel;
        DcAppNodePixelstream   pixelstream;
        DcAppNodePolygon       polygon;
        DcAppNodeRectangle     rectangle;
        DcAppNodeSet           set;
        DcAppNodeSphere        sphere;
        DcAppNodeStateEvent    state_event;
        DcAppNodeStencil       stencil;
        DcAppNodePlanetBreadcrumbs planet_breadcrumbs;
        DcAppNodePlanetContainer planet_container;
        DcAppNodePlanetEllipse planet_ellipse;
        DcAppNodePlanetImage   planet_image;
        DcAppNodePlanetLine    planet_line;
        DcAppNodePlanetPolygon planet_polygon;
        DcAppNodePlanetSphere  planet_sphere;
        DcAppNodePlanetText    planet_text;
        DcAppNodePlanetView    planet_view;
        DcAppNodeText          text;
        DcAppNodeWindow        window;
    };
} DcAppNode;

#endif
