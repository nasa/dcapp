
//-----------------------------------------------------------------------------
// [SECTION] dcapp includes
//-----------------------------------------------------------------------------

#include <dcapp-data.hpp>
#include <utils/xml-utils.hpp>
#include <utils/string-utils.hpp>

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

#include <stdlib.h> // malloc, free
#include <string.h> // memset
#include "pl.h"

#define PL_MATH_INCLUDE_FUNCTIONS // required to expose some of the color helpers
#include "pl_math.h"

// extensions
#include "pl_draw_ext.h"
#include "pl_starter_ext.h"
#include "pl_profile_ext.h"

//-----------------------------------------------------------------------------
// [SECTION] dcapp state
//-----------------------------------------------------------------------------

namespace dc
{
    static void processNodeChildren(xmlNodePtr xmlNode, DcNodeIndex nodeIndex, const std::string &directory);
    static DcNodeIndex processNode(xmlNodePtr xmlNode, DcNodeIndex parentNodeIndex, DcNodeIndex previousNodeIndex, std::string directory);
}

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plAppData
{
    // window
    plWindow *ptWindow;

    // console variable
    bool bShowHelpWindow;
} plAppData;

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

const plWindowI *gptWindows = NULL;
const plDrawI *gptDraw = NULL;
const plStarterI *gptStarter = NULL;
const plProfileI *gptProfile = NULL;
const plMemoryI *gptMemory = NULL;

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
    std::string configRelativePath = "/home/nathan/dcapp-vk/samples/test/test.xml"; //"/data/nreagan/dcapp-vk/samples/test/test.xml";

    // set paths
    std::filesystem::path fsFilePath = std::filesystem::canonical(configRelativePath);
    std::filesystem::path fsDirPath = fsFilePath.parent_path();
    std::string configFilePath = fsFilePath.string();
    std::string configDirPath = fsDirPath.string();

    // create cache and log dirs
    std::filesystem::path fsExePath = std::filesystem::canonical("/proc/self/exe");
    std::filesystem::path fsLogPath = fsExePath.parent_path() / ".." / "logs";
    std::filesystem::create_directory(fsLogPath);
    std::string logDirPath = std::filesystem::canonical(fsLogPath).string();
    std::filesystem::path fsCachePath = fsExePath.parent_path() / ".." / "cache";
    std::filesystem::create_directory(fsCachePath);
    std::string cacheDirPath = std::filesystem::canonical(fsCachePath).string();

    // begin setting up dcappData object
    dc::dcData.configFilePath = configFilePath;
    dc::dcData.configDirPath = configDirPath;
    dc::dcData.logDirPath = logDirPath;
    dc::dcData.cacheDirPath = cacheDirPath;

    // set environment (used for dcapp XMLs)
    setenv("dcappDisplayHome", dc::dcData.configDirPath.c_str(), 1);

    // load XML file
    int xmlResult = xmlCheckFilename(configFilePath.c_str());
    if (xmlResult == 0)
    {
        throw std::runtime_error("Failed to stat configuration file: " + configFilePath);
    }
    else if (xmlResult == 2)
    {
        throw std::runtime_error("Configuration file is a directory: " + configFilePath);
    }
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
    dc::processNode(rootNode, dc::DC_NODE_INDEX_UNDEFINED, dc::DC_NODE_INDEX_UNDEFINED, configDirPath);

    // validate
    // root->validate();

    // set initial window params
    dc::DcNode* windowNode = dc::indexToDcNode(dc::dcData.window);
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

    // resize
    uint32_t xSize, ySize;
    gptWindows->get_size(ptAppData->ptWindow, &xSize, &ySize);
    // root->getWindows().front()->resize(xSize, ySize);

    // this needs to be the first call when using the starter
    // extension. You must return if it returns false (usually a swapchain recreation).
    if (!gptStarter->begin_frame())
        return;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~drawing & profile API~~~~~~~~~~~~~~~~~~~~~~~~~~~

    gptProfile->begin_sample(0, "example drawing");

    plDrawLayer2D *ptFGLayer = gptStarter->get_foreground_layer();
    // DcPrimitive::setPtLayer(ptFGLayer);

    // draw dcapp
    // root->draw(true);

    gptProfile->end_sample(0);

    // must be the last function called when using the starter extension
    gptStarter->end_frame();
}

namespace dc
{
    void processNodeChildren(xmlNodePtr xmlNode, DcNodeIndex nodeIndex, const std::string &directory)
    {
        xmlNodePtr xmlChildNode = xmlNode->children;
        DcNodeIndex childNodePreviousIndex = DC_NODE_INDEX_UNDEFINED;
        while (xmlChildNode)
        {
            DcNodeIndex childNodeIndex = processNode(xmlChildNode, nodeIndex, childNodePreviousIndex, directory);
            if (childNodeIndex != DC_NODE_INDEX_UNDEFINED)
            {
                childNodePreviousIndex = childNodeIndex;
            }
            xmlChildNode = xmlChildNode->next;
        }
    }

