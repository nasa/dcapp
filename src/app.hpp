#pragma once

// dcapp includes
#define PL_EXPERIMENTAL
#include "trick.hpp"
#include "value.hpp"

// library includes
#include <libxml2/libxml/parser.h>

// c++ standard includes
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// NOTE this file is a complete mess, but the intention is to not
// clutter the apps themselves. These utils are specifically to
// be used in the app space.

// forward declarations
// TODO implement this better
typedef struct _plSharedLibrary plSharedLibrary;

// element utils
typedef enum {
    DC_APP_ELEM_TYPE_UNDEFINED,
    DC_APP_ELEM_TYPE_CONSTANT,
    DC_APP_ELEM_TYPE_CONTAINER,
    DC_APP_ELEM_TYPE_DCAPP,
    DC_APP_ELEM_TYPE_DEM,
    DC_APP_ELEM_TYPE_DUMMY,
    DC_APP_ELEM_TYPE_FALSE,
    DC_APP_ELEM_TYPE_IF,
    DC_APP_ELEM_TYPE_INCLUDE,
    DC_APP_ELEM_TYPE_LOGIC,
    DC_APP_ELEM_TYPE_MAP,
    DC_APP_ELEM_TYPE_NONELEM,
    DC_APP_ELEM_TYPE_PANEL,
    DC_APP_ELEM_TYPE_POLYGON,
    DC_APP_ELEM_TYPE_TRICK_FROM,
    DC_APP_ELEM_TYPE_TRICK_IO,
    DC_APP_ELEM_TYPE_TRICK_TO,
    DC_APP_ELEM_TYPE_TRICK_VARIABLE,
    DC_APP_ELEM_TYPE_TRUE,
    DC_APP_ELEM_TYPE_VARIABLE,
    DC_APP_ELEM_TYPE_VERTEX,
    DC_APP_ELEM_TYPE_WINDOW,
} DcAppElemType;
std::string   dc_app_elem_type_to_string(DcAppElemType type);
DcAppElemType dc_app_string_to_elem_type(std::string name);
DcAppElemType dc_app_xml_node_to_elem_type(xmlNodePtr node);

// value utils
typedef uint32_t      DcAppValueIndex;
const DcAppValueIndex dc_value_index_undefined = 0;

typedef struct _DcAppValueIndex2 {
    union {
        DcAppValueIndex x, r, lat;
    };
    union {
        DcAppValueIndex y, g, lon;
    };
} DcAppValueIndex2;

typedef struct _DcAppValueIndex3 {
    union {
        DcAppValueIndex x, r, lat;
    };
    union {
        DcAppValueIndex y, g, lon;
    };
    union {
        DcAppValueIndex z, b, ele;
    };
} DcAppValueIndex3;

typedef struct _DcAppValueIndex4 {
    union {
        DcAppValueIndex x, r;
    };
    union {
        DcAppValueIndex y, g;
    };
    union {
        DcAppValueIndex z, b;
    };
    union {
        DcAppValueIndex w, a;
    };
} DcAppValueIndex4;

// alignment utils
enum DcAppAlignType {
    DC_APP_ALIGN_TYPE_UNDEFINED,
    DC_APP_ALIGN_TYPE_LEFT,
    DC_APP_ALIGN_TYPE_CENTER,
    DC_APP_ALIGN_TYPE_RIGHT,
    DC_APP_ALIGN_TYPE_BOTTOM,
    DC_APP_ALIGN_TYPE_MIDDLE,
    DC_APP_ALIGN_TYPE_TOP,
};

// conditional utils
enum DcAppConditionalType {
    DC_APP_CONDITIONAL_TYPE_UNDEFINED,
    DC_APP_CONDITIONAL_TYPE_TRUE,
    DC_APP_CONDITIONAL_TYPE_FALSE,
    DC_APP_CONDITIONAL_TYPE_EQ,
    DC_APP_CONDITIONAL_TYPE_NE,
    DC_APP_CONDITIONAL_TYPE_LT,
    DC_APP_CONDITIONAL_TYPE_GT,
    DC_APP_CONDITIONAL_TYPE_LTE,
    DC_APP_CONDITIONAL_TYPE_GTE,
};

