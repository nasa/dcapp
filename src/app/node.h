#ifndef DC_APP_NODE_H
#define DC_APP_NODE_H

#include "app/draw_types.h"
#include "app/display_logic_callbacks.h"
#include "app/variable_registry_types.h"
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
typedef struct DcAppNodeValueIndex2 {
    union {
        DcAppVariableRegistryValueIndex x, r, lat, roll;
    };
    union {
        DcAppVariableRegistryValueIndex y, g, lon, pitch;
    };
} DcAppNodeValueIndex2;

typedef struct DcAppNodeValueIndex3 {
    union {
        DcAppVariableRegistryValueIndex x, r, lat, roll;
    };
    union {
        DcAppVariableRegistryValueIndex y, g, lon, pitch;
    };
    union {
        DcAppVariableRegistryValueIndex z, b, ele, yaw;
    };
} DcAppNodeValueIndex3;

typedef struct DcAppNodeValueIndex4 {
    union {
        DcAppVariableRegistryValueIndex x, r;
    };
    union {
        DcAppVariableRegistryValueIndex y, g;
    };
    union {
        DcAppVariableRegistryValueIndex z, b;
    };
    union {
        DcAppVariableRegistryValueIndex w, a;
    };
} DcAppNodeValueIndex4;

// Vertex data for Line and Polygon elements
typedef struct DcAppVertexData {
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 parent_align;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;
} DcAppVertexData;

typedef struct DcAppNodeArc {
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 pivot_local_align;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppNodeValueIndex2 local_align;
    DcAppNodeValueIndex2 parent_align;
    DcAppVariableRegistryValueIndex rotation; // where the arc starts (0 = right, 90 = top)
    DcAppVariableRegistryValueIndex radius;
    DcAppVariableRegistryValueIndex angle; // span of the arc in degrees
    DcAppVariableRegistryValueIndex num_segments;
    DcAppNodeValueIndex4 line_color;
    DcAppVariableRegistryValueIndex line_width;
    DcAppVariableRegistryValueIndex line_pattern;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;
} DcAppNodeArc;

typedef struct DcAppNodeBlink {
    DcAppVariableRegistryValueIndex frequency;
    DcAppVariableRegistryValueIndex duty_cycle;
    DcAppVariableRegistryValueIndex duration;
    DcAppVariableRegistryValueIndex fire_blink;
    DcAppNodeIndex child;

    // runtime state
    double remaining_duration;
    double last_frame_time;
    DcAppValue last_fire_blink_value;
} DcAppNodeBlink;

// State event node (children drawn when parent state matches)
typedef struct DcAppNodeStateEvent {
    DcAppNodeIndex child;
} DcAppNodeStateEvent;

typedef struct DcAppNodeButton {

    // standard transforms
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 dimension;
    DcAppNodeValueIndex2 virtual_dimension;
    DcAppNodeValueIndex2 pivot_local_align;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppNodeValueIndex2 local_align;
    DcAppNodeValueIndex2 parent_align;
    DcAppVariableRegistryValueIndex rotation;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;

    // children (regular child nodes, including state-conditional nodes)
    DcAppNodeIndex child;

    // state flags for conditional children to check
    uint32_t state_flags;

    // comparison values for each state
    DcAppVariableRegistryValueIndex val_enabled_on;
    DcAppVariableRegistryValueIndex val_target_on;
    DcAppVariableRegistryValueIndex val_target_off;
    DcAppVariableRegistryValueIndex val_indicator_on;

    // variable indices to be set for each state
    DcAppVariableRegistryVariableIndex var_enabled;
    DcAppVariableRegistryVariableIndex var_target;
    DcAppVariableRegistryVariableIndex var_indicator;

    // type for button
    DcAppButtonType type;
} DcAppNodeButton;

