
//-----------------------------------------------------------------------------
// [SECTION] dcapp includes
//-----------------------------------------------------------------------------

#include "../src/app.hpp"
#include "trick.hpp"
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

typedef struct _plAppData {
    // window
    plWindow      *pt_window;
    plDrawLayer2D *pt_fg_layer;

    // console variable
    bool b_show_help_window;
} pl_app_data;

//-----------------------------------------------------------------------------
// [SECTION] dcapp state
//-----------------------------------------------------------------------------

static DcAppNodeIndex _process_node_children(xmlNodePtr xml_node, DcAppNodeIndex node_index, DcAppElemType parent_elem_type, const std::string &directory);
static DcAppNodeIndex _process_node(xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, std::string directory);
static void           _draw_node_list(pl_app_data *pt_app_data, DcAppNodeIndex node_index, plMat4 *node_transform);
static void           _draw_node(pl_app_data *pt_app_data, DcAppNodeIndex node_index, plMat4 *parent_transform);

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

const plWindowI  *gpt_windows = NULL;
const plDrawI    *gpt_draw    = NULL;
const plStarterI *gpt_starter = NULL;
const plProfileI *gpt_profile = NULL;
const plMemoryI  *gpt_memory  = NULL;
const plLibraryI *gpt_library = NULL;
const plIOI      *gpt_ioi     = NULL;

#define PL_ALLOC(x) gpt_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_REALLOC(x, y) gpt_memory->tracked_realloc((x), (y), __FILE__, __LINE__)
#define PL_FREE(x) gpt_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

//-----------------------------------------------------------------------------
// [SECTION] pl_app_load
//-----------------------------------------------------------------------------s

