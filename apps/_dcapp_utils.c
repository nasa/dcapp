#ifndef _DCAPP_UTILS_C_
#define _DCAPP_UTILS_C_

#include "dcapp.h"

#include "../src/utils/string.h"

static const char *_node_type_to_string(_NodeType type) {
    switch (type) {
        case NODE_TYPE_CIRCLE:
            return "Circle";
        case NODE_TYPE_CONTAINER:
            return "Container";
        case NODE_TYPE_CONDITIONAL:
            return "Conditional";
        case NODE_TYPE_LINE:
            return "Line";
        case NODE_TYPE_PANEL:
            return "Panel";
        case NODE_TYPE_PIXELSTREAM:
            return "PixelStream";
        case NODE_TYPE_POLYGON:
            return "Polygon";
        case NODE_TYPE_RECTANGLE:
            return "Rectangle";
        case NODE_TYPE_SET:
            return "Set";
        case NODE_TYPE_TERRAIN:
            return "Terrain";
        case NODE_TYPE_TEXT:
            return "Text";
        case NODE_TYPE_WINDOW:
            return "Window";
        default:
            fprintf(stderr, "DCAPP _node_type_to_string(): Unknown node type %d\n", type);
            return "";
    }
}

static _Node *_get_node(_NodeIndex index) {
    if (index == NODE_INDEX_UNDEFINED) {
        return NULL;
    }
    return &(_dc_data.sb_nodes[index]);
}

static _NodeIndex _register_node(_Node *node) {
    sbpush(_dc_data.sb_nodes, *node);
    return sbcount(_dc_data.sb_nodes) - 1;
}

static bool _load_color_from_string(xmlNodePtr xml_node, const char *attr_name, _ValIndex4 *color_out) {

    xmlChar *raw_color = xmlGetProp(xml_node, BAD_CAST attr_name);
    if (raw_color) {

        // clean raw string
        char cleaned_color[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned_color, (const char *)(const char *)raw_color, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_color);

        // split by whitespace
        const size_t index_buffer_max = 20;
        size_t       index_buffer[index_buffer_max];
        size_t       index_count;
        dc_utils_split_string_inplace(cleaned_color, dc_utils_whitespace, index_buffer, index_buffer_max, &index_count);

        // if empty, assume no color
        if (index_count == 0) {
            return false;
        }

        // process each color
        if (index_count > 0) {
            color_out->r = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[0]]));
        } else {
            color_out->r = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 1) {
            color_out->g = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[1]]));
        } else {
            color_out->g = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 2) {
            color_out->b = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[2]]));
        } else {
            color_out->b = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 3) {
            color_out->a = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[3]]));
        } else {
            color_out->a = DC_APP_VAL_INDEX_UNDEFINED;
        }

        return true;
    } else {
        return false;
    }
}

