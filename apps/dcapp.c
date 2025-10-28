#include "dcapp.h"

#include "../src/utils/file.h"
#include "../src/utils/env.h"

PL_EXPORT void *pl_app_load(plApiRegistryI *api_registry, _PlAppData *pl_app_data) {

    if (pl_app_data) {
        _ext_windows      = pl_get_api_latest(api_registry, plWindowI);
        _ext_draw         = pl_get_api_latest(api_registry, plDrawI);
        _ext_draw_backend = pl_get_api_latest(api_registry, plDrawBackendI);
        _ext_starter      = pl_get_api_latest(api_registry, plStarterI);
        _ext_profile      = pl_get_api_latest(api_registry, plProfileI);
        _ext_memory       = pl_get_api_latest(api_registry, plMemoryI);
        _ext_library      = pl_get_api_latest(api_registry, plLibraryI);
        _ext_ioi          = pl_get_api_latest(api_registry, plIOI);
        _ext_gfx          = pl_get_api_latest(api_registry, plGraphicsI);
        _ext_vfs          = pl_get_api_latest(api_registry, plVfsI);
        _ext_shader       = pl_get_api_latest(api_registry, plShaderI);
        _ext_terrain      = pl_get_api_latest(api_registry, plTerrainI);
        _ext_camera       = pl_get_api_latest(api_registry, plCameraI);
        _ext_image        = pl_get_api_latest(api_registry, plImageI);
        return pl_app_data;
    }

    // retrieve extension registry
    const plExtensionRegistryI *extension_registry = pl_get_api_latest(api_registry, plExtensionRegistryI);

    // load required extensions
    extension_registry->load("pl_unity_ext", NULL, NULL, true);
    extension_registry->load("pl_platform_ext", NULL, NULL, false);
    extension_registry->load("pl_terrain_ext", NULL, NULL, true);

    // load extensions
    _ext_windows      = pl_get_api_latest(api_registry, plWindowI);
    _ext_draw         = pl_get_api_latest(api_registry, plDrawI);
    _ext_draw_backend = pl_get_api_latest(api_registry, plDrawBackendI);
    _ext_starter      = pl_get_api_latest(api_registry, plStarterI);
    _ext_profile      = pl_get_api_latest(api_registry, plProfileI);
    _ext_memory       = pl_get_api_latest(api_registry, plMemoryI);
    _ext_library      = pl_get_api_latest(api_registry, plLibraryI);
    _ext_ioi          = pl_get_api_latest(api_registry, plIOI);
    _ext_gfx          = pl_get_api_latest(api_registry, plGraphicsI);
    _ext_vfs          = pl_get_api_latest(api_registry, plVfsI);
    _ext_shader       = pl_get_api_latest(api_registry, plShaderI);
    _ext_terrain      = pl_get_api_latest(api_registry, plTerrainI);
    _ext_camera       = pl_get_api_latest(api_registry, plCameraI);
    _ext_image        = pl_get_api_latest(api_registry, plImageI);

    // allocate app memory
    pl_app_data = (_PlAppData *)PL_ALLOC(sizeof(_PlAppData));
    memset(pl_app_data, 0, sizeof(_PlAppData));

    // parse input arguments
    plIO *_ext_io = _ext_ioi->get_io();
    if (_ext_io->iArgc < 4) {
        fprintf(stderr, "DCApp pl_app_load(): missing dcapp config file\n");
    }

    // TODO process input arguments (constant setting)
    // std::vector<std::string> args(_ext_io->apArgv + 3, _ext_io->apArgv + _ext_io->iArgc);

    // create config
    const char *config_filepath = _ext_io->apArgv[3];
   _dc_data.config                 = dc_app_config_create(config_filepath);

    // create lookup
   _dc_data.lookup = dc_app_lookup_create();

    // set environment (used for dcapp XMLs)
    dc_utils_set_env("dcappDisplayHome",_dc_data.config->config_dir_path, 1);

    // clean XML file
    dc_app_config_clean_xml(_dc_data.config, _dc_data.lookup);

    // save to file
    char log_file[256];
    dc_utils_join_paths(_dc_data.config->cache_dir_path, "xml.log", log_file, sizeof(log_file));
    dc_app_config_save_to_file(_dc_data.config, log_file);

    // build dcapp node tree
    xmlNodePtr root_node = xmlDocGetRootElement(_dc_data.config->xml_doc);
    _process_xml_node(pl_app_data, root_node, NODE_INDEX_UNDEFINED, DC_APP_ELEM_TYPE_UNDEFINED, _dc_data.config->config_dir_path);

    // return app memory
    return pl_app_data;
}