PL_EXPORT void *
pl_app_load(plApiRegistryI *pt_api_registry, pl_app_data *pt_app_data) {
    // NOTE: on first load, "pAppData" will be NULL but on reloads
    //       it will be the value returned from this function

    // if "ptAppData" is a valid pointer, then this function is being called
    // during a hot reload.
    if (pt_app_data) {
        // re-retrieve the apis since we are now in
        // a different dll/so
        gpt_windows = pl_get_api_latest(pt_api_registry, plWindowI);
        gpt_draw    = pl_get_api_latest(pt_api_registry, plDrawI);
        gpt_starter = pl_get_api_latest(pt_api_registry, plStarterI);
        gpt_profile = pl_get_api_latest(pt_api_registry, plProfileI);
        gpt_memory  = pl_get_api_latest(pt_api_registry, plMemoryI);
        gpt_library = pl_get_api_latest(pt_api_registry, plLibraryI);
        gpt_ioi     = pl_get_api_latest(pt_api_registry, plIOI);

        return pt_app_data;
    }

    // retrieve extension registry
    const plExtensionRegistryI *pt_extension_registry = pl_get_api_latest(pt_api_registry, plExtensionRegistryI);

    // load extensions
    //   * first argument is the shared library name WITHOUT the extension
    //   * second & third argument is the load/unload functions names (use NULL for the default of "pl_load_ext" &
    //     "pl_unload_ext")
    //   * fourth argument indicates if the extension is reloadable (should we check for changes and reload if changed)
    pt_extension_registry->load("pl_unity_ext", NULL, NULL, true);
    pt_extension_registry->load("pl_platform_ext", NULL, NULL, false); // provides the file API used by the drawing ext

    // load required apis
    gpt_windows = pl_get_api_latest(pt_api_registry, plWindowI);
    gpt_draw    = pl_get_api_latest(pt_api_registry, plDrawI);
    gpt_starter = pl_get_api_latest(pt_api_registry, plStarterI);
    gpt_profile = pl_get_api_latest(pt_api_registry, plProfileI);
    gpt_memory  = pl_get_api_latest(pt_api_registry, plMemoryI);
    gpt_library = pl_get_api_latest(pt_api_registry, plLibraryI);
    gpt_ioi     = pl_get_api_latest(pt_api_registry, plIOI);

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
    // std::string config_relative_path = args.front();
    // if (config_relative_path.empty()) {
    //     throw std::runtime_error("Missing dcapp configuration file");
    // }
    // std::string config_relative_path = "/home/nathan/dcapp-vk/samples/test/test.xml";

    // parse input arguments
    plIO *gpt_io = gpt_ioi->get_io();
    if (gpt_io->iArgc < 4) {
        throw std::runtime_error("Missing dcapp configuration file");
    }
    std::vector<std::string> args(gpt_io->apArgv + 3, gpt_io->apArgv + gpt_io->iArgc);

    // TODO process input arguments (constant setting)
    std::string config_relative_path = args[0];

    // set paths
    std::filesystem::path fsFilePath       = std::filesystem::canonical(config_relative_path);
    std::filesystem::path fs_dir_path      = fsFilePath.parent_path();
    std::string           config_file_path = fsFilePath.string();
    std::string           config_dir_path  = fs_dir_path.string();

    // create cache and log dirs
    std::filesystem::path fs_exe_path = dc_utils_get_exe_filepath();
    std::filesystem::path fs_log_path = fs_exe_path.parent_path() / ".." / "logs";
    std::filesystem::create_directory(fs_log_path);
    std::string           log_dir_path  = std::filesystem::canonical(fs_log_path).string();
    std::filesystem::path fs_cache_path = fs_exe_path.parent_path() / ".." / "cache";
    std::filesystem::create_directory(fs_cache_path);
    std::string cache_dir_path = std::filesystem::canonical(fs_cache_path).string();

    // init dc_app_data object
    dc_app_init_data();

    // begin setting up dcappData object
    dc_app_data.config_file_path = config_file_path;
    dc_app_data.config_dir_path  = config_dir_path;
    dc_app_data.log_dir_path     = log_dir_path;
    dc_app_data.cache_dir_path   = cache_dir_path;

    // set environment (used for dcapp XMLs)
    setenv("dcappDisplayHome", dc_app_data.config_dir_path.c_str(), 1);

    // load XML file
    dc_app_data.doc = xmlReadFile(config_file_path.c_str(), "UTF-8", XML_PARSE_NOBLANKS);
    if (!dc_app_data.doc) {
        throw std::runtime_error("Unable to read configuration file: " + config_file_path);
    }

    // clean XML file
    dc_app_clean_xml_data();

    // save cleaned xml to file
    std::filesystem::path fs_out_file = std::filesystem::path(log_dir_path) / "config.xml";
    xmlSaveFormatFile(fs_out_file.string().c_str(), dc_app_data.doc, 1);

    // process XML
    xmlNodePtr root_node = xmlDocGetRootElement(dc_app_data.doc);
    _process_node(root_node, DC_APP_NODE_INDEX_UNDEFINED, DC_APP_ELEM_TYPE_UNDEFINED, config_dir_path);

    // configure logic file
    if (dc_app_data.logic.library) {
        // set logic functions
        dc_app_data.logic.pre_init = (void (*)(void))gpt_library->load_function(dc_app_data.logic.library, "DisplayPreInit");
        dc_app_data.logic.init     = (void (*)(void))gpt_library->load_function(dc_app_data.logic.library, "DisplayInit");
        dc_app_data.logic.draw     = (void (*)(void))gpt_library->load_function(dc_app_data.logic.library, "DisplayDraw");
        dc_app_data.logic.close    = (void (*)(void))gpt_library->load_function(dc_app_data.logic.library, "DisplayClose");

        // link variables to extern logic data
        for (auto const &[name, var_index] : dc_app_data.var_indices) {
            DcAppVar *var    = &(dc_app_data.vars[var_index]);
            var->extern_data = gpt_library->load_function(dc_app_data.logic.library, name.c_str());

            // set the extern data to the initial value
            dc_app_refresh_var_from_value(var);
        }
    }

    // validate
    // root->validate();

    // set initial window params
    DcAppNode   *window_node   = dc_app_index_to_node(dc_app_data.window);
    plWindowDesc t_window_desc = {
        .pcTitle = window_node->window.title,
        .uWidth  = (uint32_t)(dc_app_get_value(window_node->window.dimensions.x)->value_integer),
        .uHeight = (uint32_t)(dc_app_get_value(window_node->window.dimensions.y)->value_integer),
        .iXPos   = dc_app_get_value(window_node->window.position.x)->value_integer,
        .iYPos   = dc_app_get_value(window_node->window.position.y)->value_integer,
    };

    gpt_windows->create(t_window_desc, &pt_app_data->pt_window);
    gpt_windows->show(pt_app_data->pt_window);

    // initialize the starter API (handles alot of boilerplate)
    plStarterInit tStarterInit = {
        .tFlags   = PL_STARTER_FLAGS_ALL_EXTENSIONS,
        .ptWindow = pt_app_data->pt_window};
    gpt_starter->initialize(tStarterInit);
    gpt_starter->finalize();

    // return app memory
    return pt_app_data;
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_shutdown
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_shutdown(pl_app_data *pt_app_data) {
    gpt_starter->cleanup();
    gpt_windows->destroy(pt_app_data->pt_window);
    PL_FREE(pt_app_data);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_resize
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_resize(pl_app_data *pt_app_data) {
    gpt_starter->resize();
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_update(pl_app_data *pt_app_data) {
    // this needs to be the first call when using the starter
    // extension. You must return if it returns false (usually a swapchain recreation).
    if (!gpt_starter->begin_frame())
        return;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~drawing & profile API~~~~~~~~~~~~~~~~~~~~~~~~~~~

    gpt_profile->begin_sample(0, "example drawing");
    pt_app_data->pt_fg_layer = gpt_starter->get_foreground_layer();

    // send trick data
    for (int ii = 0; ii < dc_app_data.trick_contexts.size(); ii++) {

        DcAppTrickContext *trick_context = dc_app_data.trick_contexts[ii];
        DcTrick           *trick         = trick_context->trick;

        // add tx commands to buffer
        if (trick->is_connected) {
            for (int jj = 0; jj < trick_context->tx_var_contexts.size(); jj++) {

                DcAppTrickTxVarContext *tx_var_context = &(trick_context->tx_var_contexts[jj]);
                DcAppVar               *dc_app_var     = &dc_app_data.vars[tx_var_context->dcapp_var_index];
                DcValue                *curr_value     = &dc_app_data.values[dc_app_var->value_index];
                DcValue                *prev_value     = &tx_var_context->prev_value;

                // send if new value is different
                if (!dc_value_is_equal(curr_value, prev_value)) {
                    dc_trick_set_tx_var(trick, tx_var_context->trick_var_index, curr_value->value_string.c_str());
                    dc_value_copy(prev_value, curr_value);
                }
            }
        }

        // send the updated buffer, receive the new data, update the connection status
        dc_trick_update(trick);

        // receive the new data
        if (trick->has_new_data && trick->is_connected) {
            char rx_buffer[256];
            for (int jj = 0; jj < trick_context->rx_var_contexts.size(); jj++) {

                DcAppTrickRxVarContext *rx_var_context = &(trick_context->rx_var_contexts[jj]);
                DcAppVar               *dc_app_var     = &dc_app_data.vars[rx_var_context->dcapp_var_index];
                DcValue                *value          = &dc_app_data.values[dc_app_var->value_index];

                dc_trick_get_rx_var_value(trick, rx_var_context->trick_var_index, rx_buffer);
                dc_app_set_var_to_string(dc_app_var, (std::string)rx_buffer);
            }
        }
    }

    // process logic, update vars from extern_data
    dc_app_data.logic.draw();
    for (auto const &[name, var_index] : dc_app_data.var_indices) {
        DcAppVar *var = &dc_app_data.vars[var_index];
        dc_app_refresh_var_from_extern(var);
    }

    _draw_node(pt_app_data, dc_app_data.window, nullptr);

    gpt_profile->end_sample(0);

    // must be the last function called when using the starter extension
    gpt_starter->end_frame();
}

// returns the first child (if any)
DcAppNodeIndex _process_node_children(xmlNodePtr xml_node, DcAppNodeIndex node_index, DcAppElemType elem_type, const std::string &directory) {
    xmlNodePtr xml_child_node = xml_node->children;

    DcAppNodeIndex first_child_index         = DC_APP_NODE_INDEX_UNDEFINED;
    DcAppNodeIndex previous_child_node_index = DC_APP_NODE_INDEX_UNDEFINED;
    while (xml_child_node) {
        DcAppNodeIndex child_node_index = _process_node(xml_child_node, node_index, elem_type, directory);

        if (child_node_index != DC_APP_NODE_INDEX_UNDEFINED) {

            // get node addresses here since the address could change per node process
            DcAppNode *node                = dc_app_index_to_node(node_index);
            DcAppNode *child_node          = dc_app_index_to_node(child_node_index);
            DcAppNode *previous_child_node = dc_app_index_to_node(previous_child_node_index);

            // if the current node and child exists
            if (node && child_node) {
                // set child's parent
                child_node->parent = node_index;

                // set nodes's first child if this is the first child
                if (previous_child_node_index == DC_APP_NODE_INDEX_UNDEFINED) {
                    first_child_index = child_node_index;
                }
            }

            // if there is a previous node
            if (previous_child_node) {
                // set the next node of the previous node
                previous_child_node->next = child_node_index;
            }

            // set previous child node
            if (child_node_index != DC_APP_NODE_INDEX_UNDEFINED) {
                previous_child_node_index = child_node_index;
            }
        }

        // increment pointer
        xml_child_node = xml_child_node->next;
    }

    return first_child_index;
}

DcAppNodeIndex _process_node(xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, std::string directory) {
    // by default, the element is not a node
    DcAppNodeIndex node_index = DC_APP_NODE_INDEX_UNDEFINED;

    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);
    switch (elem_type) {

        // ignore non-element nodes
        case DC_APP_ELEM_TYPE_NONELEM: {
            break;
        }

        case DC_APP_ELEM_TYPE_CONSTANT: {
            // name
            char *c_name = dc_utils_get_attribute_string(xml_node, "Name");
            if (!c_name) {
                throw std::runtime_error("Non-existent node 'Name' in <Constant> definition");
            }
            std::string name = dc_app_dereference_constants(c_name);
            free(c_name);

            // value
            char *c_value = dc_utils_get_node_content_string(xml_node);
            if (!c_value) {
                throw std::runtime_error("Non-existent node content in <Constant> definition");
            }
            std::string value = dc_app_dereference_constants(c_value);
            free(c_value);

            // set constant value
            dc_app_set_constant(name, value);
            break;
        }

        case DC_APP_ELEM_TYPE_CONTAINER: {
            DcAppNode dc_node = {
                .type = DC_APP_NODE_TYPE_CONTAINER,
            };

            // xPosition
            char *c_x_position = dc_utils_get_attribute_string(xml_node, "X");
            if (c_x_position) {
                dc_node.container.position.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_position));
                free(c_x_position);
            } else {
                dc_node.container.position.x = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // y position
            char *c_y_position = dc_utils_get_attribute_string(xml_node, "Y");
            if (c_y_position) {
                dc_node.container.position.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_position));
                free(c_y_position);
            } else {
                dc_node.container.position.y = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // x origin
            char *c_x_origin = dc_utils_get_attribute_string(xml_node, "OriginX");
            if (c_x_origin) {
                dc_node.container.origin.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_origin));
                free(c_x_origin);
            } else {
                dc_node.container.origin.x = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // y origin
            char *c_y_origin = dc_utils_get_attribute_string(xml_node, "OriginY");
            if (c_y_origin) {
                dc_node.container.origin.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_origin));
                free(c_y_origin);
            } else {
                dc_node.container.origin.y = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // x dimension
            char *c_x_dimension = dc_utils_get_attribute_string(xml_node, "Width");
            if (c_x_dimension) {
                dc_node.container.dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_dimension));
                free(c_x_dimension);
            } else {
                dc_node.container.dimensions.x = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // y dimension
            char *c_y_dimension = dc_utils_get_attribute_string(xml_node, "Height");
            if (c_y_dimension) {
                dc_node.container.dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_dimension));
                free(c_y_dimension);
            } else {
                dc_node.container.dimensions.y = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // virtual x dimension
            char *c_x_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualWidth");
            if (c_x_virtual_dimension) {
                dc_node.container.virtual_dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_virtual_dimension));
                free(c_x_virtual_dimension);
            } else {
                dc_node.container.virtual_dimensions.x = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // virtual y dimension
            char *c_y_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualHeight");
            if (c_y_virtual_dimension) {
                dc_node.container.virtual_dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_virtual_dimension));
                free(c_y_virtual_dimension);
            } else {
                dc_node.container.virtual_dimensions.y = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // x align
            char *c_x_align = dc_utils_get_attribute_string(xml_node, "HorizontalAlign");
            if (c_x_align) {
                dc_node.container.alignment.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_x_align));
                free(c_x_align);
            } else {
                dc_node.container.alignment.x = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_LEFT));
            }

            // y align
            char *c_y_align = dc_utils_get_attribute_string(xml_node, "VerticalAlign");
            if (c_y_align) {
                dc_node.container.alignment.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_y_align));
                free(c_y_align);
            } else {
                dc_node.container.alignment.y = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_BOTTOM));
            }

            // rotation
            char *c_rotation = dc_utils_get_attribute_string(xml_node, "Rotate");
            if (c_rotation) {
                dc_node.container.rotation = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_rotation));
                free(c_rotation);
            } else {
                dc_node.container.rotation = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // register node
            node_index = dc_app_register_node(dc_node);

            // process children
            DcAppNodeIndex first_child_index = _process_node_children(xml_node, node_index, elem_type, directory);

            // update child index
            DcAppNode *node       = dc_app_index_to_node(node_index);
            node->container.child = first_child_index;
            break;
        }

        // really just the root element, left in for legacy reasons
        case DC_APP_ELEM_TYPE_DCAPP: {
            _process_node_children(xml_node, node_index, elem_type, directory);
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

        case DC_APP_ELEM_TYPE_FALSE:
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_IF: {

                    // process children
                    DcAppNodeIndex first_child_index = _process_node_children(xml_node, parent_node_index, elem_type, directory);

                    // update child false node
                    DcAppNode *parent_node               = dc_app_index_to_node(parent_node_index);
                    parent_node->conditional.child_false = first_child_index;
                    break;
                }
                default:
                    throw std::runtime_error("Invalid elem parent of type " + dc_app_elem_type_to_string(parent_elem_type) + " for <False>.");
            }
            break;

        case DC_APP_ELEM_TYPE_IF: {
            DcAppNode dc_node = (DcAppNode){
                .type        = DC_APP_NODE_TYPE_CONDITIONAL,
                .conditional = (DcAppNodeConditional){
                    .value1      = DC_VALUE_INDEX_UNDEFINED,
                    .value2      = DC_VALUE_INDEX_UNDEFINED,
                    .child_true  = DC_APP_NODE_INDEX_UNDEFINED,
                    .child_false = DC_APP_NODE_INDEX_UNDEFINED,
                }};

            // conditional type
            char *c_type = dc_utils_get_attribute_string(xml_node, "Operation");
            if (c_type) {
                dc_node.conditional.type = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_type));
                free(c_type);
            } else {
                dc_node.conditional.type = dc_app_register_value(dc_value_create_value_integer(DC_APP_CONDITIONAL_TYPE_TRUE));
            }

            // value1
            char *c_value = dc_utils_get_attribute_string(xml_node, "Value");
            if (c_value) {
                dc_node.conditional.value1 = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_STRING, dc_app_dereference_constants(c_value));
                free(c_value);
            } else {
                c_value = dc_utils_get_attribute_string(xml_node, "Value1");
                if (c_value) {
                    dc_node.conditional.value1 = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_STRING, dc_app_dereference_constants(c_value));
                    free(c_value);
                } else {
                    throw std::runtime_error("Invalid conditional; no value specified");
                }
            }

            // value2
            char *c_value2 = dc_utils_get_attribute_string(xml_node, "Value2");
            if (c_value2) {
                dc_node.conditional.value2 = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_STRING, dc_app_dereference_constants(c_value2));
                free(c_value2);
            }

            // register node
            node_index = dc_app_register_node(dc_node);

            // process children (assigning to true/false handled in separate cases, e.g. DC_APP_ELEM_TYPE_TRUE)
            DcAppNodeIndex first_child_index = _process_node_children(xml_node, node_index, elem_type, directory);

            // handle implicit <True> elements
            if (first_child_index != DC_APP_NODE_INDEX_UNDEFINED) {
                // ignore if True element already exists
                DcAppNode *node = dc_app_index_to_node(node_index);
                if (node->conditional.child_true == DC_APP_NODE_INDEX_UNDEFINED) {
                    node->conditional.child_true = first_child_index;
                } else {
                    printf("Warning: <If> element has <True> explicit and implicit elements. Ignoring the implicit definitions\n");
                }
            }
            break;
        }

        // at this point, just used to set the directory path
        case DC_APP_ELEM_TYPE_INCLUDE: {
            char *c_directory = dc_utils_get_attribute_string(xml_node, "Directory");
            if (c_directory) {
                node_index = _process_node_children(xml_node, parent_node_index, parent_elem_type, dc_app_dereference_constants(c_directory));
                free(c_directory);
            } else {
                // should never get here
                throw std::runtime_error("Invalid condition; <Include> node with no directory");
            }
            break;
        }

        case DC_APP_ELEM_TYPE_LOGIC: {
            if (dc_app_data.logic.library) {
                throw std::runtime_error("Duplicate <Logic> definitions");
            }
            char *c_file_path = dc_utils_get_attribute_string(xml_node, "File");
            if (c_file_path) {
                std::string filePath = dc_utils_filepath_to_canonical(dc_app_dereference_constants(c_file_path), directory);
                free(c_file_path);

                // verify filepath
                if (filePath.empty()) {
                    throw std::runtime_error("Invalid logic file of empty filename");
                }

                // open .so file
                const plLibraryDesc logic_so_desc = {
                    .tFlags = PL_LIBRARY_FLAGS_NONE,
                    .pcName = filePath.c_str(),
                };
                if (gpt_library->load(logic_so_desc, &dc_app_data.logic.library) != PL_LIBRARY_RESULT_SUCCESS) {
                    throw std::runtime_error("Failed to load logic .so file");
                }
            } else {
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

        case DC_APP_ELEM_TYPE_PANEL: {
            DcAppNode dc_node = {
                .type = DC_APP_NODE_TYPE_PANEL,
            };

            // parent dimensions
            // TODO probably don't need this.....must be a better way to architect
            dc_node.panel.parent_dimensions = dc_app_index_to_node(parent_node_index)->window.virtual_dimensions;

            // virtual x dimension
            char *c_x_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualWidth");
            if (c_x_virtual_dimension) {
                dc_node.panel.virtual_dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_virtual_dimension));
                free(c_x_virtual_dimension);
            } else {
                dc_node.panel.virtual_dimensions.x = dc_node.panel.parent_dimensions.x;
            }

            // virtual y dimension
            char *c_y_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualHeight");
            if (c_y_virtual_dimension) {
                dc_node.panel.virtual_dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_virtual_dimension));
                free(c_y_virtual_dimension);
            } else {
                dc_node.panel.virtual_dimensions.y = dc_node.panel.parent_dimensions.y;
            }

            // register node
            node_index = dc_app_register_node(dc_node);

            // process children
            DcAppNodeIndex first_child_index = _process_node_children(xml_node, node_index, elem_type, directory);

            // update child index
            DcAppNode *node   = dc_app_index_to_node(node_index);
            node->panel.child = first_child_index;

            break;
        }

        case DC_APP_ELEM_TYPE_POLYGON: {
            DcAppNode dc_node = {
                .type    = DC_APP_NODE_TYPE_POLYGON,
                .polygon = (DcAppNodePolygon){
                    .fill_enabled = false,
                    .line_enabled = false,
                }};

            // fill color
            char *c_fill_color = dc_utils_get_attribute_string(xml_node, "FillColor");
            if (c_fill_color) {
                // split string by whitespace
                std::string              s_fill_color = dc_app_dereference_constants(c_fill_color);
                std::vector<std::string> substrings   = dc_utils_split_string_by_delimiters(s_fill_color, dc_utils_string_whitespace);

                if (substrings.size() > 0) {
                    dc_node.polygon.fill_color.r = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[0]);
                    if (substrings.size() > 1) {
                        dc_node.polygon.fill_color.g = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[1]);
                        if (substrings.size() > 2) {
                            dc_node.polygon.fill_color.b = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[2]);
                            if (substrings.size() > 3) {
                                dc_node.polygon.fill_color.a = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[3]);
                            } else {
                                dc_node.polygon.fill_color.a = dc_app_register_value(dc_value_create_value_float(1.0f));
                            }
                        } else {
                            dc_node.polygon.fill_color.b = dc_app_register_value(dc_value_create_value_float(0.0f));
                        }
                    } else {
                        dc_node.polygon.fill_color.g = dc_app_register_value(dc_value_create_value_float(0.0f));
                    }
                } else {
                    dc_node.polygon.fill_color.r = dc_app_register_value(dc_value_create_value_float(0.0f));
                }
                dc_node.polygon.fill_enabled = true;
                free(c_fill_color);
            }

            // line color
            char *c_line_color = dc_utils_get_attribute_string(xml_node, "LineColor");
            if (c_line_color) {
                // split string by whitespace
                std::string              s_fill_color = dc_app_dereference_constants(c_line_color);
                std::vector<std::string> substrings   = dc_utils_split_string_by_delimiters(s_fill_color, dc_utils_string_whitespace);

                if (substrings.size() > 0) {
                    dc_node.polygon.line_color.r = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[0]);
                    if (substrings.size() > 1) {
                        dc_node.polygon.line_color.g = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[1]);
                        if (substrings.size() > 2) {
                            dc_node.polygon.line_color.b = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[2]);
                            if (substrings.size() > 3) {
                                dc_node.polygon.line_color.a = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[3]);
                            } else {
                                dc_node.polygon.line_color.a = dc_app_register_value(dc_value_create_value_float(1.0f));
                            }
                        } else {
                            dc_node.polygon.line_color.b = dc_app_register_value(dc_value_create_value_float(0.0f));
                        }
                    } else {
                        dc_node.polygon.line_color.g = dc_app_register_value(dc_value_create_value_float(0.0f));
                    }
                } else {
                    dc_node.polygon.line_color.r = dc_app_register_value(dc_value_create_value_float(0.0f));
                }
                dc_node.polygon.line_enabled = true;
                free(c_line_color);
            }

            // line width
            char *c_line_width = dc_utils_get_attribute_string(xml_node, "LineWidth");
            if (c_line_width) {
                dc_node.polygon.line_width = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_line_width));
                free(c_line_width);
            } else {
                dc_node.polygon.line_width = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // initialize points to 0
            dc_node.polygon.num_points = 0;
            dc_node.polygon.points     = nullptr;

            // register node
            node_index = dc_app_register_node(dc_node);

            // process children
            _process_node_children(xml_node, node_index, elem_type, directory);
            break;
        }

        case DC_APP_ELEM_TYPE_SET: {

            DcAppNode dc_node = {
                .type = DC_APP_NODE_TYPE_SET};

            // variable
            char *c_variable = dc_utils_get_attribute_string(xml_node, "Variable");
            if (!c_variable) {
                throw std::runtime_error("Missing attribute 'Variable' in <Set> element");
            }
            dc_node.set.var_index = dc_app_get_var_index(dc_app_dereference_constants(c_variable));
            free(c_variable);

            // operand
            char *c_operand = dc_utils_get_node_content_string(xml_node);
            if (!c_operand) {
                throw std::runtime_error("Missing content (operand) in <Set> element");
            }
            dc_node.set.operand = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_operand));
            free(c_operand);

            // operator
            char *c_operation = dc_utils_get_attribute_string(xml_node, "Operator");
            if (c_operation) {
                dc_node.set.operation = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_operation));
                free(c_operation);
            } else {
                dc_node.set.operation = dc_app_register_value(dc_value_create_value_integer(DC_APP_SET_TYPE_EQUAL));
            }

            // register node
            node_index = dc_app_register_node(dc_node);
            break;
        }

        case DC_APP_ELEM_TYPE_TEXT: {
            DcAppNode dc_node = {
                .type = DC_APP_NODE_TYPE_TEXT,
            };

            // text
            char *c_text = dc_utils_get_node_content_string(xml_node);
            if (!c_text) {
                throw std::runtime_error("Missing content (text) in <Text> element");
            }
            std::string raw_text = dc_app_dereference_constants(c_text);
            free(c_text);

            std::vector<std::string> variables;
            std::vector<std::string> formats;
            std::string              result;
            size_t                   i = 0;
            while (i < raw_text.size()) {
                if (raw_text[i] == '\\') {
                    // Escape character: skip and add next character to result
                    if (i + 1 < raw_text.size()) {
                        result += raw_text[i + 1];
                        i += 2;
                    } else {
                        result += raw_text[i++];
                    }
                    continue;
                }

                if (raw_text[i] == '@') {
                    size_t start = i;
                    ++i;

                    std::string var;
                    bool        is_braced = false;

                    if (i < raw_text.size() && raw_text[i] == '{') {
                        is_braced = true;
                        ++i;
                        size_t end = raw_text.find('}', i);
                        if (end == std::string::npos) {
                            // No closing brace, treat as normal text
                            result += raw_text.substr(start, i - start);
                            continue;
                        }
                        var = raw_text.substr(i, end - i);
                        i   = end + 1;
                    } else {
                        size_t start_var = i;
                        while (i < raw_text.size() && !isspace(static_cast<unsigned char>(raw_text[i])) && raw_text[i] != '(') {
                            ++i;
                        }
                        var = raw_text.substr(start_var, i - start_var);
                    }

                    variables.emplace_back(var);

                    // Check for format specifier
                    std::string format_spec;
                    if (i < raw_text.size() && raw_text[i] == '(') {
                        size_t close = raw_text.find(')', i);
                        if (close != std::string_view::npos && (i == 0 || raw_text[i - 1] != '\\')) {
                            format_spec = raw_text.substr(i + 1, close - i - 1);
                            i           = close + 1;

                            size_t len = 0;
                            if ((len = dc_utils_format_specifier_length_bool(format_spec)) > 0 ||
                                (len = dc_utils_format_specifier_length_int(format_spec)) > 0 ||
                                (len = dc_utils_format_specifier_length_float(format_spec)) > 0 ||
                                (len = dc_utils_format_specifier_length_string(format_spec)) > 0) {
                                formats.emplace_back(format_spec.substr(0, len));
                            } else {
                                formats.emplace_back("%s");
                            }
                        } else {
                            formats.emplace_back("%s");
                        }
                    } else {
                        formats.emplace_back("%s");
                    }

                    continue;
                }

                // Default: append character to result
                result += raw_text[i++];
            }

            // Print final result
            printf("result: %s\n", result.c_str());

            break;
        }

        case DC_APP_ELEM_TYPE_TRICK_FROM: {
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_IO: {
                    _process_node_children(xml_node, DC_APP_NODE_INDEX_UNDEFINED, elem_type, directory);
                    break;
                }
                default:
                    throw std::runtime_error("Invalid elem parent of type " + dc_app_elem_type_to_string(parent_elem_type) + " for FromTrick.");
            }
            break;
        }

        case DC_APP_ELEM_TYPE_TRICK_IO: {

            // host
            char *c_host = dc_utils_get_attribute_string(xml_node, "Host");
            if (!c_host) {
                throw std::runtime_error("Missing attribute 'Host' in <TrickIO> element");
            }
            std::string host = dc_app_dereference_constants(c_host);
            free(c_host);

            // port
            char *c_port = dc_utils_get_attribute_string(xml_node, "Port");
            if (!c_port) {
                throw std::runtime_error("Missing attribute 'Port' in <TrickIO> element");
            }
            int port = dc_utils_string_to_integer(dc_app_dereference_constants(c_port));
            free(c_port);

            // data rate
            char *c_data_rate = dc_utils_get_attribute_string(xml_node, "Type");
            float data_rate   = .1;
            if (c_data_rate) {
                data_rate = dc_utils_string_to_float(dc_app_dereference_constants(c_data_rate));
            }
            free(c_data_rate);

            // create trick instance
            DcAppTrickContext *dc_app_trick = new DcAppTrickContext();
            dc_app_trick->trick             = dc_trick_create(host.c_str(), port, data_rate, 1);
            dc_app_trick->rx_var_contexts.clear();
            dc_app_trick->tx_var_contexts.clear();
            dc_app_data.trick_contexts.push_back(dc_app_trick);

            // process children
            _process_node_children(xml_node, DC_APP_NODE_INDEX_UNDEFINED, elem_type, directory);

            break;
        }

        case DC_APP_ELEM_TYPE_TRICK_TO: {
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_IO: {
                    _process_node_children(xml_node, DC_APP_NODE_INDEX_UNDEFINED, elem_type, directory);
                    break;
                }
                default:
                    throw std::runtime_error("Invalid elem parent of type " + dc_app_elem_type_to_string(parent_elem_type) + " for ToTrick.");
            }
            break;
        }

        case DC_APP_ELEM_TYPE_TRICK_VARIABLE: {

            // check for invalid elem type
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_FROM:
                case DC_APP_ELEM_TYPE_TRICK_TO:
                    break;
                default:
                    throw std::runtime_error("Invalid elem parent of type " + dc_app_elem_type_to_string(parent_elem_type) + " for ToTrick.");
            }

            // trick var path
            char *c_trick_path = dc_utils_get_attribute_string(xml_node, "Name");
            if (!c_trick_path) {
                throw std::runtime_error("Missing attribute 'Name' in <TrickVar> element");
            }
            std::string trick_path = dc_app_dereference_constants(c_trick_path);
            free(c_trick_path);

            // dcapp var
            char *c_dcapp_var = dc_utils_get_node_content_string(xml_node);
            if (!c_dcapp_var) {
                throw std::runtime_error("Missing attribute 'Name' in <TrickVar> element");
            }
            std::string dcapp_var = dc_app_dereference_constants(c_dcapp_var);
            free(c_dcapp_var);

            // units (know this is convoluted)
            char       *c_units = dc_utils_get_attribute_string(xml_node, "Units");
            std::string units_string;
            if (c_units) {
                units_string = dc_app_dereference_constants(c_units);
                free(c_units);
                c_units = (char *)units_string.c_str();
            }

            // get current trick context
            DcAppTrickContext *dc_app_trick = dc_app_data.trick_contexts.back();

            // handle depending on parent
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_FROM: {

                    // create + add rx var
                    DcAppTrickRxVarContext var;
                    var.trick_var_index = dc_trick_add_rx_var(dc_app_trick->trick, trick_path.c_str(), c_units);
                    var.dcapp_var_index = dc_app_get_var_index(dcapp_var);
                    dc_app_trick->rx_var_contexts.push_back(var);
                    break;
                }
                case DC_APP_ELEM_TYPE_TRICK_TO: {

                    // create + add tx var
                    DcAppTrickTxVarContext var;
                    DcValue               *dc_value = dc_app_get_value(dc_app_data.vars[var.dcapp_var_index].value_index);

                    var.dcapp_var_index = dc_app_get_var_index(dcapp_var);
                    var.trick_var_index = dc_trick_add_tx_var(dc_app_trick->trick, trick_path.c_str(), c_units, dc_value->type == DC_VALUE_TYPE_STRING);
                    dc_value_copy(&var.prev_value, dc_value);
                    dc_app_trick->tx_var_contexts.push_back(var);
                    break;
                }
                default:
                    // should never reach here
                    break;
            }
            break;
        }

        case DC_APP_ELEM_TYPE_TRUE:
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_IF: {

                    // process children
                    DcAppNodeIndex first_child_index = _process_node_children(xml_node, parent_node_index, elem_type, directory);

                    // update child true node
                    DcAppNode *parent_node              = dc_app_index_to_node(parent_node_index);
                    parent_node->conditional.child_true = first_child_index;
                    break;
                }
                default:
                    throw std::runtime_error("Invalid elem parent of type " + dc_app_elem_type_to_string(parent_elem_type) + " for <True>.");
            }
            break;

        case DC_APP_ELEM_TYPE_VARIABLE: {
            // name
            char *c_name = dc_utils_get_node_content_string(xml_node);
            if (!c_name) {
                throw std::runtime_error("Non-existent node content in <Variable> definition");
            }
            std::string name = dc_app_dereference_constants(c_name);
            free(c_name);

            // type
            char       *c_type = dc_utils_get_attribute_string(xml_node, "Type");
            DcValueType type   = DC_VALUE_TYPE_STRING;
            if (c_type) {
                type = dc_value_type_from_string(dc_app_dereference_constants(c_type));
            }

            // value
            char   *c_initial_value = dc_utils_get_attribute_string(xml_node, "InitialValue");
            DcValue initial_value;
            if (c_initial_value) {
                initial_value = dc_value_create_value_string(dc_app_dereference_constants(c_initial_value));
            } else {
                initial_value = dc_value_create_value_string("");
            }
            initial_value.type       = type;
            initial_value.is_dynamic = true;

            // register var
            DcAppValueIndex index = dc_app_register_value(initial_value);
            dc_app_register_var(name, index);

            break;
        }

        case DC_APP_ELEM_TYPE_VERTEX: {
            DcAppNode *parent_node = dc_app_index_to_node(parent_node_index);
            switch (parent_node->type) {
                case DC_APP_NODE_TYPE_POLYGON: {
                    // xPosition
                    char *c_x_position = dc_utils_get_attribute_string(xml_node, "X");
                    if (!c_x_position) {
                        throw std::runtime_error("Invalid Vertex: No X attribute");
                    }

                    // yPosition
                    char *c_y_position = dc_utils_get_attribute_string(xml_node, "Y");
                    if (!c_y_position) {
                        throw std::runtime_error("Invalid Vertex: No Y attribute");
                    }

                    // reallocate and add vertices
                    parent_node->polygon.num_points++;
                    parent_node->polygon.points                                      = (DcAppValueIndex2 *)realloc(parent_node->polygon.points, parent_node->polygon.num_points * sizeof(DcAppValueIndex2));
                    parent_node->polygon.points[parent_node->polygon.num_points - 1] = (DcAppValueIndex2){
                        dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_position)),
                        dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_position))};
                    break;
                }
                default:
                    throw std::runtime_error("Invalid parent of type " + dc_app_node_type_to_string(parent_node->type) + "for vertex.");
            }
            break;
        }

        case DC_APP_ELEM_TYPE_WINDOW: {
            DcAppNode dc_node = {
                .type = DC_APP_NODE_TYPE_WINDOW,
            };

            // title
            std::string title   = "dcapp";
            char       *c_title = dc_utils_get_attribute_string(xml_node, "Title");
            if (c_title) {
                title = dc_app_dereference_constants(c_title);
                free(c_title);
            }
            dc_node.window.title = (char *)malloc(title.length() + 1);
            memcpy(dc_node.window.title, title.c_str(), title.length() + 1);

            // xPosition
            char *c_x_position = dc_utils_get_attribute_string(xml_node, "X");
            if (c_x_position) {
                dc_node.window.position.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_x_position));
                free(c_x_position);
            } else {
                dc_node.window.position.x = dc_app_register_value(dc_value_create_value_integer(0.0f));
            }

            // y position
            char *c_y_position = dc_utils_get_attribute_string(xml_node, "Y");
            if (c_y_position) {
                dc_node.window.position.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_y_position));
                free(c_y_position);
            } else {
                dc_node.window.position.y = dc_app_register_value(dc_value_create_value_integer(0.0f));
            }

            // x dimension
            char *c_x_dimension = dc_utils_get_attribute_string(xml_node, "Width");
            if (c_x_dimension) {
                dc_node.window.dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_x_dimension));
                free(c_x_dimension);
            } else {
                dc_node.window.dimensions.x = dc_app_register_value(dc_value_create_value_integer(0.0f));
            }

            // y dimension
            char *c_y_dimension = dc_utils_get_attribute_string(xml_node, "Height");
            if (c_y_dimension) {
                dc_node.window.dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_y_dimension));
                free(c_y_dimension);
            } else {
                dc_node.window.dimensions.y = dc_app_register_value(dc_value_create_value_integer(0.0f));
            }

            // virtual x dimension
            char *c_x_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualWidth");
            if (c_x_virtual_dimension) {
                dc_node.window.virtual_dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_virtual_dimension));
                free(c_x_virtual_dimension);
            } else {
                dc_node.window.virtual_dimensions.x = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // virtual y dimension
            char *c_y_virtual_dimension = dc_utils_get_attribute_string(xml_node, "VirtualHeight");
            if (c_y_virtual_dimension) {
                dc_node.window.virtual_dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_virtual_dimension));
                free(c_y_virtual_dimension);
            } else {
                dc_node.window.virtual_dimensions.y = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // register node
            node_index = dc_app_register_node(dc_node);

            // process children
            DcAppNodeIndex first_child_index = _process_node_children(xml_node, node_index, elem_type, directory);

            // update child index
            DcAppNode *node    = dc_app_index_to_node(node_index);
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

void _draw_node_list(pl_app_data *pt_app_data, DcAppNodeIndex node_index, plMat4 *node_transform) {
    DcAppNodeIndex current_node_index = node_index;
    while (current_node_index != DC_APP_NODE_INDEX_UNDEFINED) {
        _draw_node(pt_app_data, current_node_index, node_transform);
        current_node_index = dc_app_index_to_node(current_node_index)->next;
    }
}

void _draw_node(pl_app_data *pt_app_data, DcAppNodeIndex node_index, plMat4 *parent_transform) {
    if (node_index == DC_APP_NODE_INDEX_UNDEFINED) {
        throw std::runtime_error("Attempting to draw undefined node index");
    }

    DcAppNode *node = dc_app_index_to_node(node_index);
    switch (node->type) {
        case DC_APP_NODE_TYPE_CONTAINER: {
            float x_position = 0;
            switch (dc_app_get_value(node->container.alignment.x)->value_integer) {
                case DC_APP_ALIGN_TYPE_LEFT:
                    x_position = dc_app_get_value(node->container.position.x)->value_float;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    x_position = dc_app_get_value(node->container.position.x)->value_float - dc_app_get_value(node->container.dimensions.x)->value_float / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    x_position = dc_app_get_value(node->container.position.x)->value_float - dc_app_get_value(node->container.dimensions.x)->value_float;
                    break;
            }

            float y_position = 0;
            switch (dc_app_get_value(node->container.alignment.y)->value_integer) {
                case DC_APP_ALIGN_TYPE_LEFT:
                    y_position = dc_app_get_value(node->container.position.y)->value_float;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    y_position = dc_app_get_value(node->container.position.y)->value_float - dc_app_get_value(node->container.dimensions.y)->value_float / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    y_position = dc_app_get_value(node->container.position.y)->value_float - dc_app_get_value(node->container.dimensions.y)->value_float;
                    break;
            }

            plMat4 trans_origin_matrix = pl_mat4_translate_xyz(
                dc_app_get_value(node->container.origin.x)->value_float,
                dc_app_get_value(node->container.origin.y)->value_float,
                0.0f);
            plMat4 rotate_matrix = pl_mat4_rotate_vec3(
                pl_radiansf(dc_app_get_value(node->container.rotation)->value_float),
                (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_position_matrix = pl_mat4_translate_xyz(
                x_position,
                y_position,
                0.0f);
            plMat4 scale_matrix = pl_mat4_scale_xyz(
                dc_app_get_value(node->container.dimensions.x)->value_float / dc_app_get_value(node->container.virtual_dimensions.x)->value_float,
                dc_app_get_value(node->container.dimensions.y)->value_float / dc_app_get_value(node->container.virtual_dimensions.y)->value_float,
                1.0f);

            plMat4 transform = (plMat4){0};
            transform        = pl_mul_mat4t(parent_transform, &trans_origin_matrix);
            transform        = pl_mul_mat4t(&transform, &rotate_matrix);
            transform        = pl_mul_mat4t(&transform, &trans_position_matrix);
            transform        = pl_mul_mat4t(&transform, &scale_matrix);
            _draw_node_list(pt_app_data, node->container.child, &transform);
            break;
        }

        case DC_APP_NODE_TYPE_CONDITIONAL: {
            DcValue             *val1 = dc_app_get_value(node->conditional.value1);
            DcValue             *val2 = dc_app_get_value(node->conditional.value2);
            DcAppConditionalType type = (DcAppConditionalType)dc_app_get_value(node->conditional.type)->value_integer;

            // evaluate
            bool result;
            switch (type) {
                case DC_APP_CONDITIONAL_TYPE_TRUE:
                    result = val1->value_boolean;
                    break;
                case DC_APP_CONDITIONAL_TYPE_FALSE:
                    result = !(val1->value_boolean);
                    break;
                case DC_APP_CONDITIONAL_TYPE_EQ:
                    result = dc_value_is_equal(val1, val2);
                    break;
                case DC_APP_CONDITIONAL_TYPE_NE:
                    result = dc_value_is_not_equal(val1, val2);
                    break;
                case DC_APP_CONDITIONAL_TYPE_LT:
                    result = dc_value_is_less(val1, val2);
                    break;
                case DC_APP_CONDITIONAL_TYPE_GT:
                    result = dc_value_is_greater(val1, val2);
                    break;
                case DC_APP_CONDITIONAL_TYPE_LTE:
                    result = dc_value_is_less_or_equal(val1, val2);
                    break;
                case DC_APP_CONDITIONAL_TYPE_GTE:
                    result = dc_value_is_greater_or_equal(val1, val2);
                default:
                    throw std::runtime_error("Unknown conditional_type on evaluation: " + std::to_string(type));
                    break;
            }

            // process children
            if (result) {
                _draw_node_list(pt_app_data, node->conditional.child_true, parent_transform);
            } else {
                _draw_node_list(pt_app_data, node->conditional.child_false, parent_transform);
            }
            break;
        }

        case DC_APP_NODE_TYPE_MAP: {
            // TODO implement
            break;
        }

        case DC_APP_NODE_TYPE_PANEL: {
            plMat4 scale_matrix = pl_mat4_scale_xyz(
                dc_app_get_value(node->panel.parent_dimensions.x)->value_float / dc_app_get_value(node->panel.virtual_dimensions.x)->value_float,
                dc_app_get_value(node->panel.parent_dimensions.y)->value_float / dc_app_get_value(node->panel.virtual_dimensions.y)->value_float,
                1.0f);
            plMat4 transform = (plMat4){0};
            transform        = pl_mul_mat4t(parent_transform, &scale_matrix);
            _draw_node_list(pt_app_data, node->panel.child, &transform);
            break;
        }

        case DC_APP_NODE_TYPE_POLYGON: {
            // get points
            std::vector<plVec2> points;
            points.resize(node->polygon.num_points);
            for (int ii = 0; ii < node->polygon.num_points; ii++) {
                plVec4 point4 = (plVec4){
                    dc_app_get_value(node->polygon.points[ii].x)->value_float,
                    dc_app_get_value(node->polygon.points[ii].y)->value_float,
                    0, 1};
                point4     = pl_mul_mat4_vec4(parent_transform, point4);
                points[ii] = (plVec2){point4.x, point4.y};
            }

            // draw fill
            if (node->polygon.fill_enabled) {
                uint32_t fill_color = PL_COLOR_32_RGBA(
                    dc_app_get_value(node->polygon.fill_color.r)->value_float,
                    dc_app_get_value(node->polygon.fill_color.g)->value_float,
                    dc_app_get_value(node->polygon.fill_color.b)->value_float,
                    dc_app_get_value(node->polygon.fill_color.a)->value_float);
                gpt_draw->add_convex_polygon_filled(pt_app_data->pt_fg_layer, points.data(), points.size(), (plDrawSolidOptions){.uColor = fill_color});
            }

            // draw outline
            if (node->polygon.line_enabled) {
                float    lineThickness = dc_app_get_value(node->polygon.line_width)->value_float;
                uint32_t line_color    = PL_COLOR_32_RGBA(
                    dc_app_get_value(node->polygon.line_color.r)->value_float,
                    dc_app_get_value(node->polygon.line_color.g)->value_float,
                    dc_app_get_value(node->polygon.line_color.b)->value_float,
                    dc_app_get_value(node->polygon.line_color.a)->value_float);
                gpt_draw->add_polygon(pt_app_data->pt_fg_layer, points.data(), points.size(), (plDrawLineOptions){.uColor = line_color, .fThickness = lineThickness});
            }

            break;
        }

        case DC_APP_NODE_TYPE_SET: {

            DcAppVar    *var       = &dc_app_data.vars[node->set.var_index];
            DcValue     *var_value = dc_app_get_value(var->value_index);
            DcValue     *op_value  = dc_app_get_value(node->set.operand);
            DcAppSetType operation = (DcAppSetType)(dc_app_get_value(node->set.operation)->value_integer);

            // apply operation
            switch (operation) {
                case DC_APP_SET_TYPE_EQUAL:
                    switch (var_value->type) {
                        case DC_VALUE_TYPE_STRING:
                            var_value->value_string = op_value->value_string;
                            break;
                        case DC_VALUE_TYPE_INTEGER:
                            var_value->value_integer = op_value->value_integer;
                            break;
                        case DC_VALUE_TYPE_FLOAT:
                            var_value->value_float = op_value->value_float;
                            break;
                        case DC_VALUE_TYPE_BOOLEAN:
                            var_value->value_boolean = op_value->value_boolean;
                            break;
                        default:
                            break;
                    }
                    break;

                case DC_APP_SET_TYPE_ADD:
                    switch (var_value->type) {
                        case DC_VALUE_TYPE_STRING:
                            var_value->value_string += op_value->value_string;
                            break;
                        case DC_VALUE_TYPE_INTEGER:
                            var_value->value_integer += op_value->value_integer;
                            break;
                        case DC_VALUE_TYPE_FLOAT:
                            var_value->value_float += op_value->value_float;
                            break;
                        case DC_VALUE_TYPE_BOOLEAN:
                            var_value->value_boolean += op_value->value_boolean;
                            break;
                        default:
                            break;
                    }
                    break;
                case DC_APP_SET_TYPE_SUBTRACT:
                    switch (var_value->type) {
                        case DC_VALUE_TYPE_STRING:
                            break;
                        case DC_VALUE_TYPE_INTEGER:
                            var_value->value_integer -= op_value->value_integer;
                            break;
                        case DC_VALUE_TYPE_FLOAT:
                            var_value->value_float -= op_value->value_float;
                            break;
                        case DC_VALUE_TYPE_BOOLEAN:
                            var_value->value_boolean -= op_value->value_boolean;
                            break;
                        default:
                            break;
                    }
                    break;
                case DC_APP_SET_TYPE_MULTIPLY:
                    switch (var_value->type) {
                        case DC_VALUE_TYPE_STRING:
                            break;
                        case DC_VALUE_TYPE_INTEGER:
                            var_value->value_integer *= op_value->value_integer;
                            break;
                        case DC_VALUE_TYPE_FLOAT:
                            var_value->value_float *= op_value->value_float;
                            break;
                        case DC_VALUE_TYPE_BOOLEAN:
                            var_value->value_boolean *= op_value->value_boolean;
                            break;
                        default:
                            break;
                    }
                    break;
                case DC_APP_SET_TYPE_DIVIDE:
                    switch (var_value->type) {
                        case DC_VALUE_TYPE_STRING:
                            break;
                        case DC_VALUE_TYPE_INTEGER:
                            var_value->value_integer /= op_value->value_integer;
                            break;
                        case DC_VALUE_TYPE_FLOAT:
                            var_value->value_float /= op_value->value_float;
                            break;
                        case DC_VALUE_TYPE_BOOLEAN:
                            var_value->value_boolean /= op_value->value_boolean;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    throw std::runtime_error("Invalid <Set> Operator value of enum " + std::to_string(operation));
                    break;
            }

            // refresh variable
            dc_value_refresh(var_value);
            dc_app_refresh_var_from_value(var);
            break;
        }

        case DC_APP_NODE_TYPE_WINDOW: {
            // TODO move this code to only the resize() function
            // update dimensions
            uint32_t dimensionX, dimensionY;

            // TODO fix this in pilotlight for macos
            gpt_windows->get_size(pt_app_data->pt_window, &dimensionX, &dimensionY);
            DcValue *dimension_value_x       = dc_app_get_value(node->window.dimensions.x);
            DcValue *dimension_value_y       = dc_app_get_value(node->window.dimensions.y);
            dimension_value_x->value_integer = (int)dimensionX;
            dimension_value_y->value_integer = (int)dimensionY;
            dc_value_refresh(dimension_value_x);
            dc_value_refresh(dimension_value_y);

            // compute transforms
            // translate from negative to positive range
            plMat4 trans_matrix = pl_mat4_translate_xyz(
                0.0f,
                dc_app_get_value(node->window.dimensions.y)->value_float,
                0.0f);

            // scale from virtual to real dimensions, flip y axis
            plMat4 scale_matrix = pl_mat4_scale_xyz(
                dc_app_get_value(node->window.dimensions.x)->value_float / dc_app_get_value(node->window.virtual_dimensions.x)->value_float,
                dc_app_get_value(node->window.dimensions.y)->value_float / dc_app_get_value(node->window.virtual_dimensions.y)->value_float * -1.0f,
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
