#include "dcapp.h"

#include "../src/utils/env.h"
#include "../src/utils/log.h"

// static members
// TODO hate this solution, but needed for DLL lookup
// of variables. Clean this up later
// * this is really just a copy of app_data in all the functions
static _AppData *_global_app_data;

// declarations
PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, _AppData *app_data);
PL_EXPORT void  pl_app_shutdown(_AppData *app_data);
PL_EXPORT void  pl_app_resize(_AppData *app_data);
PL_EXPORT void  pl_app_update(_AppData *app_data);
static void     _flush_queued_sets(_AppData *app_data);

// -- handlers for logic files --
// * only works once all variables are registered, as pointer
// * values could change otherwise
void *get_variable_value_addr(const char *name) {

    // get variable
    DcAppLookupVar *var = dc_app_lookup_get_var_by_name(_global_app_data->lookup, name);

    // return value 
    if (var) {
        DcValue *val = dc_app_lookup_get_value(_global_app_data->lookup, var->value_index);
        return dc_value_get_addr(val);
    }
    return NULL;
}

// definitions
PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, _AppData *app_data) {

    if (app_data) {

        // load extensions
        _ext_windows        = pl_get_api_latest(api_registry, plWindowI);
        _ext_draw           = pl_get_api_latest(api_registry, dcDrawI);
        _ext_draw_backend   = pl_get_api_latest(api_registry, dcDrawBackendI);
        _ext_starter        = pl_get_api_latest(api_registry, plStarterI);
        _ext_profile        = pl_get_api_latest(api_registry, plProfileI);
        _ext_memory         = pl_get_api_latest(api_registry, plMemoryI);
        _ext_library        = pl_get_api_latest(api_registry, plLibraryI);
        _ext_ioi            = pl_get_api_latest(api_registry, plIOI);
        _ext_gfx            = pl_get_api_latest(api_registry, plGraphicsI);
        _ext_gpu_allocators = pl_get_api_latest(api_registry, plGPUAllocatorsI);
        _ext_vfs            = pl_get_api_latest(api_registry, plVfsI);
        _ext_shader         = pl_get_api_latest(api_registry, plShaderI);
        // _ext_terrain     = pl_get_api_latest(api_registry, plTerrainI);
        _ext_camera = pl_get_api_latest(api_registry, plCameraI);
        _ext_image  = pl_get_api_latest(api_registry, plImageI);

        // set global app data variable
        _global_app_data = app_data;

        // return (hot reload)
        return app_data;
    }

    // retrieve extension registry
    const plExtensionRegistryI *extension_registry = pl_get_api_latest(api_registry, plExtensionRegistryI);

    // load required extensions
    extension_registry->load("pl_unity_ext", NULL, NULL, true);
    extension_registry->load("pl_platform_ext", NULL, NULL, false);

    // load dcapp extensions (separate from pilotlight's draw extensions)
    extension_registry->load("dc_draw_ext", NULL, NULL, true);
    extension_registry->load("dc_draw_backend_ext", NULL, NULL, true);
    extension_registry->load("pl_terrain_ext", NULL, NULL, true);

    // load extensions
    _ext_windows        = pl_get_api_latest(api_registry, plWindowI);
    _ext_draw           = pl_get_api_latest(api_registry, dcDrawI);
    _ext_draw_backend   = pl_get_api_latest(api_registry, dcDrawBackendI);
    _ext_starter        = pl_get_api_latest(api_registry, plStarterI);
    _ext_profile        = pl_get_api_latest(api_registry, plProfileI);
    _ext_memory         = pl_get_api_latest(api_registry, plMemoryI);
    _ext_library        = pl_get_api_latest(api_registry, plLibraryI);
    _ext_ioi            = pl_get_api_latest(api_registry, plIOI);
    _ext_gfx            = pl_get_api_latest(api_registry, plGraphicsI);
    _ext_gpu_allocators = pl_get_api_latest(api_registry, plGPUAllocatorsI);
    _ext_vfs            = pl_get_api_latest(api_registry, plVfsI);
    _ext_shader         = pl_get_api_latest(api_registry, plShaderI);
    // _ext_terrain     = pl_get_api_latest(api_registry, plTerrainI);
    _ext_camera = pl_get_api_latest(api_registry, plCameraI);
    _ext_image  = pl_get_api_latest(api_registry, plImageI);

    // allocate app memory
    app_data = (_AppData *)PL_ALLOC(sizeof(_AppData));
    memset(app_data, 0, sizeof(_AppData));

    // set global app data variable
    _global_app_data = app_data;

    // parse input arguments
    plIO *_ext_io = _ext_ioi->get_io();
    if (_ext_io->iArgc < 4) {
        DC_LOG_ERROR("App", "Missing dcapp config file");
    }

    // create config
    const char *config_filepath = _ext_io->apArgv[3];
    if (_ext_io->iArgc < 5) {
        app_data->config = dc_app_config_create(config_filepath, NULL, 0);
    } else {
        app_data->config = dc_app_config_create(config_filepath, &(_ext_io->apArgv[4]), _ext_io->iArgc - 4);
    }

    // create lookup
    app_data->lookup = dc_app_lookup_create();

    // reserve index 0 as undefined for nodes and textures
    sbresize(app_data->sb_nodes, 1);
    sbresize(app_data->sb_textures, 1);
    sbresize(app_data->sb_texture_name_offsets, 1);
    sbresize(app_data->sb_texture_names, 1);

    // initialize subsystem contexts
    dc_trick_init();
    dc_edge_init();

    // set environment (used for dcapp XMLs)
    dc_utils_set_env("dcappDisplayHome", app_data->config->config_dir_path, 1);

    // preprocess XML file
    dc_app_config_preprocess_xml(app_data->config, app_data->lookup);

    // initialize pixelstream contexts
    dc_ps_mjpeg_init();
    dc_ps_shmem_init();

    // build dcapp node tree
    xmlNodePtr root_node = xmlDocGetRootElement(app_data->config->xml_doc);
    _process_xml_node(app_data, root_node, NODE_INDEX_UNDEFINED, DC_APP_ELEM_TYPE_UNDEFINED, app_data->config->config_dir_path);

    // initialize logic (link values)
    if (app_data->logic_pre_init) {
        app_data->logic_pre_init(get_variable_value_addr);
    }

    // call logic init
    if (app_data->logic_init) {
        app_data->logic_init();
    }

    // return app memory
    return app_data;
}

