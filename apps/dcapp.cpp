
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
    plDrawLayer2D *pt_fg_layer;

    // console variable
    bool b_show_help_window;
} pl_app_data;

//-----------------------------------------------------------------------------
// [SECTION] dcapp state
//-----------------------------------------------------------------------------

static void _refresh_variables();
static DcAppNodeIndex _process_node_children(xmlNodePtr xml_node, DcAppNodeIndex node_index, const std::string &directory);
static DcAppNodeIndex _process_node(xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, std::string directory);
static void _draw_node_list(pl_app_data *pt_app_data, DcAppNodeIndex node_index, plMat4 *node_transform);
static void _draw_node(pl_app_data *pt_app_data, DcAppNodeIndex node_index, plMat4 *parent_transform);

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

const plWindowI *gpt_windows = NULL;
const plDrawI *gpt_draw = NULL;
const plStarterI *gpt_starter = NULL;
const plProfileI *gpt_profile = NULL;
const plMemoryI *gpt_memory = NULL;
const plLibraryI *gpt_library = NULL;
const plIOI *gpt_ioi = NULL;

#define PL_ALLOC(x) gpt_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_REALLOC(x, y) gpt_memory->tracked_realloc((x), (y), __FILE__, __LINE__)
#define PL_FREE(x) gpt_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

//-----------------------------------------------------------------------------
// [SECTION] pl_app_load
//-----------------------------------------------------------------------------

