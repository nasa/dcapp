
//-----------------------------------------------------------------------------
// [SECTION] dcapp includes
//-----------------------------------------------------------------------------

#include <app.hpp>
#include <utils/file.hpp>
#include <utils/string.hpp>
#include <utils/xml.hpp>
#include <value.hpp>

DcAppData dc_app_data;

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#define PL_EXPERIMENTAL // required for experimental functionality

#include "pl.h"
#include <stdlib.h> // malloc, free
#include <string.h> // memset

#define PL_MATH_INCLUDE_FUNCTIONS // required to expose some of the color helpers
#include "pl_math.h"

// extensions
#include "pl_draw_ext.h"
#include "pl_profile_ext.h"
#include "pl_starter_ext.h"

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plAppData
{
    // window
    plWindow *ptWindow;
    plDrawLayer2D *ptFGLayer;

    // console variable
    bool bShowHelpWindow;
} plAppData;

//-----------------------------------------------------------------------------
// [SECTION] dcapp state
//-----------------------------------------------------------------------------

static void _refresh_variables();
static DcAppNodeIndex _process_node_children(xmlNodePtr xmlNode, DcAppNodeIndex nodeIndex, const std::string &directory);
static DcAppNodeIndex _process_node(xmlNodePtr xmlNode, DcAppNodeIndex parentNodeIndex, std::string directory);
static void _draw_node_list(plAppData *ptAppData, DcAppNodeIndex nodeIndex, plMat4 *nodeTransform);
static void _draw_node(plAppData *ptAppData, DcAppNodeIndex nodeIndex, plMat4 *parentTransform);

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

const plWindowI *gptWindows = NULL;
const plDrawI *gptDraw = NULL;
const plStarterI *gptStarter = NULL;
const plProfileI *gptProfile = NULL;
const plMemoryI *gptMemory = NULL;
const plLibraryI *gptLibrary = NULL;
const plIOI *gptIOI = NULL;

#define PL_ALLOC(x) gptMemory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_REALLOC(x, y) gptMemory->tracked_realloc((x), (y), __FILE__, __LINE__)
#define PL_FREE(x) gptMemory->tracked_realloc((x), 0, __FILE__, __LINE__)

//-----------------------------------------------------------------------------
// [SECTION] pl_app_load
//-----------------------------------------------------------------------------

