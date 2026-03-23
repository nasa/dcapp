#include "dcapp.h"

#define PL_JSON_IMPLEMENTATION
#include "pl_json.h"

#include "utils/env.h"
#include "utils/file.h"
#include "utils/log.h"

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
void           *get_variable_value_addr(const char *name);
static void     _flush_deferred_sets(_AppData *app_data);
static bool     _build_planet_texture(_AppData *app_data, _PlanetTextureEntry *entry, plPlanetTexture *out);
static void     _init_planets(_AppData *app_data);
static void     _update_planet_defs(_AppData *app_data);
static void     _load_apis(plApiRegistryI *api_registry);

PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, _AppData *app_data) {

    if (app_data) {
        _load_apis(api_registry);
        _global_app_data = app_data;
        return app_data;
    }

    // retrieve extension registry
    const plExtensionRegistryI *extension_registry = pl_get_api_latest(api_registry, plExtensionRegistryI);

    // load required extensions
    extension_registry->load("pl_unity_ext", NULL, NULL, true);
    extension_registry->load("pl_platform_ext", "pl_load_platform_ext", "pl_unload_platform_ext", false);

    // load dcapp extensions (separate from pilotlight's draw extensions)
    extension_registry->load("dc_draw_ext", NULL, NULL, true);
    extension_registry->load("dc_draw_backend_ext", NULL, NULL, true);
    extension_registry->load("pl_planet_processor_ext", NULL, NULL, true);
    extension_registry->load("pl_planet_ext", NULL, NULL, true);

    _load_apis(api_registry);

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

    // parse --preprocessed flag (before constants)
    const char *preprocessed_output = NULL;
    int         const_count         = 0;
    char      **const_args          = NULL;

    for (int ii = 4; ii < _ext_io->iArgc; ii++) {
        if (strcmp(_ext_io->apArgv[ii], "--preprocessed") == 0 && ii + 1 < _ext_io->iArgc) {
            preprocessed_output = _ext_io->apArgv[++ii];
        }
    }

    // collect constant args (skip --preprocessed and its value)
    if (_ext_io->iArgc > 4) {
        const_args = (char **)malloc(sizeof(char *) * (_ext_io->iArgc - 4));
        for (int ii = 4; ii < _ext_io->iArgc; ii++) {
            if (strcmp(_ext_io->apArgv[ii], "--preprocessed") == 0 && ii + 1 < _ext_io->iArgc) {
                ii++; // skip value
                continue;
            }
            const_args[const_count++] = _ext_io->apArgv[ii];
        }
    }

    // create config
    const char *config_filepath = _ext_io->apArgv[3];
    if (const_count > 0) {
        app_data->config = dc_app_config_create(config_filepath, const_args, const_count);
    } else {
        app_data->config = dc_app_config_create(config_filepath, NULL, 0);
    }
    free(const_args);

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
    dc_utils_set_env("dcappHome", app_data->config->dcapp_dir_path, 1);

    // preprocess XML file
    dc_app_config_preprocess_xml(app_data->config, app_data->lookup);

    // dump preprocessed XML for debugging
    dc_app_config_save_preprocessed(app_data->config, preprocessed_output);

    // initialize pixelstream contexts
    dc_ps_mjpeg_init();
    dc_ps_shmem_init();

    // build dcapp node tree
    xmlNodePtr root_node = xmlDocGetRootElement(app_data->config->xml_doc);
    _process_xml_node(app_data, root_node, NODE_INDEX_UNDEFINED, DC_APP_ELEM_TYPE_UNDEFINED, app_data->config->config_dir_path);

    // initialize resource manager (needed by planet texture loading)
    plResourceManagerInit resource_init = {0};
    resource_init.ptDevice              = _ext_starter->get_device();
    _ext_resource->initialize(resource_init);

    // initialize planet rendering instances
    _init_planets(app_data);

    // resize staging buffer for runtime (pixelstream frames only, max 4K RGBA)
    {
        plDevice *device = _ext_starter->get_device();
        _ext_gfx->destroy_buffer(device, app_data->pl_staging_buffer_handle);

        app_data->pl_staging_buffer_size = (size_t)_NODE_PIXELSTREAM_MAX_WIDTH * _NODE_PIXELSTREAM_MAX_HEIGHT * 4;

        const plBufferDesc staging_buffer_desc = {
            .tUsage      = PL_BUFFER_USAGE_STAGING,
            .szByteSize  = app_data->pl_staging_buffer_size,
            .pcDebugName = "staging buffer"};
        app_data->pl_staging_buffer_handle = _ext_gfx->create_buffer(device, &staging_buffer_desc, NULL);

        plBuffer *staging_buffer = _ext_gfx->get_buffer(device, app_data->pl_staging_buffer_handle);
        plDeviceMemoryAllocatorI *allocator = app_data->gpu_staging_uncached_allocator;
        const plDeviceMemoryAllocation staging_buffer_allocation = allocator->allocate(
            allocator->ptInst,
            staging_buffer->tMemoryRequirements.uMemoryTypeBits,
            staging_buffer->tMemoryRequirements.ulSize,
            staging_buffer->tMemoryRequirements.ulAlignment,
            "staging buffer memory");
        _ext_gfx->bind_buffer_to_memory(device, app_data->pl_staging_buffer_handle, &staging_buffer_allocation);
    }

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
                break;
            case NODE_TYPE_PLANET_LINE:
                sbfree(node->planet_line.sb_points_static);
                sbfree(node->planet_line.sb_points_dynamic);
                break;
            case NODE_TYPE_PLANET_POLYGON:
                sbfree(node->planet_polygon.sb_points_static);
                sbfree(node->planet_polygon.sb_points_dynamic);
                break;
            case NODE_TYPE_PLANET_VIEW:
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

    // destroy staging buffer (must happen before planet cleanup destroys GPU allocators)
    _ext_gfx->destroy_buffer(device, app_data->pl_staging_buffer_handle);

    // cleanup planet views, instances, and extension
    {
        for (int i = 0; i < sbcount(app_data->sb_planet_views); i++) {
            if (app_data->sb_planet_views[i]) {
                _ext_planet->cleanup_view(app_data->sb_planet_views[i]);
            }
        }
        for (int i = 0; i < sbcount(app_data->sb_planets); i++) {
            if (app_data->sb_planets[i]) {
                _ext_planet->cleanup_planet(app_data->sb_planets[i]);
            }
        }
        if (app_data->planet_ext_initialized) {
            _ext_planet->cleanup();
        }
        _ext_resource->cleanup();
        sbfree(app_data->sb_planet_views);
        sbfree(app_data->sb_planets);
        for (int i = 0; i < sbcount(app_data->sb_planet_defs); i++) {
            _PlanetDef *def = &app_data->sb_planet_defs[i];
            if (def->name) free(def->name);
            for (int j = 0; j < sbcount(def->sb_data_files); j++) {
                free(def->sb_data_files[j]);
            }
            sbfree(def->sb_data_files);
            for (int j = 0; j < sbcount(def->sb_shaders); j++) {
                if (def->sb_shaders[j].vertex_path) free(def->sb_shaders[j].vertex_path);
                if (def->sb_shaders[j].fragment_path) free(def->sb_shaders[j].fragment_path);
            }
            sbfree(def->sb_shaders);
            for (int j = 0; j < sbcount(def->sb_textures); j++) {
                if (def->sb_textures[j].source) free(def->sb_textures[j].source);
            }
            sbfree(def->sb_textures);
        }
        sbfree(app_data->sb_planet_defs);
        sbfree(app_data->sb_planet_view_node_indices);
    }

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

    // cleanup pixelstream sources
    for (int i = 0; i < sbcount(app_data->sb_ps_sources); i++) {
        _PixelstreamSource *src = &app_data->sb_ps_sources[i];
        if (src->type == DC_APP_PIXELSTREAM_TYPE_MJPEG) {
            if (src->frame) {
                _ext_image->free(src->frame);
            }
            if (src->mjpeg.raw_jpeg) {
                free(src->mjpeg.raw_jpeg);
            }
        } else if (src->type == DC_APP_PIXELSTREAM_TYPE_SHMEM) {
            if (src->frame) {
                free(src->frame);
            }
        }
    }
    sbfree(app_data->sb_ps_sources);
    sbfree(app_data->sb_ps_source_keys);
    sbfree(app_data->sb_ps_source_key_offsets);

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

    // cleanup fonts
    sbfree(app_data->sb_fonts);
    sbfree(app_data->sb_font_paths);
    sbfree(app_data->sb_font_path_offsets);

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

    // cleanup deferred sets
    sbfree(app_data->sb_deferred_sets);

    // cleanup draw batch system
    sbfree(app_data->sb_draw_batches);
    for (int i = 0; i < sbcount(app_data->sb_draw_list_2d_pool); i++) {
        _ext_dc_draw->return_2d_drawlist(app_data->sb_draw_list_2d_pool[i].draw_list);
    }
    sbfree(app_data->sb_draw_list_2d_pool);
    for (int i = 0; i < sbcount(app_data->sb_draw_list_3d_pool); i++) {
        _ext_dc_draw->return_3d_drawlist(app_data->sb_draw_list_3d_pool[i]);
    }
    sbfree(app_data->sb_draw_list_3d_pool);

    // cleanup lookup and config
    dc_app_lookup_cleanup(app_data->lookup);
    dc_app_config_cleanup(app_data->config);

    // cleanup draw backend (GPU buffers, bind group pool, font atlas, drawlists)
    _ext_dc_draw_backend->cleanup_font_atlas(NULL);
    _ext_dc_draw_backend->cleanup();
    _ext_dc_draw->cleanup();

    // cleanup GPU memory allocators (buddy heap, staging allocators)
    _ext_gpu_allocators->cleanup(device);

    _ext_starter->cleanup();
    _ext_windows->destroy(app_data->pl_window);
    PL_FREE(app_data);
}

