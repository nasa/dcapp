#ifndef _DCAPP_UTILS_C_
#define _DCAPP_UTILS_C_

#include "dcapp.h"

#define PL_JSON_IMPLEMENTATION
#include "../pilotlight/libs/pl_json.h"

#include "../src/utils/file.h"
#include "../src/utils/log.h"
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
        case NODE_TYPE_PLANET_VIEW:
            return "PlanetView";
        case NODE_TYPE_TEXT:
            return "Text";
        case NODE_TYPE_WINDOW:
            return "Window";
        default:
            DC_LOG_WARN("NodeType", "Unknown type: %d", type);
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

    // stencil create graphics state: increment stencil buffer
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
        .ulStencilOpPass      = PL_STENCIL_OP_INCREMENT_AND_CLAMP
    };

    // stencil remove graphics state: decrement stencil buffer
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
        .ulStencilOpPass      = PL_STENCIL_OP_DECREMENT_AND_CLAMP
    };

    // stencil cleanup graphics state: decrement stencil buffer (same as remove but separate pipeline)
    const plGraphicsState stencil_cleanup_state = {
        .ulDepthWriteEnabled  = 0,
        .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
        .ulCullMode           = PL_CULL_MODE_NONE,
        .ulStencilTestEnabled = 1,
        .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
        .ulStencilRef         = 0,
        .ulStencilMask        = 0xFF,
        .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
        .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
        .ulStencilOpPass      = PL_STENCIL_OP_DECREMENT_AND_CLAMP
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

    // Stencil cleanup 2D (decrement stencil, no color write)
    const plShaderDesc stencil_cleanup_2d_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_2d_stencil,
        .tGraphicsState        = stencil_cleanup_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_cleanup_2d_shader = _ext_gfx->create_shader(device, &stencil_cleanup_2d_desc);

    // Stencil draw 2D (one per depth level)
    for (int i = 0; i < DC_STENCIL_MAX_DEPTH; i++) {
        const plGraphicsState stencil_draw_state = {
            .ulDepthWriteEnabled  = 0,
            .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
            .ulCullMode           = PL_CULL_MODE_NONE,
            .ulStencilTestEnabled = 1,
            .ulStencilMode        = PL_COMPARE_MODE_LESS_OR_EQUAL,
            .ulStencilRef         = i + 1,
            .ulStencilMask        = 0xFF,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP
        };
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
        app_data->stencil_draw_2d_shader[i] = _ext_gfx->create_shader(device, &stencil_draw_2d_desc);
    }

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

    // Stencil cleanup SDF (decrement stencil, no color write)
    const plShaderDesc stencil_cleanup_sdf_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_sdf_stencil,
        .tGraphicsState        = stencil_cleanup_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_cleanup_sdf_shader = _ext_gfx->create_shader(device, &stencil_cleanup_sdf_desc);

    // Stencil draw SDF (one per depth level)
    for (int i = 0; i < DC_STENCIL_MAX_DEPTH; i++) {
        const plGraphicsState stencil_draw_state = {
            .ulDepthWriteEnabled  = 0,
            .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
            .ulCullMode           = PL_CULL_MODE_NONE,
            .ulStencilTestEnabled = 1,
            .ulStencilMode        = PL_COMPARE_MODE_LESS_OR_EQUAL,
            .ulStencilRef         = i + 1,
            .ulStencilMask        = 0xFF,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP
        };
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
        app_data->stencil_draw_sdf_shader[i] = _ext_gfx->create_shader(device, &stencil_draw_sdf_desc);
    }

    //-------------------------------------------------------------------------
    // 3D solid stencil shaders
    //-------------------------------------------------------------------------

    // 3D solid vertex buffer layout (pos3 + color)
    const plVertexBufferLayout vertex_layout_3d_solid = {
        .uByteStride = sizeof(float) * 4,
        .atAttributes = {
            {.uByteOffset = 0,                 .tFormat = PL_VERTEX_FORMAT_FLOAT3},
            {.uByteOffset = sizeof(float) * 3, .tFormat = PL_VERTEX_FORMAT_UINT},
        }
    };

    // load 3D shaders
    plShaderModule vert_3d          = _ext_shader->load_glsl("dc_draw_3d.vert", "main", NULL, NULL);
    plShaderModule frag_3d          = _ext_shader->load_glsl("dc_draw_3d.frag", "main", NULL, NULL);
    plShaderModule vert_3d_textured = _ext_shader->load_glsl("dc_draw_3d_textured.vert", "main", NULL, NULL);
    plShaderModule frag_3d_textured = _ext_shader->load_glsl("dc_draw_3d_textured.frag", "main", NULL, NULL);

    // Stencil create 3D solid (increment stencil, no color write)
    const plShaderDesc stencil_create_3d_solid_desc = {
        .tVertexShader         = vert_3d,
        .tFragmentShader       = frag_3d,
        .tGraphicsState        = stencil_create_state,
        .atVertexBufferLayouts = { vertex_layout_3d_solid },
        .atBlendStates         = { stencil_only_blend },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_create_3d_solid_shader = _ext_gfx->create_shader(device, &stencil_create_3d_solid_desc);

    // Stencil remove 3D solid (decrement stencil, no color write)
    const plShaderDesc stencil_remove_3d_solid_desc = {
        .tVertexShader         = vert_3d,
        .tFragmentShader       = frag_3d,
        .tGraphicsState        = stencil_remove_state,
        .atVertexBufferLayouts = { vertex_layout_3d_solid },
        .atBlendStates         = { stencil_only_blend },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_remove_3d_solid_shader = _ext_gfx->create_shader(device, &stencil_remove_3d_solid_desc);

    // Stencil cleanup 3D solid (decrement stencil, no color write)
    const plShaderDesc stencil_cleanup_3d_solid_desc = {
        .tVertexShader         = vert_3d,
        .tFragmentShader       = frag_3d,
        .tGraphicsState        = stencil_cleanup_state,
        .atVertexBufferLayouts = { vertex_layout_3d_solid },
        .atBlendStates         = { stencil_only_blend },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_cleanup_3d_solid_shader = _ext_gfx->create_shader(device, &stencil_cleanup_3d_solid_desc);

    // Stencil draw 3D solid (one per depth level, with depth test)
    for (int i = 0; i < DC_STENCIL_MAX_DEPTH; i++) {
        const plGraphicsState stencil_draw_3d_state = {
            .ulDepthWriteEnabled  = 1,
            .ulDepthMode          = PL_COMPARE_MODE_LESS,
            .ulCullMode           = PL_CULL_MODE_NONE,
            .ulStencilTestEnabled = 1,
            .ulStencilMode        = PL_COMPARE_MODE_LESS_OR_EQUAL,
            .ulStencilRef         = i + 1,
            .ulStencilMask        = 0xFF,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP
        };
        const plShaderDesc stencil_draw_3d_solid_desc = {
            .tVertexShader         = vert_3d,
            .tFragmentShader       = frag_3d,
            .tGraphicsState        = stencil_draw_3d_state,
            .atVertexBufferLayouts = { vertex_layout_3d_solid },
            .atBlendStates         = { alpha_blend },
            .tRenderPassLayout     = render_pass_layout,
            .uSubpassIndex         = 0,
            .tMSAASampleCount      = sample_count
        };
        app_data->stencil_draw_3d_solid_shader[i] = _ext_gfx->create_shader(device, &stencil_draw_3d_solid_desc);
    }

    //-------------------------------------------------------------------------
    // 3D textured stencil shaders
    //-------------------------------------------------------------------------

    // 3D textured vertex buffer layout (pos3 + uv2 + color)
    const plVertexBufferLayout vertex_layout_3d_textured = {
        .uByteStride = sizeof(float) * 6,
        .atAttributes = {
            {.uByteOffset = 0,                 .tFormat = PL_VERTEX_FORMAT_FLOAT3},
            {.uByteOffset = sizeof(float) * 3, .tFormat = PL_VERTEX_FORMAT_FLOAT2},
            {.uByteOffset = sizeof(float) * 5, .tFormat = PL_VERTEX_FORMAT_UINT},
        }
    };

    // Stencil create 3D textured (increment stencil, no color write)
    const plShaderDesc stencil_create_3d_textured_desc = {
        .tVertexShader         = vert_3d_textured,
        .tFragmentShader       = frag_3d_textured,
        .tGraphicsState        = stencil_create_state,
        .atVertexBufferLayouts = { vertex_layout_3d_textured },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_create_3d_textured_shader = _ext_gfx->create_shader(device, &stencil_create_3d_textured_desc);

    // Stencil remove 3D textured (decrement stencil, no color write)
    const plShaderDesc stencil_remove_3d_textured_desc = {
        .tVertexShader         = vert_3d_textured,
        .tFragmentShader       = frag_3d_textured,
        .tGraphicsState        = stencil_remove_state,
        .atVertexBufferLayouts = { vertex_layout_3d_textured },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_remove_3d_textured_shader = _ext_gfx->create_shader(device, &stencil_remove_3d_textured_desc);

    // Stencil cleanup 3D textured (decrement stencil, no color write)
    const plShaderDesc stencil_cleanup_3d_textured_desc = {
        .tVertexShader         = vert_3d_textured,
        .tFragmentShader       = frag_3d_textured,
        .tGraphicsState        = stencil_cleanup_state,
        .atVertexBufferLayouts = { vertex_layout_3d_textured },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_cleanup_3d_textured_shader = _ext_gfx->create_shader(device, &stencil_cleanup_3d_textured_desc);

    // Stencil draw 3D textured (one per depth level, with depth test)
    for (int i = 0; i < DC_STENCIL_MAX_DEPTH; i++) {
        const plGraphicsState stencil_draw_3d_state = {
            .ulDepthWriteEnabled  = 1,
            .ulDepthMode          = PL_COMPARE_MODE_LESS,
            .ulCullMode           = PL_CULL_MODE_NONE,
            .ulStencilTestEnabled = 1,
            .ulStencilMode        = PL_COMPARE_MODE_LESS_OR_EQUAL,
            .ulStencilRef         = i + 1,
            .ulStencilMask        = 0xFF,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP
        };
        const plShaderDesc stencil_draw_3d_textured_desc = {
            .tVertexShader         = vert_3d_textured,
            .tFragmentShader       = frag_3d_textured,
            .tGraphicsState        = stencil_draw_3d_state,
            .atVertexBufferLayouts = { vertex_layout_3d_textured },
            .atBlendStates         = { alpha_blend },
            .atBindGroupLayouts    = { sampler_layout, texture_layout },
            .tRenderPassLayout     = render_pass_layout,
            .uSubpassIndex         = 0,
            .tMSAASampleCount      = sample_count
        };
        app_data->stencil_draw_3d_textured_shader[i] = _ext_gfx->create_shader(device, &stencil_draw_3d_textured_desc);
    }
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
    planet_ext_init.ptDevice = _ext_starter->get_device();
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
        def->radius = radius;

        // extract tiles array
        uint32_t tile_count = 0;
        plJsonObject *tile_array = pl_json_array_member(root, "tiles", &tile_count);

        // build process info
        plPlanetProcessInfo process_info = {0};
        process_info.fRadius          = (float)radius;
        process_info.fMetersPerPixel  = meters_per_pixel;
        process_info.uSize            = (uint32_t)tile_size;
        process_info.uTileCount       = tile_count;
        process_info.uHorizontalTiles = (uint32_t)cols;
        process_info.uVerticalTiles   = (uint32_t)rows;
        process_info.atTiles          = (plPlanetProcessTileInfo *)PL_ALLOC(tile_count * sizeof(plPlanetProcessTileInfo));

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
        planet_init.dRadius    = radius;
        planet_init.tLoadFlags = PL_PLANET_LOAD_FLAGS_NONE;

        // create planet
        plCommandBuffer *cmd_buf = _ext_starter->get_temporary_command_buffer();
        plPlanet *planet = _ext_planet->create_planet(cmd_buf, planet_init, &process_info);
        _ext_starter->submit_temporary_command_buffer(cmd_buf);

        // initial texture overlay (if source is set at parse time)
        if (sbcount(def->sb_textures) > 0) {
            plPlanetTexture texture;
            if (_build_planet_texture(app_data, &def->sb_textures[0], &texture)) {
                DC_LOG_INFO("Planet", "  [%d] texture: %s (mpp=%.1f, lat=%.1f, lon=%.1f)",
                    i, texture.pcPath, texture.fMetersPerPixel, texture.fLatitude, texture.fLongitude);
                _ext_planet->set_texture(planet, &texture);
            }
        }

        // store planet
        sbpush(app_data->sb_planets, planet);
        def->index = (uint8_t)(sbcount(app_data->sb_planets) - 1); // index 0 is sentinel

        // force shader mismatch on first update
        if (def->shader_index != DC_APP_VAL_INDEX_UNDEFINED) {
            int initial = (int)dc_app_lookup_get_value(app_data->lookup, def->shader_index)->value_integer;
            def->active_shader_index = initial + 1;
        }

        DC_LOG_INFO("Planet", "  [%d] '%s' created (radius=%.0f, %u tiles)", i, def->name, radius, tile_count);

        // cleanup
        PL_FREE(process_info.atTiles);
        pl_unload_json(&root);
        free(json_str);
    }

    // Phase 2: create views from PlanetView nodes
    for (int i = 0; i < view_count; i++) {
        _NodeIndex node_index = app_data->sb_planet_view_node_indices[i];
        _Node *node = _get_node(app_data, node_index);

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
        view_init.uOutputWidth  = (uint32_t)output_width;
        view_init.uOutputHeight = (uint32_t)output_height;

        plCommandBuffer *cmd_buf = _ext_starter->get_temporary_command_buffer();
        plPlanetView *view = _ext_planet->create_view(planet, cmd_buf, view_init);
        _ext_starter->submit_temporary_command_buffer(cmd_buf);

        sbpush(app_data->sb_planet_views, view);
        node->planet_view.planet_view_index = (uint8_t)(sbcount(app_data->sb_planet_views) - 1); // index 0 is sentinel

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

        // shader swap check
        if (def->shader_index != DC_APP_VAL_INDEX_UNDEFINED && sbcount(def->sb_shaders) > 0) {
            int desired_idx = (int)dc_app_lookup_get_value(app_data->lookup, def->shader_index)->value_integer;
            if (desired_idx != def->active_shader_index) {
                _PlanetShaderEntry *found = NULL;
                for (int j = 0; j < sbcount(def->sb_shaders); j++) {
                    if (def->sb_shaders[j].index == desired_idx) {
                        found = &def->sb_shaders[j];
                        break;
                    }
                }
                _ext_planet->set_shaders(planet, found ? found->vertex_path : NULL, found ? found->fragment_path : NULL);
                def->active_shader_index = desired_idx;
            }
        }

        // texture refresh check
        for (int t = 0; t < sbcount(def->sb_textures); t++) {
            _PlanetTextureEntry *tex = &def->sb_textures[t];
            if (tex->fire_refresh == DC_APP_VAL_INDEX_UNDEFINED) continue;
            DcValue *refresh_val = dc_app_lookup_get_value(app_data->lookup, tex->fire_refresh);
            if (!dc_value_is_equal(refresh_val, &tex->last_fire_refresh_value)) {
                plPlanetTexture texture;
                if (_build_planet_texture(app_data, tex, &texture)) {
                    _ext_planet->set_texture(planet, &texture);
                } else {
                    _ext_planet->set_texture(planet, NULL);
                }
                tex->last_fire_refresh_value = *refresh_val;
            }
        }

        // prepare planet (once per planet per frame)
        plCommandBuffer *cmd_buf = _ext_starter->get_temporary_command_buffer();
        _ext_planet->prepare(planet, cmd_buf);
        _ext_starter->submit_temporary_command_buffer(cmd_buf);
    }
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