PL_EXPORT void *
pl_app_load(plApiRegistryI *ptApiRegistry, plAppData *ptAppData)
{
    // NOTE: on first load, "pAppData" will be NULL but on reloads
    //       it will be the value returned from this function

    // if "ptAppData" is a valid pointer, then this function is being called
    // during a hot reload.
    if (ptAppData)
    {
        // re-retrieve the apis since we are now in
        // a different dll/so
        gptWindows = pl_get_api_latest(ptApiRegistry, plWindowI);
        gptDraw = pl_get_api_latest(ptApiRegistry, plDrawI);
        gptStarter = pl_get_api_latest(ptApiRegistry, plStarterI);
        gptProfile = pl_get_api_latest(ptApiRegistry, plProfileI);
        gptMemory = pl_get_api_latest(ptApiRegistry, plMemoryI);
        gptLibrary = pl_get_api_latest(ptApiRegistry, plLibraryI);
        gptIOI = pl_get_api_latest(ptApiRegistry, plIOI);

        return ptAppData;
    }

    // retrieve extension registry
    const plExtensionRegistryI *ptExtensionRegistry = pl_get_api_latest(ptApiRegistry, plExtensionRegistryI);

    // load extensions
    //   * first argument is the shared library name WITHOUT the extension
    //   * second & third argument is the load/unload functions names (use NULL for the default of "pl_load_ext" &
    //     "pl_unload_ext")
    //   * fourth argument indicates if the extension is reloadable (should we check for changes and reload if changed)
    ptExtensionRegistry->load("pl_unity_ext", NULL, NULL, true);
    ptExtensionRegistry->load("pl_platform_ext", NULL, NULL, false); // provides the file API used by the drawing ext

    // load required apis
    gptWindows = pl_get_api_latest(ptApiRegistry, plWindowI);
    gptDraw = pl_get_api_latest(ptApiRegistry, plDrawI);
    gptStarter = pl_get_api_latest(ptApiRegistry, plStarterI);
    gptProfile = pl_get_api_latest(ptApiRegistry, plProfileI);
    gptMemory = pl_get_api_latest(ptApiRegistry, plMemoryI);
    gptLibrary = pl_get_api_latest(ptApiRegistry, plLibraryI);
    gptIOI = pl_get_api_latest(ptApiRegistry, plIOI);

    // allocate app memory
    ptAppData = (plAppData *)PL_ALLOC(sizeof(plAppData));
    memset(ptAppData, 0, sizeof(plAppData));

    // default values
    ptAppData->bShowHelpWindow = true;

    // parse input arguments
    // if (argc < 2) {
    //     throw std::runtime_error("Missing dcapp configuration file");
    // }
    // std::vector<std::string> args(argv + 1, argv + argc);

    // // check for config file
    // std::string configRelativePath = args.front();
    // if (configRelativePath.empty()) {
    //     throw std::runtime_error("Missing dcapp configuration file");
    // }
    // std::string configRelativePath = "/home/nathan/dcapp-vk/samples/test/test.xml";

    // parse input arguments
    plIO *gptIo = gptIOI->get_io();
    if (gptIo->iArgc < 4)
    {
        throw std::runtime_error("Missing dcapp configuration file");
    }
    std::vector<std::string> args(gptIo->apArgv + 3, gptIo->apArgv + gptIo->iArgc);

    // TODO process input arguments (constant setting)
    std::string configRelativePath = args[0];

    // set paths
    std::filesystem::path fsFilePath = std::filesystem::canonical(configRelativePath);
    std::filesystem::path fsDirPath = fsFilePath.parent_path();
    std::string configFilePath = fsFilePath.string();
    std::string configDirPath = fsDirPath.string();

    // create cache and log dirs
    std::filesystem::path fsExePath = dc_utils_get_exe_filepath();
    std::filesystem::path fsLogPath = fsExePath.parent_path() / ".." / "logs";
    std::filesystem::create_directory(fsLogPath);
    std::string logDirPath = std::filesystem::canonical(fsLogPath).string();
    std::filesystem::path fsCachePath = fsExePath.parent_path() / ".." / "cache";
    std::filesystem::create_directory(fsCachePath);
    std::string cacheDirPath = std::filesystem::canonical(fsCachePath).string();

    // init dc_app_data object
    dc_app_init_data();

    // begin setting up dcappData object
    dc_app_data.configFilePath = configFilePath;
    dc_app_data.configDirPath = configDirPath;
    dc_app_data.logDirPath = logDirPath;
    dc_app_data.cacheDirPath = cacheDirPath;

    // set environment (used for dcapp XMLs)
    setenv("dcappDisplayHome", dc_app_data.configDirPath.c_str(), 1);

    // load XML file
    dc_app_data.doc = xmlReadFile(configFilePath.c_str(), "UTF-8", XML_PARSE_NOBLANKS);
    if (!dc_app_data.doc)
    {
        throw std::runtime_error("Unable to read configuration file: " + configFilePath);
    }

    // clean XML file
    dc_app_clean_xml_data();

    // save cleaned xml to file
    std::filesystem::path fsOutFile = std::filesystem::path(logDirPath) / "config.xml";
    xmlSaveFormatFile(fsOutFile.string().c_str(), dc_app_data.doc, 1);

    // process XML
    xmlNodePtr rootNode = xmlDocGetRootElement(dc_app_data.doc);
    _process_node(rootNode, DC_NODE_INDEX_UNDEFINED, configDirPath);

    // configure logic file
    if (dc_app_data.logic.library)
    {
        // set logic functions
        dc_app_data.logic.preInit = (void (*)(void))gptLibrary->load_function(dc_app_data.logic.library, "DisplayPreInit");
        dc_app_data.logic.init = (void (*)(void))gptLibrary->load_function(dc_app_data.logic.library, "DisplayInit");
        dc_app_data.logic.draw = (void (*)(void))gptLibrary->load_function(dc_app_data.logic.library, "DisplayDraw");
        dc_app_data.logic.close = (void (*)(void))gptLibrary->load_function(dc_app_data.logic.library, "DisplayClose");

        // set variables
        for (auto const &[name, variable] : dc_app_data.variables)
        {
            dc_app_data.variables[name].externData = gptLibrary->load_function(dc_app_data.logic.library, name.c_str());
        }
    }

    // validate
    // root->validate();

    // set initial window params
    DcAppNode *windowNode = dc_app_index_to_node(dc_app_data.window);
    plWindowDesc tWindowDesc = {
        .pcTitle = windowNode->window.title,
        .uWidth = (uint32_t)(dc_app_index_to_dc_value(windowNode->window.dimensions.x)->valueInteger),
        .uHeight = (uint32_t)(dc_app_index_to_dc_value(windowNode->window.dimensions.y)->valueInteger),
        .iXPos = dc_app_index_to_dc_value(windowNode->window.position.x)->valueInteger,
        .iYPos = dc_app_index_to_dc_value(windowNode->window.position.y)->valueInteger,
    };

    gptWindows->create(tWindowDesc, &ptAppData->ptWindow);
    gptWindows->show(ptAppData->ptWindow);

    // initialize the starter API (handles alot of boilerplate)
    plStarterInit tStarterInit = {
        .tFlags = PL_STARTER_FLAGS_ALL_EXTENSIONS,
        .ptWindow = ptAppData->ptWindow};
    gptStarter->initialize(tStarterInit);
    gptStarter->finalize();

    // return app memory
    return ptAppData;
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_shutdown
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_shutdown(plAppData *ptAppData)
{
    gptStarter->cleanup();
    gptWindows->destroy(ptAppData->ptWindow);
    PL_FREE(ptAppData);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_resize
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_resize(plAppData *ptAppData)
{
    gptStarter->resize();
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_update(plAppData *ptAppData)
{
    // this needs to be the first call when using the starter
    // extension. You must return if it returns false (usually a swapchain recreation).
    if (!gptStarter->begin_frame())
        return;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~drawing & profile API~~~~~~~~~~~~~~~~~~~~~~~~~~~

    gptProfile->begin_sample(0, "example drawing");

    ptAppData->ptFGLayer = gptStarter->get_foreground_layer();
    dc_app_data.logic.draw();
    _refresh_variables();
    _draw_node(ptAppData, dc_app_data.window, nullptr);

    gptProfile->end_sample(0);

    // must be the last function called when using the starter extension
    gptStarter->end_frame();
}

// update all variables
void _refresh_variables()
{
    for (auto const &[name, variable] : dc_app_data.variables)
    {
        DcValue *value = dc_app_index_to_dc_value(variable.valueIndex);
        void *externData = variable.externData;

        switch (value->type)
        {
        case DC_APP_VALUE_TYPE_FLOAT:
        {
            value->valueFloat = *((float *)(variable.externData));
            break;
        }
        case DC_APP_VALUE_TYPE_INTEGER:
        {
            value->valueInteger = *((int *)(variable.externData));
            break;
        }
        case DC_APP_VALUE_TYPE_STRING:
        {
            value->valueString = *((std::string *)(variable.externData));
            break;
        }
        case DC_APP_VALUE_TYPE_BOOLEAN:
        {
            value->valueBoolean = *((bool *)(variable.externData));
            break;
        }
        default:
            throw std::runtime_error("invalid DcValue type for variable");
            break;
        }
        dc_value_refresh_value(value);
    }
}

// returns the first child (if any)
DcAppNodeIndex _process_node_children(xmlNodePtr xmlNode, DcAppNodeIndex nodeIndex, const std::string &directory)
{
    xmlNodePtr xmlChildNode = xmlNode->children;

    DcAppNodeIndex firstChildIndex = DC_NODE_INDEX_UNDEFINED;
    DcAppNodeIndex previousChildNodeIndex = DC_NODE_INDEX_UNDEFINED;
    while (xmlChildNode)
    {
        DcAppNodeIndex childNodeIndex = _process_node(xmlChildNode, nodeIndex, directory);

        // get node addresses here since the address could change per node process
        DcAppNode *node = dc_app_index_to_node(nodeIndex);
        DcAppNode *childNode = dc_app_index_to_node(childNodeIndex);
        DcAppNode *previousChildNode = dc_app_index_to_node(previousChildNodeIndex);

        // if the current node and child exists
        if (node && childNode)
        {
            // set child's parent
            childNode->parent = nodeIndex;

            // set nodes's first child if this is the first child
            if (previousChildNodeIndex == DC_NODE_INDEX_UNDEFINED)
            {
                firstChildIndex = childNodeIndex;
            }
        }

        // if there is a previous node
        if (previousChildNode)
        {
            // set the next node of the previous node
            previousChildNode->next = childNodeIndex;
        }

        // set previous child node
        if (childNodeIndex != DC_NODE_INDEX_UNDEFINED)
        {
            previousChildNodeIndex = childNodeIndex;
        }

        // increment pointer
        xmlChildNode = xmlChildNode->next;
    }

    return firstChildIndex;
}

DcAppNodeIndex _process_node(xmlNodePtr xmlNode, DcAppNodeIndex parentNodeIndex, std::string directory)
{
    // by default, the element is not a node
    DcAppNodeIndex nodeIndex = DC_NODE_INDEX_UNDEFINED;

    switch (dc_app_xml_node_to_elem_type(xmlNode))
    {

    // ignore non-element nodes
    case DC_APP_ELEM_TYPE_NONELEM:
    {
        break;
    }

    case DC_APP_ELEM_TYPE_CONSTANT:
    {
        // name
        char *cName = dc_utils_get_attribute_string(xmlNode, "Name");
        if (!cName)
        {
            throw std::runtime_error("Non-existent node 'Name' in <Constant> definition");
        }
        std::string name = dc_app_dereference_constants(cName);
        free(cName);

        // value
        char *cValue = dc_utils_get_node_content_string(xmlNode);
        if (!cValue)
        {
            throw std::runtime_error("Non-existent node content in <Constant> definition");
        }
        std::string value = dc_app_dereference_constants(cValue);
        free(cValue);

        // set constant value
        dc_app_set_constant(name, value);
        break;
    }

    case DC_APP_ELEM_TYPE_CONTAINER:
    {
        DcAppNode dcNode = {
            .type = DC_APP_NODE_TYPE_CONTAINER,
        };

        // xPosition
        char *cXPosition = dc_utils_get_attribute_string(xmlNode, "X");
        if (cXPosition)
        {
            dcNode.container.position.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cXPosition));
            free(cXPosition);
        }
        else
        {
            dcNode.container.position.x = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // y position
        char *cYPosition = dc_utils_get_attribute_string(xmlNode, "Y");
        if (cYPosition)
        {
            dcNode.container.position.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cYPosition));
            free(cYPosition);
        }
        else
        {
            dcNode.container.position.y = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // x origin
        char *cXOrigin = dc_utils_get_attribute_string(xmlNode, "OriginX");
        if (cXOrigin)
        {
            dcNode.container.origin.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cXOrigin));
            free(cXOrigin);
        }
        else
        {
            dcNode.container.origin.x = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // y origin
        char *cYOrigin = dc_utils_get_attribute_string(xmlNode, "OriginY");
        if (cYOrigin)
        {
            dcNode.container.origin.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cYOrigin));
            free(cYOrigin);
        }
        else
        {
            dcNode.container.origin.y = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // x dimension
        char *cXDimension = dc_utils_get_attribute_string(xmlNode, "Width");
        if (cXDimension)
        {
            dcNode.container.dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cXDimension));
            free(cXDimension);
        }
        else
        {
            dcNode.container.dimensions.x = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // y dimension
        char *cYDimension = dc_utils_get_attribute_string(xmlNode, "Height");
        if (cYDimension)
        {
            dcNode.container.dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cYDimension));
            free(cYDimension);
        }
        else
        {
            dcNode.container.dimensions.y = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // virtual x dimension
        char *cXVirtualDimension = dc_utils_get_attribute_string(xmlNode, "VirtualWidth");
        if (cXVirtualDimension)
        {
            dcNode.container.virtualDimensions.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cXVirtualDimension));
            free(cXVirtualDimension);
        }
        else
        {
            dcNode.container.virtualDimensions.x = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // virtual y dimension
        char *cYVirtualDimension = dc_utils_get_attribute_string(xmlNode, "VirtualHeight");
        if (cYVirtualDimension)
        {
            dcNode.container.virtualDimensions.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cYVirtualDimension));
            free(cYVirtualDimension);
        }
        else
        {
            dcNode.container.virtualDimensions.y = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // x align
        char *cXAlign = dc_utils_get_attribute_string(xmlNode, "HorizontalAlign");
        if (cXAlign)
        {
            dcNode.container.alignment.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(cXAlign));
            free(cXAlign);
        }
        else
        {
            dcNode.container.alignment.x = dc_app_register_dc_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_LEFT));
        }

        // y align
        char *cYAlign = dc_utils_get_attribute_string(xmlNode, "VerticalAlign");
        if (cYAlign)
        {
            dcNode.container.alignment.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(cYAlign));
            free(cYAlign);
        }
        else
        {
            dcNode.container.alignment.y = dc_app_register_dc_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_BOTTOM));
        }

        // rotation
        char *cRotation = dc_utils_get_attribute_string(xmlNode, "Rotate");
        if (cRotation)
        {
            dcNode.container.rotation = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cRotation));
            free(cRotation);
        }
        else
        {
            dcNode.container.rotation = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // register node
        nodeIndex = dc_app_register_node(dcNode);

        // process children
        DcAppNodeIndex firstChildIndex = _process_node_children(xmlNode, nodeIndex, directory);

        // update child index
        DcAppNode *node = dc_app_index_to_node(nodeIndex);
        node->container.child = firstChildIndex;
        break;
    }

    // really just the root element, left in for legacy reasons
    case DC_APP_ELEM_TYPE_DCAPP:
    {
        _process_node_children(xmlNode, nodeIndex, directory);
        break;
    }

        // case DC_APP_ELEM_TYPE_DEM:
        // {
        //     DcAppNodeType parentType = dc_app_index_to_node(parentNodeIndex)->type;
        //     switch (parentType)
        //     {
        //     case DC_APP_ELEM_TYPE_MAP:
        //     {
        //         std::string filename = dc_utils_filepath_to_canonical(dc_utils_get_attribute_string(node, "File"), directory);
        //         ((DcMap *)parent)->addDem(filename);
        //         break;
        //     }

        //     default:
        //         throw std::runtime_error("Adding DEM to invalid parent of type " + std::to_string(parentType));
        //         break;
        //     }
        //     break;
        // }

    case DC_APP_ELEM_TYPE_IF:
    {
        DcAppNode dcNode = (DcAppNode){
            .type = DC_APP_NODE_TYPE_CONDITIONAL,
            .conditional = (DcAppNodeConditional){
                .value1 = DC_VALUE_INDEX_UNDEFINED,
                .value2 = DC_VALUE_INDEX_UNDEFINED,
                .childTrue = DC_NODE_INDEX_UNDEFINED,
                .childFalse = DC_NODE_INDEX_UNDEFINED,
            }};

        // conditional type
        char *cType = dc_utils_get_attribute_string(xmlNode, "Operation");
        if (cType)
        {
            dcNode.conditional.type = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(cType));
            free(cType);
        }
        else
        {
            dcNode.conditional.type = dc_app_register_dc_value(dc_value_create_value_integer(DC_APP_CONDITIONAL_TYPE_TRUE));
        }

        // value1
        char *cValue = dc_utils_get_attribute_string(xmlNode, "Value");
        if (cValue)
        {
            dcNode.conditional.value1 = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_STRING, dc_app_dereference_constants(cValue));
            free(cValue);
        }
        else
        {
            cValue = dc_utils_get_attribute_string(xmlNode, "Value1");
            if (cValue)
            {
                dcNode.conditional.value1 = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_STRING, dc_app_dereference_constants(cValue));
                free(cValue);
            }
            else
            {
                throw std::runtime_error("Invalid conditional; no value specified");
            }
        }

        // value2
        char *cValue2 = dc_utils_get_attribute_string(xmlNode, "Value2");
        if (cValue2)
        {
            dcNode.conditional.value2 = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_STRING, dc_app_dereference_constants(cValue2));
            free(cValue2);
        }

        // register node
        nodeIndex = dc_app_register_node(dcNode);

        // process children (assigning to true/false handled in separate cases, e.g. DC_APP_ELEM_TYPE_TRUE)
        DcAppNodeIndex firstChildIndex = _process_node_children(xmlNode, nodeIndex, directory);

        // handle implicit <True> elements
        if (firstChildIndex != DC_NODE_INDEX_UNDEFINED)
        {
            // ignore if True element already exists
            DcAppNode *node = dc_app_index_to_node(nodeIndex);
            if (node->conditional.childTrue == DC_NODE_INDEX_UNDEFINED)
            {
                node->conditional.childTrue = firstChildIndex;
            }
            else
            {
                printf("Warning: <If> element has <True> explicit and implicit elements. Ignoring the implicit definitions\n");
            }
        }
        break;
    }

    // at this point, just used to set the directory path
    case DC_APP_ELEM_TYPE_INCLUDE:
    {
        char *cDirectory = dc_utils_get_attribute_string(xmlNode, "Directory");
        if (cDirectory)
        {
            nodeIndex = _process_node_children(xmlNode, parentNodeIndex, dc_app_dereference_constants(cDirectory));
            free(cDirectory);
        }
        else
        {
            // should never get here
            throw std::runtime_error("Invalid condition; <Include> node with no directory");
        }
        break;
    }

    case DC_APP_ELEM_TYPE_LOGIC:
    {
        if (dc_app_data.logic.library)
        {
            throw std::runtime_error("Duplicate <Logic> definitions");
        }
        char *cFilePath = dc_utils_get_attribute_string(xmlNode, "File");
        if (cFilePath)
        {
            std::string filePath = dc_utils_filepath_to_canonical(dc_app_dereference_constants(cFilePath), directory);
            free(cFilePath);

            // verify filepath
            if (filePath.empty())
            {
                throw std::runtime_error("Invalid logic file of empty filename");
            }

            // open .so file
            const plLibraryDesc logicSoDesc = {
                .tFlags = PL_LIBRARY_FLAGS_NONE,
                .pcName = filePath.c_str(),
            };
            if (gptLibrary->load(logicSoDesc, &dc_app_data.logic.library) != PL_LIBRARY_RESULT_SUCCESS)
            {
                throw std::runtime_error("Failed to load logic .so file");
            }
        }
        else
        {
            throw std::runtime_error("Invalid condition; <Logic> node with no file");
        }
        break;
    }

        // case DC_APP_ELEM_TYPE_MAP:
        // {
        //     DcValue *xPosition = attributeToDcValue(node, "X");
        //     DcValue *yPosition = attributeToDcValue(node, "Y");
        //     DcValue *xOrigin = attributeToDcValue(node, "OriginX");
        //     DcValue *yOrigin = attributeToDcValue(node, "OriginY");
        //     DcValue *width = attributeToDcValue(node, "Width");
        //     DcValue *height = attributeToDcValue(node, "Height");
        //     DcValue *horizontalAlign = attributeToDcValue(node, "HorizontalAlign");
        //     DcValue *verticalAlign = attributeToDcValue(node, "VerticalAlign");
        //     DcValue *rotation = attributeToDcValue(node, "Rotation");
        //     DcValue *latitude = attributeToDcValue(node, "Latitude");
        //     DcValue *longitude = attributeToDcValue(node, "Longitude");
        //     DcValue *yaw = attributeToDcValue(node, "Yaw");

        //     DcMap *map = new DcMap((DcParent *)parent, xPosition, yPosition, xOrigin, yOrigin, width, height, horizontalAlign, verticalAlign, rotation, latitude, longitude, yaw);
        //     processElementChildren(map, node, directory);
        //     break;
        // }

    case DC_APP_ELEM_TYPE_PANEL:
    {
        DcAppNode dcNode = {
            .type = DC_APP_NODE_TYPE_PANEL,
        };

        // parent dimensions
        // TODO probably don't need this.....must be a better way to architect
        dcNode.panel.parentDimensions = dc_app_index_to_node(parentNodeIndex)->window.virtualDimensions;

        // virtual x dimension
        char *cXVirtualDimension = dc_utils_get_attribute_string(xmlNode, "VirtualWidth");
        if (cXVirtualDimension)
        {
            dcNode.panel.virtualDimensions.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cXVirtualDimension));
            free(cXVirtualDimension);
        }
        else
        {
            dcNode.panel.virtualDimensions.x = dcNode.panel.parentDimensions.x;
        }

        // virtual y dimension
        char *cYVirtualDimension = dc_utils_get_attribute_string(xmlNode, "VirtualHeight");
        if (cYVirtualDimension)
        {
            dcNode.panel.virtualDimensions.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cYVirtualDimension));
            free(cYVirtualDimension);
        }
        else
        {
            dcNode.panel.virtualDimensions.y = dcNode.panel.parentDimensions.y;
        }

        // register node
        nodeIndex = dc_app_register_node(dcNode);

        // process children
        DcAppNodeIndex firstChildIndex = _process_node_children(xmlNode, nodeIndex, directory);

        // update child index
        DcAppNode *node = dc_app_index_to_node(nodeIndex);
        node->panel.child = firstChildIndex;

        break;
    }

    case DC_APP_ELEM_TYPE_POLYGON:
    {
        DcAppNode dcNode = {
            .type = DC_APP_NODE_TYPE_POLYGON,
            .polygon = (DcAppNodePolygon){
                .fillEnabled = false,
                .lineEnabled = false,
            }};

        // fill color
        char *cFillColor = dc_utils_get_attribute_string(xmlNode, "FillColor");
        if (cFillColor)
        {
            // split string by whitespace
            std::string sFillColor = dc_app_dereference_constants(cFillColor);
            std::vector<std::string> substrings = dc_utils_split_string_by_delimiters(sFillColor, dc_utils_string_whitespace);

            if (substrings.size() > 0)
            {
                dcNode.polygon.fillColor.r = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[0]);
                if (substrings.size() > 1)
                {
                    dcNode.polygon.fillColor.g = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[1]);
                    if (substrings.size() > 2)
                    {
                        dcNode.polygon.fillColor.b = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[2]);
                        if (substrings.size() > 3)
                        {
                            dcNode.polygon.fillColor.a = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[3]);
                        }
                        else
                        {
                            dcNode.polygon.fillColor.a = dc_app_register_dc_value(dc_value_create_value_float(1.0f));
                        }
                    }
                    else
                    {
                        dcNode.polygon.fillColor.b = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
                    }
                }
                else
                {
                    dcNode.polygon.fillColor.g = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
                }
            }
            else
            {
                dcNode.polygon.fillColor.r = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
            }
            dcNode.polygon.fillEnabled = true;
            free(cFillColor);
        }

        // line color
        char *cLineColor = dc_utils_get_attribute_string(xmlNode, "LineColor");
        if (cLineColor)
        {
            // split string by whitespace
            std::string sFillColor = dc_app_dereference_constants(cLineColor);
            std::vector<std::string> substrings = dc_utils_split_string_by_delimiters(sFillColor, dc_utils_string_whitespace);

            if (substrings.size() > 0)
            {
                dcNode.polygon.lineColor.r = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[0]);
                if (substrings.size() > 1)
                {
                    dcNode.polygon.lineColor.g = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[1]);
                    if (substrings.size() > 2)
                    {
                        dcNode.polygon.lineColor.b = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[2]);
                        if (substrings.size() > 3)
                        {
                            dcNode.polygon.lineColor.a = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[3]);
                        }
                        else
                        {
                            dcNode.polygon.lineColor.a = dc_app_register_dc_value(dc_value_create_value_float(1.0f));
                        }
                    }
                    else
                    {
                        dcNode.polygon.lineColor.b = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
                    }
                }
                else
                {
                    dcNode.polygon.lineColor.g = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
                }
            }
            else
            {
                dcNode.polygon.lineColor.r = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
            }
            dcNode.polygon.lineEnabled = true;
            free(cLineColor);
        }

        // line width
        char *cLineWidth = dc_utils_get_attribute_string(xmlNode, "LineWidth");
        if (cLineWidth)
        {
            dcNode.polygon.lineWidth = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cLineWidth));
            free(cLineWidth);
        }
        else
        {
            dcNode.polygon.lineWidth = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // initialize points to 0
        dcNode.polygon.numPoints = 0;
        dcNode.polygon.points = nullptr;

        // register node
        nodeIndex = dc_app_register_node(dcNode);

        // process children
        _process_node_children(xmlNode, nodeIndex, directory);
        break;
    }

    case DC_APP_ELEM_TYPE_VARIABLE:
    {
        // name
        char *cName = dc_utils_get_node_content_string(xmlNode);
        if (!cName)
        {
            throw std::runtime_error("Non-existent node content in <Variable> definition");
        }
        std::string name = dc_app_dereference_constants(cName);
        free(cName);

        // type
        char *cType = dc_utils_get_attribute_string(xmlNode, "Type");
        DcValueType type = DC_APP_VALUE_TYPE_STRING;
        if (cType)
        {
            type = dc_value_type_from_string(dc_app_dereference_constants(cType));
        }

        // value
        char *cInitialValue = dc_utils_get_attribute_string(xmlNode, "InitialValue");
        DcValue initialValue;
        if (cInitialValue)
        {
            initialValue = dc_value_create_value_string(dc_app_dereference_constants(cInitialValue));
        }
        else
        {
            initialValue = dc_value_create_value_string("");
        }
        initialValue.type = type;
        initialValue.isDynamic = true;

        // register variable
        DcAppValueIndex index = dc_app_register_dc_value(initialValue);
        dc_app_set_variable(name, index);

        break;
    }

    case DC_APP_ELEM_TYPE_VERTEX:
    {
        DcAppNode *parentNode = dc_app_index_to_node(parentNodeIndex);
        switch (parentNode->type)
        {
        case DC_APP_NODE_TYPE_POLYGON:
        {
            // xPosition
            char *cXPosition = dc_utils_get_attribute_string(xmlNode, "X");
            if (!cXPosition)
            {
                throw std::runtime_error("Invalid Vertex: No X attribute");
            }

            // yPosition
            char *cYPosition = dc_utils_get_attribute_string(xmlNode, "Y");
            if (!cYPosition)
            {
                throw std::runtime_error("Invalid Vertex: No Y attribute");
            }

            // reallocate and add vertices
            parentNode->polygon.numPoints++;
            parentNode->polygon.points = (DcAppValueIndex2 *)realloc(parentNode->polygon.points, parentNode->polygon.numPoints * sizeof(DcAppValueIndex2));
            parentNode->polygon.points[parentNode->polygon.numPoints - 1] = (DcAppValueIndex2){
                dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cXPosition)),
                dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cYPosition))};
            break;
        }
        default:
            // TODO add a nodeTypeToString() function
            throw std::runtime_error("Invalid parent of type " + std::string("<Unknown>") + "for vertex.");
        }
        break;
    }

    case DC_APP_ELEM_TYPE_WINDOW:
    {
        DcAppNode dcNode = {
            .type = DC_APP_NODE_TYPE_WINDOW,
        };

        // title
        std::string title = "dcapp";
        char *cTitle = dc_utils_get_attribute_string(xmlNode, "Title");
        if (cTitle)
        {
            title = dc_app_dereference_constants(cTitle);
            free(cTitle);
        }
        dcNode.window.title = (char *)malloc(title.length() + 1);
        memcpy(dcNode.window.title, title.c_str(), title.length() + 1);

        // xPosition
        char *cXPosition = dc_utils_get_attribute_string(xmlNode, "X");
        if (cXPosition)
        {
            dcNode.window.position.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(cXPosition));
            free(cXPosition);
        }
        else
        {
            dcNode.window.position.x = dc_app_register_dc_value(dc_value_create_value_integer(0.0f));
        }

        // y position
        char *cYPosition = dc_utils_get_attribute_string(xmlNode, "Y");
        if (cYPosition)
        {
            dcNode.window.position.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(cYPosition));
            free(cYPosition);
        }
        else
        {
            dcNode.window.position.y = dc_app_register_dc_value(dc_value_create_value_integer(0.0f));
        }

        // x dimension
        char *cXDimension = dc_utils_get_attribute_string(xmlNode, "Width");
        if (cXDimension)
        {
            dcNode.window.dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(cXDimension));
            free(cXDimension);
        }
        else
        {
            dcNode.window.dimensions.x = dc_app_register_dc_value(dc_value_create_value_integer(0.0f));
        }

        // y dimension
        char *cYDimension = dc_utils_get_attribute_string(xmlNode, "Height");
        if (cYDimension)
        {
            dcNode.window.dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(cYDimension));
            free(cYDimension);
        }
        else
        {
            dcNode.window.dimensions.y = dc_app_register_dc_value(dc_value_create_value_integer(0.0f));
        }

        // virtual x dimension
        char *cXVirtualDimension = dc_utils_get_attribute_string(xmlNode, "VirtualWidth");
        if (cXVirtualDimension)
        {
            dcNode.window.virtualDimensions.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cXVirtualDimension));
            free(cXVirtualDimension);
        }
        else
        {
            dcNode.window.virtualDimensions.x = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // virtual y dimension
        char *cYVirtualDimension = dc_utils_get_attribute_string(xmlNode, "VirtualHeight");
        if (cYVirtualDimension)
        {
            dcNode.window.virtualDimensions.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(cYVirtualDimension));
            free(cYVirtualDimension);
        }
        else
        {
            dcNode.window.virtualDimensions.y = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // register node
        nodeIndex = dc_app_register_node(dcNode);

        // process children
        DcAppNodeIndex firstChildIndex = _process_node_children(xmlNode, nodeIndex, directory);

        // update child index
        DcAppNode *node = dc_app_index_to_node(nodeIndex);
        node->window.child = firstChildIndex;

        // set global window
        dc_app_data.window = nodeIndex;
        break;
    }
    default:
        throw std::runtime_error("Invalid node in _process_node()");
    }

    return nodeIndex;
}

