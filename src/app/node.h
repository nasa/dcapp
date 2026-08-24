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

//~ forward declarations

struct DcAppContext;
struct DcAppDrawContext;
struct DcAppDrawFuncArgs;
struct DcAppPlanet;
struct DcAppPlanetView;

//~ shared value indices

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

// vertex data shared by lines and polygons
typedef struct DcAppVertexData {
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 parent_align;
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;
} DcAppVertexData;

//~ display nodes

//- arcs and blinking

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

    //- runtime state
    double remaining_duration;
    double last_frame_time;
    DcAppValue last_fire_blink_value;
} DcAppNodeBlink;

//- state and buttons

// children render when their parent state matches
typedef struct DcAppNodeStateEvent {
    DcAppNodeIndex child;
} DcAppNodeStateEvent;

typedef struct DcAppNodeButton {

    //- transforms
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

    //- children
    DcAppNodeIndex child;

    //- conditional state
    uint32_t state_flags;

    // values compared for each state
    DcAppVariableRegistryValueIndex val_enabled_on;
    DcAppVariableRegistryValueIndex val_target_on;
    DcAppVariableRegistryValueIndex val_target_off;
    DcAppVariableRegistryValueIndex val_indicator_on;

    // variables written for each state
    DcAppVariableRegistryVariableIndex var_enabled;
    DcAppVariableRegistryVariableIndex var_target;
    DcAppVariableRegistryVariableIndex var_indicator;

    // button behavior
    DcAppButtonType type;
} DcAppNodeButton;

//- ellipses and conditions

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

//- containers images and lines

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

//- interaction panels and streams

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

//- polygons and rectangles

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

//- assignments and callbacks