#define DC_APP_NODE_ELLIPSE_MAX_SEGMENTS 1000
typedef struct DcAppNodeEllipse {
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 pivot_local_align;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppNodeValueIndex2 local_align;
    DcAppNodeValueIndex2 parent_align;
    DcAppVariableRegistryValueIndex rotation; // where the wedge starts (0 = right, 90 = top)
    DcAppVariableRegistryValueIndex angle;    // span of the wedge in degrees (360 = full ellipse)
    DcAppVariableRegistryValueIndex radius_x;
    DcAppVariableRegistryValueIndex radius_y;
    DcAppVariableRegistryValueIndex num_segments;
    DcAppNodeValueIndex4 fill_color;
    DcAppNodeValueIndex4 line_color;
    DcAppVariableRegistryValueIndex line_width;
    DcAppVariableRegistryValueIndex line_pattern;

    DcAppNodeIndex child;
    uint32_t state_flags;
    uint8_t config_flags;

    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;
} DcAppNodeEllipse;

typedef struct DcAppNodeConditional {
    DcAppVariableRegistryValueIndex type;
    DcAppVariableRegistryValueIndex value1;
    DcAppVariableRegistryValueIndex value2;
    DcAppNodeIndex child;
    uint32_t state_flags;
} DcAppNodeConditional;

typedef struct DcAppNodeContainer {
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 dimension;
    DcAppNodeValueIndex2 virtual_dimension;
    DcAppNodeValueIndex2 pivot_local_align;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppNodeValueIndex2 local_align;
    DcAppNodeValueIndex2 parent_align;
    DcAppVariableRegistryValueIndex rotation;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;
    DcAppNodeIndex child;
    uint32_t state_flags;
    uint8_t config_flags;
} DcAppNodeContainer;

typedef struct DcAppNodeImage {
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 dimension;
    DcAppNodeValueIndex2 pivot_local_align;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppNodeValueIndex2 local_align;
    DcAppNodeValueIndex2 parent_align;
    DcAppVariableRegistryValueIndex rotation;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;

    DcAppTextureIndex texture_index;

    DcAppNodeIndex child;
    uint32_t state_flags;
    uint8_t config_flags;
} DcAppNodeImage;

#define DC_APP_NODE_LINE_MAX_POINTS 1000
typedef struct DcAppNodeLine {
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppVariableRegistryValueIndex rotation;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;
    DcAppNodeValueIndex4 line_color;
    DcAppVariableRegistryValueIndex line_width;
    DcAppVariableRegistryValueIndex line_pattern;

    DcAppVertexData *sb_vertices;
    uint8_t config_flags;
} DcAppNodeLine;

typedef struct DcAppNodeMouseMotion {
    DcAppVariableRegistryVariableIndex var_x;
    DcAppVariableRegistryVariableIndex var_y;
} DcAppNodeMouseMotion;

typedef struct DcAppNodePanel {
    DcAppNodeValueIndex2 parent_dimension;
    DcAppNodeValueIndex2 virtual_dimension;
    DcAppNodeValueIndex4 background_color;
    DcAppVariableRegistryValueIndex index;
    uint8_t config_flags;
    DcAppNodeIndex child;
} DcAppNodePanel;

typedef struct DcAppNodePixelstream {
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 dimension;
    DcAppNodeValueIndex2 pivot_local_align;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppNodeValueIndex2 local_align;
    DcAppNodeValueIndex2 parent_align;
    DcAppVariableRegistryValueIndex rotation;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;

    DcAppTextureIndex test_pattern_texture_index;

    DcAppNodeIndex child;
    uint32_t state_flags;
    uint8_t config_flags;

    DcAppPixelstreamSourceIndex source_index;
} DcAppNodePixelstream;

#define DC_APP_NODE_POLYGON_MAX_POINTS 1000
typedef struct DcAppNodePolygon {
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 parent_align;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppVariableRegistryValueIndex rotation;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;
    DcAppNodeValueIndex4 fill_color;
    DcAppNodeValueIndex4 line_color;
    DcAppVariableRegistryValueIndex line_width;
    DcAppVariableRegistryValueIndex line_pattern;

    DcAppNodeIndex child;
    uint32_t state_flags;
    uint8_t config_flags;

    DcAppVertexData *sb_vertices;
    DcAppVariableRegistryValueIndex rounded;
} DcAppNodePolygon;

