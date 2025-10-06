#ifndef _DC_APP_DRAW_
#define _DC_APP_DRAW_

#include "lookup.h"
#include "../trick.h"

#include <stddef.h>
#include <stdint.h>

#include <libxml/parser.h>

// values
typedef struct _DcAppValIndex2 {
    union {
        DcAppValIndex x, r, lat, roll;
    };
    union {
        DcAppValIndex y, g, lon, pitch;
    };
} DcAppValIndex2;

typedef struct _DcAppValIndex3 {
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

typedef struct _DcAppValIndex4 {
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

typedef enum _DcAppNodeType {
    DC_APP_NODE_TYPE_UNDEFINED,
    DC_APP_NODE_TYPE_CONTAINER,
    DC_APP_NODE_TYPE_CONDITIONAL,
    DC_APP_NODE_TYPE_PANEL,
    DC_APP_NODE_TYPE_POLYGON,
    DC_APP_NODE_TYPE_SET,
    DC_APP_NODE_TYPE_TERRAIN,
    DC_APP_NODE_TYPE_TEXT,
    DC_APP_NODE_TYPE_WINDOW,
} DcAppNodeType;

typedef uint32_t     DcAppNodeIndex;
const DcAppNodeIndex DC_APP_NODE_INDEX_UNDEFINED = -1;

typedef struct _DcAppNodeConditional {
    DcAppValIndex  type;
    DcAppValIndex  value1;
    DcAppValIndex  value2;
    DcAppNodeIndex child_true;
    DcAppNodeIndex child_false;
} DcAppNodeConditional;

typedef struct _DcAppNodeContainer {
    DcAppValIndex2 position;
    DcAppValIndex2 origin;
    DcAppValIndex2 dimensions;
    DcAppValIndex2 virtual_dimensions;
    DcAppValIndex2 alignment;
    DcAppValIndex  rotation;
    DcAppNodeIndex child;
} DcAppNodeContainer;

typedef struct _DcAppNodeMap {
    DcAppValIndex2 position;
    DcAppValIndex2 origin;
    DcAppValIndex2 dimensions;
    DcAppValIndex2 virtual_dimensions;
    DcAppValIndex2 alignment;
    DcAppValIndex3 lle;
    DcAppValIndex  yaw;
    DcAppValIndex  rotation;
    // DcDemManager *demManager;
} DcAppNodeMap;

typedef struct _DcAppNodePanel {
    DcAppValIndex2 parent_dimensions;
    DcAppValIndex2 virtual_dimensions;
    DcAppValIndex  index;
    DcAppNodeIndex child;
} DcAppNodePanel;

typedef struct _DcAppNodePolygon {
    DcAppValIndex4  fill_color;
    DcAppValIndex4  line_color;
    DcAppValIndex   line_width;
    DcAppValIndex2 *points;
    uint32_t        num_points;
    bool            fill_enabled;
    bool            line_enabled;
} DcAppNodePolygon;

typedef struct _DcAppNodeSet {
    DcAppVarIndex var_index;
    DcAppValIndex operation; // because operator was taken :(
    DcAppValIndex operand;
} DcAppNodeSet;

typedef struct _DcAppNodeText {
    DcAppValIndex2 position;
    DcAppValIndex2 origin;
    DcAppValIndex2 alignment;
    DcAppValIndex2 pivot_align;
    DcAppValIndex2 pivot_point;
    DcAppValIndex  rotation;
    DcAppValIndex  size;
    DcAppValIndex4 fill_color;
    DcAppValIndex4 line_color;
    bool           fill_enabled;
    bool           line_enabled;
    // DcAppValIndex  font;

    // stretchy buffers contains values and formats
    // TODO move this to a context
    DcAppValIndex *sb_vals;
    char          *sb_fillers;
    uint8_t       *sb_filler_indices;
    char          *sb_formats;
    uint8_t       *sb_format_indices;
    DcValueType   *sb_format_types;
} DcAppNodeText;

typedef struct _DcAppNodeTerrain {

    // general positioning of display
    DcAppValIndex2 dimensions;
    DcAppValIndex2 position;
    DcAppValIndex2 origin;
    DcAppValIndex2 alignment;
    DcAppValIndex2 pivot_align;
    DcAppValIndex2 pivot_point;
    DcAppValIndex  rotation;

    // terrain specific
    DcAppValIndex3 lle;
    DcAppValIndex3 rpy;
    uint8_t        terrain_index;

} DcAppNodeTerrain;

typedef struct _DcAppNodeWindow {
    DcAppValIndex2 position;
    DcAppValIndex2 dimensions;
    DcAppValIndex2 virtual_dimensions;
    DcAppNodeIndex child;
    char          *title;
} DcAppNodeWindow;

typedef struct _DcAppNode {
    DcAppNodeType  type;
    DcAppNodeIndex parent;
    DcAppNodeIndex next;
    union {
        DcAppNodeConditional conditional;
        DcAppNodeContainer   container;
        DcAppNodeMap         map;
        DcAppNodePanel       panel;
        DcAppNodePolygon     polygon;
        DcAppNodeSet         set;
        DcAppNodeTerrain     terrain;
        DcAppNodeText        text;
        DcAppNodeWindow      window;
    };
} DcAppNode;

// logic
typedef struct _DcAppLogic {
    void (*pre_init)();
    void (*init)();
    void (*draw)();
    void (*close)();
} DcAppLogic;

// trick utils
typedef struct _DcAppTrickTxVarContext {
    DcAppVarIndex   dcapp_var_index;
    DcTrickVarIndex trick_var_index;
    DcValue         prev_value;
} DcAppTrickTxVarContext;

typedef struct _DcAppTrickRxVarContext {
    DcAppVarIndex   dcapp_var_index;
    DcTrickVarIndex trick_var_index;
} DcAppTrickRxVarContext;

typedef struct _DcAppTrickContext {
    DcTrick                            *trick;
    std::vector<DcAppTrickTxVarContext> tx_var_contexts;
    std::vector<DcAppTrickRxVarContext> rx_var_contexts;
} DcAppTrickContext;

typedef struct _DcAppData {
    // string path to config file, directories
    std::string config_file_path;
    std::string config_dir_path;
    std::string cache_dir_path;
    std::string log_dir_path;

    // values
    std::map<std::string, std::string>   constants;
    std::map<std::string, DcAppVarIndex> var_indices;
    std::vector<DcAppVar>                vars;
    std::vector<DcValue>                 values;

    // nodes
    std::vector<DcAppNode> nodes;

    // window (root node)
    DcAppNodeIndex window;

    // logic
    DcAppLogic logic;

    // trick
    std::vector<DcAppTrickContext *> trick_contexts;

    // xml data
    xmlDocPtr doc;
} DcAppData;

#ifdef __cplusplus
extern "C" {
#endif

// node utils
std::string    dc_app_node_type_to_string(DcAppNodeType type);
DcAppNode     *dc_app_index_to_node(DcAppNodeIndex index);
DcAppNodeIndex dc_app_register_node(DcAppNode node);

#ifdef __cplusplus
}
#endif

#endif
