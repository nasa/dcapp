
//-----------------------------------------------------------------------------
// [SECTION] dcapp includes
//-----------------------------------------------------------------------------

#include "../src/app.hpp"
#include "pl_draw_backend_ext.h"
#include "sb.hpp"
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
#include "pl_graphics_ext.h"

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plAppData {
    // window
    plWindow      *window;
    plDrawLayer2D *layer;
    plDrawList2D  *draw_list;

    // font
    plFont *cousine_sdf_font;

    // console variable
    bool show_help_window;
} pl_app_data;

//-----------------------------------------------------------------------------
// [SECTION] dcapp state
//-----------------------------------------------------------------------------

static DcAppNodeIndex _process_node_children(xmlNodePtr xml_node, DcAppNodeIndex node_index, DcAppElemType parent_elem_type, const std::string &directory);
static DcAppNodeIndex _process_node(xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, std::string directory);
static void           _draw_node_list(pl_app_data *app_data, DcAppNodeIndex node_index, plMat4 *node_transform);
static void           _draw_node(pl_app_data *app_data, DcAppNodeIndex node_index, plMat4 *parent_transform);

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

const plWindowI      *ext_windows      = NULL;
const plDrawI        *ext_draw         = NULL;
const plDrawBackendI *ext_draw_backend = NULL;
const plStarterI     *ext_starter      = NULL;
const plProfileI     *ext_profile      = NULL;
const plMemoryI      *ext_memory       = NULL;
const plLibraryI     *ext_library      = NULL;
const plIOI          *ext_ioi          = NULL;
const plGraphicsI    *ext_gfx          = NULL;

#define PL_ALLOC(x) ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_REALLOC(x, y) ext_memory->tracked_realloc((x), (y), __FILE__, __LINE__)
#define PL_FREE(x) ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

//-----------------------------------------------------------------------------
// [SECTION] pl_app_load
//-----------------------------------------------------------------------------s