PL_EXPORT void *
pl_app_load(plApiRegistryI *pt_api_registry, pl_app_data *pt_app_data)
{
    // NOTE: on first load, "pAppData" will be NULL but on reloads
    //       it will be the value returned from this function

    // if "ptAppData" is a valid pointer, then this function is being called
    // during a hot reload.
    if (pt_app_data)
    {
        // re-retrieve the apis since we are now in
        // a different dll/so
        gpt_windows = pl_get_api_latest(pt_api_registry, plWindowI);
        gpt_draw = pl_get_api_latest(pt_api_registry, plDrawI);
        gpt_starter = pl_get_api_latest(pt_api_registry, plStarterI);
        gpt_profile = pl_get_api_latest(pt_api_registry, plProfileI);
        gpt_memory = pl_get_api_latest(pt_api_registry, plMemoryI);
        gpt_library = pl_get_api_latest(pt_api_registry, plLibraryI);
        gpt_ioi = pl_get_api_latest(pt_api_registry, plIOI);

        return pt_app_data;
    }

    // retrieve extension registry
    const plExtensionRegistryI *ptExtensionRegistry = pl_get_api_latest(pt_api_registry, plExtensionRegistryI);

    // load extensions
    //   * first argument is the shared library name WITHOUT the extension
    //   * second & third argument is the load/unload functions names (use NULL for the default of "pl_load_ext" &
    //     "pl_unload_ext")
    //   * fourth argument indicates if the extension is reloadable (should we check for changes and reload if changed)
    ptExtensionRegistry->load("pl_unity_ext", NULL, NULL, true);
    ptExtensionRegistry->load("pl_platform_ext", NULL, NULL, false); // provides the file API used by the drawing ext

    // load required apis
    gpt_windows = pl_get_api_latest(pt_api_registry, plWindowI);
    gpt_draw = pl_get_api_latest(pt_api_registry, plDrawI);
    gpt_starter = pl_get_api_latest(pt_api_registry, plStarterI);
    gpt_profile = pl_get_api_latest(pt_api_registry, plProfileI);
    gpt_memory = pl_get_api_latest(pt_api_registry, plMemoryI);
    gpt_library = pl_get_api_latest(pt_api_registry, plLibraryI);
    gpt_ioi = pl_get_api_latest(pt_api_registry, plIOI);

    // allocate app memory
    pt_app_data = (pl_app_data *)PL_ALLOC(sizeof(pl_app_data));
    memset(pt_app_data, 0, sizeof(pl_app_data));

    // default values
    pt_app_data->b_show_help_window = true;

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
    plIO *gpt_io = gpt_ioi->get_io();
    if (gpt_io->iArgc < 4)
    {
        throw std::runtime_error("Missing dcapp configuration file");
    }
    std::vector<std::string> args(gpt_io->apArgv + 3, gpt_io->apArgv + gpt_io->iArgc);

    // TODO process input arguments (constant setting)
    std::string configRelativePath = args[0];

    // set paths
    std::filesystem::path fsFilePath = std::filesystem::canonical(configRelativePath);
    std::filesystem::path fs_dir_path = fsFilePath.parent_path();
    std::string configFilePath = fsFilePath.string();
    std::string config_dir_path = fs_dir_path.string();

    // create cache and log dirs
    std::filesystem::path fsExePath = dc_utils_get_exe_filepath();
    std::filesystem::path fs_log_path = fsExePath.parent_path() / ".." / "logs";
    std::filesystem::create_directory(fs_log_path);
    std::string log_dir_path = std::filesystem::canonical(fs_log_path).string();
    std::filesystem::path fs_cache_path = fsExePath.parent_path() / ".." / "cache";
    std::filesystem::create_directory(fs_cache_path);
    std::string cache_dir_path = std::filesystem::canonical(fs_cache_path).string();

    // init dc_app_data object
    dc_app_init_data();

    // begin setting up dcappData object
    dc_app_data.configFilePath = configFilePath;
    dc_app_data.config_dir_path = config_dir_path;
    dc_app_data.log_dir_path = log_dir_path;
    dc_app_data.cache_dir_path = cache_dir_path;

    // set environment (used for dcapp XMLs)
    setenv("dcappDisplayHome", dc_app_data.config_dir_path.c_str(), 1);

    // load XML file
    dc_app_data.doc = xmlReadFile(configFilePath.c_str(), "UTF-8", XML_PARSE_NOBLANKS);
    if (!dc_app_data.doc)
    {
        throw std::runtime_error("Unable to read configuration file: " + configFilePath);
    }

    // clean XML file
    dc_app_clean_xml_data();

    // save cleaned xml to file
    std::filesystem::path fsOutFile = std::filesystem::path(log_dir_path) / "config.xml";
    xmlSaveFormatFile(fsOutFile.string().c_str(), dc_app_data.doc, 1);

    // process XML
    xmlNodePtr root_node = xmlDocGetRootElement(dc_app_data.doc);
    _process_node(root_node, DC_APP_NODE_INDEX_UNDEFINED, config_dir_path);

    // configure logic file
    if (dc_app_data.logic.library)
    {
        // set logic functions
        dc_app_data.logic.pre_init = (void (*)(void))gpt_library->load_function(dc_app_data.logic.library, "DisplayPreInit");
        dc_app_data.logic.init = (void (*)(void))gpt_library->load_function(dc_app_data.logic.library, "DisplayInit");
        dc_app_data.logic.draw = (void (*)(void))gpt_library->load_function(dc_app_data.logic.library, "DisplayDraw");
        dc_app_data.logic.close = (void (*)(void))gpt_library->load_function(dc_app_data.logic.library, "DisplayClose");

        // set variables
        for (auto const &[name, variable] : dc_app_data.variables)
        {
            dc_app_data.variables[name].externData = gpt_library->load_function(dc_app_data.logic.library, name.c_str());
        }
    }

    // validate
    // root->validate();

    // set initial window params
    DcAppNode *windowNode = dc_app_index_to_node(dc_app_data.window);
    plWindowDesc t_window_desc = {
        .pcTitle = windowNode->window.title,
        .uWidth = (uint32_t)(dc_app_index_to_dc_value(windowNode->window.dimensions.x)->value_integer),
        .uHeight = (uint32_t)(dc_app_index_to_dc_value(windowNode->window.dimensions.y)->value_integer),
        .iXPos = dc_app_index_to_dc_value(windowNode->window.position.x)->value_integer,
        .iYPos = dc_app_index_to_dc_value(windowNode->window.position.y)->value_integer,
    };

    gpt_windows->create(t_window_desc, &pt_app_data->ptWindow);
    gpt_windows->show(pt_app_data->ptWindow);

    // initialize the starter API (handles alot of boilerplate)
    plStarterInit tStarterInit = {
        .tFlags = PL_STARTER_FLAGS_ALL_EXTENSIONS,
        .ptWindow = pt_app_data->ptWindow};
    gpt_starter->initialize(tStarterInit);
    gpt_starter->finalize();

    // return app memory
    return pt_app_data;
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_shutdown
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_shutdown(pl_app_data *pt_app_data)
{
    gpt_starter->cleanup();
    gpt_windows->destroy(pt_app_data->ptWindow);
    PL_FREE(pt_app_data);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_resize
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_resize(pl_app_data *pt_app_data)
{
    gpt_starter->resize();
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_update(pl_app_data *pt_app_data)
{
    // this needs to be the first call when using the starter
    // extension. You must return if it returns false (usually a swapchain recreation).
    if (!gpt_starter->begin_frame())
        return;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~drawing & profile API~~~~~~~~~~~~~~~~~~~~~~~~~~~

    gpt_profile->begin_sample(0, "example drawing");

    pt_app_data->pt_fg_layer = gpt_starter->get_foreground_layer();
    dc_app_data.logic.draw();
    _refresh_variables();
    _draw_node(pt_app_data, dc_app_data.window, nullptr);

    gpt_profile->end_sample(0);

    // must be the last function called when using the starter extension
    gpt_starter->end_frame();
}

// update all variables
void _refresh_variables()
{
    for (auto const &[name, variable] : dc_app_data.variables)
    {
        DcValue *value = dc_app_index_to_dc_value(variable.value_index);
        void *externData = variable.externData;

        switch (value->type)
        {
        case DC_APP_VALUE_TYPE_FLOAT:
        {
            value->value_float = *((float *)(variable.externData));
            break;
        }
        case DC_APP_VALUE_TYPE_INTEGER:
        {
            value->value_integer = *((int *)(variable.externData));
            break;
        }
        case DC_APP_VALUE_TYPE_STRING:
        {
            value->value_string = *((std::string *)(variable.externData));
            break;
        }
        case DC_APP_VALUE_TYPE_BOOLEAN:
        {
            value->value_boolean = *((bool *)(variable.externData));
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
DcAppNodeIndex _process_node_children(xmlNodePtr xml_node, DcAppNodeIndex node_index, const std::string &directory)
{
    xmlNodePtr xml_child_node = xml_node->children;

    DcAppNodeIndex first_child_index = DC_APP_NODE_INDEX_UNDEFINED;
    DcAppNodeIndex previous_child_node_index = DC_APP_NODE_INDEX_UNDEFINED;
    while (xml_child_node)
    {
        DcAppNodeIndex child_node_index = _process_node(xml_child_node, node_index, directory);

        // get node addresses here since the address could change per node process
        DcAppNode *node = dc_app_index_to_node(node_index);
        DcAppNode *child_node = dc_app_index_to_node(child_node_index);
        DcAppNode *previous_child_node = dc_app_index_to_node(previous_child_node_index);

        // if the current node and child exists
        if (node && child_node)
        {
            // set child's parent
            child_node->parent = node_index;

            // set nodes's first child if this is the first child
            if (previous_child_node_index == DC_APP_NODE_INDEX_UNDEFINED)
            {
                first_child_index = child_node_index;
            }
        }

        // if there is a previous node
        if (previous_child_node)
        {
            // set the next node of the previous node
            previous_child_node->next = child_node_index;
        }

        // set previous child node
        if (child_node_index != DC_APP_NODE_INDEX_UNDEFINED)
        {
            previous_child_node_index = child_node_index;
        }

        // increment pointer
        xml_child_node = xml_child_node->next;
    }

    return first_child_index;
}

DcAppNodeIndex _process_node(xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, std::string directory)
{
    // by default, the element is not a node
    DcAppNodeIndex node_index = DC_APP_NODE_INDEX_UNDEFINED;

    switch (dc_app_xml_node_to_elem_type(xml_node))
    {

    // ignore non-element nodes
    case DC_APP_ELEM_TYPE_NONELEM:
    {
        break;
    }

    case DC_APP_ELEM_TYPE_CONSTANT:
    {
        // name
        char *c_name = dc_utils_get_attribute_string(xml_node, "Name");
        if (!c_name)
        {
            throw std::runtime_error("Non-existent node 'Name' in <Constant> definition");
        }
        std::string name = dc_app_dereference_constants(c_name);
        free(c_name);

        // value
        char *c_value = dc_utils_get_node_content_string(xml_node);
        if (!c_value)
        {
            throw std::runtime_error("Non-existent node content in <Constant> definition");
        }
        std::string value = dc_app_dereference_constants(c_value);
        free(c_value);

        // set constant value
        dc_app_set_constant(name, value);
        break;
    }

    case DC_APP_ELEM_TYPE_CONTAINER:
    {
        DcAppNode dc_node = {
            .type = DC_APP_NODE_TYPE_CONTAINER,
        };

        // xPosition
        char *c_x_position = dc_utils_get_attribute_string(xml_node, "X");
        if (c_x_position)
        {
            dc_node.container.position.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_position));
            free(c_x_position);
        }
        else
        {
            dc_node.container.position.x = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // y position
        char *c_y_position = dc_utils_get_attribute_string(xml_node, "Y");
        if (c_y_position)
        {
            dc_node.container.position.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_position));
            free(c_y_position);
        }
        else
        {
            dc_node.container.position.y = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // x origin
        char *c_x_origin = dc_utils_get_attribute_string(xml_node, "OriginX");
        if (c_x_origin)
        {
            dc_node.container.origin.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_origin));
            free(c_x_origin);
        }
        else
        {
            dc_node.container.origin.x = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // y origin
        char *c_y_origin = dc_utils_get_attribute_string(xml_node, "OriginY");
        if (c_y_origin)
        {
            dc_node.container.origin.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_origin));
            free(c_y_origin);
        }
        else
        {
            dc_node.container.origin.y = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // x dimension
        char *c_x_dimension = dc_utils_get_attribute_string(xml_node, "Width");
        if (c_x_dimension)
        {
            dc_node.container.dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_dimension));
            free(c_x_dimension);
        }
        else
        {
            dc_node.container.dimensions.x = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // y dimension
        char *c_y_dimension = dc_utils_get_attribute_string(xml_node, "Height");
        if (c_y_dimension)
        {
            dc_node.container.dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_dimension));
            free(c_y_dimension);
        }
        else
        {
            dc_node.container.dimensions.y = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // virtual x dimension
        char *c_x_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualWidth");
        if (c_x_virtual_dimension)
        {
            dc_node.container.virtual_dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_virtual_dimension));
            free(c_x_virtual_dimension);
        }
        else
        {
            dc_node.container.virtual_dimensions.x = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // virtual y dimension
        char *c_y_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualHeight");
        if (c_y_virtual_dimension)
        {
            dc_node.container.virtual_dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_virtual_dimension));
            free(c_y_virtual_dimension);
        }
        else
        {
            dc_node.container.virtual_dimensions.y = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // x align
        char *c_x_align = dc_utils_get_attribute_string(xml_node, "HorizontalAlign");
        if (c_x_align)
        {
            dc_node.container.alignment.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_x_align));
            free(c_x_align);
        }
        else
        {
            dc_node.container.alignment.x = dc_app_register_dc_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_LEFT));
        }

        // y align
        char *c_y_align = dc_utils_get_attribute_string(xml_node, "VerticalAlign");
        if (c_y_align)
        {
            dc_node.container.alignment.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_y_align));
            free(c_y_align);
        }
        else
        {
            dc_node.container.alignment.y = dc_app_register_dc_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_BOTTOM));
        }

        // rotation
        char *c_rotation = dc_utils_get_attribute_string(xml_node, "Rotate");
        if (c_rotation)
        {
            dc_node.container.rotation = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_rotation));
            free(c_rotation);
        }
        else
        {
            dc_node.container.rotation = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // register node
        node_index = dc_app_register_node(dc_node);

        // process children
        DcAppNodeIndex first_child_index = _process_node_children(xml_node, node_index, directory);

        // update child index
        DcAppNode *node = dc_app_index_to_node(node_index);
        node->container.child = first_child_index;
        break;
    }

    // really just the root element, left in for legacy reasons
    case DC_APP_ELEM_TYPE_DCAPP:
    {
        _process_node_children(xml_node, node_index, directory);
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
        DcAppNode dc_node = (DcAppNode){
            .type = DC_APP_NODE_TYPE_CONDITIONAL,
            .conditional = (DcAppNodeConditional){
                .value1 = dc_value_index_undefined,
                .value2 = dc_value_index_undefined,
                .child_true = DC_APP_NODE_INDEX_UNDEFINED,
                .child_false = DC_APP_NODE_INDEX_UNDEFINED,
            }};

        // conditional type
        char *c_type = dc_utils_get_attribute_string(xml_node, "Operation");
        if (c_type)
        {
            dc_node.conditional.type = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_type));
            free(c_type);
        }
        else
        {
            dc_node.conditional.type = dc_app_register_dc_value(dc_value_create_value_integer(DC_APP_CONDITIONAL_TYPE_TRUE));
        }

        // value1
        char *c_value = dc_utils_get_attribute_string(xml_node, "Value");
        if (c_value)
        {
            dc_node.conditional.value1 = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_STRING, dc_app_dereference_constants(c_value));
            free(c_value);
        }
        else
        {
            c_value = dc_utils_get_attribute_string(xml_node, "Value1");
            if (c_value)
            {
                dc_node.conditional.value1 = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_STRING, dc_app_dereference_constants(c_value));
                free(c_value);
            }
            else
            {
                throw std::runtime_error("Invalid conditional; no value specified");
            }
        }

        // value2
        char *c_value2 = dc_utils_get_attribute_string(xml_node, "Value2");
        if (c_value2)
        {
            dc_node.conditional.value2 = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_STRING, dc_app_dereference_constants(c_value2));
            free(c_value2);
        }

        // register node
        node_index = dc_app_register_node(dc_node);

        // process children (assigning to true/false handled in separate cases, e.g. DC_APP_ELEM_TYPE_TRUE)
        DcAppNodeIndex first_child_index = _process_node_children(xml_node, node_index, directory);

        // handle implicit <True> elements
        if (first_child_index != DC_APP_NODE_INDEX_UNDEFINED)
        {
            // ignore if True element already exists
            DcAppNode *node = dc_app_index_to_node(node_index);
            if (node->conditional.child_true == DC_APP_NODE_INDEX_UNDEFINED)
            {
                node->conditional.child_true = first_child_index;
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
        char *c_directory = dc_utils_get_attribute_string(xml_node, "Directory");
        if (c_directory)
        {
            node_index = _process_node_children(xml_node, parent_node_index, dc_app_dereference_constants(c_directory));
            free(c_directory);
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
        char *c_file_path = dc_utils_get_attribute_string(xml_node, "File");
        if (c_file_path)
        {
            std::string filePath = dc_utils_filepath_to_canonical(dc_app_dereference_constants(c_file_path), directory);
            free(c_file_path);

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
            if (gpt_library->load(logicSoDesc, &dc_app_data.logic.library) != PL_LIBRARY_RESULT_SUCCESS)
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
        DcAppNode dc_node = {
            .type = DC_APP_NODE_TYPE_PANEL,
        };

        // parent dimensions
        // TODO probably don't need this.....must be a better way to architect
        dc_node.panel.parentDimensions = dc_app_index_to_node(parent_node_index)->window.virtual_dimensions;

        // virtual x dimension
        char *c_x_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualWidth");
        if (c_x_virtual_dimension)
        {
            dc_node.panel.virtual_dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_virtual_dimension));
            free(c_x_virtual_dimension);
        }
        else
        {
            dc_node.panel.virtual_dimensions.x = dc_node.panel.parentDimensions.x;
        }

        // virtual y dimension
        char *c_y_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualHeight");
        if (c_y_virtual_dimension)
        {
            dc_node.panel.virtual_dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_virtual_dimension));
            free(c_y_virtual_dimension);
        }
        else
        {
            dc_node.panel.virtual_dimensions.y = dc_node.panel.parentDimensions.y;
        }

        // register node
        node_index = dc_app_register_node(dc_node);

        // process children
        DcAppNodeIndex first_child_index = _process_node_children(xml_node, node_index, directory);

        // update child index
        DcAppNode *node = dc_app_index_to_node(node_index);
        node->panel.child = first_child_index;

        break;
    }

    case DC_APP_ELEM_TYPE_POLYGON:
    {
        DcAppNode dc_node = {
            .type = DC_APP_NODE_TYPE_POLYGON,
            .polygon = (DcAppNodePolygon){
                .fill_enabled = false,
                .line_enabled = false,
            }};

        // fill color
        char *c_fill_color = dc_utils_get_attribute_string(xml_node, "FillColor");
        if (c_fill_color)
        {
            // split string by whitespace
            std::string s_fill_color = dc_app_dereference_constants(c_fill_color);
            std::vector<std::string> substrings = dc_utils_split_string_by_delimiters(s_fill_color, dc_utils_string_whitespace);

            if (substrings.size() > 0)
            {
                dc_node.polygon.fillColor.r = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[0]);
                if (substrings.size() > 1)
                {
                    dc_node.polygon.fillColor.g = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[1]);
                    if (substrings.size() > 2)
                    {
                        dc_node.polygon.fillColor.b = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[2]);
                        if (substrings.size() > 3)
                        {
                            dc_node.polygon.fillColor.a = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[3]);
                        }
                        else
                        {
                            dc_node.polygon.fillColor.a = dc_app_register_dc_value(dc_value_create_value_float(1.0f));
                        }
                    }
                    else
                    {
                        dc_node.polygon.fillColor.b = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
                    }
                }
                else
                {
                    dc_node.polygon.fillColor.g = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
                }
            }
            else
            {
                dc_node.polygon.fillColor.r = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
            }
            dc_node.polygon.fill_enabled = true;
            free(c_fill_color);
        }

        // line color
        char *c_line_color = dc_utils_get_attribute_string(xml_node, "LineColor");
        if (c_line_color)
        {
            // split string by whitespace
            std::string s_fill_color = dc_app_dereference_constants(c_line_color);
            std::vector<std::string> substrings = dc_utils_split_string_by_delimiters(s_fill_color, dc_utils_string_whitespace);

            if (substrings.size() > 0)
            {
                dc_node.polygon.line_color.r = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[0]);
                if (substrings.size() > 1)
                {
                    dc_node.polygon.line_color.g = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[1]);
                    if (substrings.size() > 2)
                    {
                        dc_node.polygon.line_color.b = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[2]);
                        if (substrings.size() > 3)
                        {
                            dc_node.polygon.line_color.a = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, substrings[3]);
                        }
                        else
                        {
                            dc_node.polygon.line_color.a = dc_app_register_dc_value(dc_value_create_value_float(1.0f));
                        }
                    }
                    else
                    {
                        dc_node.polygon.line_color.b = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
                    }
                }
                else
                {
                    dc_node.polygon.line_color.g = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
                }
            }
            else
            {
                dc_node.polygon.line_color.r = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
            }
            dc_node.polygon.line_enabled = true;
            free(c_line_color);
        }

        // line width
        char *c_line_width = dc_utils_get_attribute_string(xml_node, "LineWidth");
        if (c_line_width)
        {
            dc_node.polygon.line_width = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_line_width));
            free(c_line_width);
        }
        else
        {
            dc_node.polygon.line_width = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // initialize points to 0
        dc_node.polygon.num_points = 0;
        dc_node.polygon.points = nullptr;

        // register node
        node_index = dc_app_register_node(dc_node);

        // process children
        _process_node_children(xml_node, node_index, directory);
        break;
    }

    case DC_APP_ELEM_TYPE_VARIABLE:
    {
        // name
        char *c_name = dc_utils_get_node_content_string(xml_node);
        if (!c_name)
        {
            throw std::runtime_error("Non-existent node content in <Variable> definition");
        }
        std::string name = dc_app_dereference_constants(c_name);
        free(c_name);

        // type
        char *c_type = dc_utils_get_attribute_string(xml_node, "Type");
        DcValueType type = DC_APP_VALUE_TYPE_STRING;
        if (c_type)
        {
            type = dc_value_type_from_string(dc_app_dereference_constants(c_type));
        }

        // value
        char *c_initial_value = dc_utils_get_attribute_string(xml_node, "InitialValue");
        DcValue initial_value;
        if (c_initial_value)
        {
            initial_value = dc_value_create_value_string(dc_app_dereference_constants(c_initial_value));
        }
        else
        {
            initial_value = dc_value_create_value_string("");
        }
        initial_value.type = type;
        initial_value.is_dynamic = true;

        // register variable
        DcAppValueIndex index = dc_app_register_dc_value(initial_value);
        dc_app_set_variable(name, index);

        break;
    }

    case DC_APP_ELEM_TYPE_VERTEX:
    {
        DcAppNode *parent_node = dc_app_index_to_node(parent_node_index);
        switch (parent_node->type)
        {
        case DC_APP_NODE_TYPE_POLYGON:
        {
            // xPosition
            char *c_x_position = dc_utils_get_attribute_string(xml_node, "X");
            if (!c_x_position)
            {
                throw std::runtime_error("Invalid Vertex: No X attribute");
            }

            // yPosition
            char *c_y_position = dc_utils_get_attribute_string(xml_node, "Y");
            if (!c_y_position)
            {
                throw std::runtime_error("Invalid Vertex: No Y attribute");
            }

            // reallocate and add vertices
            parent_node->polygon.num_points++;
            parent_node->polygon.points = (DcAppValueIndex2 *)realloc(parent_node->polygon.points, parent_node->polygon.num_points * sizeof(DcAppValueIndex2));
            parent_node->polygon.points[parent_node->polygon.num_points - 1] = (DcAppValueIndex2){
                dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_position)),
                dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_position))};
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
        DcAppNode dc_node = {
            .type = DC_APP_NODE_TYPE_WINDOW,
        };

        // title
        std::string title = "dcapp";
        char *c_title = dc_utils_get_attribute_string(xml_node, "Title");
        if (c_title)
        {
            title = dc_app_dereference_constants(c_title);
            free(c_title);
        }
        dc_node.window.title = (char *)malloc(title.length() + 1);
        memcpy(dc_node.window.title, title.c_str(), title.length() + 1);

        // xPosition
        char *c_x_position = dc_utils_get_attribute_string(xml_node, "X");
        if (c_x_position)
        {
            dc_node.window.position.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_x_position));
            free(c_x_position);
        }
        else
        {
            dc_node.window.position.x = dc_app_register_dc_value(dc_value_create_value_integer(0.0f));
        }

        // y position
        char *c_y_position = dc_utils_get_attribute_string(xml_node, "Y");
        if (c_y_position)
        {
            dc_node.window.position.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_y_position));
            free(c_y_position);
        }
        else
        {
            dc_node.window.position.y = dc_app_register_dc_value(dc_value_create_value_integer(0.0f));
        }

        // x dimension
        char *c_x_dimension = dc_utils_get_attribute_string(xml_node, "Width");
        if (c_x_dimension)
        {
            dc_node.window.dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_x_dimension));
            free(c_x_dimension);
        }
        else
        {
            dc_node.window.dimensions.x = dc_app_register_dc_value(dc_value_create_value_integer(0.0f));
        }

        // y dimension
        char *c_y_dimension = dc_utils_get_attribute_string(xml_node, "Height");
        if (c_y_dimension)
        {
            dc_node.window.dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_y_dimension));
            free(c_y_dimension);
        }
        else
        {
            dc_node.window.dimensions.y = dc_app_register_dc_value(dc_value_create_value_integer(0.0f));
        }

        // virtual x dimension
        char *c_x_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualWidth");
        if (c_x_virtual_dimension)
        {
            dc_node.window.virtual_dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_virtual_dimension));
            free(c_x_virtual_dimension);
        }
        else
        {
            dc_node.window.virtual_dimensions.x = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // virtual y dimension
        char *c_y_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualHeight");
        if (c_y_virtual_dimension)
        {
            dc_node.window.virtual_dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_APP_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_virtual_dimension));
            free(c_y_virtual_dimension);
        }
        else
        {
            dc_node.window.virtual_dimensions.y = dc_app_register_dc_value(dc_value_create_value_float(0.0f));
        }

        // register node
        node_index = dc_app_register_node(dc_node);

        // process children
        DcAppNodeIndex first_child_index = _process_node_children(xml_node, node_index, directory);

        // update child index
        DcAppNode *node = dc_app_index_to_node(node_index);
        node->window.child = first_child_index;

        // set global window
        dc_app_data.window = node_index;
        break;
    }
    default:
        throw std::runtime_error("Invalid node in _process_node()");
    }

    return node_index;
}