static void _init_pl_app_data(_PlAppData *pl_app_data, _Node *window_node) {

    // mount VFS dirs
    _ext_vfs->mount_directory("/shaders-terrain", "../../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/assets", "../../data", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/cache", "cache", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/shaders", "../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/shader-temp", "../shader-temp", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/tiles", "../../data", PL_VFS_MOUNT_FLAGS_NONE);

    // set initial window params
    plWindowDesc window_desc = {};
    window_desc.pcTitle      = window_node->window.title;
    window_desc.uWidth       = (uint32_t)(dc_app_lookup_get_value(_dc_data.lookup, window_node->window.init_dimension.x)->value_integer);
    window_desc.uHeight      = (uint32_t)(dc_app_lookup_get_value(_dc_data.lookup, window_node->window.init_dimension.y)->value_integer);
    window_desc.iXPos        = dc_app_lookup_get_value(_dc_data.lookup, window_node->window.init_position.x)->value_integer;
    window_desc.iYPos        = dc_app_lookup_get_value(_dc_data.lookup, window_node->window.init_position.y)->value_integer;
    _ext_windows->create(window_desc, &(pl_app_data->window));
    _ext_windows->show(pl_app_data->window);

    // initialize the starter API (handles alot of boilerplate)
    plStarterInit tStarterInit = {
        .tFlags   = PL_STARTER_FLAGS_ALL_EXTENSIONS & (~PL_STARTER_FLAGS_DRAW_EXT) & (~PL_STARTER_FLAGS_SHADER_EXT) | PL_STARTER_FLAGS_MSAA,
        .ptWindow = pl_app_data->window};
    _ext_starter->initialize(tStarterInit);

    // init draw extension
    _ext_draw->initialize(NULL);

    // init draw backend
    plDevice *device = _ext_starter->get_device();
    _ext_draw_backend->initialize(device);

    // init default staging buffer
    {
        // set size to 1MB
        pl_app_data->staging_buffer_size = 1048576;

        // description
        const plBufferDesc staging_buffer_desc = {
            .tUsage      = PL_BUFFER_USAGE_STAGING,
            .szByteSize  = pl_app_data->staging_buffer_size,
            .pcDebugName = "staging buffer"};
        pl_app_data->staging_buffer_handle = _ext_gfx->create_buffer(device, &staging_buffer_desc, NULL);

        // retrieve buffer to get memory allocation requirements
        plBuffer *staging_buffer = _ext_gfx->get_buffer(device, pl_app_data->staging_buffer_handle);

        // allocate memory for the vertex buffer
        const plDeviceMemoryAllocation staging_buffer_allocation = _ext_gfx->allocate_memory(
            device,
            staging_buffer->tMemoryRequirements.ulSize,
            PL_MEMORY_FLAGS_HOST_VISIBLE | PL_MEMORY_FLAGS_HOST_COHERENT,
            staging_buffer->tMemoryRequirements.uMemoryTypeBits,
            "staging buffer memory");

        // bind the buffer to the new memory allocation
        _ext_gfx->bind_buffer_to_memory(device, pl_app_data->staging_buffer_handle, &staging_buffer_allocation);
    }

    // create font atlas
    {
        plFontAtlas *pt_atlas = _ext_draw->create_font_atlas();
        _ext_draw->set_font_atlas(pt_atlas);

        // typical font range (you can also add individual characters)
        const plFontRange font_range = {
            .iFirstCodePoint = 0x0020,
            .uCharCount      = 0x00FF - 0x0020};

        // adding previous font but as a signed distance field (SDF)
        plFontConfig font_config   = {};
        font_config.bSdf           = true; // only works with ttf
        font_config.fSize          = 25.0f;
        font_config.uHOverSampling = 1;
        font_config.uVOverSampling = 1;
        font_config.ucOnEdgeValue  = 180;
        font_config.iSdfPadding    = 1;
        font_config.uRangeCount    = 1;
        font_config.ptRanges       = &font_range;

        pl_app_data->cousine_sdf_font = _ext_draw->add_font_from_file_ttf(_ext_draw->get_current_font_atlas(), font_config, "../data/pilotlight-assets-master/fonts/Cousine-Regular.ttf");
    }

    // register our app drawlist
    pl_app_data->draw_list = _ext_draw->request_2d_drawlist();

    // request layers (allows drawing out of order)
    pl_app_data->layer = _ext_draw->request_2d_layer(pl_app_data->draw_list);

    // initialize shader compiler
    plShaderOptions shader_options          = {};
    shader_options.apcIncludeDirectories[0] = "/shaders/";
    shader_options.apcIncludeDirectories[1] = "/shaders-terrain/";
    shader_options.apcDirectories[0]        = "/shaders/";
    shader_options.apcDirectories[1]        = "/shaders-terrain/";
    shader_options.pcCacheOutputDirectory   = "/shader-temp/";
    shader_options.tFlags                   = PL_SHADER_FLAGS_AUTO_OUTPUT | PL_SHADER_FLAGS_INCLUDE_DEBUG | PL_SHADER_FLAGS_ALWAYS_COMPILE;
    _ext_shader->initialize(&shader_options);

    // wraps up
    _ext_starter->finalize();

    // init terrain backend
    // _ext_starter->get_device();
    // _ext_terrain->initialize(_ext_starter->get_device());
    // plCommandBuffer *temp_cmd_buffer = _ext_starter->get_temporary_command_buffer();
    // _ext_terrain->load_mesh(temp_cmd_buffer, "/assets/terrain.bin", 7, 128);

    // plTerrainInit tTerrainInit = {};
    // pl_app_data->terrain          = _ext_terrain->create_terrain_from_file(temp_cmd_buffer, "/assets/moon_terrain.json");
    // _ext_starter->submit_temporary_command_buffer(temp_cmd_buffer);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~font atlas texture~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // draw backend handles creating the font atlas texture and
    // uploading to the GPU but it requires a command buffer (in an non recording state).
    // Later examples will go into command buffers without using the starter ext

    plCommandBuffer *raw_cmd_buffer = _ext_starter->get_raw_command_buffer(); // not recording

    // actually record, submit, & wait
    _ext_draw_backend->build_font_atlas(raw_cmd_buffer, _ext_draw->get_current_font_atlas());

    // return back to the pool
    _ext_starter->return_raw_command_buffer(raw_cmd_buffer);

    // initialize frame data
    _frame_data.pressed_node      = NODE_INDEX_UNDEFINED;
    _frame_data.next_pressed_node = NODE_INDEX_UNDEFINED;
    _frame_data.hovered_node      = NODE_INDEX_UNDEFINED;
    _frame_data.next_hovered_node = NODE_INDEX_UNDEFINED;
    _frame_data.released_node     = NODE_INDEX_UNDEFINED;
    _frame_data.active_node       = NODE_INDEX_UNDEFINED;
}

static _Texture _create_texture(_PlAppData *pl_app_data, uint32_t texture_width, uint32_t texture_height, const char *texture_name) {

    // get device
    plDevice *device = _ext_starter->get_device();

    // create new texture desc
    plTextureDesc pl_texture_desc;
    memset(&pl_texture_desc, 0, sizeof(plTextureDesc));
    pl_texture_desc.tDimensions = (plVec3){(float)texture_width, (float)texture_height, 1.0f};
    pl_texture_desc.tFormat     = PL_FORMAT_R8G8B8A8_UNORM;
    pl_texture_desc.uLayers     = 1;
    pl_texture_desc.uMips       = 1;
    pl_texture_desc.tType       = PL_TEXTURE_TYPE_2D;
    pl_texture_desc.tUsage      = PL_TEXTURE_USAGE_SAMPLED;
    pl_texture_desc.pcDebugName = texture_name;

    // create texture
    plTexture      *pl_texture;
    plTextureHandle pl_texture_handle = _ext_gfx->create_texture(device, &pl_texture_desc, &pl_texture);

    // allocate memory
    const plDeviceMemoryAllocation pl_texture_allocation = _ext_gfx->allocate_memory(device, pl_texture->tMemoryRequirements.ulSize, PL_MEMORY_FLAGS_DEVICE_LOCAL, pl_texture->tMemoryRequirements.uMemoryTypeBits, NULL);

    // bind memory
    _ext_gfx->bind_texture_to_memory(device, pl_texture_handle, &pl_texture_allocation);

    // create bind group
    plBindGroupHandle pl_bind_group_handle = _ext_draw_backend->create_bind_group_for_texture(pl_texture_handle);

    // create _Texture struct
    _Texture texture = {
        pl_texture_handle,
        pl_bind_group_handle};
    return texture;
}

#endif