PL_EXPORT void *
pl_app_load(plApiRegistryI *api_registry, pl_app_data *app_data) {
    // NOTE: on first load, "pAppData" will be NULL but on reloads
    //       it will be the value returned from this function

    // if "ptAppData" is a valid pointer, then this function is being called
    // during a hot reload.
    if (app_data) {
        // re-retrieve the apis since we are now in
        // a different dll/so
        ext_windows      = pl_get_api_latest(api_registry, plWindowI);
        ext_draw         = pl_get_api_latest(api_registry, plDrawI);
        ext_draw_backend = pl_get_api_latest(api_registry, plDrawBackendI);
        ext_starter      = pl_get_api_latest(api_registry, plStarterI);
        ext_profile      = pl_get_api_latest(api_registry, plProfileI);
        ext_memory       = pl_get_api_latest(api_registry, plMemoryI);
        ext_library      = pl_get_api_latest(api_registry, plLibraryI);
        ext_ioi          = pl_get_api_latest(api_registry, plIOI);
        ext_gfx          = pl_get_api_latest(api_registry, plGraphicsI);

        return app_data;
    }

    // retrieve extension registry
    const plExtensionRegistryI *extension_registry = pl_get_api_latest(api_registry, plExtensionRegistryI);

    // load extensions
    //   * first argument is the shared library name WITHOUT the extension
    //   * second & third argument is the load/unload functions names (use NULL for the default of "pl_load_ext" &
    //     "pl_unload_ext")
    //   * fourth argument indicates if the extension is reloadable (should we check for changes and reload if changed)
    extension_registry->load("pl_unity_ext", NULL, NULL, true);
    extension_registry->load("pl_platform_ext", NULL, NULL, false); // provides the file API used by the drawing ext

    // load required apis
    ext_windows      = pl_get_api_latest(api_registry, plWindowI);
    ext_draw         = pl_get_api_latest(api_registry, plDrawI);
    ext_draw_backend = pl_get_api_latest(api_registry, plDrawBackendI);
    ext_starter      = pl_get_api_latest(api_registry, plStarterI);
    ext_profile      = pl_get_api_latest(api_registry, plProfileI);
    ext_memory       = pl_get_api_latest(api_registry, plMemoryI);
    ext_library      = pl_get_api_latest(api_registry, plLibraryI);
    ext_ioi          = pl_get_api_latest(api_registry, plIOI);
    ext_gfx          = pl_get_api_latest(api_registry, plGraphicsI);

    // allocate app memory
    app_data = (pl_app_data *)PL_ALLOC(sizeof(pl_app_data));
    memset(app_data, 0, sizeof(pl_app_data));

    // default values
    app_data->show_help_window = true;

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
    plIO *ext_io = ext_ioi->get_io();
    if (ext_io->iArgc < 4) {
        throw std::runtime_error("Missing dcapp configuration file");
    }
    std::vector<std::string> args(ext_io->apArgv + 3, ext_io->apArgv + ext_io->iArgc);

    // TODO process input arguments (constant setting)
    std::string config_relative_path = args[0];

    // set paths
    std::filesystem::path fs_file_path     = std::filesystem::canonical(config_relative_path);
    std::filesystem::path fs_dir_path      = fs_file_path.parent_path();
    std::string           config_file_path = fs_file_path.string();
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
        dc_app_data.logic.pre_init = (void (*)(void))ext_library->load_function(dc_app_data.logic.library, "DisplayPreInit");
        dc_app_data.logic.init     = (void (*)(void))ext_library->load_function(dc_app_data.logic.library, "DisplayInit");
        dc_app_data.logic.draw     = (void (*)(void))ext_library->load_function(dc_app_data.logic.library, "DisplayDraw");
        dc_app_data.logic.close    = (void (*)(void))ext_library->load_function(dc_app_data.logic.library, "DisplayClose");

        // link variables to extern logic data
        for (auto const &[name, var_index] : dc_app_data.var_indices) {
            DcAppVar *var    = &(dc_app_data.vars[var_index]);
            var->extern_data = ext_library->load_function(dc_app_data.logic.library, name.c_str());

            // set the extern data to the initial value
            dc_app_refresh_var_from_value(var);
        }
    }

    // validate
    // root->validate();

    // set initial window params
    DcAppNode   *window_node = dc_app_index_to_node(dc_app_data.window);
    plWindowDesc window_desc = {
        .pcTitle = window_node->window.title,
        .uWidth  = (uint32_t)(dc_app_get_value(window_node->window.dimensions.x)->value_integer),
        .uHeight = (uint32_t)(dc_app_get_value(window_node->window.dimensions.y)->value_integer),
        .iXPos   = dc_app_get_value(window_node->window.position.x)->value_integer,
        .iYPos   = dc_app_get_value(window_node->window.position.y)->value_integer,
    };

    ext_windows->create(window_desc, &app_data->window);
    ext_windows->show(app_data->window);

    // initialize the starter API (handles alot of boilerplate)
    plStarterInit tStarterInit = {
        .tFlags   = PL_STARTER_FLAGS_ALL_EXTENSIONS & (~PL_STARTER_FLAGS_DRAW_EXT) | PL_STARTER_FLAGS_MSAA,
        .ptWindow = app_data->window};
    ext_starter->initialize(tStarterInit);

    // init draw extension
    ext_draw->initialize(NULL);

    // init draw backend
    plDevice *device = ext_starter->get_device();
    ext_draw_backend->initialize(device);

    // create font atlas
    plFontAtlas *pt_atlas = ext_draw->create_font_atlas();
    ext_draw->set_font_atlas(pt_atlas);

    // typical font range (you can also add individual characters)
    const plFontRange font_range = {
        .iFirstCodePoint = 0x0020,
        .uCharCount      = 0x00FF - 0x0020};

    // adding previous font but as a signed distance field (SDF)
    plFontConfig font_config = {};
    font_config.bSdf           = true; // only works with ttf
    font_config.fSize          = 100.0f;
    font_config.uHOverSampling = 1;
    font_config.uVOverSampling = 1;
    font_config.ucOnEdgeValue  = 180;
    font_config.iSdfPadding    = 1;
    font_config.uRangeCount    = 1;
    font_config.ptRanges       = &font_range;

    app_data->cousine_sdf_font = ext_draw->add_font_from_file_ttf(ext_draw->get_current_font_atlas(), font_config, "../data/pilotlight-assets-master/fonts/Cousine-Regular.ttf");

    // register our app drawlist
    app_data->draw_list = ext_draw->request_2d_drawlist();

    // request layers (allows drawing out of order)
    app_data->layer = ext_draw->request_2d_layer(app_data->draw_list);

    // wraps up
    ext_starter->finalize();

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~font atlas texture~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // draw backend handles creating the font atlas texture and
    // uploading to the GPU but it requires a command buffer (in an non recording state).
    // Later examples will go into command buffers without using the starter ext

    plCommandBuffer *cmd_buffer = ext_starter->get_raw_command_buffer(); // not recording

    // actually record, submit, & wait
    ext_draw_backend->build_font_atlas(cmd_buffer, ext_draw->get_current_font_atlas());

    // return back to the pool
    ext_starter->return_raw_command_buffer(cmd_buffer);

    // return app memory
    return app_data;
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_shutdown
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_shutdown(pl_app_data *app_data) {

    // ensure device is done with resources
    plDevice *device = ext_starter->get_device();
    ext_gfx->flush_device(device); // waits for the GPU to be done with all work

    // cleans up texture and other resources
    ext_draw_backend->cleanup_font_atlas(ext_draw->get_current_font_atlas());

    ext_starter->cleanup();
    ext_windows->destroy(app_data->window);
    PL_FREE(app_data);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_resize
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_resize(pl_app_data *app_data) {
    ext_starter->resize();
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_update(pl_app_data *app_data) {
    // this needs to be the first call when using the starter
    // extension. You must return if it returns false (usually a swapchain recreation).
    if (!ext_starter->begin_frame())
        return;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~drawing & profile API~~~~~~~~~~~~~~~~~~~~~~~~~~~

    ext_draw_backend->new_frame();

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

    _draw_node(app_data, dc_app_data.window, nullptr);

    // submit draw layer
    ext_draw->submit_2d_layer(app_data->layer);

    // start main pass & return the encoder being used
    plRenderEncoder *encoder = ext_starter->begin_main_pass();

    // submit our drawlist
    plIO *ptIO = ext_ioi->get_io();
    ext_draw_backend->submit_2d_drawlist(app_data->draw_list, encoder, ptIO->tMainViewportSize.x, ptIO->tMainViewportSize.y, ext_gfx->get_swapchain_info(ext_starter->get_swapchain()).tSampleCount);

    // allows the starter extension to handle some things then ends the main pass
    ext_starter->end_main_pass();

    // must be the last function called when using the starter extension
    ext_starter->end_frame();
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

                // set child's next (empty for now)
                child_node->next = DC_APP_NODE_INDEX_UNDEFINED;

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
            char *c_rotation = dc_utils_get_attribute_string(xml_node, "Rotation");
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
                if (ext_library->load(logic_so_desc, &dc_app_data.logic.library) != PL_LIBRARY_RESULT_SUCCESS) {
                    throw std::runtime_error("Failed to load logic .so file");
                }
            } else {
                throw std::runtime_error("Invalid condition; <Logic> node with no file");
            }
            break;
        }

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

        case DC_APP_ELEM_TYPE_TERRAIN: {
            DcAppNode dc_node = {};
            dc_node.type      = DC_APP_NODE_TYPE_TERRAIN;

            // xPosition
            char *c_x_position = dc_utils_get_attribute_string(xml_node, "X");
            if (c_x_position) {
                dc_node.terrain.position.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_position));
                free(c_x_position);
            } else {
                dc_node.terrain.position.x = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // y position
            char *c_y_position = dc_utils_get_attribute_string(xml_node, "Y");
            if (c_y_position) {
                dc_node.terrain.position.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_position));
                free(c_y_position);
            } else {
                dc_node.terrain.position.y = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // x origin
            char *c_x_origin = dc_utils_get_attribute_string(xml_node, "OriginX");
            if (c_x_origin) {
                dc_node.terrain.origin.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_origin));
                free(c_x_origin);
            } else {
                dc_node.terrain.origin.x = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // y origin
            char *c_y_origin = dc_utils_get_attribute_string(xml_node, "OriginY");
            if (c_y_origin) {
                dc_node.terrain.origin.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_origin));
                free(c_y_origin);
            } else {
                dc_node.terrain.origin.y = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // x align
            char *c_x_align = dc_utils_get_attribute_string(xml_node, "HorizontalAlign");
            if (c_x_align) {
                dc_node.terrain.alignment.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_x_align));
                free(c_x_align);
            } else {
                dc_node.terrain.alignment.x = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_LEFT));
            }

            // y align
            char *c_y_align = dc_utils_get_attribute_string(xml_node, "VerticalAlign");
            if (c_y_align) {
                dc_node.terrain.alignment.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_y_align));
                free(c_y_align);
            } else {
                dc_node.terrain.alignment.y = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_BOTTOM));
            }

            // rotation
            char *c_rotation = dc_utils_get_attribute_string(xml_node, "Rotation");
            if (c_rotation) {
                dc_node.terrain.rotation = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_rotation));
                free(c_rotation);
            } else {
                dc_node.terrain.rotation = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // pivots
            char *c_pivot_point_x = dc_utils_get_attribute_string(xml_node, "PivotPointX");
            char *c_pivot_point_y = dc_utils_get_attribute_string(xml_node, "PivotPointY");
            if (c_pivot_point_x && c_pivot_point_y) {
                dc_node.terrain.pivot_point.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_pivot_point_x));
                dc_node.terrain.pivot_align.x = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_UNDEFINED));
                free(c_pivot_point_x);

                dc_node.terrain.pivot_point.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_pivot_point_y));
                dc_node.terrain.pivot_align.y = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_UNDEFINED));
                free(c_pivot_point_y);
            } else if (!c_pivot_point_x && !c_pivot_point_y) {
                char *c_pivot_align_x = dc_utils_get_attribute_string(xml_node, "PivotAlignX");
                if (c_pivot_align_x) {
                    dc_node.terrain.pivot_align.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_pivot_align_x));
                    free(c_pivot_align_x);
                } else {
                    dc_node.terrain.pivot_align.x = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_CENTER));
                }

                char *c_pivot_align_y = dc_utils_get_attribute_string(xml_node, "PivotAlignY");
                if (c_pivot_align_y) {
                    dc_node.terrain.pivot_align.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_pivot_align_y));
                    free(c_pivot_align_y);
                } else {
                    dc_node.terrain.pivot_align.y = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_MIDDLE));
                }
            } else {
                throw std::runtime_error("Invalid Pivot parameters: must use both PivotPoint params, or none. Using just one is not allowed.");
            }

            // x dimension
            char *c_x_dimension = dc_utils_get_attribute_string(xml_node, "Width");
            if (c_x_dimension) {
                dc_node.terrain.dimensions.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_dimension));
                free(c_x_dimension);
            } else {
                dc_node.terrain.dimensions.x = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // y dimension
            char *c_y_dimension = dc_utils_get_attribute_string(xml_node, "Height");
            if (c_y_dimension) {
                dc_node.terrain.dimensions.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_dimension));
                free(c_y_dimension);
            } else {
                dc_node.terrain.dimensions.y = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // lat
            char *c_lat = dc_utils_get_attribute_string(xml_node, "Latitude");
            if (c_lat) {
                dc_node.terrain.lle.lat = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_lat));
                free(c_lat);
            } else {
                throw std::runtime_error("Must supply attribute Latitude with Terrain primitive");
            }

            // lon
            char *c_lon = dc_utils_get_attribute_string(xml_node, "Longitude");
            if (c_lon) {
                dc_node.terrain.lle.lon = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_lon));
                free(c_lon);
            } else {
                throw std::runtime_error("Must supply attribute Longitude with Terrain primitive");
            }

            // ele
            char *c_ele = dc_utils_get_attribute_string(xml_node, "Elevation");
            if (c_ele) {
                dc_node.terrain.lle.ele = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_ele));
                free(c_ele);
            } else {
                throw std::runtime_error("Must supply attribute Elevation with Terrain primitive");
            }

            // roll
            char *c_roll = dc_utils_get_attribute_string(xml_node, "Roll");
            if (c_roll) {
                dc_node.terrain.rpy.roll = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_roll));
                free(c_roll);
            } else {
                throw std::runtime_error("Must supply attribute Roll with Terrain primitive");
            }

            // pitch
            char *c_pitch = dc_utils_get_attribute_string(xml_node, "Pitch");
            if (c_pitch) {
                dc_node.terrain.rpy.pitch = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_pitch));
                free(c_pitch);
            } else {
                throw std::runtime_error("Must supply attribute Pitch with Terrain primitive");
            }

            // yaw
            char *c_yaw = dc_utils_get_attribute_string(xml_node, "Yaw");
            if (c_yaw) {
                dc_node.terrain.rpy.yaw = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_yaw));
                free(c_yaw);
            } else {
                throw std::runtime_error("Must supply attribute Yaw with Terrain primitive");
            }

            // register node
            node_index = dc_app_register_node(dc_node);

            // process children
            _process_node_children(xml_node, node_index, elem_type, directory);
            break;
        }

        case DC_APP_ELEM_TYPE_TERRAIN_DEM: {

            DcAppNode *parent_node = dc_app_index_to_node(parent_node_index);
            switch (parent_node->type) {
                case DC_APP_NODE_TYPE_TERRAIN: {

                    // file
                    char *c_file = dc_utils_get_attribute_string(xml_node, "File");
                    if (!c_file) {
                        throw std::runtime_error("Invalid TerrainDEM: No File attribute");
                    }
                    std::string file = dc_utils_filepath_to_canonical(dc_app_dereference_constants(c_file), directory);

                    // TODO open file
                    // print for now
                    printf("TerrainDEM file: %s\n", file.c_str());
                    break;
                }
                default:
                    throw std::runtime_error("Invalid parent of type " + dc_app_node_type_to_string(parent_node->type) + "for TerrainDEM.");
            }
            break;
        }

        case DC_APP_ELEM_TYPE_TEXT: {
            DcAppNode dc_node;
            dc_node.type                   = DC_APP_NODE_TYPE_TEXT;
            dc_node.text.sb_vals           = NULL;
            dc_node.text.sb_fillers        = NULL;
            dc_node.text.sb_filler_indices = NULL;
            dc_node.text.sb_formats        = NULL;
            dc_node.text.sb_format_indices = NULL;
            dc_node.text.sb_format_types   = NULL;

            // text
            char *c_text = dc_utils_get_node_content_string(xml_node);
            if (!c_text) {
                throw std::runtime_error("Missing content (text) in <Text> element");
            }
            std::string raw_text = dc_app_dereference_constants(c_text);
            free(c_text);

            char *sb_curr_filler = NULL;
            for (size_t ii = 0; ii < raw_text.size();) {
                if (raw_text[ii] == '\\') {
                    // Escape character: skip and add next character to result
                    if (ii + 1 < raw_text.size()) {
                        sbpush(sb_curr_filler, raw_text[ii + 1]);
                        ii += 2;
                    } else {
                        sbpush(sb_curr_filler, raw_text[ii++]);
                    }
                    continue;
                }

                if (raw_text[ii] == '@') {
                    size_t start = ii;
                    ii++;

                    std::string var;
                    bool        is_braced = false;

                    if (ii < raw_text.size() && raw_text[ii] == '{') {
                        is_braced = true;
                        ii++;
                        size_t end = raw_text.find('}', ii);
                        if (end == std::string::npos) {
                            // No closing brace, treat as normal text
                            std::string substr = raw_text.substr(start, ii - start);
                            sbpushn(sb_curr_filler, substr.c_str(), substr.length());
                            continue;
                        }
                        var = raw_text.substr(ii, end - ii);
                        ii  = end + 1;
                    } else {
                        size_t start_var = ii;
                        while (ii < raw_text.size() &&
                               !isspace(static_cast<unsigned char>(raw_text[ii])) &&
                               raw_text[ii] != '(') {
                            ii++;
                        }
                        var = raw_text.substr(start_var, ii - start_var);
                    }

                    DcAppValueIndex val_index = dc_app_data.vars[dc_app_get_var_index(var)].value_index;
                    sbpush(dc_node.text.sb_vals, val_index);

                    // Check for format specifier
                    std::string format_spec;
                    if (ii < raw_text.size() && raw_text[ii] == '(') {
                        size_t close = raw_text.find(')', ii);
                        if (close != std::string_view::npos && (ii == 0 || raw_text[ii - 1] != '\\')) {
                            format_spec = raw_text.substr(ii + 1, close - ii - 1);
                            ii          = close + 1;

                            // get format + type
                            if (dc_utils_is_format_specifier_int(format_spec)) {
                                sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_INTEGER);
                                sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                sbpushn(dc_node.text.sb_formats, format_spec.c_str(), format_spec.length() + 1);
                            } else if (dc_utils_is_format_specifier_float(format_spec)) {
                                sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_FLOAT);
                                sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                sbpushn(dc_node.text.sb_formats, format_spec.c_str(), format_spec.length() + 1);
                            } else if (dc_utils_is_format_specifier_string(format_spec)) {
                                sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                                sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                sbpushn(dc_node.text.sb_formats, format_spec.c_str(), format_spec.length() + 1);
                            } else if (dc_utils_is_format_specifier_bool(format_spec)) {
                                sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_BOOLEAN);
                                sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                sbpushn(dc_node.text.sb_formats, format_spec.c_str(), format_spec.length() + 1);
                            } else {
                                // unsupported type
                                throw std::runtime_error("Unknown format specifier in Text element: " + format_spec);
                            }
                        } else {
                            sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                            sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                            sbpushn(dc_node.text.sb_formats, "%s", 3);
                        }
                    } else {
                        sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                        sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                        sbpushn(dc_node.text.sb_formats, "%s", 3);
                    }

                    // add the current filler to list of fillers
                    sbpush(dc_node.text.sb_filler_indices, sbcount(dc_node.text.sb_fillers));
                    sbpushn(dc_node.text.sb_fillers, sb_curr_filler, sbcount(sb_curr_filler));
                    sbpush(dc_node.text.sb_fillers, '\0');
                    sbclear(sb_curr_filler);

                    continue;
                }

                // Default: append character to result
                sbpush(sb_curr_filler, raw_text[ii++]);
            }

            // append the remaining filler
            sbpush(dc_node.text.sb_filler_indices, sbcount(dc_node.text.sb_fillers));
            sbpushn(dc_node.text.sb_fillers, sb_curr_filler, sbcount(sb_curr_filler));
            sbpush(dc_node.text.sb_fillers, '\0');

            // clear temp buffer
            sbclear(sb_curr_filler);

            // xPosition
            char *c_x_position = dc_utils_get_attribute_string(xml_node, "X");
            if (c_x_position) {
                dc_node.text.position.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_position));
                free(c_x_position);
            } else {
                dc_node.text.position.x = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // y position
            char *c_y_position = dc_utils_get_attribute_string(xml_node, "Y");
            if (c_y_position) {
                dc_node.text.position.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_position));
                free(c_y_position);
            } else {
                dc_node.text.position.y = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // x origin
            char *c_x_origin = dc_utils_get_attribute_string(xml_node, "OriginX");
            if (c_x_origin) {
                dc_node.text.origin.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_x_origin));
                free(c_x_origin);
            } else {
                dc_node.text.origin.x = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // y origin
            char *c_y_origin = dc_utils_get_attribute_string(xml_node, "OriginY");
            if (c_y_origin) {
                dc_node.text.origin.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_y_origin));
                free(c_y_origin);
            } else {
                dc_node.text.origin.y = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // x align
            char *c_x_align = dc_utils_get_attribute_string(xml_node, "HorizontalAlign");
            if (c_x_align) {
                dc_node.text.alignment.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_x_align));
                free(c_x_align);
            } else {
                dc_node.text.alignment.x = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_LEFT));
            }

            // y align
            char *c_y_align = dc_utils_get_attribute_string(xml_node, "VerticalAlign");
            if (c_y_align) {
                dc_node.text.alignment.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_y_align));
                free(c_y_align);
            } else {
                dc_node.text.alignment.y = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_BOTTOM));
            }

            // rotation
            char *c_rotation = dc_utils_get_attribute_string(xml_node, "Rotation");
            if (c_rotation) {
                dc_node.text.rotation = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_rotation));
                free(c_rotation);
            } else {
                dc_node.text.rotation = dc_app_register_value(dc_value_create_value_float(0.0f));
            }

            // pivots
            char *c_pivot_point_x = dc_utils_get_attribute_string(xml_node, "PivotPointX");
            char *c_pivot_point_y = dc_utils_get_attribute_string(xml_node, "PivotPointY");
            if (c_pivot_point_x && c_pivot_point_y) {
                dc_node.text.pivot_point.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_pivot_point_x));
                dc_node.text.pivot_align.x = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_UNDEFINED));
                free(c_pivot_point_x);

                dc_node.text.pivot_point.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_pivot_point_y));
                dc_node.text.pivot_align.y = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_UNDEFINED));
                free(c_pivot_point_y);
            } else if (!c_pivot_point_x && !c_pivot_point_y) {
                char *c_pivot_align_x = dc_utils_get_attribute_string(xml_node, "PivotAlignX");
                if (c_pivot_align_x) {
                    dc_node.text.pivot_align.x = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_pivot_align_x));
                    free(c_pivot_align_x);
                } else {
                    dc_node.text.pivot_align.x = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_CENTER));
                }

                char *c_pivot_align_y = dc_utils_get_attribute_string(xml_node, "PivotAlignY");
                if (c_pivot_align_y) {
                    dc_node.text.pivot_align.y = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_INTEGER, dc_app_dereference_constants(c_pivot_align_y));
                    free(c_pivot_align_y);
                } else {
                    dc_node.text.pivot_align.y = dc_app_register_value(dc_value_create_value_integer(DC_APP_ALIGN_TYPE_MIDDLE));
                }
            } else {
                throw std::runtime_error("Invalid Pivot parameters: must use both PivotPoint params, or none. Using just one is not allowed.");
            }

            // size
            char *c_size = dc_utils_get_attribute_string(xml_node, "Size");
            if (c_size) {
                dc_node.text.size = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, dc_app_dereference_constants(c_size));
                free(c_size);
            } else {
                throw std::runtime_error("Missing field 'Size' on <Text> element");
            }

            // fill color
            char *c_fill_color = dc_utils_get_attribute_string(xml_node, "FillColor");
            if (c_fill_color) {
                // split string by whitespace
                std::string              s_fill_color = dc_app_dereference_constants(c_fill_color);
                std::vector<std::string> substrings   = dc_utils_split_string_by_delimiters(s_fill_color, dc_utils_string_whitespace);

                if (substrings.size() > 0) {
                    dc_node.text.fill_color.r = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[0]);
                    if (substrings.size() > 1) {
                        dc_node.text.fill_color.g = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[1]);
                        if (substrings.size() > 2) {
                            dc_node.text.fill_color.b = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[2]);
                            if (substrings.size() > 3) {
                                dc_node.text.fill_color.a = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[3]);
                            } else {
                                dc_node.text.fill_color.a = dc_app_register_value(dc_value_create_value_float(1.0f));
                            }
                        } else {
                            dc_node.text.fill_color.b = dc_app_register_value(dc_value_create_value_float(0.0f));
                        }
                    } else {
                        dc_node.text.fill_color.g = dc_app_register_value(dc_value_create_value_float(0.0f));
                    }
                } else {
                    dc_node.text.fill_color.r = dc_app_register_value(dc_value_create_value_float(0.0f));
                }
                dc_node.text.fill_enabled = true;
                free(c_fill_color);
            }

            // line color
            char *c_line_color = dc_utils_get_attribute_string(xml_node, "LineColor");
            if (c_line_color) {
                // split string by whitespace
                std::string              s_fill_color = dc_app_dereference_constants(c_line_color);
                std::vector<std::string> substrings   = dc_utils_split_string_by_delimiters(s_fill_color, dc_utils_string_whitespace);

                if (substrings.size() > 0) {
                    dc_node.text.line_color.r = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[0]);
                    if (substrings.size() > 1) {
                        dc_node.text.line_color.g = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[1]);
                        if (substrings.size() > 2) {
                            dc_node.text.line_color.b = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[2]);
                            if (substrings.size() > 3) {
                                dc_node.text.line_color.a = dc_app_create_and_register_typed_value_from_string(DC_VALUE_TYPE_FLOAT, substrings[3]);
                            } else {
                                dc_node.text.line_color.a = dc_app_register_value(dc_value_create_value_float(1.0f));
                            }
                        } else {
                            dc_node.text.line_color.b = dc_app_register_value(dc_value_create_value_float(0.0f));
                        }
                    } else {
                        dc_node.text.line_color.g = dc_app_register_value(dc_value_create_value_float(0.0f));
                    }
                } else {
                    dc_node.text.line_color.r = dc_app_register_value(dc_value_create_value_float(0.0f));
                }
                dc_node.text.line_enabled = true;
                free(c_line_color);
            }

            // register node
            node_index = dc_app_register_node(dc_node);
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
                    var.dcapp_var_index   = dc_app_get_var_index(dcapp_var);
                    DcValue *dc_var_value = dc_app_get_value(dc_app_data.vars[var.dcapp_var_index].value_index);
                    var.trick_var_index   = dc_trick_add_tx_var(dc_app_trick->trick, trick_path.c_str(), c_units, dc_var_value->type == DC_VALUE_TYPE_STRING);
                    dc_value_copy(&var.prev_value, dc_var_value);
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