typedef struct DcAppNodeSet {
    DcAppVariableRegistryVariableIndex var_index;
    DcAppVariableRegistryValueIndex operation; // operator was already taken
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

//- sphere stencil and text

typedef struct DcAppNodeSphere {
    //- orthographic placement
    DcAppNodeValueIndex2 position;
    DcAppNodeValueIndex2 pivot_local_align;
    DcAppNodeValueIndex2 pivot_parent_align;
    DcAppNodeValueIndex2 pivot_position;
    DcAppNodeValueIndex2 local_align;
    DcAppNodeValueIndex2 parent_align;
    DcAppVariableRegistryValueIndex rotation; // external two dimensional rotation in orthographic view
    DcAppVariableRegistryValueIndex negate_x;
    DcAppVariableRegistryValueIndex negate_y;

    //- sphere appearance
    DcAppVariableRegistryValueIndex radius;
    DcAppNodeValueIndex4 fill_color;

    // sphere-local roll pitch and yaw
    DcAppNodeValueIndex3 rpy;

    // optional surface texture
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

    // todo move parsed value and format storage to a context
    DcAppVariableRegistryValueIndex *sb_vals;
    char *sb_fillers;
    uint8_t *sb_filler_indices;
    char *sb_formats;
    uint8_t *sb_format_indices;
    DcAppValueType *sb_format_types;
    char *sb_cached_text;
} DcAppNodeText;

//~ planet resources

typedef struct DcAppPlanetShaderEntry {
    char *vertex_path;   // heap-allocated with null keeping the default planet vertex shader
    char *fragment_path; // heap-allocated with null keeping the default planet fragment shader
    int index;
} DcAppPlanetShaderEntry;

typedef struct DcAppPlanetTextureEntry {
    DcAppVariableRegistryValueIndex file;         // string value: texture path, passed through unchanged
    DcAppNodeValueIndex2 lle;                     // double vars: geodetic center in degrees
    DcAppNodeValueIndex3 xyz;                     // double vars: native cartesian center
    DcAppVariableRegistryValueIndex mpp;          // double var: meters per pixel
    DcAppVariableRegistryValueIndex originX;      // projected crs meters
    DcAppVariableRegistryValueIndex originY;      // projected crs meters
    DcAppVariableRegistryValueIndex enabled;      // boolean var: load/remove this texture slot
    DcAppVariableRegistryValueIndex fire_refresh; // var: change triggers texture reload
    DcAppValue last_fire_refresh_value;           // edge detection (fire on change)
    DcAppPlanetCrs crs;
    uint8_t slot; // assigned by xml declaration order
    bool last_enabled;
    bool enabled_initialized;
} DcAppPlanetTextureEntry;

#define PLANET_INDEX_UNDEFINED 0
#define PLANET_VIEW_INDEX_UNDEFINED 0

typedef struct DcAppPlanetDefinition {
    char *name; // lookup key from the name attribute

    //- terrain data
    char **sb_data_files; // stretchy buffer of heap-allocated file paths
    double radius;        // resolved from json during init
    DcGeoCrsGeodetic geodetic_crs;
    DcGeoCrsCartesian cartesian_crs;
    DcGeoCrsPolarStereo polar_crs;
    bool legacy_projected_origin;

    //- texture overlays
    DcAppPlanetTextureEntry *sb_textures; // stretchy buffer

    //- shader overrides selected per view
    DcAppPlanetShaderEntry *sb_shaders; // stretchy buffer

    //- lighting
    DcAppNodeValueIndex3 light_direction;

    // coordinate system inherited by texture overlays
    DcAppPlanetCrs crs;

    //- gpu memory
    uint32_t mesh_cache_size_mb; // combined cache size in mib or zero for the renderer default

    //- runtime handles
    uint8_t index;              // 1-based index into sb_planets
    struct DcAppPlanet *handle; // dcapp handle for xml and logic interop
} DcAppPlanetDefinition;

//~ planet nodes

//- containers and ellipses

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

//- vertex data

// fixed geojson point baked during parsing
typedef struct DcAppPlanetVertexStatic {
    double lon;
    double lat;
    double alt; // meters above surface
    bool has_alt;
} DcAppPlanetVertexStatic;

// variable-bound xml point resolved while drawing
typedef struct DcAppPlanetVertexDynamic {
    DcAppVariableRegistryValueIndex lat;
    DcAppVariableRegistryValueIndex lon;
    DcAppVariableRegistryValueIndex alt;
    DcAppNodeValueIndex3 xyz;
} DcAppPlanetVertexDynamic;

//- lines images and breadcrumbs

typedef struct DcAppNodePlanetLine {
    DcAppPlanetVertexStatic *sb_points_static;   // fixed geojson points
    DcAppPlanetVertexDynamic *sb_points_dynamic; // variable-bound xml points
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

//- polygons spheres and text

typedef struct DcAppNodePlanetPolygon {
    DcAppPlanetVertexStatic *sb_points_static;   // fixed geojson points
    DcAppPlanetVertexDynamic *sb_points_dynamic; // variable-bound xml points
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

    //- parsed text content
    DcAppVariableRegistryValueIndex *sb_vals;
    char *sb_fillers;
    uint8_t *sb_filler_indices;
    char *sb_formats;
    uint8_t *sb_format_indices;
    DcAppValueType *sb_format_types;
} DcAppNodePlanetText;

//- views and windows

typedef struct DcAppNodePlanetView {

    //- display placement
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

    //- camera
    DcAppNodeValueIndex3 lle;
    DcAppNodeValueIndex3 xyz;
    DcAppNodeValueIndex3 rpy;
    DcAppVariableRegistryValueIndex fov;
    DcAppVariableRegistryValueIndex orthographic;
    DcAppPlanetCrs crs;
    DcAppPlanetAttitudeFrame attitude_frame;

    //- per-view shader selection
    DcAppVariableRegistryValueIndex shader_index; // variable holding active shader index
    int active_shader_index;                      // last-applied index (for change detection)

    //- level of detail
    DcAppVariableRegistryValueIndex tau; // default 0.3 with lower thresholds requesting more detail

    //- flattening
    DcAppVariableRegistryValueIndex flatten;

    //- references
    uint8_t planet_def_index;       // index into sb_planet_defs (resolved at parse time)
    uint8_t planet_view_index;      // 1-based index into sb_planet_views
    struct DcAppPlanetView *handle; // dcapp handle for xml and logic interop

    //- overlay children
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

//~ scene storage

// index-linked lists remain valid when scene storage moves
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