PL_EXPORT void pl_app_shutdown(_AppData *app_data) {

    // call logic close
    if (app_data->logic_close) {
        app_data->logic_close();
    }

    // get device
    plDevice *device = _ext_starter->get_device();

    // wait for GPU to finish all work before destroying resources
    _ext_gfx->flush_device(device);

    // cleanup per-node resources (stretchy buffers and malloc'd memory)
    for (int i = NODE_FIRST_INDEX; i < sbcount(app_data->sb_nodes); i++) {
        _Node *node = &app_data->sb_nodes[i];
        switch (node->type) {
            case NODE_TYPE_LINE:
                sbfree(node->line.sb_vertices);
                break;
            case NODE_TYPE_PIXELSTREAM:
                if (node->pixelstream.type == DC_APP_PIXELSTREAM_TYPE_MJPEG) {
                    if (node->pixelstream.frame) {
                        _ext_image->free(node->pixelstream.frame);
                    }
                    if (node->pixelstream.mjpeg.raw_jpeg) {
                        free(node->pixelstream.mjpeg.raw_jpeg);
                    }
                } else if (node->pixelstream.type == DC_APP_PIXELSTREAM_TYPE_SHMEM) {
                    if (node->pixelstream.frame) {
                        free(node->pixelstream.frame);
                    }
                }
                break;
            case NODE_TYPE_POLYGON:
                sbfree(node->polygon.sb_vertices);
                break;
            case NODE_TYPE_STENCIL:
                sbfree(node->stencil.sb_children);
                break;
            case NODE_TYPE_TEXT:
                sbfree(node->text.sb_vals);
                sbfree(node->text.sb_fillers);
                sbfree(node->text.sb_filler_indices);
                sbfree(node->text.sb_formats);
                sbfree(node->text.sb_format_indices);
                sbfree(node->text.sb_format_types);
                break;
            case NODE_TYPE_WINDOW:
                if (node->window.title) {
                    free(node->window.title);
                }
                break;
            default:
                break;
        }
    }
    sbfree(app_data->sb_nodes);

    // cleanup trick contexts
    for (int i = 0; i < sbcount(app_data->sb_tricks); i++) {
        _TrickContext *ctx = &app_data->sb_tricks[i];
        sbfree(ctx->sb_tx_var_contexts);
        sbfree(ctx->sb_rx_var_contexts);
        dc_trick_cleanup(ctx->trick);
    }
    sbfree(app_data->sb_tricks);

    // cleanup edge contexts
    for (int i = 0; i < sbcount(app_data->sb_edges); i++) {
        _EdgeContext *ctx = &app_data->sb_edges[i];
        sbfree(ctx->sb_tx_var_contexts);
        sbfree(ctx->sb_rx_var_contexts);
        dc_edge_cleanup(ctx->edge);
    }
    sbfree(app_data->sb_edges);

    // cleanup pixelstream global contexts
    dc_ps_mjpeg_cleanup();
    dc_ps_shmem_cleanup();

    // cleanup textures
    for (int i = TEXTURE_FIRST_INDEX; i < sbcount(app_data->sb_textures); i++) {
        _ext_gfx->destroy_bind_group(device, app_data->sb_textures[i].bind_group_handle);
        _ext_gfx->destroy_texture(device, app_data->sb_textures[i].texture_handle);
    }
    sbfree(app_data->sb_textures);
    sbfree(app_data->sb_texture_names);
    sbfree(app_data->sb_texture_name_offsets);

    // cleanup shaders
    _ext_gfx->destroy_shader(device, app_data->stencil_create_2d_shader);
    _ext_gfx->destroy_shader(device, app_data->stencil_remove_2d_shader);
    _ext_gfx->destroy_shader(device, app_data->stencil_cleanup_2d_shader);
    _ext_gfx->destroy_shader(device, app_data->stencil_create_sdf_shader);
    _ext_gfx->destroy_shader(device, app_data->stencil_remove_sdf_shader);
    _ext_gfx->destroy_shader(device, app_data->stencil_cleanup_sdf_shader);
    for (int i = 0; i < DC_STENCIL_MAX_DEPTH; i++) {
        _ext_gfx->destroy_shader(device, app_data->stencil_draw_2d_shader[i]);
        _ext_gfx->destroy_shader(device, app_data->stencil_draw_sdf_shader[i]);
    }

    // cleanup queued sets
    sbfree(app_data->sb_queued_sets);

    // cleanup draw batch system
    sbfree(app_data->sb_draw_batches);
    for (int i = 0; i < sbcount(app_data->sb_draw_list_2d_pool); i++) {
        _ext_draw->return_2d_drawlist(app_data->sb_draw_list_2d_pool[i].draw_list);
    }
    sbfree(app_data->sb_draw_list_2d_pool);
    for (int i = 0; i < sbcount(app_data->sb_draw_list_3d_pool); i++) {
        _ext_draw->return_3d_drawlist(app_data->sb_draw_list_3d_pool[i]);
    }
    sbfree(app_data->sb_draw_list_3d_pool);

    // destroy staging buffer
    _ext_gfx->destroy_buffer(device, app_data->pl_staging_buffer_handle);

    // cleanup lookup and config
    dc_app_lookup_cleanup(app_data->lookup);
    dc_app_config_cleanup(app_data->config);

    // cleanup draw backend (GPU buffers, bind group pool, font atlas, drawlists)
    _ext_draw_backend->cleanup_font_atlas(NULL);
    _ext_draw_backend->cleanup();

    // cleanup GPU memory allocators (buddy heap, staging allocators)
    _ext_gpu_allocators->cleanup(device);

    _ext_starter->cleanup();
    _ext_windows->destroy(app_data->pl_window);
    PL_FREE(app_data);
}