PL_EXPORT void pl_app_shutdown(_PlAppData *pl_app_data) {

    // ensure device is done with resources
    plDevice *device = _ext_starter->get_device();
    _ext_gfx->flush_device(device); // waits for the GPU to be done with all work

    // cleans up texture and other resources
    _ext_draw_backend->cleanup_font_atlas(_ext_draw->get_current_font_atlas());

    _ext_starter->cleanup();
    _ext_windows->destroy(pl_app_data->window);
    PL_FREE(pl_app_data);
}

PL_EXPORT void pl_app_resize(_PlAppData *pl_app_data) {
    _ext_starter->resize();
}

PL_EXPORT void pl_app_update(_PlAppData *pl_app_data) {
    // this needs to be the first call when using the starter
    // extension. You must return if it returns false (usually a swapchain recreation).
    if (!_ext_starter->begin_frame()) {
        return;
    }

    _frame_data.count++;

    // get mouse button status
    if (_ext_ioi->is_mouse_down(PL_MOUSE_BUTTON_LEFT)) {
        _frame_data.is_mouse_pressed  = !_frame_data.is_mouse_down;
        _frame_data.is_mouse_released = false;
        _frame_data.is_mouse_down     = true;
    } else {
        _frame_data.is_mouse_pressed  = false;
        _frame_data.is_mouse_released = _frame_data.is_mouse_down;
        _frame_data.is_mouse_down     = false;
    }

    // get mouse position
    _frame_data.mouse_position = _ext_ioi->get_mouse_pos();

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~drawing & profile API~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _ext_draw_backend->new_frame();

    // send trick data
    for (int ii = 0; ii < sbcount(_dc_data.sb_tricks); ii++) {

        _TrickContext *trick_context = &(_dc_data.sb_tricks[ii]);
        DcTrick       *trick         = trick_context->trick;

        // add tx commands to buffer
        if (trick->is_connected) {
            for (int jj = 0; jj < sbcount(trick_context->sb_tx_var_contexts); jj++) {

                _TrickTxVarContext *tx_var_context = &(trick_context->sb_tx_var_contexts[jj]);
                DcAppLookupVar     *dc_app_var     = dc_app_lookup_get_var(_dc_data.lookup, tx_var_context->dcapp_var_index);
                DcValue            *curr_value     = dc_app_lookup_get_value(_dc_data.lookup, dc_app_var->value_index);
                DcValue            *prev_value     = &tx_var_context->prev_value;

                // send if new value is different
                if (!dc_value_is_equal(curr_value, prev_value)) {
                    dc_trick_set_tx_var(trick, tx_var_context->trick_var_index, curr_value->value_string);
                    dc_value_copy(prev_value, curr_value);
                }
            }
        }

        // send the updated buffer, receive the new data, update the connection status
        dc_trick_update(trick);

        // receive the new data
        if (trick->has_new_data && trick->is_connected) {
            char rx_buffer[256];
            for (int jj = 0; jj < sbcount(trick_context->sb_rx_var_contexts); jj++) {

                _TrickRxVarContext *rx_var_context = &(trick_context->sb_rx_var_contexts[jj]);
                DcAppLookupVar     *dc_app_var     = dc_app_lookup_get_var(_dc_data.lookup, rx_var_context->dcapp_var_index);
                DcValue            *value          = dc_app_lookup_get_value(_dc_data.lookup, dc_app_var->value_index);

                dc_trick_get_rx_var_value(trick, rx_var_context->trick_var_index, rx_buffer);
                dc_app_lookup_set_var_to_string(_dc_data.lookup, rx_var_context->dcapp_var_index, rx_buffer);
            }
        }
    }

    // process logic, update vars from extern_data
    if (_dc_data.logic_draw) {
        _dc_data.logic_draw();
        for (int var_index = 0; var_index < dc_app_lookup_get_var_count(_dc_data.lookup); var_index++) {
            dc_app_lookup_refresh_var_from_extern(_dc_data.lookup, var_index);
        }
    }
    // double lat   = dc_utils_degrees_to_radians(dc_app_lookup_get_value(_dc_data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["LATITUDE_deg"]])->value_index)->value_double);
    // double lon   = dc_utils_degrees_to_radians(dc_app_lookup_get_value(_dc_data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["LONGITUDE_deg"]])->value_index)->value_double);
    // double alt   = dc_app_lookup_get_value(_dc_data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["ALTITUDE_m"]])->value_index)->value_double;
    // double roll  = dc_utils_degrees_to_radians(dc_app_lookup_get_value(_dc_data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["ROLL_deg"]])->value_index)->value_double);
    // double pitch = dc_utils_degrees_to_radians(dc_app_lookup_get_value(_dc_data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["PITCH_deg"]])->value_index)->value_double);
    // double yaw   = dc_utils_degrees_to_radians(dc_app_lookup_get_value(_dc_data.lookup, (&dc_app_data.vars[dc_app_data.var_indices["YAW_deg"]])->value_index)->value_double);

    // // calculate x, y, z
    // //  r = 2 * R * tan(pi/4 - lat/2)
    // //  theta = lon
    // //  x = r * sin(theta)
    // //  y = altitude
    // //  z = r * cos(theta)
    // double r = 2 * 1737400 * tan(M_PI_4 - fabs(lat) / 2);
    // double x = r * sin(lon);
    // double y = alt;
    // double z = r * cos(lon);

    // // set rotation
    // pitch = 270.0;
    // roll = 0.0;
    // yaw = 0.0;

    // // process camera
    // // ext_terrain->set_camera_pos(app_data->terrain, x, y, z);
    // ext_terrain->set_camera_orientation(app_data->terrain, pitch, yaw, roll);

    // start terrain rendering
    // plCommandBuffer *cmd_buffer = ext_starter->get_command_buffer();
    // ext_terrain->render(app_data->terrain, cmd_buffer);
    // ext_starter->submit_command_buffer(cmd_buffer);
    // ext_draw->add_image(app_data->layer, ext_terrain->get_terrain_texture(app_data->terrain).uIndex, {0.0f, 0.0f}, {1000.0f, 1000.0f});

    // draw node
    _draw_node(pl_app_data, _dc_data.window, NULL, NULL, NULL);

    // submit draw layer
    _ext_draw->submit_2d_layer(pl_app_data->layer);

    // start main pass & return the encoder being used
    plRenderEncoder *encoder = _ext_starter->begin_main_pass();

    // submit our 2d drawlist
    plIO *ptIO = _ext_ioi->get_io();
    _ext_draw_backend->submit_2d_drawlist(pl_app_data->draw_list, encoder, ptIO->tMainViewportSize.x, ptIO->tMainViewportSize.y, _ext_gfx->get_swapchain_info(_ext_starter->get_swapchain()).tSampleCount);

    _ext_starter->end_main_pass();

    // must be the last function called when using the starter extension
    _ext_starter->end_frame();

    // update node states
    _frame_data.pressed_node = _frame_data.next_pressed_node;
    _frame_data.hovered_node = _frame_data.next_hovered_node;
    if (_frame_data.is_mouse_pressed) {
        _frame_data.active_node = _frame_data.next_pressed_node;
    }
    if (_frame_data.is_mouse_released) {
        _frame_data.released_node = _frame_data.active_node;
        _frame_data.active_node   = NODE_INDEX_UNDEFINED;
    } else {
        _frame_data.released_node = NODE_INDEX_UNDEFINED;
    }
    _frame_data.next_hovered_node = NODE_INDEX_UNDEFINED;
    _frame_data.next_pressed_node = NODE_INDEX_UNDEFINED;
}

#include "_dcapp_draw.c"
#include "_dcapp_process_xml.c"
#include "_dcapp_utils.c"