typedef struct DcAppNodeRectangle {
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 dimension;
    DcAppNodeValueIndex2 pivot_local_align;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppNodeValueIndex2 local_align;
    DcAppNodeValueIndex2 parent_align;
    DcAppVariableRegistryValueIndex rotation;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;
    DcAppNodeValueIndex4 fill_color;
    DcAppNodeValueIndex4 line_color;
    DcAppVariableRegistryValueIndex line_width;
    DcAppVariableRegistryValueIndex line_pattern;

    DcAppNodeIndex child;
    uint32_t state_flags;
    uint8_t config_flags;
    DcAppVariableRegistryValueIndex rounded;
} DcAppNodeRectangle;

typedef struct DcAppNodeSet {
    DcAppVariableRegistryVariableIndex var_index;
    DcAppVariableRegistryValueIndex operation; // because operator was taken :(
    DcAppVariableRegistryValueIndex operand;
    DcAppVariableRegistryValueIndex deferred; // defer to end of draw pass
} DcAppNodeSet;

typedef struct DcAppNodeFunction {
    DcAppDisplayLogicFunctionFn callback;
    DcAppVariableRegistryValueIndex fire_call;
    DcAppValue last_fire_call_value;
} DcAppNodeFunction;

typedef struct DcAppDrawFunctionArg {
    DcAppValueType type;
    DcAppVariableRegistryValueIndex value;
} DcAppDrawFunctionArg;

typedef struct DcAppNodeDrawFunction {
    DcAppDisplayLogicDrawFunctionFn callback;
    DcAppDrawFunctionArg *sb_args;
} DcAppNodeDrawFunction;

typedef struct DcAppNodeSphere {
    // 2D positioning (where to draw in the orthographic view)
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 pivot_local_align;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppNodeValueIndex2 local_align;
    DcAppNodeValueIndex2 parent_align;
    DcAppVariableRegistryValueIndex rotation; // external 2D rotation in orthographic view
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;

    // sphere properties
    DcAppVariableRegistryValueIndex radius;
    DcAppNodeValueIndex4 fill_color;

    // internal rotation (roll, pitch, yaw of the sphere itself)
    DcAppNodeValueIndex3 rpy;

    // optional texture
    DcAppTextureIndex texture_index;
} DcAppNodeSphere;

typedef struct DcAppStencilChild {
    DcAppNodeIndex child;
    DcAppStencilChildType type;
} DcAppStencilChild;

typedef struct DcAppNodeStencil {
    DcAppStencilChild *sb_children;
} DcAppNodeStencil;

#define DC_APP_NODE_TEXT_MAX_LINES 256
typedef struct DcAppNodeText {
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 local_align;
    DcAppNodeValueIndex2 parent_align;
    DcAppNodeValueIndex2 pivot_local_align;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppVariableRegistryValueIndex rotation;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;
    DcAppVariableRegistryValueIndex size;
    DcAppVariableRegistryValueIndex log;
    DcAppNodeValueIndex4 fill_color;
    DcAppNodeValueIndex4 line_color;
    DcAppNodeValueIndex4 background_color;
    DcAppVariableRegistryValueIndex bold;
    DcAppVariableRegistryValueIndex italic;
    uint8_t config_flags;
    DcAppVariableRegistryValueIndex shadow_offset;
    DcAppVariableRegistryValueIndex update_rate;
    double last_update_time;
    int font_index; // 1-based index into sb_fonts (0 = default)

    // stretchy buffers contains values and formats
    // TODO move this to a context
    DcAppVariableRegistryValueIndex *sb_vals;
    char *sb_fillers;
    uint8_t *sb_filler_indices;
    char *sb_formats;
    uint8_t *sb_format_indices;
    DcAppValueType *sb_format_types;
    char *sb_cached_text;
} DcAppNodeText;