PL_EXPORT void pl_app_resize(_AppData *app_data) {
    _ext_starter->resize();
}

static void _update_pixelstream_sources(_AppData *app_data) {

    for (int ii = 0; ii < sbcount(app_data->sb_ps_sources); ii++) {
        _PixelstreamSource *src = &app_data->sb_ps_sources[ii];

        switch (src->type) {

            case DC_APP_PIXELSTREAM_TYPE_MJPEG: {

                src->is_connected = dc_ps_mjpeg_server_is_connected(src->mjpeg.handle);
                if (!src->is_connected) break;

                if (dc_ps_mjpeg_server_has_new_data(src->mjpeg.handle)) {

                    size_t jpeg_size;
                    dc_ps_mjpeg_get_server_data(src->mjpeg.handle, src->mjpeg.raw_jpeg, src->mjpeg.raw_jpeg_size, &jpeg_size);

                    _ext_image->free(src->frame);

                    int channels;
                    src->frame = _ext_image->load(src->mjpeg.raw_jpeg, (int)jpeg_size, &src->frame_width, &src->frame_height, &channels, 4);

                    if (src->frame_height * src->frame_width > _NODE_PIXELSTREAM_MAX_WIDTH * _NODE_PIXELSTREAM_MAX_HEIGHT) {
                        DC_LOG_WARN("PixelStream", "Max image dimensions exceeded");
                        _ext_image->free(src->frame);
                        src->frame = NULL;
                        break;
                    }
                }
                break;
            }

            case DC_APP_PIXELSTREAM_TYPE_SHMEM: {

                src->is_connected = dc_ps_shmem_is_connected(src->shmem.handle);
                if (!src->is_connected) break;

                if (dc_ps_shmem_has_new_data(src->shmem.handle)) {

                    uint32_t width  = dc_ps_shmem_get_width(src->shmem.handle);
                    uint32_t height = dc_ps_shmem_get_height(src->shmem.handle);

                    if (width * height > _NODE_PIXELSTREAM_MAX_WIDTH * _NODE_PIXELSTREAM_MAX_HEIGHT) {
                        DC_LOG_WARN("PixelStream", "Shmem max image dimensions exceeded");
                        break;
                    }

                    size_t frame_size = (size_t)width * height * 4;
                    src->frame = realloc(src->frame, frame_size);

                    size_t out_size;
                    dc_ps_shmem_get_data(src->shmem.handle, src->frame, frame_size, &out_size);

                    src->frame_width  = (int)width;
                    src->frame_height = (int)height;
                }
                break;
            }

            default:
                break;
        }

        // upload to GPU if we have frame data
        if (src->frame && src->is_connected) {
            _Texture texture = app_data->sb_textures[src->texture_index];
            plDevice *device = _ext_starter->get_device();

            plBlitEncoder *encoder = _ext_starter->get_blit_encoder();
            _ext_gfx->set_texture_usage(encoder, texture.texture_handle, PL_TEXTURE_USAGE_SAMPLED, 0);

            plBuffer *staging_buffer = _ext_gfx->get_buffer(device, app_data->pl_staging_buffer_handle);
            memcpy(staging_buffer->tMemoryAllocation.pHostMapped, src->frame, src->frame_width * src->frame_height * 4);

            plBufferImageCopy buffer_image_copy;
            memset(&buffer_image_copy, 0, sizeof(plBufferImageCopy));
            buffer_image_copy.uImageWidth    = (uint32_t)src->frame_width;
            buffer_image_copy.uImageHeight   = (uint32_t)src->frame_height;
            buffer_image_copy.uImageDepth    = 1;
            buffer_image_copy.uLayerCount    = 1;
            buffer_image_copy.szBufferOffset = 0;
            _ext_gfx->copy_buffer_to_texture(encoder, app_data->pl_staging_buffer_handle, texture.texture_handle, 1, &buffer_image_copy);

            _ext_starter->return_blit_encoder(encoder);
        }
    }
}

