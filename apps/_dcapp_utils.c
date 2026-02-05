#ifndef _DCAPP_UTILS_C_
#define _DCAPP_UTILS_C_

#include "dcapp.h"

#include "../src/utils/string.h"

static const char *_node_type_to_string(_NodeType type) {
    switch (type) {
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

static _Node *_get_node(_AppData *app_data, _NodeIndex index) {
    if (index == NODE_INDEX_UNDEFINED) {
        return NULL;
    }
    return &(app_data->sb_nodes[index]);
}

static _NodeIndex _register_node(_AppData *app_data, _Node *node) {
    sbpush(app_data->sb_nodes, *node);
    return sbcount(app_data->sb_nodes) - 1;
}

static bool _load_color_from_string(_AppData *app_data, xmlNodePtr xml_node, const char *attr_name, _ValIndex4 *color_out) {

    xmlChar *raw_color = xmlGetProp(xml_node, BAD_CAST attr_name);
    if (raw_color) {

        // clean raw string
        char cleaned_color[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned_color, (const char *)(const char *)raw_color, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_color);

        // split by whitespace
        // assume no more than 20 splits
        size_t       index_buffer[20];
        size_t       index_count;
        dc_utils_split_string_inplace(cleaned_color, dc_utils_whitespace, index_buffer, 20, &index_count);

        // if empty, assume no color
        if (index_count == 0) {
            return false;
        }

        // process each color
        if (index_count > 0) {
            color_out->r = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[0]]));
        } else {
            color_out->r = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 1) {
            color_out->g = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[1]]));
        } else {
            color_out->g = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 2) {
            color_out->b = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[2]]));
        } else {
            color_out->b = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 3) {
            color_out->a = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[3]]));
        } else {
            color_out->a = DC_APP_VAL_INDEX_UNDEFINED;
        }

        return true;
    } else {
        return false;
    }
}