// node utils
enum DcAppNodeType {
    DC_APP_NODE_TYPE_UNDEFINED,
    DC_APP_NODE_TYPE_CONTAINER,
    DC_APP_NODE_TYPE_CONDITIONAL,
    DC_APP_NODE_TYPE_MAP,
    DC_APP_NODE_TYPE_PANEL,
    DC_APP_NODE_TYPE_POLYGON,
    DC_APP_NODE_TYPE_SET,
    DC_APP_NODE_TYPE_WINDOW,
};

typedef uint32_t     DcAppNodeIndex;
const DcAppNodeIndex DC_APP_NODE_INDEX_UNDEFINED = 0;

typedef struct _DcAppNodeConditional {
    DcAppValueIndex type;
    DcAppValueIndex value1;
    DcAppValueIndex value2;
    DcAppNodeIndex  child_true;
    DcAppNodeIndex  child_false;
} DcAppNodeConditional;

typedef struct _DcAppNodeContainer {
    DcAppValueIndex2 position;
    DcAppValueIndex2 origin;
    DcAppValueIndex2 dimensions;
    DcAppValueIndex2 virtual_dimensions;
    DcAppValueIndex2 alignment;
    DcAppValueIndex  rotation;
    DcAppNodeIndex   child;
} DcAppNodeContainer;

typedef struct _DcAppNodeMap {
    DcAppValueIndex2 position;
    DcAppValueIndex2 origin;
    DcAppValueIndex2 dimensions;
    DcAppValueIndex2 virtual_dimensions;
    DcAppValueIndex2 alignment;
    DcAppValueIndex3 lle;
    DcAppValueIndex  yaw;
    DcAppValueIndex  rotation;
    // DcDemManager *demManager;
} DcAppNodeMap;

typedef struct _DcAppNodePanel {
    DcAppValueIndex2 parent_dimensions;
    DcAppValueIndex2 virtual_dimensions;
    DcAppValueIndex  index;
    DcAppNodeIndex   child;
} DcAppNodePanel;

typedef struct _DcAppNodePolygon {
    DcAppValueIndex4  fill_color;
    DcAppValueIndex4  line_color;
    DcAppValueIndex   line_width;
    DcAppValueIndex2 *points;
    uint32_t          num_points;
    bool              fill_enabled;
    bool              line_enabled;
} DcAppNodePolygon;

typedef struct _DcAppNodeWindow {
    DcAppValueIndex2 position;
    DcAppValueIndex2 dimensions;
    DcAppValueIndex2 virtual_dimensions;
    DcAppNodeIndex   child;
    char            *title;
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
        DcAppNodeWindow      window;
    };
} DcAppNode;

// logic
typedef struct _DcAppLogic {
    plSharedLibrary *library;
    void (*pre_init)();
    void (*init)();
    void (*draw)();
    void (*close)();
} DcAppLogic;

// variable utils
typedef uint32_t DcAppVarIndex;
typedef struct _DcAppVar {
    void           *extern_data;
    DcAppValueIndex value_index;
} DcAppVar;

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

// constant utils
void        dc_app_set_constant(const std::string &name, const std::string &text);
std::string dc_app_get_constant(const std::string &name);
std::string dc_app_dereference_constants(const char *text);
std::string dc_app_dereference_constants(std::string text);

// value utils
DcValue        *dc_app_get_value(DcAppValueIndex index);
DcAppValueIndex dc_app_register_value(DcValue value);
DcAppValueIndex dc_app_create_and_register_typed_value_from_string(DcValueType type, const char *text);
DcAppValueIndex dc_app_create_and_register_typed_value_from_string(DcValueType type, std::string text);

// variable utils
void          dc_app_register_var(const std::string &name, DcAppValueIndex value_index);
DcAppVarIndex dc_app_get_var_index(const std::string &name);
void          dc_app_set_var_to_string(DcAppVar *var, const std::string value);
void          dc_app_refresh_var_from_extern(DcAppVar *var);
void          dc_app_refresh_var_from_value(DcAppVar *var);

// node utils
std::string    dc_app_node_type_to_string(DcAppNodeType type);
DcAppNode     *dc_app_index_to_node(DcAppNodeIndex index);
DcAppNodeIndex dc_app_register_node(DcAppNode node);

// larger scale functions
void dc_app_init_data();
void dc_app_clean_xml_data();

// app data
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

extern DcAppData dc_app_data;