PL_EXPORT void pl_app_update(_AppData *app_data) {
    // this needs to be the first call when using the starter
    // extension. You must return if it returns false (usually a swapchain recreation).
    if (!_ext_starter->begin_frame()) {
        return;
    }
    _ext_resource->new_frame();
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
                DcAppLookupVar *dc_app_var = dc_app_lookup_get_var(app_data->lookup, tx_var_context->dcapp_var_index);
                DcValue        *curr_value = dc_app_lookup_get_value(app_data->lookup, dc_app_var->value_index);
                DcValue        *prev_value = &tx_var_context->prev_value;

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
                DcValue *value        = dc_app_lookup_get_value(app_data->lookup, connected_var->value_index);
                bool     is_connected = dc_trick_is_connected(trick);
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
                DcAppLookupVar *dc_app_var = dc_app_lookup_get_var(app_data->lookup, rx_var_context->dcapp_var_index);
                DcValue        *value      = dc_app_lookup_get_value(app_data->lookup, dc_app_var->value_index);

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
                DcAppLookupVar *dc_app_var = dc_app_lookup_get_var(app_data->lookup, tx_var_context->dcapp_var_index);
                DcValue        *curr_value = dc_app_lookup_get_value(app_data->lookup, dc_app_var->value_index);
                DcValue        *prev_value = &tx_var_context->prev_value;

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
                is_connected   = dc_edge_is_connected(edge);
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
                DcAppLookupVar *dc_app_var = dc_app_lookup_get_var(app_data->lookup, rx_var_context->dcapp_var_index);
                DcValue        *value      = dc_app_lookup_get_value(app_data->lookup, dc_app_var->value_index);

                dc_edge_get_rx_var_value(edge, rx_var_context->edge_var_index, rx_buffer);
                dc_app_lookup_set_var_to_string(app_data->lookup, rx_var_context->dcapp_var_index, rx_buffer);
            }
        }
    }

    // process pixelstream data
    dc_ps_mjpeg_update();
    dc_ps_shmem_update();
    _update_pixelstream_sources(app_data);

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
    _ext_dc_draw_backend->new_frame();

    // reset draw batch system for new frame
    _draw_batch_reset(app_data);

    // update planet definitions (texture reload, prepare)
    _update_planet_defs(app_data);

    // draw node
    _draw_node(app_data, app_data->window, NULL, NULL, NULL);

    // flush deferred set operations (deferred Sets applied atomically)
    _flush_deferred_sets(app_data);

    // reset any unpoped variable stacks
    dc_app_lookup_reset_var_stacks(app_data->lookup);

    // start main pass & return the encoder being used
    plRenderEncoder *encoder = _ext_starter->begin_main_pass();

    // submit draw lists from batch system in order
    plIO *ptIO = _ext_ioi->get_io();
    {
        // orthographic MVP for 3D objects in 2D space
        // Note: dcapp uses bottom-left origin, so Y is NOT flipped here (parent_transform handles it)
        float  w          = ptIO->tMainViewportSize.x;
        float  h          = ptIO->tMainViewportSize.y;
        float  n          = -1000.0f;
        float  f          = 1000.0f;
        plMat4 ortho_proj = {
            .col = {
                {2.0f / w, 0.0f, 0.0f, 0.0f},
                {0.0f, 2.0f / h, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f / (f - n), 0.0f},
                {-1.0f, -1.0f, -n / (f - n), 1.0f}}};

        int batch_count = sbcount(app_data->sb_draw_batches);
        for (int i = 0; i < batch_count; i++) {
            _DrawBatch *batch = &app_data->sb_draw_batches[i];
            if (batch->type == DRAW_BATCH_TYPE_2D) {
                _ext_dc_draw->submit_2d_layer(batch->draw_list_2d.layer);
                _ext_dc_draw_backend->submit_2d_drawlist(
                    batch->draw_list_2d.draw_list,
                    encoder,
                    ptIO->tMainViewportSize.x,
                    ptIO->tMainViewportSize.y,
                    _ext_gfx->get_swapchain_info(_ext_starter->get_swapchain()).tSampleCount);
            } else if (batch->type == DRAW_BATCH_TYPE_3D && batch->draw_list_3d) {
                _ext_dc_draw_backend->submit_3d_drawlist(
                    batch->draw_list_3d,
                    encoder,
                    ptIO->tMainViewportSize.x,
                    ptIO->tMainViewportSize.y,
                    &ortho_proj,
                    DC_DRAW_FLAG_DEPTH_TEST | DC_DRAW_FLAG_DEPTH_WRITE,
                    _ext_gfx->get_swapchain_info(_ext_starter->get_swapchain()).tSampleCount);
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

static bool _build_planet_texture(_AppData *app_data, _PlanetTextureEntry *entry, plPlanetTexture *out) {
    if (!entry->source || entry->source[0] == '\0') return false;
    if (!_ext_vfs->does_file_exist(entry->source)) return false;
    memset(out, 0, sizeof(*out));
    out->pcPath = entry->source;
    if (entry->mpp != DC_APP_VAL_INDEX_UNDEFINED)
        out->fMetersPerPixel = (float)dc_app_lookup_get_value(app_data->lookup, entry->mpp)->value_double;
    if (entry->lat != DC_APP_VAL_INDEX_UNDEFINED)
        out->fLatitude = (float)dc_app_lookup_get_value(app_data->lookup, entry->lat)->value_double;
    if (entry->lon != DC_APP_VAL_INDEX_UNDEFINED)
        out->fLongitude = (float)dc_app_lookup_get_value(app_data->lookup, entry->lon)->value_double;
    return true;
}

static void _init_planets(_AppData *app_data) {
    int def_count  = sbcount(app_data->sb_planet_defs);
    int view_count = sbcount(app_data->sb_planet_view_node_indices);
    if (def_count == 0 && view_count == 0) return;

    DC_LOG_INFO("Planet", "Initializing %d planet def(s), %d view(s)", def_count, view_count);

    // initialize planet extension
    plPlanetExtInit planet_ext_init = {0};
    planet_ext_init.ptDevice        = _ext_starter->get_device();
    _ext_planet->initialize(planet_ext_init);
    app_data->planet_ext_initialized = true;

    // reserve index 0 as sentinel (PLANET_INDEX_UNDEFINED / PLANET_VIEW_INDEX_UNDEFINED)
    sbpush(app_data->sb_planets, NULL);
    sbpush(app_data->sb_planet_views, NULL);

    // Phase 1: create planets from definitions
    for (int i = 0; i < def_count; i++) {
        _PlanetDef *def = &app_data->sb_planet_defs[i];

        int file_count = sbcount(def->sb_data_files);
        if (file_count == 0) {
            DC_LOG_WARN("Planet", "  [%d] '%s': no PlanetData file specified, skipping", i, def->name ? def->name : "?");
            continue;
        }

        // use first data file
        const char *json_path = def->sb_data_files[0];

        DC_LOG_INFO("Planet", "  [%d] '%s' loading: %s", i, def->name, json_path);

        // load JSON file
        char *json_str = dc_utils_load_text_file(json_path);
        if (!json_str) {
            DC_LOG_ERROR("Planet", "  [%d] failed to load file: %s", i, json_path);
            continue;
        }

        // parse JSON
        plJsonObject *root = NULL;
        if (!pl_load_json(json_str, &root)) {
            DC_LOG_ERROR("Planet", "  [%d] failed to parse JSON: %s", i, json_path);
            free(json_str);
            continue;
        }

        // extract metadata
        double radius           = pl_json_double_member(root, "radius", 0.0);
        float  meters_per_pixel = pl_json_float_member(root, "meters_per_pixel", 0.0f);
        int    tile_size        = pl_json_int_member(root, "tile_size", 0);
        int    cols             = pl_json_int_member(root, "cols", 0);
        int    rows             = pl_json_int_member(root, "rows", 0);
        float  min_height       = pl_json_float_member(root, "min_height", 0.0f);
        float  max_height       = pl_json_float_member(root, "max_height", 0.0f);
        int    tree_depth       = pl_json_int_member(root, "tree_depth", 0);
        float  max_base_error   = pl_json_float_member(root, "max_base_error", 0.0f);
        def->radius             = radius;

        // extract tiles array
        uint32_t      tile_count = 0;
        plJsonObject *tile_array = pl_json_array_member(root, "tiles", &tile_count);

        // build process info
        plPlanetProcessInfo process_info = {0};
        process_info.fRadius             = (float)radius;
        process_info.fMetersPerPixel     = meters_per_pixel;
        process_info.uSize               = (uint32_t)tile_size;
        process_info.uTileCount          = tile_count;
        process_info.uHorizontalTiles    = (uint32_t)cols;
        process_info.uVerticalTiles      = (uint32_t)rows;
        process_info.atTiles             = (plPlanetProcessTileInfo *)PL_ALLOC(tile_count * sizeof(plPlanetProcessTileInfo));

        // get directory of the JSON file for resolving relative chunk paths
        char json_dir[DC_VALUE_STRING_BUFFER_SIZE];
        dc_utils_get_directory(json_path, json_dir, sizeof(json_dir));

        for (uint32_t t = 0; t < tile_count; t++) {
            plJsonObject *tile_obj = pl_json_member_by_index(tile_array, t);

            plPlanetProcessTileInfo *tile = &process_info.atTiles[t];
            memset(tile, 0, sizeof(plPlanetProcessTileInfo));
            tile->fLatitude     = pl_json_float_member(tile_obj, "lat", 0.0f);
            tile->fLongitude    = pl_json_float_member(tile_obj, "lon", 0.0f);
            tile->fMaxBaseError = max_base_error;
            tile->fMaxHeight    = max_height;
            tile->fMinHeight    = min_height;
            tile->iTreeDepth    = tree_depth;

            // resolve chunk file path
            char chunk_file[256] = {0};
            pl_json_string_member(tile_obj, "file", chunk_file, sizeof(chunk_file));

            char abs_chunk_path[DC_VALUE_STRING_BUFFER_SIZE];
            if (dc_utils_is_relative_path(chunk_file)) {
                dc_utils_join_paths(json_dir, chunk_file, abs_chunk_path, sizeof(abs_chunk_path));
            } else {
                strncpy(abs_chunk_path, chunk_file, sizeof(abs_chunk_path) - 1);
            }
            strncpy(tile->acOutputFile, abs_chunk_path, sizeof(tile->acOutputFile) - 1);
        }

        // build planet init
        plPlanetInit planet_init = {0};
        planet_init.dRadius      = radius;
        planet_init.tLoadFlags   = PL_PLANET_LOAD_FLAGS_NONE;
        if (def->mesh_cache_size > 0) {
            planet_init.uVertexBufferSize = def->mesh_cache_size / 2;
            planet_init.uIndexBufferSize  = def->mesh_cache_size / 2;
        }

        // create planet
        plCommandBuffer *cmd_buf = _ext_starter->get_temporary_command_buffer();
        plPlanet        *planet  = _ext_planet->create_planet(cmd_buf, planet_init, &process_info);
        _ext_starter->submit_temporary_command_buffer(cmd_buf);

        // initial texture overlay (if source is set at parse time)
        if (sbcount(def->sb_textures) > 0) {
            plPlanetTexture texture;
            if (_build_planet_texture(app_data, &def->sb_textures[0], &texture)) {
                DC_LOG_INFO("Planet", "  [%d] texture: %s (mpp=%.1f, lat=%.1f, lon=%.1f)",
                            i, texture.pcPath, texture.fMetersPerPixel, texture.fLatitude, texture.fLongitude);
                _ext_planet->set_texture(planet, &texture, 0);
            }
        }

        // store planet
        sbpush(app_data->sb_planets, planet);
        def->index = (uint8_t)(sbcount(app_data->sb_planets) - 1); // index 0 is sentinel

        DC_LOG_INFO("Planet", "  [%d] '%s' created (radius=%.0f, %u tiles)", i, def->name, radius, tile_count);

        // cleanup
        PL_FREE(process_info.atTiles);
        pl_unload_json(&root);
        free(json_str);
    }

    // Phase 2: create views from PlanetView nodes
    for (int i = 0; i < view_count; i++) {
        _NodeIndex node_index = app_data->sb_planet_view_node_indices[i];
        _Node     *node       = _get_node(app_data, node_index);

        uint8_t def_idx = node->planet_view.planet_def_index;
        if (def_idx >= def_count) {
            DC_LOG_ERROR("PlanetView", "  [%d] invalid planet def index", i);
            continue;
        }

        _PlanetDef *def = &app_data->sb_planet_defs[def_idx];
        if (def->index == PLANET_INDEX_UNDEFINED) {
            DC_LOG_ERROR("PlanetView", "  [%d] planet '%s' not initialized", i, def->name ? def->name : "?");
            continue;
        }

        // get output dimensions from the view node (default 1024x1024)
        float output_width  = 1024.0f;
        float output_height = 1024.0f;

        plPlanet *planet = app_data->sb_planets[def->index];

        plPlanetViewInit view_init = {0};
        view_init.uOutputWidth     = (uint32_t)output_width;
        view_init.uOutputHeight    = (uint32_t)output_height;

        plCommandBuffer *cmd_buf = _ext_starter->get_temporary_command_buffer();
        plPlanetView    *view    = _ext_planet->create_view(planet, cmd_buf, view_init);
        _ext_starter->submit_temporary_command_buffer(cmd_buf);

        sbpush(app_data->sb_planet_views, view);
        node->planet_view.planet_view_index = (uint8_t)(sbcount(app_data->sb_planet_views) - 1); // index 0 is sentinel

        // force shader mismatch so first draw applies the shader
        if (node->planet_view.shader_index != DC_APP_VAL_INDEX_UNDEFINED) {
            int initial                          = (int)dc_app_lookup_get_value(app_data->lookup, node->planet_view.shader_index)->value_integer;
            node->planet_view.active_shader_index = initial + 1;
        }

        DC_LOG_INFO("PlanetView", "  [%d] created view for '%s' (%ux%u)", i, def->name, (uint32_t)output_width, (uint32_t)output_height);
    }
}

static void _update_planet_defs(_AppData *app_data) {
    int def_count = sbcount(app_data->sb_planet_defs);
    if (def_count == 0) return;
    for (int i = 0; i < def_count; i++) {
        _PlanetDef *def = &app_data->sb_planet_defs[i];
        if (def->index == PLANET_INDEX_UNDEFINED) continue;

        plPlanet *planet = app_data->sb_planets[def->index];
        if (!planet) continue;

        // texture refresh check
        for (int t = 0; t < sbcount(def->sb_textures); t++) {
            _PlanetTextureEntry *tex = &def->sb_textures[t];
            if (tex->fire_refresh == DC_APP_VAL_INDEX_UNDEFINED) continue;
            DcValue *refresh_val = dc_app_lookup_get_value(app_data->lookup, tex->fire_refresh);
            if (!dc_value_is_equal(refresh_val, &tex->last_fire_refresh_value)) {
                plPlanetTexture texture;
                if (_build_planet_texture(app_data, tex, &texture)) {
                    _ext_planet->set_texture(planet, &texture, 0);
                } else {
                    _ext_planet->set_texture(planet, NULL, 0);
                }
                tex->last_fire_refresh_value = *refresh_val;
            }
        }

        // light direction
        if (def->light_direction.x != DC_APP_VAL_INDEX_UNDEFINED ||
            def->light_direction.y != DC_APP_VAL_INDEX_UNDEFINED ||
            def->light_direction.z != DC_APP_VAL_INDEX_UNDEFINED) {
            plPlanetRuntimeOptions opts = _ext_planet->get_runtime_options(planet);
            plVec3 prev = opts.tLightDirection;
            if (def->light_direction.x != DC_APP_VAL_INDEX_UNDEFINED)
                opts.tLightDirection.x = (float)dc_app_lookup_get_value(app_data->lookup, def->light_direction.x)->value_double;
            if (def->light_direction.y != DC_APP_VAL_INDEX_UNDEFINED)
                opts.tLightDirection.y = (float)dc_app_lookup_get_value(app_data->lookup, def->light_direction.y)->value_double;
            if (def->light_direction.z != DC_APP_VAL_INDEX_UNDEFINED)
                opts.tLightDirection.z = (float)dc_app_lookup_get_value(app_data->lookup, def->light_direction.z)->value_double;
            if (prev.x != opts.tLightDirection.x || prev.y != opts.tLightDirection.y || prev.z != opts.tLightDirection.z)
                _ext_planet->set_runtime_options(planet, opts);
        }

        // prepare planet (once per planet per frame)
        plCommandBuffer *cmd_buf = _ext_starter->get_temporary_command_buffer();
        _ext_planet->prepare(planet, cmd_buf);
        _ext_starter->submit_temporary_command_buffer(cmd_buf);
    }
}

static void _load_apis(plApiRegistryI *api_registry) {
    _ext_windows          = pl_get_api_latest(api_registry, plWindowI);
    _ext_dc_draw             = pl_get_api_latest(api_registry, dcDrawI);
    _ext_dc_draw_backend     = pl_get_api_latest(api_registry, dcDrawBackendI);
    _ext_starter          = pl_get_api_latest(api_registry, plStarterI);
    _ext_profile          = pl_get_api_latest(api_registry, plProfileI);
    _ext_memory           = pl_get_api_latest(api_registry, plMemoryI);
    _ext_library          = pl_get_api_latest(api_registry, plLibraryI);
    _ext_ioi              = pl_get_api_latest(api_registry, plIOI);
    _ext_gfx              = pl_get_api_latest(api_registry, plGraphicsI);
    _ext_gpu_allocators   = pl_get_api_latest(api_registry, plGPUAllocatorsI);
    _ext_vfs              = pl_get_api_latest(api_registry, plVfsI);
    _ext_shader           = pl_get_api_latest(api_registry, plShaderI);
    _ext_planet           = pl_get_api_latest(api_registry, plPlanetI);
    _ext_draw          = pl_get_api_latest(api_registry, plDrawI);
    _ext_planet_processor = pl_get_api_latest(api_registry, plPlanetProcessorI);
    _ext_camera           = pl_get_api_latest(api_registry, plCameraI);
    _ext_image            = pl_get_api_latest(api_registry, plImageI);
    _ext_resource         = pl_get_api_latest(api_registry, plResourceI);
    _ext_screen_log       = pl_get_api_latest(api_registry, plScreenLogI);
}

#include "draw.c"
#include "xml.c"