    DcNodeIndex processNode(xmlNodePtr xmlNode, DcNodeIndex parentNodeIndex, DcNodeIndex previousNodeIndex, std::string directory)
    {
        bool isDcNode = false;
        DcNodeIndex nodeIndex = DC_NODE_INDEX_UNDEFINED;

        switch (xmlNodeToElementType(xmlNode))
        {

        // ignore non-element nodes
        case DC_ELEM_TYPE_NONELEM:
        {
            isDcNode = false;
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
                dcNode.container.position.x = createAndRegisterDcValueFromString(dereferenceConstants(cXPosition));
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
                dcNode.container.position.y = createAndRegisterDcValueFromString(dereferenceConstants(cYPosition));
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
                dcNode.container.origin.x = createAndRegisterDcValueFromString(dereferenceConstants(cXOrigin));
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
                dcNode.container.origin.y = createAndRegisterDcValueFromString(dereferenceConstants(cYOrigin));
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
                dcNode.container.dimensions.x = createAndRegisterDcValueFromString(dereferenceConstants(cXDimension));
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
                dcNode.container.dimensions.y = createAndRegisterDcValueFromString(dereferenceConstants(cYDimension));
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
                dcNode.container.virtualDimensions.x = createAndRegisterDcValueFromString(dereferenceConstants(cXVirtualDimension));
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
                dcNode.container.virtualDimensions.y = createAndRegisterDcValueFromString(dereferenceConstants(cYVirtualDimension));
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
                dcNode.container.alignment.x = createAndRegisterDcValueFromString(dereferenceConstants(cXAlign));
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
                dcNode.container.alignment.y = createAndRegisterDcValueFromString(dereferenceConstants(cYAlign));
                free(cYAlign);
            }
            else
            {
                dcNode.container.alignment.y = registerDcValue(createValueInteger(DC_ALIGN_TYPE_BOTTOM));
            }

            // rotation
            char *cRotation = getAttributeString(xmlNode, "Rotation");
            if (cRotation)
            {
                dcNode.container.rotation = createAndRegisterDcValueFromString(dereferenceConstants(cRotation));
                free(cRotation);
            }
            else
            {
                dcNode.container.rotation = registerDcValue(createValueFloat(0.0f));
            }

            // register node
            nodeIndex = registerDcNode(dcNode);

            // process children
            processNodeChildren(xmlNode, nodeIndex, directory);

            isDcNode = true;
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

        // at this point, just used to set the directory path
        case DC_ELEM_TYPE_INCLUDE:
        {
            char *cDirectory = getAttributeString(xmlNode, "Directory");
            if (cDirectory)
            {
                processNodeChildren(xmlNode, nodeIndex, dereferenceConstants(cDirectory));
                free(cDirectory);
            }
            else
            {
                // should never get here
                throw std::runtime_error("Invalid condition; <Include> node with no directory");
            }
            break;
        }

            // case DC_ELEM_TYPE_LOGIC:
            // {
            //     std::string filePath = filepathToCanonical(getAttributeString(node, "File"), directory);
            //     DcValue *file = new DcValue(filePath);
            //     DcLogic *logic = new DcLogic((DcParent *)parent, file);
            //     DcVariable::setLogic(logic);
            //     break;
            // }

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

            // virtual x dimension
            char *cXVirtualDimension = getAttributeString(xmlNode, "VirtualWidth");
            if (cXVirtualDimension)
            {
                dcNode.panel.virtualDimensions.x = createAndRegisterDcValueFromString(dereferenceConstants(cXVirtualDimension));
                free(cXVirtualDimension);
            }
            else
            {
                dcNode.panel.virtualDimensions.x = registerDcValue(createValueFloat(0.0f));
            }

            // virtual y dimension
            char *cYVirtualDimension = getAttributeString(xmlNode, "VirtualHeight");
            if (cYVirtualDimension)
            {
                dcNode.panel.virtualDimensions.y = createAndRegisterDcValueFromString(dereferenceConstants(cYVirtualDimension));
                free(cYVirtualDimension);
            }
            else
            {
                dcNode.panel.virtualDimensions.y = registerDcValue(createValueFloat(0.0f));
            }

            // register node
            nodeIndex = registerDcNode(dcNode);

            // process children
            processNodeChildren(xmlNode, nodeIndex, directory);

            isDcNode = true;
            break;
        }

        case DC_ELEM_TYPE_POLYGON:
        {
            DcNode dcNode = {
                .type = DC_NODE_TYPE_POLYGON,
            };

            // fill color
            char *cFillColor = getAttributeString(xmlNode, "FillColor");
            if (cFillColor)
            {
                // split string by whitespace
                std::string sFillColor = dereferenceConstants(cFillColor);
                std::vector<std::string> substrings = splitStringByDelimiters(sFillColor, stringWhitespace);

                if (substrings.size() > 0)
                {
                    dcNode.polygon.fillColor.r = createAndRegisterDcValueFromString(substrings[0]);
                    if (substrings.size() > 1)
                    {
                        dcNode.polygon.fillColor.g = createAndRegisterDcValueFromString(substrings[1]);
                        if (substrings.size() > 2)
                        {
                            dcNode.polygon.fillColor.b = createAndRegisterDcValueFromString(substrings[2]);
                            if (substrings.size() > 3)
                            {
                                dcNode.polygon.fillColor.a = createAndRegisterDcValueFromString(substrings[3]);
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
                free(cFillColor);
            }
            else
            {
                dcNode.polygon.fillEnabled = false;
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
                    dcNode.polygon.lineColor.r = createAndRegisterDcValueFromString(substrings[0]);
                    if (substrings.size() > 1)
                    {
                        dcNode.polygon.lineColor.g = createAndRegisterDcValueFromString(substrings[1]);
                        if (substrings.size() > 2)
                        {
                            dcNode.polygon.lineColor.b = createAndRegisterDcValueFromString(substrings[2]);
                            if (substrings.size() > 3)
                            {
                                dcNode.polygon.lineColor.a = createAndRegisterDcValueFromString(substrings[3]);
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
                free(cLineColor);
            }
            else
            {
                dcNode.polygon.fillEnabled = false;
            }

            // line width
            char *cLineWidth = getAttributeString(xmlNode, "LineWidth");
            if (cLineWidth)
            {
                dcNode.polygon.lineWidth = createAndRegisterDcValueFromString(dereferenceConstants(cLineWidth));
                free(cLineWidth);
            }
            else
            {
                dcNode.polygon.lineWidth = registerDcValue(createValueFloat(0.0f));
            }

            // register node
            nodeIndex = registerDcNode(dcNode);
            isDcNode = true;
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

            // case DC_ELEM_TYPE_VERTEX:
            // {
            //     elem_t parentType = parent->getType();
            //     switch (parentType)
            //     {
            //     case DC_ELEM_TYPE_POLYGON:
            //     {
            //         ((DcPolygon *)parent)->addVertex(attributeToDcValue(node, "X"), attributeToDcValue(node, "Y"));
            //         break;
            //     }

            //     default:
            //         throw std::runtime_error("Adding vertex to invalid parent of type " + std::to_string(parentType));
            //         break;
            //     }
            //     break;
            // }

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
                dcNode.window.position.x = createAndRegisterDcValueFromString(dereferenceConstants(cXPosition));
                free(cXPosition);
            }
            else
            {
                dcNode.window.position.x = registerDcValue(createValueFloat(0.0f));
            }

            // y position
            char *cYPosition = getAttributeString(xmlNode, "Y");
            if (cYPosition)
            {
                dcNode.window.position.y = createAndRegisterDcValueFromString(dereferenceConstants(cYPosition));
                free(cYPosition);
            }
            else
            {
                dcNode.window.position.y = registerDcValue(createValueFloat(0.0f));
            }

            // x dimension
            char *cXDimension = getAttributeString(xmlNode, "Width");
            if (cXDimension)
            {
                dcNode.window.dimensions.x = createAndRegisterDcValueFromString(dereferenceConstants(cXDimension));
                free(cXDimension);
            }
            else
            {
                dcNode.window.dimensions.x = registerDcValue(createValueFloat(0.0f));
            }

            // y dimension
            char *cYDimension = getAttributeString(xmlNode, "Height");
            if (cYDimension)
            {
                dcNode.window.dimensions.y = createAndRegisterDcValueFromString(dereferenceConstants(cYDimension));
                free(cYDimension);
            }
            else
            {
                dcNode.window.dimensions.y = registerDcValue(createValueFloat(0.0f));
            }

            // virtual x dimension
            char *cXVirtualDimension = getAttributeString(xmlNode, "VirtualWidth");
            if (cXVirtualDimension)
            {
                dcNode.window.virtualDimensions.x = createAndRegisterDcValueFromString(dereferenceConstants(cXVirtualDimension));
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
                dcNode.window.virtualDimensions.y = createAndRegisterDcValueFromString(dereferenceConstants(cYVirtualDimension));
                free(cYVirtualDimension);
            }
            else
            {
                dcNode.window.virtualDimensions.y = registerDcValue(createValueFloat(0.0f));
            }

            // register node
            dcData.window = registerDcNode(dcNode);
            isDcNode = true;
            break;
        }
        }

        if (isDcNode)
        {
            DcNode *node = indexToDcNode(nodeIndex);
            DcNode *parentNode = indexToDcNode(parentNodeIndex);
            DcNode *previousNode = indexToDcNode(previousNodeIndex);

            // if there is a parent
            if (parentNodeIndex != DC_NODE_INDEX_UNDEFINED)
            {
                // set parent
                node->parent = parentNodeIndex;

                // set parent's first child if this is the first child
                if (previousNodeIndex == DC_NODE_INDEX_UNDEFINED)
                {
                    parentNode->child = nodeIndex;
                }
            }

            // if there is a previous node
            if (previousNodeIndex != DC_NODE_INDEX_UNDEFINED)
            {
                // set the next node of the previous node
                previousNode->next = nodeIndex;
            }

            // return node index
            return nodeIndex;
        }
        else
        {
            return DC_NODE_INDEX_UNDEFINED;
        }
    }

}