typedef struct DcAppPlanetShaderEntry {
    char *vertex_path;   // heap-allocated (NULL = keep default "planet.vert")
    char *fragment_path; // heap-allocated (NULL = keep default "planet.frag")
    int index;
} DcAppPlanetShaderEntry;

typedef struct DcAppPlanetTextureEntry {
    DcAppVariableRegistryValueIndex file;         // string value: texture path, passed through unchanged
    DcAppNodeValueIndex2 lle;                     // double vars: geodetic center in degrees
    DcAppNodeValueIndex3 xyz;                     // double vars: native Cartesian center
    DcAppVariableRegistryValueIndex mpp;          // double var: meters per pixel
    DcAppVariableRegistryValueIndex originX;      // double var: meters in projected CRS
    DcAppVariableRegistryValueIndex originY;      // double var: meters in projected CRS
    DcAppVariableRegistryValueIndex enabled;      // boolean var: load/remove this texture slot
    DcAppVariableRegistryValueIndex fire_refresh; // var: change triggers texture reload
    DcAppValue last_fire_refresh_value;           // edge detection (fire on change)
    DcAppPlanetCrs crs;
    uint8_t slot; // assigned by XML declaration order
    bool last_enabled;
    bool enabled_initialized;
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
    DcAppNodeValueIndex3 light_direction;

    // coordinate reference system inherited by PlanetTexture
    DcAppPlanetCrs crs;

    // VRAM
    uint32_t mesh_cache_size_mb; // combined cache size in MiB, 0 = renderer default

    // runtime
    uint8_t index;              // 1-based index into sb_planets
    struct DcAppPlanet *handle; // dcapp handle for xml and logic interop
} DcAppPlanetDefinition;

typedef struct DcAppNodePlanetContainer {
    DcAppVariableRegistryValueIndex lat;
    DcAppVariableRegistryValueIndex lon;
    DcAppVariableRegistryValueIndex height_above_terrain;
    DcAppVariableRegistryValueIndex scale;
    DcAppVariableRegistryValueIndex rotation;
    DcAppVariableRegistryValueIndex enabled;
    uint8_t planet_def_index;
    DcAppNodeIndex child;
} DcAppNodePlanetContainer;

typedef struct DcAppNodePlanetEllipse {
    DcAppVariableRegistryValueIndex lat;
    DcAppVariableRegistryValueIndex lon;
    DcAppNodeValueIndex3 xyz;
    DcAppVariableRegistryValueIndex radius_x;
    DcAppVariableRegistryValueIndex radius_y;
    DcAppVariableRegistryValueIndex rotation;
    DcAppVariableRegistryValueIndex height_above_terrain;
    DcAppNodeValueIndex4 line_color;
    DcAppVariableRegistryValueIndex line_width;
    DcAppNodeValueIndex4 fill_color;
    DcAppVariableRegistryValueIndex segments;
    DcAppVariableRegistryValueIndex enabled;
    uint8_t config_flags;
    uint8_t planet_def_index;
    DcAppPlanetCrs crs;
} DcAppNodePlanetEllipse;

// fixed point (GeoJSON, baked at parse time)
typedef struct DcAppPlanetVertexStatic {
    double lon;
    double lat;
    double alt; // meters above surface
    bool has_alt;
} DcAppPlanetVertexStatic;

// variable-bound point (XML, resolved at draw time)
typedef struct DcAppPlanetVertexDynamic {
    DcAppVariableRegistryValueIndex lat;
    DcAppVariableRegistryValueIndex lon;
    DcAppVariableRegistryValueIndex alt;
    DcAppNodeValueIndex3 xyz;
} DcAppPlanetVertexDynamic;

