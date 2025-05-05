
//-----------------------------------------------------------------------------
// [SECTION] dcapp includes
//-----------------------------------------------------------------------------

#include <dcapp-data.hpp>
#include <utils/file-utils.hpp>
#include <utils/string-utils.hpp>
#include <utils/xml-utils.hpp>
#include <value.hpp>

namespace dc
{
    DcappData dcData;
}

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

namespace dc
{
    static void refreshVariables();
    static DcNodeIndex processNodeChildren(xmlNodePtr xmlNode, DcNodeIndex nodeIndex, const std::string &directory);
    static DcNodeIndex processNode(xmlNodePtr xmlNode, DcNodeIndex parentNodeIndex, std::string directory);
    static void drawNodeList(plAppData *ptAppData, DcNodeIndex nodeIndex, plMat4 *nodeTransform);
    static void drawNode(plAppData *ptAppData, DcNodeIndex nodeIndex, plMat4 *parentTransform);
} // namespace dc

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
    std::filesystem::path fsExePath = getExeFilepath();
    std::filesystem::path fsLogPath = fsExePath.parent_path() / ".." / "logs";
    std::filesystem::create_directory(fsLogPath);
    std::string logDirPath = std::filesystem::canonical(fsLogPath).string();
    std::filesystem::path fsCachePath = fsExePath.parent_path() / ".." / "cache";
    std::filesystem::create_directory(fsCachePath);
    std::string cacheDirPath = std::filesystem::canonical(fsCachePath).string();

    // init dcData object
    dc::initData();

    // begin setting up dcappData object
    dc::dcData.configFilePath = configFilePath;
    dc::dcData.configDirPath = configDirPath;
    dc::dcData.logDirPath = logDirPath;
    dc::dcData.cacheDirPath = cacheDirPath;

    // set environment (used for dcapp XMLs)
    setenv("dcappDisplayHome", dc::dcData.configDirPath.c_str(), 1);

    // load XML file
    dc::dcData.doc = xmlReadFile(configFilePath.c_str(), "UTF-8", XML_PARSE_NOBLANKS);
    if (!dc::dcData.doc)
    {
        throw std::runtime_error("Unable to read configuration file: " + configFilePath);
    }

    // clean XML file
    dc::cleanXmlData();

    // save cleaned xml to file
    std::filesystem::path fsOutFile = std::filesystem::path(logDirPath) / "config.xml";
    xmlSaveFormatFile(fsOutFile.string().c_str(), dc::dcData.doc, 1);

    // process XML
    xmlNodePtr rootNode = xmlDocGetRootElement(dc::dcData.doc);
    dc::processNode(rootNode, dc::DC_NODE_INDEX_UNDEFINED, configDirPath);

    // configure logic file
    if (dc::dcData.logic.library)
    {
        // set logic functions
        dc::dcData.logic.preInit = (void (*)(void))gptLibrary->load_function(dc::dcData.logic.library, "DisplayPreInit");
        dc::dcData.logic.init = (void (*)(void))gptLibrary->load_function(dc::dcData.logic.library, "DisplayInit");
        dc::dcData.logic.draw = (void (*)(void))gptLibrary->load_function(dc::dcData.logic.library, "DisplayDraw");
        dc::dcData.logic.close = (void (*)(void))gptLibrary->load_function(dc::dcData.logic.library, "DisplayClose");

        // set variables
        for (auto const &[name, variable] : dc::dcData.variables)
        {
            dc::dcData.variables[name].externData = gptLibrary->load_function(dc::dcData.logic.library, name.c_str());
        }
    }

    // validate
    // root->validate();

    // set initial window params
    dc::DcNode *windowNode = dc::indexToDcNode(dc::dcData.window);
    plWindowDesc tWindowDesc = {
        .pcTitle = windowNode->window.title,
        .uWidth = (uint32_t)(dc::indexToDcValue(windowNode->window.dimensions.x)->valueInteger),
        .uHeight = (uint32_t)(dc::indexToDcValue(windowNode->window.dimensions.y)->valueInteger),
        .iXPos = dc::indexToDcValue(windowNode->window.position.x)->valueInteger,
        .iYPos = dc::indexToDcValue(windowNode->window.position.y)->valueInteger,
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
    dc::dcData.logic.draw();
    dc::refreshVariables();
    dc::drawNode(ptAppData, dc::dcData.window, nullptr);

    gptProfile->end_sample(0);

    // must be the last function called when using the starter extension
    gptStarter->end_frame();
}