PL_EXPORT void pl_app_resize(_AppData *app_data) {
    _ext_starter->resize();
}

PL_EXPORT void pl_app_update(_AppData *app_data) {
    // this needs to be the first call when using the starter
    // extension. You must return if it returns false (usually a swapchain recreation).
    if (!_ext_starter->begin_frame()) {
        return;
    }
    app_data->frame_data.count++;

    // send trick data
    for (int ii = 0; ii < sbcount(app_data->sb_tricks); ii++) {

        _TrickContext *trick_context = &(app_data->sb_tricks[ii]);
        DcTrickHandle  trick         = trick_context->trick;

        // on (re)connect, zero out prev_values so all tx vars get sent to initialize the sim
        bool is_trick_connected = dc_trick_is_connected(trick);
        if (is_trick_connected && !trick_context->was_connected) {
            for (int jj = 0; jj < sbcount(trick_context->sb_tx_var_contexts); jj++) {
                memset(&trick_context->sb_tx_var_contexts[jj].prev_value, 0, sizeof(DcValue));
            }
        }
        trick_context->was_connected = is_trick_connected;

        // add tx commands to buffer
        if (is_trick_connected) {
            for (int jj = 0; jj < sbcount(trick_context->sb_tx_var_contexts); jj++) {

                _TrickTxVarContext *tx_var_context = &(trick_context->sb_tx_var_contexts[jj]);
                if (tx_var_context->dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) continue;
                DcAppLookupVar     *dc_app_var     = dc_app_lookup_get_var(app_data->lookup, tx_var_context->dcapp_var_index);
                DcValue            *curr_value     = dc_app_lookup_get_value(app_data->lookup, dc_app_var->value_index);
                DcValue            *prev_value     = &tx_var_context->prev_value;

                // send if new value is different
                if (!dc_value_is_equal(curr_value, prev_value)) {
                    dc_trick_set_tx_var(trick, tx_var_context->trick_var_index, curr_value->value_string);
                    *prev_value = *curr_value;
                }
            }
        }

        // send the updated buffer, receive the new data, update the connection status
        dc_trick_update(trick);

        // update connected variable if defined
        if (trick_context->connected_var_index != DC_APP_VAR_INDEX_UNDEFINED) {
            DcAppLookupVar *connected_var = dc_app_lookup_get_var(app_data->lookup, trick_context->connected_var_index);
            if (connected_var) {
                DcValue *value = dc_app_lookup_get_value(app_data->lookup, connected_var->value_index);
                bool is_connected = dc_trick_is_connected(trick);
                switch (value->type) {
                    case DC_VALUE_TYPE_BOOLEAN:
                        value->value_boolean = is_connected;
                        break;
                    case DC_VALUE_TYPE_INTEGER:
                        value->value_integer = is_connected ? 1 : 0;
                        break;
                    case DC_VALUE_TYPE_STRING:
                        strncpy(value->value_string, is_connected ? "true" : "false", DC_VALUE_STRING_BUFFER_SIZE - 1);
                        break;
                    default:
                        break;
                }
                dc_value_refresh(value);
            }
        }

        // receive the new data
        if (dc_trick_has_new_data(trick) && dc_trick_is_connected(trick)) {
            char rx_buffer[256];
            for (int jj = 0; jj < sbcount(trick_context->sb_rx_var_contexts); jj++) {

                _TrickRxVarContext *rx_var_context = &(trick_context->sb_rx_var_contexts[jj]);
                if (rx_var_context->dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) continue;
                DcAppLookupVar     *dc_app_var     = dc_app_lookup_get_var(app_data->lookup, rx_var_context->dcapp_var_index);
                DcValue            *value          = dc_app_lookup_get_value(app_data->lookup, dc_app_var->value_index);

                dc_trick_get_rx_var_value(trick, rx_var_context->trick_var_index, rx_buffer);
                dc_app_lookup_set_var_to_string(app_data->lookup, rx_var_context->dcapp_var_index, rx_buffer);
            }
        }
    }

    // send edge data
    for (int ii = 0; ii < sbcount(app_data->sb_edges); ii++) {

        _EdgeContext *edge_context = &(app_data->sb_edges[ii]);
        DcEdgeHandle  edge         = edge_context->edge;

        // on (re)connect, zero out prev_values so all tx vars get sent to initialize the scene
        bool is_connected = dc_edge_is_connected(edge);
        if (is_connected && !edge_context->was_connected) {
            for (int jj = 0; jj < sbcount(edge_context->sb_tx_var_contexts); jj++) {
                memset(&edge_context->sb_tx_var_contexts[jj].prev_value, 0, sizeof(DcValue));
            }
        }
        edge_context->was_connected = is_connected;

        // add tx commands to buffer
        if (is_connected) {
            for (int jj = 0; jj < sbcount(edge_context->sb_tx_var_contexts); jj++) {

                _EdgeTxVarContext *tx_var_context = &(edge_context->sb_tx_var_contexts[jj]);
                if (tx_var_context->dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) continue;
                DcAppLookupVar    *dc_app_var     = dc_app_lookup_get_var(app_data->lookup, tx_var_context->dcapp_var_index);
                DcValue           *curr_value     = dc_app_lookup_get_value(app_data->lookup, dc_app_var->value_index);
                DcValue           *prev_value     = &tx_var_context->prev_value;

                // send if new value is different
                if (!dc_value_is_equal(curr_value, prev_value)) {
                    dc_edge_set_tx_var(edge, tx_var_context->edge_var_index, curr_value->value_string);
                    *prev_value = *curr_value;
                }
            }
        }

        // send the updated buffer, receive the new data, update the connection status
        dc_edge_update(edge);

        // update connected variable if defined
        if (edge_context->connected_var_index != DC_APP_VAR_INDEX_UNDEFINED) {
            DcAppLookupVar *connected_var = dc_app_lookup_get_var(app_data->lookup, edge_context->connected_var_index);
            if (connected_var) {
                DcValue *value = dc_app_lookup_get_value(app_data->lookup, connected_var->value_index);
                bool is_connected = dc_edge_is_connected(edge);
                switch (value->type) {
                    case DC_VALUE_TYPE_BOOLEAN:
                        value->value_boolean = is_connected;
                        break;
                    case DC_VALUE_TYPE_INTEGER:
                        value->value_integer = is_connected ? 1 : 0;
                        break;
                    case DC_VALUE_TYPE_STRING:
                        strncpy(value->value_string, is_connected ? "true" : "false", DC_VALUE_STRING_BUFFER_SIZE - 1);
                        break;
                    default:
                        break;
                }
                dc_value_refresh(value);
            }
        }

        // receive the new data
        if (dc_edge_has_new_data(edge) && dc_edge_is_connected(edge)) {
            char rx_buffer[256];
            for (int jj = 0; jj < sbcount(edge_context->sb_rx_var_contexts); jj++) {

                _EdgeRxVarContext *rx_var_context = &(edge_context->sb_rx_var_contexts[jj]);
                if (rx_var_context->dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) continue;
                DcAppLookupVar    *dc_app_var     = dc_app_lookup_get_var(app_data->lookup, rx_var_context->dcapp_var_index);
                DcValue           *value          = dc_app_lookup_get_value(app_data->lookup, dc_app_var->value_index);

                dc_edge_get_rx_var_value(edge, rx_var_context->edge_var_index, rx_buffer);
                dc_app_lookup_set_var_to_string(app_data->lookup, rx_var_context->dcapp_var_index, rx_buffer);
            }
        }
    }

    // process pixelstream data
    dc_ps_mjpeg_update();
    dc_ps_shmem_update();

    // process logic
    if (app_data->logic_draw) {
        app_data->logic_draw();
    }

    // refresh variables
    for (int ii = DC_APP_LOOKUP_FIRST_INDEX; ii < dc_app_lookup_get_var_count(app_data->lookup); ii++) {
        DcAppLookupVar *var   = dc_app_lookup_get_var(app_data->lookup, ii);
        DcValue        *value = dc_app_lookup_get_value(app_data->lookup, var->value_index);
        dc_value_refresh(value);
    }

    // get mouse button status
    if (_ext_ioi->is_mouse_down(PL_MOUSE_BUTTON_LEFT)) {
        app_data->frame_data.is_mouse_pressed  = !app_data->frame_data.is_mouse_down;
        app_data->frame_data.is_mouse_released = false;
        app_data->frame_data.is_mouse_down     = true;
    } else {
        app_data->frame_data.is_mouse_pressed  = false;
        app_data->frame_data.is_mouse_released = app_data->frame_data.is_mouse_down;
        app_data->frame_data.is_mouse_down     = false;
    }

    // get mouse position
    app_data->frame_data.mouse_position = _ext_ioi->get_mouse_pos();

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~drawing & profile API~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // new_frame: backend calls draw's new_frame + allocates dynamic data block
    _ext_draw_backend->new_frame();

    // reset draw batch system for new frame
    _draw_batch_reset(app_data);

    // draw node
    _draw_node(app_data, app_data->window, NULL, NULL, NULL);

    // flush queued set operations (deferred Sets applied atomically)
    _flush_queued_sets(app_data);

    // reset any unpoped variable stacks
    dc_app_lookup_reset_var_stacks(app_data->lookup);

    // start main pass & return the encoder being used
    plRenderEncoder *encoder = _ext_starter->begin_main_pass();

    // submit draw lists from batch system in order
    plIO *ptIO = _ext_ioi->get_io();
    {
        // orthographic MVP for 3D objects in 2D space
        // Note: dcapp uses bottom-left origin, so Y is NOT flipped here (parent_transform handles it)
        float w = ptIO->tMainViewportSize.x;
        float h = ptIO->tMainViewportSize.y;
        float n = -1000.0f;
        float f = 1000.0f;
        plMat4 ortho_proj = {
            .col = {
                { 2.0f / w,   0.0f,         0.0f,           0.0f },
                { 0.0f,       2.0f / h,     0.0f,           0.0f },
                { 0.0f,       0.0f,         1.0f / (f - n), 0.0f },
                {-1.0f,      -1.0f,        -n / (f - n),    1.0f }
            }
        };

        int batch_count = sbcount(app_data->sb_draw_batches);
        for (int i = 0; i < batch_count; i++) {
            _DrawBatch *batch = &app_data->sb_draw_batches[i];
            if (batch->type == DRAW_BATCH_TYPE_2D) {
                _ext_draw->submit_2d_layer(batch->draw_list_2d.layer);
                _ext_draw_backend->submit_2d_drawlist(
                    batch->draw_list_2d.draw_list,
                    encoder,
                    ptIO->tMainViewportSize.x,
                    ptIO->tMainViewportSize.y,
                    _ext_gfx->get_swapchain_info(_ext_starter->get_swapchain()).tSampleCount
                );
            } else if (batch->type == DRAW_BATCH_TYPE_3D && batch->draw_list_3d) {
                _ext_draw_backend->submit_3d_drawlist(
                    batch->draw_list_3d,
                    encoder,
                    ptIO->tMainViewportSize.x,
                    ptIO->tMainViewportSize.y,
                    &ortho_proj,
                    PL_DRAW_FLAG_DEPTH_TEST | PL_DRAW_FLAG_DEPTH_WRITE,
                    _ext_gfx->get_swapchain_info(_ext_starter->get_swapchain()).tSampleCount
                );
            }
        }
    }

    _ext_starter->end_main_pass();

    // must be the last function called when using the starter extension
    _ext_starter->end_frame();

    // update node states
    app_data->frame_data.pressed_node = app_data->frame_data.next_pressed_node;
    app_data->frame_data.hovered_node = app_data->frame_data.next_hovered_node;
    if (app_data->frame_data.is_mouse_pressed) {
        app_data->frame_data.active_node = app_data->frame_data.next_pressed_node;
    }
    if (app_data->frame_data.is_mouse_released) {
        app_data->frame_data.released_node = app_data->frame_data.active_node;
        app_data->frame_data.active_node   = NODE_INDEX_UNDEFINED;
    } else {
        app_data->frame_data.released_node = NODE_INDEX_UNDEFINED;
    }
    app_data->frame_data.next_hovered_node = NODE_INDEX_UNDEFINED;
    app_data->frame_data.next_pressed_node = NODE_INDEX_UNDEFINED;
}

#include "_dcapp_draw.c"
#include "_dcapp_process_xml.c"
