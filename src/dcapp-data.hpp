#pragma once

// dcapp includes
#define PL_EXPERIMENTAL 
#include <value.hpp>

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

namespace dc
{
    // element utils
    enum DcElemType
    {
        DC_ELEM_TYPE_UNDEFINED,
        DC_ELEM_TYPE_CONSTANT,
        DC_ELEM_TYPE_CONTAINER,
        DC_ELEM_TYPE_DCAPP,
        DC_ELEM_TYPE_DEM,
        DC_ELEM_TYPE_DUMMY,
        DC_ELEM_TYPE_FALSE,
        DC_ELEM_TYPE_IF,
        DC_ELEM_TYPE_INCLUDE,
        DC_ELEM_TYPE_LOGIC,
        DC_ELEM_TYPE_MAP,
        DC_ELEM_TYPE_NONELEM,
        DC_ELEM_TYPE_PANEL,
        DC_ELEM_TYPE_POLYGON,
        DC_ELEM_TYPE_TRUE,
        DC_ELEM_TYPE_VARIABLE,
        DC_ELEM_TYPE_VERTEX,
        DC_ELEM_TYPE_WINDOW,
    };
    std::string elemToString(DcElemType type);
    DcElemType stringToElem(std::string name);
    DcElemType xmlNodeToElementType(xmlNodePtr node);

    // value utils
    typedef uint32_t DcValueIndex;
    const DcValueIndex DC_VALUE_INDEX_UNDEFINED = 0;

    typedef struct _DcValueIndex2
    {
        union
        {
            DcValueIndex x, r, lat;
        };
        union
        {
            DcValueIndex y, g, lon;
        };
    } DcValueIndex2;

    typedef struct _DcValueIndex3
    {
        union
        {
            DcValueIndex x, r, lat;
        };
        union
        {
            DcValueIndex y, g, lon;
        };
        union
        {
            DcValueIndex z, b, ele;
        };
    } DcValueIndex3;

    typedef struct _DcValueIndex4
    {
        union
        {
            DcValueIndex x, r;
        };
        union
        {
            DcValueIndex y, g;
        };
        union
        {
            DcValueIndex z, b;
        };
        union
        {
            DcValueIndex w, a;
        };
    } DcValueIndex4;

    // alignment utils
    enum DcAlignType
    {
        DC_ALIGN_TYPE_UNDEFINED,
        DC_ALIGN_TYPE_LEFT,
        DC_ALIGN_TYPE_CENTER,
        DC_ALIGN_TYPE_RIGHT,
        DC_ALIGN_TYPE_BOTTOM,
        DC_ALIGN_TYPE_MIDDLE,
        DC_ALIGN_TYPE_TOP,
    };

    // conditional utils
    enum DcConditionalType
    {
        DC_CONDITIONAL_TYPE_UNDEFINED,
        DC_CONDITIONAL_TYPE_TRUE,
        DC_CONDITIONAL_TYPE_FALSE,
        DC_CONDITIONAL_TYPE_EQ,
        DC_CONDITIONAL_TYPE_NE,
        DC_CONDITIONAL_TYPE_LT,
        DC_CONDITIONAL_TYPE_GT,
        DC_CONDITIONAL_TYPE_LTE,
        DC_CONDITIONAL_TYPE_GTE,
    };

    // node utils
    enum DcNodeType
    {
        DC_NODE_TYPE_UNDEFINED,
        DC_NODE_TYPE_CONTAINER,
        DC_NODE_TYPE_CONDITIONAL,
        DC_NODE_TYPE_MAP,
        DC_NODE_TYPE_PANEL,
        DC_NODE_TYPE_POLYGON,
        DC_NODE_TYPE_SET,
        DC_NODE_TYPE_WINDOW,
    };

    typedef uint32_t DcNodeIndex;
    const DcNodeIndex DC_NODE_INDEX_UNDEFINED = 0;