void _draw_node_list(plAppData *ptAppData, DcAppNodeIndex nodeIndex, plMat4 *nodeTransform)
{
    DcAppNodeIndex currentNodeIndex = nodeIndex;
    while (currentNodeIndex != DC_NODE_INDEX_UNDEFINED)
    {
        _draw_node(ptAppData, currentNodeIndex, nodeTransform);
        currentNodeIndex = dc_app_index_to_node(currentNodeIndex)->next;
    }
}

void _draw_node(plAppData *ptAppData, DcAppNodeIndex nodeIndex, plMat4 *parentTransform)
{
    if (nodeIndex == DC_NODE_INDEX_UNDEFINED)
    {
        throw std::runtime_error("Attempting to draw undefined node index");
    }

    DcAppNode *node = dc_app_index_to_node(nodeIndex);
    switch (node->type)
    {
    case DC_APP_NODE_TYPE_CONTAINER:
    {
        float xPosition = 0;
        switch (dc_app_index_to_dc_value(node->container.alignment.x)->valueInteger)
        {
        case DC_APP_ALIGN_TYPE_LEFT:
            xPosition = dc_app_index_to_dc_value(node->container.position.x)->valueFloat;
            break;
        case DC_APP_ALIGN_TYPE_CENTER:
            xPosition = dc_app_index_to_dc_value(node->container.position.x)->valueFloat - dc_app_index_to_dc_value(node->container.dimensions.x)->valueFloat / 2;
            break;
        case DC_APP_ALIGN_TYPE_RIGHT:
            xPosition = dc_app_index_to_dc_value(node->container.position.x)->valueFloat - dc_app_index_to_dc_value(node->container.dimensions.x)->valueFloat;
            break;
        }

        float yPosition = 0;
        switch (dc_app_index_to_dc_value(node->container.alignment.y)->valueInteger)
        {
        case DC_APP_ALIGN_TYPE_LEFT:
            yPosition = dc_app_index_to_dc_value(node->container.position.y)->valueFloat;
            break;
        case DC_APP_ALIGN_TYPE_CENTER:
            yPosition = dc_app_index_to_dc_value(node->container.position.y)->valueFloat - dc_app_index_to_dc_value(node->container.dimensions.y)->valueFloat / 2;
            break;
        case DC_APP_ALIGN_TYPE_RIGHT:
            yPosition = dc_app_index_to_dc_value(node->container.position.y)->valueFloat - dc_app_index_to_dc_value(node->container.dimensions.y)->valueFloat;
            break;
        }

        plMat4 transOriginMatrix = pl_mat4_translate_xyz(
            dc_app_index_to_dc_value(node->container.origin.x)->valueFloat,
            dc_app_index_to_dc_value(node->container.origin.y)->valueFloat,
            0.0f);
        plMat4 rotateMatrix = pl_mat4_rotate_vec3(
            pl_radiansf(dc_app_index_to_dc_value(node->container.rotation)->valueFloat),
            (plVec3){0.0f, 0.0f, 1.0f});
        plMat4 transPositionMatrix = pl_mat4_translate_xyz(
            xPosition,
            yPosition,
            0.0f);
        plMat4 scaleMatrix = pl_mat4_scale_xyz(
            dc_app_index_to_dc_value(node->container.dimensions.x)->valueFloat / dc_app_index_to_dc_value(node->container.virtualDimensions.x)->valueFloat,
            dc_app_index_to_dc_value(node->container.dimensions.y)->valueFloat / dc_app_index_to_dc_value(node->container.virtualDimensions.y)->valueFloat,
            1.0f);

        plMat4 transform = (plMat4){0};
        transform = pl_mul_mat4t(parentTransform, &transOriginMatrix);
        transform = pl_mul_mat4t(&transform, &rotateMatrix);
        transform = pl_mul_mat4t(&transform, &transPositionMatrix);
        transform = pl_mul_mat4t(&transform, &scaleMatrix);
        _draw_node_list(ptAppData, node->container.child, &transform);
        break;
    }

    case DC_APP_NODE_TYPE_CONDITIONAL:
    {
        // TODO implement
        break;
    }

    case DC_APP_NODE_TYPE_MAP:
    {
        // TODO implement
        break;
    }

    case DC_APP_NODE_TYPE_PANEL:
    {
        plMat4 scaleMatrix = pl_mat4_scale_xyz(
            dc_app_index_to_dc_value(node->panel.parentDimensions.x)->valueFloat / dc_app_index_to_dc_value(node->panel.virtualDimensions.x)->valueFloat,
            dc_app_index_to_dc_value(node->panel.parentDimensions.y)->valueFloat / dc_app_index_to_dc_value(node->panel.virtualDimensions.y)->valueFloat,
            1.0f);
        plMat4 transform = (plMat4){0};
        transform = pl_mul_mat4t(parentTransform, &scaleMatrix);
        _draw_node_list(ptAppData, node->panel.child, &transform);
        break;
    }

    case DC_APP_NODE_TYPE_POLYGON:
    {
        // get points
        std::vector<plVec2> points;
        points.resize(node->polygon.numPoints);
        for (int ii = 0; ii < node->polygon.numPoints; ii++)
        {
            plVec4 point4 = (plVec4){
                dc_app_index_to_dc_value(node->polygon.points[ii].x)->valueFloat,
                dc_app_index_to_dc_value(node->polygon.points[ii].y)->valueFloat,
                0, 1};
            point4 = pl_mul_mat4_vec4(parentTransform, point4);
            points[ii] = (plVec2){point4.x, point4.y};
        }

        // draw fill
        if (node->polygon.fillEnabled)
        {
            uint32_t fillColor = PL_COLOR_32_RGBA(
                dc_app_index_to_dc_value(node->polygon.fillColor.r)->valueFloat,
                dc_app_index_to_dc_value(node->polygon.fillColor.g)->valueFloat,
                dc_app_index_to_dc_value(node->polygon.fillColor.b)->valueFloat,
                dc_app_index_to_dc_value(node->polygon.fillColor.a)->valueFloat);
            gptDraw->add_convex_polygon_filled(ptAppData->ptFGLayer, points.data(), points.size(), (plDrawSolidOptions){.uColor = fillColor});
        }

        // draw outline
        if (node->polygon.lineEnabled)
        {
            float lineThickness = dc_app_index_to_dc_value(node->polygon.lineWidth)->valueFloat;
            uint32_t lineColor = PL_COLOR_32_RGBA(
                dc_app_index_to_dc_value(node->polygon.lineColor.r)->valueFloat,
                dc_app_index_to_dc_value(node->polygon.lineColor.g)->valueFloat,
                dc_app_index_to_dc_value(node->polygon.lineColor.b)->valueFloat,
                dc_app_index_to_dc_value(node->polygon.lineColor.a)->valueFloat);
            gptDraw->add_polygon(ptAppData->ptFGLayer, points.data(), points.size(), (plDrawLineOptions){.uColor = lineColor, .fThickness = lineThickness});
        }

        break;
    }

    case DC_APP_NODE_TYPE_SET:
    {
        // todo implement this
        break;
    }

    case DC_APP_NODE_TYPE_WINDOW:
    {
        // TODO move this code to only the resize() function
        // update dimensions
        uint32_t dimensionX, dimensionY;

        // TODO fix this in pilotlight for macos
        gptWindows->get_size(ptAppData->ptWindow, &dimensionX, &dimensionY);
        DcValue *dimensionValueX = dc_app_index_to_dc_value(node->window.dimensions.x);
        DcValue *dimensionValueY = dc_app_index_to_dc_value(node->window.dimensions.y);
        dimensionValueX->valueInteger = (int)dimensionX;
        dimensionValueY->valueInteger = (int)dimensionY;
        dc_value_refresh_value(dimensionValueX);
        dc_value_refresh_value(dimensionValueY);

        // compute transforms
        // translate from negative to positive range
        plMat4 transMatrix = pl_mat4_translate_xyz(
            0.0f,
            dc_app_index_to_dc_value(node->window.dimensions.y)->valueFloat,
            0.0f);

        // scale from virtual to real dimensions, flip y axis
        plMat4 scaleMatrix = pl_mat4_scale_xyz(
            dc_app_index_to_dc_value(node->window.dimensions.x)->valueFloat / dc_app_index_to_dc_value(node->window.virtualDimensions.x)->valueFloat,
            dc_app_index_to_dc_value(node->window.dimensions.y)->valueFloat / dc_app_index_to_dc_value(node->window.virtualDimensions.y)->valueFloat * -1.0f,
            1.0f);

        plMat4 transform;
        transform = pl_mul_mat4t(&transMatrix, &scaleMatrix);
        _draw_node_list(ptAppData, node->window.child, &transform);
        break;
    }

    default:
        break;
    }
}