void _draw_node_list(pl_app_data *app_data, DcAppNodeIndex node_index, plMat4 *node_transform) {
    DcAppNodeIndex current_node_index = node_index;
    while (current_node_index != DC_APP_NODE_INDEX_UNDEFINED) {
        _draw_node(app_data, current_node_index, node_transform);
        current_node_index = dc_app_index_to_node(current_node_index)->next;
    }
}

void _draw_node(pl_app_data *app_data, DcAppNodeIndex node_index, plMat4 *parent_transform) {
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
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    y_position = dc_app_get_value(node->container.position.y)->value_float;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    y_position = dc_app_get_value(node->container.position.y)->value_float - dc_app_get_value(node->container.dimensions.y)->value_float / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
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
            _draw_node_list(app_data, node->container.child, &transform);
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
                _draw_node_list(app_data, node->conditional.child_true, parent_transform);
            } else {
                _draw_node_list(app_data, node->conditional.child_false, parent_transform);
            }
            break;
        }

        case DC_APP_NODE_TYPE_PANEL: {
            plMat4 scale_matrix = pl_mat4_scale_xyz(
                dc_app_get_value(node->panel.parent_dimensions.x)->value_float / dc_app_get_value(node->panel.virtual_dimensions.x)->value_float,
                dc_app_get_value(node->panel.parent_dimensions.y)->value_float / dc_app_get_value(node->panel.virtual_dimensions.y)->value_float,
                1.0f);
            plMat4 transform = (plMat4){0};
            transform        = pl_mul_mat4t(parent_transform, &scale_matrix);
            _draw_node_list(app_data, node->panel.child, &transform);
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
                ext_draw->add_convex_polygon_filled(app_data->layer, points.data(), points.size(), (plDrawSolidOptions){.uColor = fill_color});
            }

            // draw outline
            if (node->polygon.line_enabled) {
                float    lineThickness = dc_app_get_value(node->polygon.line_width)->value_float;
                uint32_t line_color    = PL_COLOR_32_RGBA(
                    dc_app_get_value(node->polygon.line_color.r)->value_float,
                    dc_app_get_value(node->polygon.line_color.g)->value_float,
                    dc_app_get_value(node->polygon.line_color.b)->value_float,
                    dc_app_get_value(node->polygon.line_color.a)->value_float);
                ext_draw->add_polygon(app_data->layer, points.data(), points.size(), (plDrawLineOptions){.uColor = line_color, .fThickness = lineThickness});
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

        case DC_APP_NODE_TYPE_TERRAIN: {

            // all transform parameters
            float          position[2]    = {dc_app_get_value(node->terrain.position.x)->value_float, dc_app_get_value(node->terrain.position.y)->value_float};
            float          origin[2]      = {dc_app_get_value(node->terrain.origin.x)->value_float, dc_app_get_value(node->terrain.origin.y)->value_float};
            DcAppAlignType alignment[2]   = {(DcAppAlignType)dc_app_get_value(node->terrain.alignment.x)->value_integer, (DcAppAlignType)dc_app_get_value(node->terrain.alignment.y)->value_integer};
            DcAppAlignType pivot_align[2] = {(DcAppAlignType)dc_app_get_value(node->terrain.pivot_align.x)->value_integer, (DcAppAlignType)dc_app_get_value(node->terrain.pivot_align.y)->value_integer};
            float          rotation       = dc_app_get_value(node->terrain.rotation)->value_float;
            float          size[2]        = {dc_app_get_value(node->terrain.dimensions.x)->value_float, dc_app_get_value(node->terrain.dimensions.y)->value_float};

            // local flip over x axis
            plMat4 scale_invert_y_xform = pl_mat4_scale_xyz(
                1.0f,
                -1.0f,
                1.0f);

            // move position
            plMat4 trans_position_xform = pl_mat4_translate_xyz(
                position[0],
                position[1],
                0.0f);

            // move from top-left reference to bottom-left
            plMat4 trans_pl_origin_xform = pl_mat4_translate_xyz(
                0,
                size[1],
                0.0f);

            // move origin
            plMat4 trans_origin_xform = pl_mat4_translate_xyz(
                origin[0],
                origin[1],
                0.0f);

            // move alignment
            float trans_align_vec[2];
            switch (alignment[0]) {
                break;
                case DC_APP_ALIGN_TYPE_LEFT:
                    trans_align_vec[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    trans_align_vec[0] = -1 * size[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    trans_align_vec[0] = -1 * size[0];
                    break;
                default:
                    throw std::runtime_error("Unknown alignment in <Text> draw call: " + std::to_string(alignment[0]));
                    break;
            }
            switch (alignment[1]) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    trans_align_vec[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    trans_align_vec[1] = -1 * size[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    trans_align_vec[1] = -1 * size[1];
                    break;
                default:
                    throw std::runtime_error("Unknown alignment in <Text> draw call: " + std::to_string(alignment[1]));
                    break;
            }
            plMat4 trans_align_xform = pl_mat4_translate_xyz(
                trans_align_vec[0],
                trans_align_vec[1],
                0.0f);

            // move to pivot
            // either both pivot points need to be set, or neither
            bool  use_local_pivot = pivot_align[0] != DC_APP_ALIGN_TYPE_UNDEFINED;
            float trans_pivot_vec[2];
            if (use_local_pivot) {
                switch (pivot_align[0]) {
                    case DC_APP_ALIGN_TYPE_LEFT:
                        trans_pivot_vec[0] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_CENTER:
                        trans_pivot_vec[0] = -1 * size[0] / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_RIGHT:
                        trans_pivot_vec[0] = -1 * size[0];
                        break;
                    default:
                        throw std::runtime_error("Unknown pivot alignment in <Text> draw call: " + std::to_string(pivot_align[0]));
                        break;
                }
                switch (pivot_align[1]) {
                    case DC_APP_ALIGN_TYPE_BOTTOM:
                        trans_pivot_vec[1] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_MIDDLE:
                        trans_pivot_vec[1] = -1 * size[1] / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_TOP:
                        trans_pivot_vec[1] = -1 * size[1];
                        break;
                    default:
                        throw std::runtime_error("Unknown pivot alignment in <Text> draw call: " + std::to_string(pivot_align[1]));
                        break;
                }
            } else {
                float pivot_point_x = dc_app_get_value(node->terrain.pivot_point.x)->value_float;
                trans_pivot_vec[0]  = -1 * pivot_point_x;
                float pivot_point_y = dc_app_get_value(node->terrain.pivot_point.y)->value_float;
                trans_pivot_vec[1]  = -1 * pivot_point_y;
            }
            plMat4 trans_to_pivot_xform = pl_mat4_translate_xyz(
                trans_pivot_vec[0],
                trans_pivot_vec[1],
                0.0f);

            // rotate
            plMat4 rotate_xform = pl_mat4_rotate_vec3(
                pl_radiansf(rotation),
                (plVec3){0.0f, 0.0f, 1.0f});

            // reverse pivot move
            plMat4 trans_from_pivot_xform = pl_mat4_translate_xyz(
                -1 * trans_pivot_vec[0],
                -1 * trans_pivot_vec[1],
                0.0f);

            // compute transform
            plMat4 transform4 = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            if (!use_local_pivot) {
                transform4 = pl_mul_mat4t(&transform4, &trans_from_pivot_xform);
                transform4 = pl_mul_mat4t(&transform4, &rotate_xform);
                transform4 = pl_mul_mat4t(&transform4, &trans_to_pivot_xform);
            }
            transform4 = pl_mul_mat4t(&transform4, &trans_align_xform);
            transform4 = pl_mul_mat4t(&transform4, &trans_position_xform);
            transform4 = pl_mul_mat4t(&transform4, &trans_origin_xform);
            if (use_local_pivot) {
                transform4 = pl_mul_mat4t(&transform4, &trans_from_pivot_xform);
                transform4 = pl_mul_mat4t(&transform4, &rotate_xform);
                transform4 = pl_mul_mat4t(&transform4, &trans_to_pivot_xform);
            }
            transform4 = pl_mul_mat4t(&transform4, &trans_pl_origin_xform);
            transform4 = pl_mul_mat4t(&transform4, &scale_invert_y_xform);
            transform4 = pl_mul_mat4t(parent_transform, &transform4);

            // convert to 3D matrix
            plMat3 transform3 = (plMat3){0};
            transform3.x11    = transform4.x11;
            transform3.x12    = transform4.x12;
            transform3.x13    = transform4.x14;
            transform3.x21    = transform4.x21;
            transform3.x22    = transform4.x22;
            transform3.x23    = transform4.x24;
            transform3.x31    = transform4.x31;
            transform3.x32    = transform4.x32;
            transform3.x33    = transform4.x33;

            // update text options
            // text_options.tTransform = transform3;

            // draw
            // ext_draw->add_text(app_data->layer, (plVec2){0, 0}, sb_text, text_options);
            break;
        }

        case DC_APP_NODE_TYPE_TEXT: {

            // expand text
            static char *sb_text = NULL;
            sbclear(sb_text);
            for (int ii = 0; ii < sbcount(node->text.sb_vals); ii++) {

                // filler
                char *filler = &(node->text.sb_fillers[node->text.sb_filler_indices[ii]]);
                sbpushn(sb_text, filler, strlen(filler));

                // value
                DcValueType format_type = node->text.sb_format_types[ii];
                char       *format      = &(node->text.sb_formats[node->text.sb_format_indices[ii]]);
                DcValue    *val         = dc_app_get_value(node->text.sb_vals[ii]);
                static char val_str[256]; // assume text won't be that long..
                switch (format_type) {
                    case DC_VALUE_TYPE_STRING:
                        snprintf(val_str, sizeof(val_str), format, val->value_string.c_str());
                        break;
                    case DC_VALUE_TYPE_INTEGER:
                        snprintf(val_str, sizeof(val_str), format, val->value_integer);
                        break;
                    case DC_VALUE_TYPE_FLOAT:
                        snprintf(val_str, sizeof(val_str), format, val->value_float);
                        break;
                    case DC_VALUE_TYPE_BOOLEAN:
                        snprintf(val_str, sizeof(val_str), format, val->value_boolean);
                        break;
                    default:
                        throw std::runtime_error("Uknown value type for text: " + std::to_string(format_type));
                }
                sbpushn(sb_text, val_str, strlen(val_str));
            }

            // ending filler
            char *filler = &(node->text.sb_fillers[node->text.sb_filler_indices[sbcount(node->text.sb_vals)]]);
            sbpushn(sb_text, filler, strlen(filler));
            sbpush(sb_text, '\0');

            // get text dimensions
            plDrawTextOptions text_options = {0};
            text_options.ptFont            = app_data->cousine_sdf_font;
            text_options.uColor            = PL_COLOR_32_RGBA(
                dc_app_get_value(node->text.fill_color.r)->value_float,
                dc_app_get_value(node->text.fill_color.g)->value_float,
                dc_app_get_value(node->text.fill_color.b)->value_float,
                dc_app_get_value(node->text.fill_color.a)->value_float);
            text_options.fSize = dc_app_get_value(node->text.size)->value_float;
            plVec2 pl_size     = ext_draw->calculate_text_size(sb_text, text_options);

            // all transform parameters
            float          position[2]    = {dc_app_get_value(node->text.position.x)->value_float, dc_app_get_value(node->text.position.y)->value_float};
            float          origin[2]      = {dc_app_get_value(node->text.origin.x)->value_float, dc_app_get_value(node->text.origin.y)->value_float};
            DcAppAlignType alignment[2]   = {(DcAppAlignType)dc_app_get_value(node->text.alignment.x)->value_integer, (DcAppAlignType)dc_app_get_value(node->text.alignment.y)->value_integer};
            DcAppAlignType pivot_align[2] = {(DcAppAlignType)dc_app_get_value(node->text.pivot_align.x)->value_integer, (DcAppAlignType)dc_app_get_value(node->text.pivot_align.y)->value_integer};
            float          rotation       = dc_app_get_value(node->text.rotation)->value_float;
            float          size[2]        = {pl_size.x, text_options.fSize};

            // local flip over x axis
            plMat4 scale_invert_y_xform = pl_mat4_scale_xyz(
                1.0f,
                -1.0f,
                1.0f);

            // move position
            plMat4 trans_position_xform = pl_mat4_translate_xyz(
                position[0],
                position[1],
                0.0f);

            // move from top-left reference to bottom-left
            plMat4 trans_pl_origin_xform = pl_mat4_translate_xyz(
                0,
                size[1],
                0.0f);

            // move origin
            plMat4 trans_origin_xform = pl_mat4_translate_xyz(
                origin[0],
                origin[1],
                0.0f);

            // move alignment
            float trans_align_vec[2];
            switch (alignment[0]) {
                break;
                case DC_APP_ALIGN_TYPE_LEFT:
                    trans_align_vec[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    trans_align_vec[0] = -1 * size[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    trans_align_vec[0] = -1 * size[0];
                    break;
                default:
                    throw std::runtime_error("Unknown alignment in <Text> draw call: " + std::to_string(alignment[0]));
                    break;
            }
            switch (alignment[1]) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    trans_align_vec[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    trans_align_vec[1] = -1 * size[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    trans_align_vec[1] = -1 * size[1];
                    break;
                default:
                    throw std::runtime_error("Unknown alignment in <Text> draw call: " + std::to_string(alignment[1]));
                    break;
            }
            plMat4 trans_align_xform = pl_mat4_translate_xyz(
                trans_align_vec[0],
                trans_align_vec[1],
                0.0f);

            // move to pivot
            // either both pivot points need to be set, or neither
            bool  use_local_pivot = pivot_align[0] != DC_APP_ALIGN_TYPE_UNDEFINED;
            float trans_pivot_vec[2];
            if (use_local_pivot) {
                switch (pivot_align[0]) {
                    case DC_APP_ALIGN_TYPE_LEFT:
                        trans_pivot_vec[0] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_CENTER:
                        trans_pivot_vec[0] = -1 * size[0] / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_RIGHT:
                        trans_pivot_vec[0] = -1 * size[0];
                        break;
                    default:
                        throw std::runtime_error("Unknown pivot alignment in <Text> draw call: " + std::to_string(pivot_align[0]));
                        break;
                }
                switch (pivot_align[1]) {
                    case DC_APP_ALIGN_TYPE_BOTTOM:
                        trans_pivot_vec[1] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_MIDDLE:
                        trans_pivot_vec[1] = -1 * size[1] / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_TOP:
                        trans_pivot_vec[1] = -1 * size[1];
                        break;
                    default:
                        throw std::runtime_error("Unknown pivot alignment in <Text> draw call: " + std::to_string(pivot_align[1]));
                        break;
                }
            } else {
                float pivot_point_x = dc_app_get_value(node->text.pivot_point.x)->value_float;
                trans_pivot_vec[0]  = -1 * pivot_point_x;
                float pivot_point_y = dc_app_get_value(node->text.pivot_point.y)->value_float;
                trans_pivot_vec[1]  = -1 * pivot_point_y;
            }
            plMat4 trans_to_pivot_xform = pl_mat4_translate_xyz(
                trans_pivot_vec[0],
                trans_pivot_vec[1],
                0.0f);

            // rotate
            plMat4 rotate_xform = pl_mat4_rotate_vec3(
                pl_radiansf(rotation),
                (plVec3){0.0f, 0.0f, 1.0f});

            // reverse pivot move
            plMat4 trans_from_pivot_xform = pl_mat4_translate_xyz(
                -1 * trans_pivot_vec[0],
                -1 * trans_pivot_vec[1],
                0.0f);

            // compute transform
            plMat4 transform4 = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            if (!use_local_pivot) {
                transform4 = pl_mul_mat4t(&transform4, &trans_from_pivot_xform);
                transform4 = pl_mul_mat4t(&transform4, &rotate_xform);
                transform4 = pl_mul_mat4t(&transform4, &trans_to_pivot_xform);
            }
            transform4 = pl_mul_mat4t(&transform4, &trans_align_xform);
            transform4 = pl_mul_mat4t(&transform4, &trans_position_xform);
            transform4 = pl_mul_mat4t(&transform4, &trans_origin_xform);
            if (use_local_pivot) {
                transform4 = pl_mul_mat4t(&transform4, &trans_from_pivot_xform);
                transform4 = pl_mul_mat4t(&transform4, &rotate_xform);
                transform4 = pl_mul_mat4t(&transform4, &trans_to_pivot_xform);
            }
            transform4 = pl_mul_mat4t(&transform4, &trans_pl_origin_xform);
            transform4 = pl_mul_mat4t(&transform4, &scale_invert_y_xform);
            transform4 = pl_mul_mat4t(parent_transform, &transform4);

            // convert to 3D matrix
            plMat3 transform3 = (plMat3){0};
            transform3.x11    = transform4.x11;
            transform3.x12    = transform4.x12;
            transform3.x13    = transform4.x14;
            transform3.x21    = transform4.x21;
            transform3.x22    = transform4.x22;
            transform3.x23    = transform4.x24;
            transform3.x31    = transform4.x31;
            transform3.x32    = transform4.x32;
            transform3.x33    = transform4.x33;

            // update text options
            text_options.tTransform = transform3;

            // draw
            ext_draw->add_text(app_data->layer, (plVec2){0, 0}, sb_text, text_options);

            break;
        }

        case DC_APP_NODE_TYPE_WINDOW: {
            // TODO move this code to only the resize() function
            // update dimensions
            uint32_t dimensionX, dimensionY;

            // TODO fix this in pilotlight for macos
            ext_windows->get_size(app_data->window, &dimensionX, &dimensionY);
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
            _draw_node_list(app_data, node->window.child, &transform);
            break;
        }

        default:
            break;
    }
}