typedef struct DcAppNodePlanetLine {
    DcAppPlanetVertexStatic *sb_points_static;   // fixed points (GeoJSON)
    DcAppPlanetVertexDynamic *sb_points_dynamic; // variable-bound points (XML)
    bool is_dynamic;
    DcAppVariableRegistryValueIndex height_above_terrain;
    DcAppNodeValueIndex4 line_color;
    DcAppVariableRegistryValueIndex line_width;
    DcAppVariableRegistryValueIndex line_pattern;
    DcAppVariableRegistryValueIndex enabled;
    uint8_t config_flags;
    uint8_t planet_def_index;
    DcAppPlanetCrs crs;
} DcAppNodePlanetLine;

typedef struct DcAppNodePlanetImage {
    DcAppVariableRegistryValueIndex lat;
    DcAppVariableRegistryValueIndex lon;
    DcAppNodeValueIndex3 xyz;
    DcAppVariableRegistryValueIndex height_above_terrain;
    DcAppNodeValueIndex2 dimension;
    DcAppNodeValueIndex4 tint_color;
    DcAppVariableRegistryValueIndex enabled;
    DcAppTextureIndex texture_index;
    uint8_t config_flags;
    uint8_t planet_def_index;
    DcAppPlanetCrs crs;
} DcAppNodePlanetImage;

typedef struct DcAppNodePlanetBreadcrumbs {
    DcAppVariableRegistryValueIndex lat;
    DcAppVariableRegistryValueIndex lon;
    DcAppVariableRegistryValueIndex alt;
    DcAppNodeValueIndex3 xyz;
    DcAppVariableRegistryValueIndex height_above_terrain;
    DcAppVariableRegistryValueIndex point_spacing;
    DcAppVariableRegistryValueIndex max_points;
    DcAppVariableRegistryValueIndex clear;
    DcAppValue last_clear_value;
    bool clear_value_initialized;
    DcAppVariableRegistryValueIndex enabled;
    DcAppNodeValueIndex4 line_color;
    DcAppVariableRegistryValueIndex line_width;
    DcAppVariableRegistryValueIndex line_pattern;
    plVec3d *sb_points;
    uint8_t config_flags;
    uint8_t planet_def_index;
    DcAppPlanetCrs crs;
} DcAppNodePlanetBreadcrumbs;

typedef struct DcAppNodePlanetPolygon {
    DcAppPlanetVertexStatic *sb_points_static;   // fixed points (GeoJSON)
    DcAppPlanetVertexDynamic *sb_points_dynamic; // variable-bound points (XML)
    bool is_dynamic;
    DcAppVariableRegistryValueIndex height_above_terrain;
    DcAppNodeValueIndex4 line_color;
    DcAppVariableRegistryValueIndex line_width;
    DcAppVariableRegistryValueIndex line_pattern;
    DcAppNodeValueIndex4 fill_color;
    DcAppVariableRegistryValueIndex enabled;
    uint8_t config_flags;
    uint8_t planet_def_index;
    DcAppPlanetCrs crs;
} DcAppNodePlanetPolygon;

typedef struct DcAppNodePlanetSphere {
    DcAppVariableRegistryValueIndex lat;
    DcAppVariableRegistryValueIndex lon;
    DcAppNodeValueIndex3 xyz;
    DcAppVariableRegistryValueIndex height_above_terrain;
    DcAppVariableRegistryValueIndex radius;
    DcAppNodeValueIndex4 fill_color;
    DcAppVariableRegistryValueIndex enabled;
    uint8_t config_flags;
    uint8_t planet_def_index;
    DcAppPlanetCrs crs;
} DcAppNodePlanetSphere;