static void _init_stencil_pipelines(_AppData* app_data, plDevice* device, plRenderPassHandle render_pass)
{
    plRenderPassLayoutHandle render_pass_layout = _ext_gfx->get_render_pass(device, render_pass)->tDesc.tLayout;
    uint32_t sample_count = _ext_gfx->get_swapchain_info(_ext_starter->get_swapchain()).tSampleCount;

    // common vertex buffer layout for 2D drawing
    const plVertexBufferLayout vertex_layout = {
        .uByteStride = sizeof(float) * 5,
        .atAttributes = {
            {.uByteOffset = 0,                 .tFormat = PL_VERTEX_FORMAT_FLOAT2},
            {.uByteOffset = sizeof(float) * 2, .tFormat = PL_VERTEX_FORMAT_FLOAT2},
            {.uByteOffset = sizeof(float) * 4, .tFormat = PL_VERTEX_FORMAT_UINT},
        }
    };

    // common bind group layouts
    const plBindGroupLayoutDesc sampler_layout = {
        .atSamplerBindings = {
            {.uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT}
        }
    };
    const plBindGroupLayoutDesc texture_layout = {
        .atTextureBindings = {
            {.uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED}
        }
    };

    // stencil create graphics state: write 1 to stencil buffer
    const plGraphicsState stencil_create_state = {
        .ulDepthWriteEnabled  = 0,
        .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
        .ulCullMode           = PL_CULL_MODE_NONE,
        .ulStencilTestEnabled = 1,
        .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
        .ulStencilRef         = 1,
        .ulStencilMask        = 0xFF,
        .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
        .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
        .ulStencilOpPass      = PL_STENCIL_OP_REPLACE
    };

    // stencil remove graphics state: write 0 to stencil buffer
    const plGraphicsState stencil_remove_state = {
        .ulDepthWriteEnabled  = 0,
        .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
        .ulCullMode           = PL_CULL_MODE_NONE,
        .ulStencilTestEnabled = 1,
        .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
        .ulStencilRef         = 0,
        .ulStencilMask        = 0xFF,
        .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
        .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
        .ulStencilOpPass      = PL_STENCIL_OP_ZERO
    };

    // stencil draw graphics state: only draw where stencil == 1
    const plGraphicsState stencil_draw_state = {
        .ulDepthWriteEnabled  = 0,
        .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
        .ulCullMode           = PL_CULL_MODE_NONE,
        .ulStencilTestEnabled = 1,
        .ulStencilMode        = PL_COMPARE_MODE_EQUAL,
        .ulStencilRef         = 1,
        .ulStencilMask        = 0xFF,
        .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
        .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
        .ulStencilOpPass      = PL_STENCIL_OP_KEEP
    };

    // blend states
    const plBlendState stencil_only_blend = {
        .bBlendEnabled   = false,
        .uColorWriteMask = PL_COLOR_WRITE_MASK_NONE  // don't write to color buffer
    };
    const plBlendState alpha_blend = {
        .bBlendEnabled   = true,
        .uColorWriteMask = PL_COLOR_WRITE_MASK_ALL,
        .tSrcColorFactor = PL_BLEND_FACTOR_SRC_ALPHA,
        .tDstColorFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .tColorOp        = PL_BLEND_OP_ADD,
        .tSrcAlphaFactor = PL_BLEND_FACTOR_SRC_ALPHA,
        .tDstAlphaFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .tAlphaOp        = PL_BLEND_OP_ADD
    };

    // load shaders (pilotlight draw shaders)
    plShaderModule vert_2d  = _ext_shader->load_glsl("draw_2d.vert", "main", NULL, NULL);
    plShaderModule frag_2d  = _ext_shader->load_glsl("draw_2d.frag", "main", NULL, NULL);
    plShaderModule frag_sdf = _ext_shader->load_glsl("draw_2d_sdf.frag", "main", NULL, NULL);

    // load stencil shaders (dcapp-specific, use discard for transparent pixels)
    plShaderModule frag_2d_stencil  = _ext_shader->load_glsl("dc_draw_2d_stencil.frag", "main", NULL, NULL);
    plShaderModule frag_sdf_stencil = _ext_shader->load_glsl("dc_draw_2d_sdf_stencil.frag", "main", NULL, NULL);

    //-------------------------------------------------------------------------
    // 2D stencil shaders
    //-------------------------------------------------------------------------

    // Stencil create 2D (uses discard for transparent pixels, colorWriteMask=0)
    const plShaderDesc stencil_create_2d_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_2d_stencil,
        .tGraphicsState        = stencil_create_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_create_2d_shader = _ext_gfx->create_shader(device, &stencil_create_2d_desc);

    // Stencil remove 2D (uses discard for transparent pixels, colorWriteMask=0)
    const plShaderDesc stencil_remove_2d_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_2d_stencil,
        .tGraphicsState        = stencil_remove_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_remove_2d_shader = _ext_gfx->create_shader(device, &stencil_remove_2d_desc);

    // Stencil draw 2D
    const plShaderDesc stencil_draw_2d_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_2d,
        .tGraphicsState        = stencil_draw_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { alpha_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_draw_2d_shader = _ext_gfx->create_shader(device, &stencil_draw_2d_desc);

    //-------------------------------------------------------------------------
    // SDF stencil shaders
    //-------------------------------------------------------------------------

    // Stencil create SDF (uses discard for non-glyph pixels, colorWriteMask=0)
    const plShaderDesc stencil_create_sdf_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_sdf_stencil,
        .tGraphicsState        = stencil_create_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_create_sdf_shader = _ext_gfx->create_shader(device, &stencil_create_sdf_desc);

    // Stencil remove SDF (uses discard for non-glyph pixels, colorWriteMask=0)
    const plShaderDesc stencil_remove_sdf_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_sdf_stencil,
        .tGraphicsState        = stencil_remove_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_remove_sdf_shader = _ext_gfx->create_shader(device, &stencil_remove_sdf_desc);

    // Stencil draw SDF
    const plShaderDesc stencil_draw_sdf_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_sdf,
        .tGraphicsState        = stencil_draw_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { alpha_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_draw_sdf_shader = _ext_gfx->create_shader(device, &stencil_draw_sdf_desc);
}

static void _init_app_data(_AppData *app_data, _Node *window_node) {

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
    window_desc.uWidth       = (uint32_t)window_node->window.init_dimension.x;
    window_desc.uHeight      = (uint32_t)window_node->window.init_dimension.y;
    window_desc.iXPos        = (int)window_node->window.init_position.x;
    window_desc.iYPos        = (int)window_node->window.init_position.y;
    _ext_windows->create(window_desc, &(app_data->pl_window));
    _ext_windows->show(app_data->pl_window);

    // initialize the starter API (handles alot of boilerplate)
    plStarterInit tStarterInit = {
        .tFlags   = PL_STARTER_FLAGS_ALL_EXTENSIONS & (~PL_STARTER_FLAGS_SHADER_EXT) | PL_STARTER_FLAGS_MSAA | PL_STARTER_FLAGS_DEPTH_BUFFER,
        .ptWindow = app_data->pl_window};
    _ext_starter->initialize(tStarterInit);

    // get device
    plDevice *device = _ext_starter->get_device();

    // initialize dc_draw_ext and dc_draw_backend_ext (pl_starter doesn't do this since we use dcDrawI)
    plDrawInit tDrawInit = {0};
    _ext_draw->initialize(&tDrawInit);
    _ext_draw_backend->initialize(device);

    // initialize GPU memory allocators
    app_data->gpu_local_dedicated_allocator  = _ext_gpu_allocators->get_local_dedicated_allocator(device);
    app_data->gpu_local_buddy_allocator      = _ext_gpu_allocators->get_local_buddy_allocator(device);
    app_data->gpu_staging_uncached_allocator = _ext_gpu_allocators->get_staging_uncached_allocator(device);

    // init default staging buffer
    {
        // set size to 10 MB
        app_data->pl_staging_buffer_size = 100 * 1048576;

        // description
        const plBufferDesc staging_buffer_desc = {
            .tUsage      = PL_BUFFER_USAGE_STAGING,
            .szByteSize  = app_data->pl_staging_buffer_size,
            .pcDebugName = "staging buffer"};
        app_data->pl_staging_buffer_handle = _ext_gfx->create_buffer(device, &staging_buffer_desc, NULL);

        // retrieve buffer to get memory allocation requirements
        plBuffer *staging_buffer = _ext_gfx->get_buffer(device, app_data->pl_staging_buffer_handle);

        // allocate memory using staging allocator
        plDeviceMemoryAllocatorI *allocator = app_data->gpu_staging_uncached_allocator;
        const plDeviceMemoryAllocation staging_buffer_allocation = allocator->allocate(
            allocator->ptInst,
            staging_buffer->tMemoryRequirements.uMemoryTypeBits,
            staging_buffer->tMemoryRequirements.ulSize,
            staging_buffer->tMemoryRequirements.ulAlignment,
            "staging buffer memory");

        // bind the buffer to the new memory allocation
        _ext_gfx->bind_buffer_to_memory(device, app_data->pl_staging_buffer_handle, &staging_buffer_allocation);
    }

    // create font atlas
    {
        // create and set font atlas
        plFontAtlas* font_atlas = _ext_draw->create_font_atlas();
        _ext_draw->set_font_atlas(font_atlas);

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

        app_data->pl_vera_sdf_font = _ext_draw->add_font_from_file_ttf(font_atlas, font_config, "../../assets/fonts/bitstream-vera-sans/Vera.ttf");

        // build font atlas (CPU prepare + GPU upload - backend handles prepare internally)
        plCommandBuffer* command_buffer = _ext_gfx->request_command_buffer(_ext_starter->get_current_command_pool(), "dcapp font atlas");
        _ext_draw_backend->build_font_atlas(command_buffer, font_atlas);
        _ext_gfx->wait_on_command_buffer(command_buffer);
        _ext_gfx->return_command_buffer(command_buffer);
    }
    // Note: don't call set_default_font - pl_starter uses plDrawI with its own font atlas,
    // while dcapp uses dcDrawI with a separate font atlas. Let pl_starter create its own default font.

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

    // initialize stencil pipelines for clipping
    _init_stencil_pipelines(app_data, _ext_starter->get_device(), _ext_starter->get_render_pass());

    // register our app drawlist
    app_data->pl_draw_list = _ext_draw->request_2d_drawlist();

    // request layers (allows drawing out of order)
    app_data->pl_layer = _ext_draw->request_2d_layer(app_data->pl_draw_list);

    // initialize frame data
    app_data->frame_data.pressed_node      = NODE_INDEX_UNDEFINED;
    app_data->frame_data.next_pressed_node = NODE_INDEX_UNDEFINED;
    app_data->frame_data.hovered_node      = NODE_INDEX_UNDEFINED;
    app_data->frame_data.next_hovered_node = NODE_INDEX_UNDEFINED;
    app_data->frame_data.released_node     = NODE_INDEX_UNDEFINED;
    app_data->frame_data.active_node       = NODE_INDEX_UNDEFINED;
}

static _Texture _create_texture(_AppData *app_data, uint32_t texture_width, uint32_t texture_height, const char *texture_name) {

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

    // choose allocator based on texture size
    // use buddy allocator for smaller textures, dedicated for large ones
    plDeviceMemoryAllocatorI *allocator = app_data->gpu_local_buddy_allocator;
    if (pl_texture->tMemoryRequirements.ulSize > _ext_gpu_allocators->get_buddy_block_size())
        allocator = app_data->gpu_local_dedicated_allocator;

    const plDeviceMemoryAllocation pl_texture_allocation = allocator->allocate(
        allocator->ptInst,
        pl_texture->tMemoryRequirements.uMemoryTypeBits,
        pl_texture->tMemoryRequirements.ulSize,
        pl_texture->tMemoryRequirements.ulAlignment,
        texture_name);

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