namespace dc
{
    // update all variables
    void refreshVariables()
    {
        for (auto const &[name, variable] : dc::dcData.variables)
        {
            DcValue *value = indexToDcValue(variable.valueIndex);
            void *externData = variable.externData;

            switch (value->type)
            {
            case DC_VALUE_TYPE_FLOAT:
            {
                value->valueFloat = *((float *)(variable.externData));
                break;
            }
            case DC_VALUE_TYPE_INTEGER:
            {
                value->valueInteger = *((int *)(variable.externData));
                break;
            }
            case DC_VALUE_TYPE_STRING:
            {
                value->valueString = *((std::string *)(variable.externData));
                break;
            }
            case DC_VALUE_TYPE_BOOLEAN:
            {
                value->valueBoolean = *((bool *)(variable.externData));
                break;
            }
            default:
                throw std::runtime_error("invalid DcValue type for variable");
                break;
            }
            refreshValue(value);
        }
    }

    // returns the first child (if any)
    DcNodeIndex processNodeChildren(xmlNodePtr xmlNode, DcNodeIndex nodeIndex, const std::string &directory)
    {
        xmlNodePtr xmlChildNode = xmlNode->children;

        DcNodeIndex firstChildIndex = DC_NODE_INDEX_UNDEFINED;
        DcNodeIndex previousChildNodeIndex = DC_NODE_INDEX_UNDEFINED;
        while (xmlChildNode)
        {
            DcNodeIndex childNodeIndex = processNode(xmlChildNode, nodeIndex, directory);

            // get node addresses here since the address could change per node process
            DcNode *node = indexToDcNode(nodeIndex);
            DcNode *childNode = indexToDcNode(childNodeIndex);
            DcNode *previousChildNode = indexToDcNode(previousChildNodeIndex);

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

    DcNodeIndex processNode(xmlNodePtr xmlNode, DcNodeIndex parentNodeIndex, std::string directory)
    {
        // by default, the element is not a node
        DcNodeIndex nodeIndex = DC_NODE_INDEX_UNDEFINED;

        switch (xmlNodeToElementType(xmlNode))
        {

        // ignore non-element nodes
        case DC_ELEM_TYPE_NONELEM:
        {
            break;
        }

        case DC_ELEM_TYPE_CONSTANT:
        {
            // name
            char *cName = getAttributeString(xmlNode, "Name");
            if (!cName)
            {
                throw std::runtime_error("Non-existent node 'Name' in <Constant> definition");
            }
            std::string name = dereferenceConstants(cName);
            free(cName);

            // value
            char *cValue = getNodeContentString(xmlNode);
            if (!cValue)
            {
                throw std::runtime_error("Non-existent node content in <Constant> definition");
            }
            std::string value = dereferenceConstants(cValue);
            free(cValue);

            // set constant value
            setConstant(name, value);
            break;
        }

        case DC_ELEM_TYPE_CONTAINER:
        {
            DcNode dcNode = {
                .type = DC_NODE_TYPE_CONTAINER,
            };

            // xPosition
            char *cXPosition = getAttributeString(xmlNode, "X");
            if (cXPosition)
            {
                dcNode.container.position.x = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cXPosition));
                free(cXPosition);
            }
            else
            {
                dcNode.container.position.x = registerDcValue(createValueFloat(0.0f));
            }

            // y position
            char *cYPosition = getAttributeString(xmlNode, "Y");
            if (cYPosition)
            {
                dcNode.container.position.y = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cYPosition));
                free(cYPosition);
            }
            else
            {
                dcNode.container.position.y = registerDcValue(createValueFloat(0.0f));
            }

            // x origin
            char *cXOrigin = getAttributeString(xmlNode, "OriginX");
            if (cXOrigin)
            {
                dcNode.container.origin.x = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cXOrigin));
                free(cXOrigin);
            }
            else
            {
                dcNode.container.origin.x = registerDcValue(createValueFloat(0.0f));
            }

            // y origin
            char *cYOrigin = getAttributeString(xmlNode, "OriginY");
            if (cYOrigin)
            {
                dcNode.container.origin.y = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cYOrigin));
                free(cYOrigin);
            }
            else
            {
                dcNode.container.origin.y = registerDcValue(createValueFloat(0.0f));
            }

            // x dimension
            char *cXDimension = getAttributeString(xmlNode, "Width");
            if (cXDimension)
            {
                dcNode.container.dimensions.x = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cXDimension));
                free(cXDimension);
            }
            else
            {
                dcNode.container.dimensions.x = registerDcValue(createValueFloat(0.0f));
            }

            // y dimension
            char *cYDimension = getAttributeString(xmlNode, "Height");
            if (cYDimension)
            {
                dcNode.container.dimensions.y = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cYDimension));
                free(cYDimension);
            }
            else
            {
                dcNode.container.dimensions.y = registerDcValue(createValueFloat(0.0f));
            }

            // virtual x dimension
            char *cXVirtualDimension = getAttributeString(xmlNode, "VirtualWidth");
            if (cXVirtualDimension)
            {
                dcNode.container.virtualDimensions.x = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cXVirtualDimension));
                free(cXVirtualDimension);
            }
            else
            {
                dcNode.container.virtualDimensions.x = registerDcValue(createValueFloat(0.0f));
            }

            // virtual y dimension
            char *cYVirtualDimension = getAttributeString(xmlNode, "VirtualHeight");
            if (cYVirtualDimension)
            {
                dcNode.container.virtualDimensions.y = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cYVirtualDimension));
                free(cYVirtualDimension);
            }
            else
            {
                dcNode.container.virtualDimensions.y = registerDcValue(createValueFloat(0.0f));
            }

            // x align
            char *cXAlign = getAttributeString(xmlNode, "HorizontalAlign");
            if (cXAlign)
            {
                dcNode.container.alignment.x = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_INTEGER, dereferenceConstants(cXAlign));
                free(cXAlign);
            }
            else
            {
                dcNode.container.alignment.x = registerDcValue(createValueInteger(DC_ALIGN_TYPE_LEFT));
            }

            // y align
            char *cYAlign = getAttributeString(xmlNode, "VerticalAlign");
            if (cYAlign)
            {
                dcNode.container.alignment.y = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_INTEGER, dereferenceConstants(cYAlign));
                free(cYAlign);
            }
            else
            {
                dcNode.container.alignment.y = registerDcValue(createValueInteger(DC_ALIGN_TYPE_BOTTOM));
            }

            // rotation
            char *cRotation = getAttributeString(xmlNode, "Rotate");
            if (cRotation)
            {
                dcNode.container.rotation = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cRotation));
                free(cRotation);
            }
            else
            {
                dcNode.container.rotation = registerDcValue(createValueFloat(0.0f));
            }

            // register node
            nodeIndex = registerDcNode(dcNode);

            // process children
            DcNodeIndex firstChildIndex = processNodeChildren(xmlNode, nodeIndex, directory);

            // update child index
            DcNode *node = indexToDcNode(nodeIndex);
            node->container.child = firstChildIndex;
            break;
        }

        // really just the root element, left in for legacy reasons
        case DC_ELEM_TYPE_DCAPP:
        {
            processNodeChildren(xmlNode, nodeIndex, directory);
            break;
        }

            // case DC_ELEM_TYPE_DEM:
            // {
            //     DcNodeType parentType = indexToDcNode(parentNodeIndex)->type;
            //     switch (parentType)
            //     {
            //     case DC_ELEM_TYPE_MAP:
            //     {
            //         std::string filename = filepathToCanonical(getAttributeString(node, "File"), directory);
            //         ((DcMap *)parent)->addDem(filename);
            //         break;
            //     }

            //     default:
            //         throw std::runtime_error("Adding DEM to invalid parent of type " + std::to_string(parentType));
            //         break;
            //     }
            //     break;
            // }

        case DC_ELEM_TYPE_IF:
        {
            DcNode dcNode = (DcNode){
                .type = DC_NODE_TYPE_CONDITIONAL,
                .conditional = (DcNodeConditional){
                    .value1 = DC_VALUE_INDEX_UNDEFINED,
                    .value2 = DC_VALUE_INDEX_UNDEFINED,
                    .childTrue = DC_NODE_INDEX_UNDEFINED,
                    .childFalse = DC_NODE_INDEX_UNDEFINED,
                }};

            // conditional type
            char *cType = getAttributeString(xmlNode, "Operation");
            if (cType)
            {
                dcNode.conditional.type = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_INTEGER, dereferenceConstants(cType));
                free(cType);
            }
            else
            {
                dcNode.conditional.type = registerDcValue(createValueInteger(DC_CONDITIONAL_TYPE_TRUE));
            }

            // value1
            char *cValue = getAttributeString(xmlNode, "Value");
            if (cValue)
            {
                dcNode.conditional.value1 = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_STRING, dereferenceConstants(cValue));
                free(cValue);
            }
            else
            {
                cValue = getAttributeString(xmlNode, "Value1");
                if (cValue)
                {
                    dcNode.conditional.value1 = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_STRING, dereferenceConstants(cValue));
                    free(cValue);
                }
                else
                {
                    throw std::runtime_error("Invalid conditional; no value specified");
                }
            }

            // value2
            char *cValue2 = getAttributeString(xmlNode, "Value2");
            if (cValue2)
            {
                dcNode.conditional.value2 = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_STRING, dereferenceConstants(cValue2));
                free(cValue2);
            }

            // register node
            nodeIndex = registerDcNode(dcNode);

            // process children (assigning to true/false handled in separate cases, e.g. DC_ELEM_TYPE_TRUE)
            DcNodeIndex firstChildIndex = processNodeChildren(xmlNode, nodeIndex, directory);

            // handle implicit <True> elements
            if (firstChildIndex != DC_NODE_INDEX_UNDEFINED)
            {
                // ignore if True element already exists
                DcNode *node = indexToDcNode(nodeIndex);
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
        case DC_ELEM_TYPE_INCLUDE:
        {
            char *cDirectory = getAttributeString(xmlNode, "Directory");
            if (cDirectory)
            {
                nodeIndex = processNodeChildren(xmlNode, parentNodeIndex, dereferenceConstants(cDirectory));
                free(cDirectory);
            }
            else
            {
                // should never get here
                throw std::runtime_error("Invalid condition; <Include> node with no directory");
            }
            break;
        }

        case DC_ELEM_TYPE_LOGIC:
        {
            if (dcData.logic.library)
            {
                throw std::runtime_error("Duplicate <Logic> definitions");
            }
            char *cFilePath = getAttributeString(xmlNode, "File");
            if (cFilePath)
            {
                std::string filePath = filepathToCanonical(dereferenceConstants(cFilePath), directory);
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
                if (gptLibrary->load(logicSoDesc, &dcData.logic.library) != PL_LIBRARY_RESULT_SUCCESS)
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

            // case DC_ELEM_TYPE_MAP:
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

        case DC_ELEM_TYPE_PANEL:
        {
            DcNode dcNode = {
                .type = DC_NODE_TYPE_PANEL,
            };

            // parent dimensions
            // TODO probably don't need this.....must be a better way to architect
            dcNode.panel.parentDimensions = indexToDcNode(parentNodeIndex)->window.virtualDimensions;

            // virtual x dimension
            char *cXVirtualDimension = getAttributeString(xmlNode, "VirtualWidth");
            if (cXVirtualDimension)
            {
                dcNode.panel.virtualDimensions.x = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cXVirtualDimension));
                free(cXVirtualDimension);
            }
            else
            {
                dcNode.panel.virtualDimensions.x = dcNode.panel.parentDimensions.x;
            }

            // virtual y dimension
            char *cYVirtualDimension = getAttributeString(xmlNode, "VirtualHeight");
            if (cYVirtualDimension)
            {
                dcNode.panel.virtualDimensions.y = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cYVirtualDimension));
                free(cYVirtualDimension);
            }
            else
            {
                dcNode.panel.virtualDimensions.y = dcNode.panel.parentDimensions.y;
            }

            // register node
            nodeIndex = registerDcNode(dcNode);

            // process children
            DcNodeIndex firstChildIndex = processNodeChildren(xmlNode, nodeIndex, directory);

            // update child index
            DcNode *node = indexToDcNode(nodeIndex);
            node->panel.child = firstChildIndex;

            break;
        }

        case DC_ELEM_TYPE_POLYGON:
        {
            DcNode dcNode = {
                .type = DC_NODE_TYPE_POLYGON,
                .polygon = (DcNodePolygon){
                    .fillEnabled = false,
                    .lineEnabled = false,
                }};

            // fill color
            char *cFillColor = getAttributeString(xmlNode, "FillColor");
            if (cFillColor)
            {
                // split string by whitespace
                std::string sFillColor = dereferenceConstants(cFillColor);
                std::vector<std::string> substrings = splitStringByDelimiters(sFillColor, stringWhitespace);

                if (substrings.size() > 0)
                {
                    dcNode.polygon.fillColor.r = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, substrings[0]);
                    if (substrings.size() > 1)
                    {
                        dcNode.polygon.fillColor.g = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, substrings[1]);
                        if (substrings.size() > 2)
                        {
                            dcNode.polygon.fillColor.b = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, substrings[2]);
                            if (substrings.size() > 3)
                            {
                                dcNode.polygon.fillColor.a = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, substrings[3]);
                            }
                            else
                            {
                                dcNode.polygon.fillColor.a = registerDcValue(createValueFloat(1.0f));
                            }
                        }
                        else
                        {
                            dcNode.polygon.fillColor.b = registerDcValue(createValueFloat(0.0f));
                        }
                    }
                    else
                    {
                        dcNode.polygon.fillColor.g = registerDcValue(createValueFloat(0.0f));
                    }
                }
                else
                {
                    dcNode.polygon.fillColor.r = registerDcValue(createValueFloat(0.0f));
                }
                dcNode.polygon.fillEnabled = true;
                free(cFillColor);
            }

            // line color
            char *cLineColor = getAttributeString(xmlNode, "LineColor");
            if (cLineColor)
            {
                // split string by whitespace
                std::string sFillColor = dereferenceConstants(cLineColor);
                std::vector<std::string> substrings = splitStringByDelimiters(sFillColor, stringWhitespace);

                if (substrings.size() > 0)
                {
                    dcNode.polygon.lineColor.r = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, substrings[0]);
                    if (substrings.size() > 1)
                    {
                        dcNode.polygon.lineColor.g = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, substrings[1]);
                        if (substrings.size() > 2)
                        {
                            dcNode.polygon.lineColor.b = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, substrings[2]);
                            if (substrings.size() > 3)
                            {
                                dcNode.polygon.lineColor.a = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, substrings[3]);
                            }
                            else
                            {
                                dcNode.polygon.lineColor.a = registerDcValue(createValueFloat(1.0f));
                            }
                        }
                        else
                        {
                            dcNode.polygon.lineColor.b = registerDcValue(createValueFloat(0.0f));
                        }
                    }
                    else
                    {
                        dcNode.polygon.lineColor.g = registerDcValue(createValueFloat(0.0f));
                    }
                }
                else
                {
                    dcNode.polygon.lineColor.r = registerDcValue(createValueFloat(0.0f));
                }
                dcNode.polygon.lineEnabled = true;
                free(cLineColor);
            }

            // line width
            char *cLineWidth = getAttributeString(xmlNode, "LineWidth");
            if (cLineWidth)
            {
                dcNode.polygon.lineWidth = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cLineWidth));
                free(cLineWidth);
            }
            else
            {
                dcNode.polygon.lineWidth = registerDcValue(createValueFloat(0.0f));
            }

            // initialize points to 0
            dcNode.polygon.numPoints = 0;
            dcNode.polygon.points = nullptr;

            // register node
            nodeIndex = registerDcNode(dcNode);

            // process children
            processNodeChildren(xmlNode, nodeIndex, directory);
            break;
        }

        case DC_ELEM_TYPE_VARIABLE:
        {
            // name
            char *cName = getNodeContentString(xmlNode);
            if (!cName)
            {
                throw std::runtime_error("Non-existent node content in <Variable> definition");
            }
            std::string name = dereferenceConstants(cName);
            free(cName);

            // type
            char *cType = getAttributeString(xmlNode, "Type");
            DcValueType type = DC_VALUE_TYPE_STRING;
            if (cType)
            {
                type = valueTypeFromString(dereferenceConstants(cType));
            }

            // value
            char *cInitialValue = getAttributeString(xmlNode, "InitialValue");
            DcValue initialValue;
            if (cInitialValue)
            {
                initialValue = createValueString(dereferenceConstants(cInitialValue));
            }
            else
            {
                initialValue = createValueString("");
            }
            initialValue.type = type;
            initialValue.isDynamic = true;

            // register variable
            DcValueIndex index = registerDcValue(initialValue);
            setVariable(name, index);

            break;
        }

        case DC_ELEM_TYPE_VERTEX:
        {
            DcNode *parentNode = indexToDcNode(parentNodeIndex);
            switch (parentNode->type)
            {
            case DC_NODE_TYPE_POLYGON:
            {
                // xPosition
                char *cXPosition = getAttributeString(xmlNode, "X");
                if (!cXPosition)
                {
                    throw std::runtime_error("Invalid Vertex: No X attribute");
                }

                // yPosition
                char *cYPosition = getAttributeString(xmlNode, "Y");
                if (!cYPosition)
                {
                    throw std::runtime_error("Invalid Vertex: No Y attribute");
                }

                // reallocate and add vertices
                parentNode->polygon.numPoints++;
                parentNode->polygon.points = (DcValueIndex2 *)realloc(parentNode->polygon.points, parentNode->polygon.numPoints * sizeof(DcValueIndex2));
                parentNode->polygon.points[parentNode->polygon.numPoints - 1] = (DcValueIndex2){
                    createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cXPosition)),
                    createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cYPosition))};
                break;
            }
            default:
                // TODO add a nodeTypeToString() function
                throw std::runtime_error("Invalid parent of type " + std::string("<Unknown>") + "for vertex.");
            }
            break;
        }

        case DC_ELEM_TYPE_WINDOW:
        {
            DcNode dcNode = {
                .type = DC_NODE_TYPE_WINDOW,
            };

            // title
            std::string title = "dcapp";
            char *cTitle = getAttributeString(xmlNode, "Title");
            if (cTitle)
            {
                title = dereferenceConstants(cTitle);
                free(cTitle);
            }
            dcNode.window.title = (char *)malloc(title.length() + 1);
            memcpy(dcNode.window.title, title.c_str(), title.length() + 1);

            // xPosition
            char *cXPosition = getAttributeString(xmlNode, "X");
            if (cXPosition)
            {
                dcNode.window.position.x = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_INTEGER, dereferenceConstants(cXPosition));
                free(cXPosition);
            }
            else
            {
                dcNode.window.position.x = registerDcValue(createValueInteger(0.0f));
            }

            // y position
            char *cYPosition = getAttributeString(xmlNode, "Y");
            if (cYPosition)
            {
                dcNode.window.position.y = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_INTEGER, dereferenceConstants(cYPosition));
                free(cYPosition);
            }
            else
            {
                dcNode.window.position.y = registerDcValue(createValueInteger(0.0f));
            }

            // x dimension
            char *cXDimension = getAttributeString(xmlNode, "Width");
            if (cXDimension)
            {
                dcNode.window.dimensions.x = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_INTEGER, dereferenceConstants(cXDimension));
                free(cXDimension);
            }
            else
            {
                dcNode.window.dimensions.x = registerDcValue(createValueInteger(0.0f));
            }

            // y dimension
            char *cYDimension = getAttributeString(xmlNode, "Height");
            if (cYDimension)
            {
                dcNode.window.dimensions.y = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_INTEGER, dereferenceConstants(cYDimension));
                free(cYDimension);
            }
            else
            {
                dcNode.window.dimensions.y = registerDcValue(createValueInteger(0.0f));
            }

            // virtual x dimension
            char *cXVirtualDimension = getAttributeString(xmlNode, "VirtualWidth");
            if (cXVirtualDimension)
            {
                dcNode.window.virtualDimensions.x = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cXVirtualDimension));
                free(cXVirtualDimension);
            }
            else
            {
                dcNode.window.virtualDimensions.x = registerDcValue(createValueFloat(0.0f));
            }

            // virtual y dimension
            char *cYVirtualDimension = getAttributeString(xmlNode, "VirtualHeight");
            if (cYVirtualDimension)
            {
                dcNode.window.virtualDimensions.y = createAndRegisterTypedDcValueFromString(DC_VALUE_TYPE_FLOAT, dereferenceConstants(cYVirtualDimension));
                free(cYVirtualDimension);
            }
            else
            {
                dcNode.window.virtualDimensions.y = registerDcValue(createValueFloat(0.0f));
            }

            // register node
            nodeIndex = registerDcNode(dcNode);

            // process children
            DcNodeIndex firstChildIndex = processNodeChildren(xmlNode, nodeIndex, directory);

            // update child index
            DcNode *node = indexToDcNode(nodeIndex);
            node->window.child = firstChildIndex;

            // set global window
            dcData.window = nodeIndex;
            break;
        }
        default:
            throw std::runtime_error("Invalid node in processNode()");
        }

        return nodeIndex;
    }

    void drawNodeList(plAppData *ptAppData, DcNodeIndex nodeIndex, plMat4 *nodeTransform)
    {
        DcNodeIndex currentNodeIndex = nodeIndex;
        while (currentNodeIndex != DC_NODE_INDEX_UNDEFINED)
        {
            drawNode(ptAppData, currentNodeIndex, nodeTransform);
            currentNodeIndex = indexToDcNode(currentNodeIndex)->next;
        }
    }

    void drawNode(plAppData *ptAppData, DcNodeIndex nodeIndex, plMat4 *parentTransform)
    {
        if (nodeIndex == DC_NODE_INDEX_UNDEFINED)
        {
            throw std::runtime_error("Attempting to draw undefined node index");
        }

        DcNode *node = indexToDcNode(nodeIndex);
        switch (node->type)
        {
        case DC_NODE_TYPE_CONTAINER:
        {
            float xPosition = 0;
            switch (indexToDcValue(node->container.alignment.x)->valueInteger)
            {
            case DC_ALIGN_TYPE_LEFT:
                xPosition = indexToDcValue(node->container.position.x)->valueFloat;
                break;
            case DC_ALIGN_TYPE_CENTER:
                xPosition = indexToDcValue(node->container.position.x)->valueFloat - indexToDcValue(node->container.dimensions.x)->valueFloat / 2;
                break;
            case DC_ALIGN_TYPE_RIGHT:
                xPosition = indexToDcValue(node->container.position.x)->valueFloat - indexToDcValue(node->container.dimensions.x)->valueFloat;
                break;
            }

            float yPosition = 0;
            switch (indexToDcValue(node->container.alignment.y)->valueInteger)
            {
            case DC_ALIGN_TYPE_LEFT:
                yPosition = indexToDcValue(node->container.position.y)->valueFloat;
                break;
            case DC_ALIGN_TYPE_CENTER:
                yPosition = indexToDcValue(node->container.position.y)->valueFloat - indexToDcValue(node->container.dimensions.y)->valueFloat / 2;
                break;
            case DC_ALIGN_TYPE_RIGHT:
                yPosition = indexToDcValue(node->container.position.y)->valueFloat - indexToDcValue(node->container.dimensions.y)->valueFloat;
                break;
            }

            plMat4 transOriginMatrix = pl_mat4_translate_xyz(
                indexToDcValue(node->container.origin.x)->valueFloat,
                indexToDcValue(node->container.origin.y)->valueFloat,
                0.0f);
            plMat4 rotateMatrix = pl_mat4_rotate_vec3(
                pl_radiansf(indexToDcValue(node->container.rotation)->valueFloat),
                (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 transPositionMatrix = pl_mat4_translate_xyz(
                xPosition,
                yPosition,
                0.0f);
            plMat4 scaleMatrix = pl_mat4_scale_xyz(
                indexToDcValue(node->container.dimensions.x)->valueFloat / indexToDcValue(node->container.virtualDimensions.x)->valueFloat,
                indexToDcValue(node->container.dimensions.y)->valueFloat / indexToDcValue(node->container.virtualDimensions.y)->valueFloat,
                1.0f);

            plMat4 transform = (plMat4){0};
            transform = pl_mul_mat4t(parentTransform, &transOriginMatrix);
            transform = pl_mul_mat4t(&transform, &rotateMatrix);
            transform = pl_mul_mat4t(&transform, &transPositionMatrix);
            transform = pl_mul_mat4t(&transform, &scaleMatrix);
            drawNodeList(ptAppData, node->container.child, &transform);
            break;
        }

        case DC_NODE_TYPE_CONDITIONAL:
        {
            // TODO implement
            break;
        }

        case DC_NODE_TYPE_MAP:
        {
            // TODO implement
            break;
        }

        case DC_NODE_TYPE_PANEL:
        {
            plMat4 scaleMatrix = pl_mat4_scale_xyz(
                indexToDcValue(node->panel.parentDimensions.x)->valueFloat / indexToDcValue(node->panel.virtualDimensions.x)->valueFloat,
                indexToDcValue(node->panel.parentDimensions.y)->valueFloat / indexToDcValue(node->panel.virtualDimensions.y)->valueFloat,
                1.0f);
            plMat4 transform = (plMat4){0};
            transform = pl_mul_mat4t(parentTransform, &scaleMatrix);
            drawNodeList(ptAppData, node->panel.child, &transform);
            break;
        }

        case DC_NODE_TYPE_POLYGON:
        {
            // get points
            std::vector<plVec2> points;
            points.resize(node->polygon.numPoints);
            for (int ii = 0; ii < node->polygon.numPoints; ii++)
            {
                plVec4 point4 = (plVec4){
                    indexToDcValue(node->polygon.points[ii].x)->valueFloat,
                    indexToDcValue(node->polygon.points[ii].y)->valueFloat,
                    0, 1};
                point4 = pl_mul_mat4_vec4(parentTransform, point4);
                points[ii] = (plVec2){point4.x, point4.y};
            }

            // draw fill
            if (node->polygon.fillEnabled)
            {
                uint32_t fillColor = PL_COLOR_32_RGBA(
                    indexToDcValue(node->polygon.fillColor.r)->valueFloat,
                    indexToDcValue(node->polygon.fillColor.g)->valueFloat,
                    indexToDcValue(node->polygon.fillColor.b)->valueFloat,
                    indexToDcValue(node->polygon.fillColor.a)->valueFloat);
                gptDraw->add_convex_polygon_filled(ptAppData->ptFGLayer, points.data(), points.size(), (plDrawSolidOptions){.uColor = fillColor});
            }

            // draw outline
            if (node->polygon.lineEnabled)
            {
                float lineThickness = indexToDcValue(node->polygon.lineWidth)->valueFloat;
                uint32_t lineColor = PL_COLOR_32_RGBA(
                    indexToDcValue(node->polygon.lineColor.r)->valueFloat,
                    indexToDcValue(node->polygon.lineColor.g)->valueFloat,
                    indexToDcValue(node->polygon.lineColor.b)->valueFloat,
                    indexToDcValue(node->polygon.lineColor.a)->valueFloat);
                gptDraw->add_polygon(ptAppData->ptFGLayer, points.data(), points.size(), (plDrawLineOptions){.uColor = lineColor, .fThickness = lineThickness});
            }

            break;
        }

        case DC_NODE_TYPE_SET:
        {
            // todo implement this
            break;
        }

        case DC_NODE_TYPE_WINDOW:
        {
            // TODO move this code to only the resize() function
            // update dimensions
            uint32_t dimensionX, dimensionY;

            // TODO fix this in pilotlight for macos
            gptWindows->get_size(ptAppData->ptWindow, &dimensionX, &dimensionY);
            DcValue *dimensionValueX = indexToDcValue(node->window.dimensions.x);
            DcValue *dimensionValueY = indexToDcValue(node->window.dimensions.y);
            dimensionValueX->valueInteger = (int)dimensionX;
            dimensionValueY->valueInteger = (int)dimensionY;
            refreshValue(dimensionValueX);
            refreshValue(dimensionValueY);

            // compute transforms
            // translate from negative to positive range
            plMat4 transMatrix = pl_mat4_translate_xyz(
                0.0f,
                indexToDcValue(node->window.dimensions.y)->valueFloat,
                0.0f);

            // scale from virtual to real dimensions, flip y axis
            plMat4 scaleMatrix = pl_mat4_scale_xyz(
                indexToDcValue(node->window.dimensions.x)->valueFloat / indexToDcValue(node->window.virtualDimensions.x)->valueFloat,
                indexToDcValue(node->window.dimensions.y)->valueFloat / indexToDcValue(node->window.virtualDimensions.y)->valueFloat * -1.0f,
                1.0f);

            plMat4 transform;
            transform = pl_mul_mat4t(&transMatrix, &scaleMatrix);
            drawNodeList(ptAppData, node->window.child, &transform);
            break;
        }

        default:
            break;
        }
    }

} // namespace dc