typedef struct DcAppNodePlanetText {
    DcAppVariableRegistryValueIndex lat;
    DcAppVariableRegistryValueIndex lon;
    DcAppNodeValueIndex3 xyz;
    DcAppVariableRegistryValueIndex height_above_terrain;
    DcAppVariableRegistryValueIndex size;
    DcAppNodeValueIndex4 fill_color;
    DcAppVariableRegistryValueIndex enabled;
    uint8_t config_flags;
    uint8_t planet_def_index;
    DcAppPlanetCrs crs;

    // text content (same pattern as DcAppNodeText)
    DcAppVariableRegistryValueIndex *sb_vals;
    char *sb_fillers;
    uint8_t *sb_filler_indices;
    char *sb_formats;
    uint8_t *sb_format_indices;
    DcAppValueType *sb_format_types;
} DcAppNodePlanetText;

typedef struct DcAppNodePlanetView {

    // general positioning of display
    DcAppNodeValueIndex2 dimension;
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 local_align;
    DcAppNodeValueIndex2 parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_local_align;
    DcAppVariableRegistryValueIndex rotation;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;

    // camera
    DcAppNodeValueIndex3 lle;
    DcAppNodeValueIndex3 xyz;
    DcAppNodeValueIndex3 rpy;
    DcAppVariableRegistryValueIndex fov;
    DcAppVariableRegistryValueIndex orthographic;
    DcAppPlanetCrs crs;
    DcAppPlanetAttitudeFrame attitude_frame;

    // shader selection (per-view; indexes into parent PlanetDef's sb_shaders)
    DcAppVariableRegistryValueIndex shader_index; // variable holding active shader index
    int active_shader_index;                      // last-applied index (for change detection)

    // LOD
    DcAppVariableRegistryValueIndex tau; // LOD error threshold (default 0.3, lower = more aggressive)

    // flattening
    DcAppVariableRegistryValueIndex flatten;

    // references
    uint8_t planet_def_index;       // index into sb_planet_defs (resolved at parse time)
    uint8_t planet_view_index;      // 1-based index into sb_planet_views
    struct DcAppPlanetView *handle; // dcapp handle for xml and logic interop

    // children (PlanetEllipse, etc.)
    DcAppNodeIndex child;

} DcAppNodePlanetView;

typedef struct DcAppNodeWindow {
    plVec2 init_position;
    plVec2 init_dimension;
    DcAppNodeValueIndex2 virtual_dimension;
    DcAppNodeIndex child;
    char *title;
    DcAppVariableRegistryValueIndex active_display;
    DcAppVariableRegistryValueIndex update_rate;
    bool fullscreen;
} DcAppNodeWindow;

// Nodes use index-linked child and sibling lists because scene storage may move.
typedef struct DcAppNode {
    DcAppNodeType type;
    DcAppNodeIndex parent;
    DcAppNodeIndex next;
    union {
        DcAppNodeArc arc;
        DcAppNodeBlink blink;
        DcAppNodeButton button;
        DcAppNodeConditional conditional;
        DcAppNodeDrawFunction draw_function;
        DcAppNodeEllipse ellipse;
        DcAppNodeContainer container;
        DcAppNodeFunction function;
        DcAppNodeImage image;
        DcAppNodeLine line;
        DcAppNodeMouseMotion mouse_motion;
        DcAppNodePanel panel;
        DcAppNodePixelstream pixelstream;
        DcAppNodePolygon polygon;
        DcAppNodeRectangle rectangle;
        DcAppNodeSet set;
        DcAppNodeSphere sphere;
        DcAppNodeStateEvent state_event;
        DcAppNodeStencil stencil;
        DcAppNodePlanetBreadcrumbs planet_breadcrumbs;
        DcAppNodePlanetContainer planet_container;
        DcAppNodePlanetEllipse planet_ellipse;
        DcAppNodePlanetImage planet_image;
        DcAppNodePlanetLine planet_line;
        DcAppNodePlanetPolygon planet_polygon;
        DcAppNodePlanetSphere planet_sphere;
        DcAppNodePlanetText planet_text;
        DcAppNodePlanetView planet_view;
        DcAppNodeText text;
        DcAppNodeWindow window;
    };
} DcAppNode;

#endif