void _draw_node_list(pl_app_data *pt_app_data, DcAppNodeIndex node_index, plMat4 *node_transform)
{
    DcAppNodeIndex current_node_index = node_index;
    while (current_node_index != DC_APP_NODE_INDEX_UNDEFINED)
    {
        _draw_node(pt_app_data, current_node_index, node_transform);
        current_node_index = dc_app_index_to_node(current_node_index)->next;
    }
}

void _draw_node(pl_app_data *pt_app_data, DcAppNodeIndex node_index, plMat4 *parent_transform)
{
    if (node_index == DC_APP_NODE_INDEX_UNDEFINED)
    {
        throw std::runtime_error("Attempting to draw undefined node index");
    }

    DcAppNode *node = dc_app_index_to_node(node_index);
    switch (node->type)
    {
    case DC_APP_NODE_TYPE_CONTAINER:
    {
        float x_position = 0;
        switch (dc_app_index_to_dc_value(node->container.alignment.x)->value_integer)
        {
        case DC_APP_ALIGN_TYPE_LEFT:
            x_position = dc_app_index_to_dc_value(node->container.position.x)->value_float;
            break;
        case DC_APP_ALIGN_TYPE_CENTER:
            x_position = dc_app_index_to_dc_value(node->container.position.x)->value_float - dc_app_index_to_dc_value(node->container.dimensions.x)->value_float / 2;
            break;
        case DC_APP_ALIGN_TYPE_RIGHT:
            x_position = dc_app_index_to_dc_value(node->container.position.x)->value_float - dc_app_index_to_dc_value(node->container.dimensions.x)->value_float;
            break;
        }

        float y_position = 0;
        switch (dc_app_index_to_dc_value(node->container.alignment.y)->value_integer)
        {
        case DC_APP_ALIGN_TYPE_LEFT:
            y_position = dc_app_index_to_dc_value(node->container.position.y)->value_float;
            break;
        case DC_APP_ALIGN_TYPE_CENTER:
            y_position = dc_app_index_to_dc_value(node->container.position.y)->value_float - dc_app_index_to_dc_value(node->container.dimensions.y)->value_float / 2;
            break;
        case DC_APP_ALIGN_TYPE_RIGHT:
            y_position = dc_app_index_to_dc_value(node->container.position.y)->value_float - dc_app_index_to_dc_value(node->container.dimensions.y)->value_float;
            break;
        }

        plMat4 trans_origin_matrix = pl_mat4_translate_xyz(
            dc_app_index_to_dc_value(node->container.origin.x)->value_float,
            dc_app_index_to_dc_value(node->container.origin.y)->value_float,
            0.0f);
        plMat4 rotate_matrix = pl_mat4_rotate_vec3(
            pl_radiansf(dc_app_index_to_dc_value(node->container.rotation)->value_float),
            (plVec3){0.0f, 0.0f, 1.0f});
        plMat4 trans_position_matrix = pl_mat4_translate_xyz(
            x_position,
            y_position,
            0.0f);
        plMat4 scale_matrix = pl_mat4_scale_xyz(
            dc_app_index_to_dc_value(node->container.dimensions.x)->value_float / dc_app_index_to_dc_value(node->container.virtual_dimensions.x)->value_float,
            dc_app_index_to_dc_value(node->container.dimensions.y)->value_float / dc_app_index_to_dc_value(node->container.virtual_dimensions.y)->value_float,
            1.0f);

        plMat4 transform = (plMat4){0};
        transform = pl_mul_mat4t(parent_transform, &trans_origin_matrix);
        transform = pl_mul_mat4t(&transform, &rotate_matrix);
        transform = pl_mul_mat4t(&transform, &trans_position_matrix);
        transform = pl_mul_mat4t(&transform, &scale_matrix);
        _draw_node_list(pt_app_data, node->container.child, &transform);
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
        plMat4 scale_matrix = pl_mat4_scale_xyz(
            dc_app_index_to_dc_value(node->panel.parentDimensions.x)->value_float / dc_app_index_to_dc_value(node->panel.virtual_dimensions.x)->value_float,
            dc_app_index_to_dc_value(node->panel.parentDimensions.y)->value_float / dc_app_index_to_dc_value(node->panel.virtual_dimensions.y)->value_float,
            1.0f);
        plMat4 transform = (plMat4){0};
        transform = pl_mul_mat4t(parent_transform, &scale_matrix);
        _draw_node_list(pt_app_data, node->panel.child, &transform);
        break;
    }

    case DC_APP_NODE_TYPE_POLYGON:
    {
        // get points
        std::vector<plVec2> points;
        points.resize(node->polygon.num_points);
        for (int ii = 0; ii < node->polygon.num_points; ii++)
        {
            plVec4 point4 = (plVec4){
                dc_app_index_to_dc_value(node->polygon.points[ii].x)->value_float,
                dc_app_index_to_dc_value(node->polygon.points[ii].y)->value_float,
                0, 1};
            point4 = pl_mul_mat4_vec4(parent_transform, point4);
            points[ii] = (plVec2){point4.x, point4.y};
        }

        // draw fill
        if (node->polygon.fill_enabled)
        {
            uint32_t fillColor = PL_COLOR_32_RGBA(
                dc_app_index_to_dc_value(node->polygon.fillColor.r)->value_float,
                dc_app_index_to_dc_value(node->polygon.fillColor.g)->value_float,
                dc_app_index_to_dc_value(node->polygon.fillColor.b)->value_float,
                dc_app_index_to_dc_value(node->polygon.fillColor.a)->value_float);
            gpt_draw->add_convex_polygon_filled(pt_app_data->pt_fg_layer, points.data(), points.size(), (plDrawSolidOptions){.uColor = fillColor});
        }

        // draw outline
        if (node->polygon.line_enabled)
        {
            float lineThickness = dc_app_index_to_dc_value(node->polygon.line_width)->value_float;
            uint32_t line_color = PL_COLOR_32_RGBA(
                dc_app_index_to_dc_value(node->polygon.line_color.r)->value_float,
                dc_app_index_to_dc_value(node->polygon.line_color.g)->value_float,
                dc_app_index_to_dc_value(node->polygon.line_color.b)->value_float,
                dc_app_index_to_dc_value(node->polygon.line_color.a)->value_float);
            gpt_draw->add_polygon(pt_app_data->pt_fg_layer, points.data(), points.size(), (plDrawLineOptions){.uColor = line_color, .fThickness = lineThickness});
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
        gpt_windows->get_size(pt_app_data->ptWindow, &dimensionX, &dimensionY);
        DcValue *dimension_value_x = dc_app_index_to_dc_value(node->window.dimensions.x);
        DcValue *dimension_value_y = dc_app_index_to_dc_value(node->window.dimensions.y);
        dimension_value_x->value_integer = (int)dimensionX;
        dimension_value_y->value_integer = (int)dimensionY;
        dc_value_refresh_value(dimension_value_x);
        dc_value_refresh_value(dimension_value_y);

        // compute transforms
        // translate from negative to positive range
        plMat4 trans_matrix = pl_mat4_translate_xyz(
            0.0f,
            dc_app_index_to_dc_value(node->window.dimensions.y)->value_float,
            0.0f);

        // scale from virtual to real dimensions, flip y axis
        plMat4 scale_matrix = pl_mat4_scale_xyz(
            dc_app_index_to_dc_value(node->window.dimensions.x)->value_float / dc_app_index_to_dc_value(node->window.virtual_dimensions.x)->value_float,
            dc_app_index_to_dc_value(node->window.dimensions.y)->value_float / dc_app_index_to_dc_value(node->window.virtual_dimensions.y)->value_float * -1.0f,
            1.0f);

        plMat4 transform;
        transform = pl_mul_mat4t(&trans_matrix, &scale_matrix);
        _draw_node_list(pt_app_data, node->window.child, &transform);
        break;
    }

    default:
        break;
    }
}