    typedef struct _DcNodeConditional
    {
        DcValueIndex type;
        DcValueIndex value1;
        DcValueIndex value2;
        DcNodeIndex childTrue;
        DcNodeIndex childFalse;
    } DcNodeConditional;

    typedef struct _DcNodeContainer
    {
        DcValueIndex2 position;
        DcValueIndex2 origin;
        DcValueIndex2 dimensions;
        DcValueIndex2 virtualDimensions;
        DcValueIndex2 alignment;
        DcValueIndex rotation;
        DcNodeIndex child;
    } DcNodeContainer;

    typedef struct _DcNodeMap
    {
        DcValueIndex2 position;
        DcValueIndex2 origin;
        DcValueIndex2 dimensions;
        DcValueIndex2 virtualDimensions;
        DcValueIndex2 alignment;
        DcValueIndex3 lle;
        DcValueIndex yaw;
        DcValueIndex rotation;
        // DcDemManager *demManager;
    } DcNodeMap;

    typedef struct _DcNodePanel
    {
        DcValueIndex2 parentDimensions;
        DcValueIndex2 virtualDimensions;
        DcValueIndex index;
        DcNodeIndex child;
    } DcNodePanel;

    typedef struct _DcNodePolygon
    {
        DcValueIndex4 fillColor;
        DcValueIndex4 lineColor;
        DcValueIndex lineWidth;
        DcValueIndex2 *points;
        uint32_t numPoints;
        bool fillEnabled;
        bool lineEnabled;
    } DcNodePolygon;

    typedef struct _DcNodeWindow
    {
        DcValueIndex2 position;
        DcValueIndex2 dimensions;
        DcValueIndex2 virtualDimensions;
        DcNodeIndex child;
        char *title;
    } DcNodeWindow;

    typedef struct _DcNode
    {
        DcNodeType type;
        DcNodeIndex parent;
        DcNodeIndex next;
        union
        {
            DcNodeConditional conditional;
            DcNodeContainer container;
            DcNodeMap map;
            DcNodePanel panel;
            DcNodePolygon polygon;
            DcNodeWindow window;
        };
    } DcNode;

    // logic
    typedef struct _DcLogic
    {
        plSharedLibrary *library;
        void (*preInit)();
        void (*init)();
        void (*draw)();
        void (*close)();
    } DcLogic;

    // variables
    typedef struct _DcVariable
    {
        void *externData;
        DcValueIndex valueIndex;
    } DcVariable;

    // constant utils
    void setConstant(const std::string &name, const std::string &text);
    std::string getConstant(const std::string &name);
    std::string dereferenceConstants(const char *text);
    std::string dereferenceConstants(std::string text);

    // value utils
    DcValue *indexToDcValue(DcValueIndex index);
    DcValueIndex registerDcValue(DcValue value);
    DcValueIndex createAndRegisterTypedDcValueFromString(DcValueType type, const char *text);
    DcValueIndex createAndRegisterTypedDcValueFromString(DcValueType type, std::string text);

    // variable utils
    void setVariable(const std::string &name, DcValueIndex valueIndex);

    // node utils
    std::string dcNodeTypeToString(DcNodeType type);
    DcNode *indexToDcNode(DcNodeIndex index);
    DcNodeIndex registerDcNode(DcNode node);

    // larger scale functions
    void initData();
    void cleanXmlData();

    // app data
    typedef struct _DcappData
    {
        // string path to config file, directories
        std::string configFilePath;
        std::string configDirPath;
        std::string cacheDirPath;
        std::string logDirPath;

        // values
        std::map<std::string, std::string> constants;
        std::map<std::string, DcVariable> variables;
        std::vector<DcValue> values;

        // nodes
        std::vector<DcNode> nodes;

        // window (root node)
        DcNodeIndex window;

        // logic
        DcLogic logic;

        // xml data
        xmlDocPtr doc;
    } DcappData;

    extern DcappData dcData;
} // namespace dc
