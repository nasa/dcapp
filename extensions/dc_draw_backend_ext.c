/*
   dc_draw_backend_ext.c
     * forked from pilotlight (https://github.com/PilotLightTech/pilotlight)
*/

/*
Index of this file:
// [SECTION] includes
// [SECTION] internal structs
// [SECTION] globals
// [SECTION] internal api
// [SECTION] public api implementation
// [SECTION] extension loading
// [SECTION] unity build
*/

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <float.h>
#include "pl.h"
#include "pl_memory.h"
#define PL_MATH_INCLUDE_FUNCTIONS
#include "pl_math.h"

// extensions
#include "pl_graphics_ext.h"
#include "pl_shader_ext.h"
#include "dc_draw_backend_ext.h"
#include "dc_draw_ext.h"
#include "pl_stats_ext.h"
#include "pl_log_ext.h"

#ifdef PL_UNITY_BUILD
    #include "pl_unity_ext.inc"
#else
    static const plMemoryI*  gptMemory = NULL;
    #define PL_ALLOC(x)      gptMemory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
    #define PL_REALLOC(x, y) gptMemory->tracked_realloc((x), (y), __FILE__, __LINE__)
    #define PL_FREE(x)       gptMemory->tracked_realloc((x), 0, __FILE__, __LINE__)

    #ifndef PL_DS_ALLOC
        #define PL_DS_ALLOC(x)                      gptMemory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
        #define PL_DS_ALLOC_INDIRECT(x, FILE, LINE) gptMemory->tracked_realloc(NULL, (x), FILE, LINE)
        #define PL_DS_FREE(x)                       gptMemory->tracked_realloc((x), 0, __FILE__, __LINE__)
    #endif

    static const plGraphicsI* gptGfx    = NULL;
    static const plStatsI*    gptStats  = NULL;
    static const dcDrawI*     gptDraw   = NULL;
    static const plShaderI*   gptShader = NULL;
    static const plLogI*      gptLog    = NULL;
    static const plIOI*       gptIOI    = NULL;
    static plIO*              gptIO     = NULL;
#endif

#include "pl_ds.h"

//-----------------------------------------------------------------------------
// [SECTION] internal structs
//-----------------------------------------------------------------------------

typedef struct _dcPipelineEntry
{
    plRenderPassHandle tRenderPass;
    uint32_t           uMSAASampleCount;
    plShaderHandle     tRegularPipeline;
    plShaderHandle     tSecondaryPipeline;
    dcDrawFlags        tFlags;
    uint32_t           uSubpassIndex;
} dcPipelineEntry;

typedef struct _dcBufferInfo
{
    plBufferHandle tVertexBuffer;
    uint32_t       uVertexBufferSize;
    uint32_t       uVertexBufferOffset;
} dcBufferInfo;

typedef struct _dcDrawBackendContext
{
    plDevice*            ptDevice;
    plTempAllocator      tTempAllocator;
    plSamplerHandle      tFontSampler;
    plSamplerHandle      tNearSampler;
    plBindGroupHandle    tFontSamplerBindGroup;
    plBindGroupHandle    tNearSamplerBindGroup;
    plBindGroupHandle    tCurrentSamplerBindGroup;
    dcPipelineEntry*     sbt3dPipelineEntries;
    dcPipelineEntry*     sbt3dTexturedPipelineEntries;
    dcPipelineEntry*     sbt2dPipelineEntries;
    plBindGroupPool*     ptBindGroupPool;

    // bind group layouts
    plBindGroupLayoutHandle tSamplerBindGroupLayout;
    plBindGroupLayoutHandle tTextureBindGroupLayout;

    // shared resources
    plBufferHandle atIndexBuffer[PL_MAX_FRAMES_IN_FLIGHT];
    uint32_t       auIndexBufferSize[PL_MAX_FRAMES_IN_FLIGHT];
    uint32_t       auIndexBufferOffset[PL_MAX_FRAMES_IN_FLIGHT];

    dcBufferInfo atBufferInfo[PL_MAX_FRAMES_IN_FLIGHT];
    dcBufferInfo at3DBufferInfo[PL_MAX_FRAMES_IN_FLIGHT];
    dcBufferInfo at3DTexturedBufferInfo[PL_MAX_FRAMES_IN_FLIGHT];
    dcBufferInfo atLineBufferInfo[PL_MAX_FRAMES_IN_FLIGHT];

    // dynamic buffer system
    plDynamicDataBlock tCurrentDynamicDataBlock;

    // current shader overrides (set via callback during submit)
    plShaderHandle* pt2dShaderOverride;
    plShaderHandle* ptSdfShaderOverride;
    bool            bCustomShaderActive;

    // current 3D shader overrides (set via callback during submit)
    plShaderHandle* pt3dSolidShaderOverride;
    plShaderHandle* pt3dTexturedShaderOverride;
    bool            bCustom3DShaderActive;
} dcDrawBackendContext;

//-----------------------------------------------------------------------------
// [SECTION] global data
//-----------------------------------------------------------------------------

static dcDrawBackendContext* gptDrawBackendCtx = NULL;
static uint64_t uLogChannelDrawBackend = UINT64_MAX;

//-----------------------------------------------------------------------------
// [SECTION] internal api
//-----------------------------------------------------------------------------

static plBufferHandle         pl__create_staging_buffer(const plBufferDesc*, const char* pcName, uint32_t uIdentifier);
static const dcPipelineEntry* pl__get_3d_pipeline              (plRenderPassHandle, uint32_t uMSAASampleCount, dcDrawFlags, uint32_t uSubpassIndex);
static const dcPipelineEntry* pl__get_3d_textured_pipeline     (plRenderPassHandle, uint32_t uMSAASampleCount, dcDrawFlags, uint32_t uSubpassIndex);
static const dcPipelineEntry* pl__get_2d_pipeline              (plRenderPassHandle, uint32_t uMSAASampleCount, uint32_t uSubpassIndex);
static plBindGroupHandle      pl_create_bind_group_for_texture(plTextureHandle);

//-----------------------------------------------------------------------------
// [SECTION] public api implementation
//-----------------------------------------------------------------------------

void
pl_initialize_draw_backend(plDevice* ptDevice)
{
    gptDrawBackendCtx->ptDevice = ptDevice;

    pl_sb_reserve(gptDrawBackendCtx->sbt3dPipelineEntries, 32);
    pl_sb_reserve(gptDrawBackendCtx->sbt2dPipelineEntries, 32);

    // create initial buffers
    const plBufferDesc tIndexBufferDesc = {
        .tUsage      = PL_BUFFER_USAGE_INDEX,
        .szByteSize  = 4096,
        .pcDebugName = "Draw Ext Idx Buffer"
    };

    const plBufferDesc tVertexBufferDesc = {
        .tUsage      = PL_BUFFER_USAGE_VERTEX,
        .szByteSize  = 4096,
        .pcDebugName = "Draw Ext Vtx Buffer"
    }; 

    for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
    {
        gptDrawBackendCtx->auIndexBufferSize[i] = 4096;
        gptDrawBackendCtx->atBufferInfo[i].uVertexBufferSize = 4096;
        gptDrawBackendCtx->at3DBufferInfo[i].uVertexBufferSize = 4096;
        gptDrawBackendCtx->at3DTexturedBufferInfo[i].uVertexBufferSize = 4096;
        gptDrawBackendCtx->atLineBufferInfo[i].uVertexBufferSize = 4096;
        gptDrawBackendCtx->atIndexBuffer[i] = pl__create_staging_buffer(&tIndexBufferDesc, "draw idx buffer", i);
        gptDrawBackendCtx->atBufferInfo[i].tVertexBuffer= pl__create_staging_buffer(&tVertexBufferDesc, "draw vtx buffer", i);
        gptDrawBackendCtx->at3DBufferInfo[i].tVertexBuffer= pl__create_staging_buffer(&tVertexBufferDesc, "3d draw vtx buffer", i);
        gptDrawBackendCtx->at3DTexturedBufferInfo[i].tVertexBuffer= pl__create_staging_buffer(&tVertexBufferDesc, "3d textured draw vtx buffer", i);
        gptDrawBackendCtx->atLineBufferInfo[i].tVertexBuffer= pl__create_staging_buffer(&tVertexBufferDesc, "3d line draw vtx buffer", i);
    }

    // 2d
    const plSamplerDesc tSamplerDesc = {
        .tMagFilter      = PL_FILTER_LINEAR,
        .tMinFilter      = PL_FILTER_LINEAR,
        .fMinMip         = -1000.0f,
        .fMaxMip         = 1000.0f,
        .fMaxAnisotropy  = 1.0f,
        .tVAddressMode   = PL_ADDRESS_MODE_WRAP,
        .tUAddressMode   = PL_ADDRESS_MODE_WRAP,
        .tMipmapMode     = PL_MIPMAP_MODE_LINEAR,
        .pcDebugName     = "2D Drawing Font Sampler"
    };
    gptDrawBackendCtx->tFontSampler = gptGfx->create_sampler(ptDevice, &tSamplerDesc);

    const plSamplerDesc tNearSamplerDesc = {
        .tMagFilter      = PL_FILTER_NEAREST,
        .tMinFilter      = PL_FILTER_NEAREST,
        .fMinMip         = -1000.0f,
        .fMaxMip         = 1000.0f,
        .fMaxAnisotropy  = 1.0f,
        .tVAddressMode   = PL_ADDRESS_MODE_WRAP,
        .tUAddressMode   = PL_ADDRESS_MODE_WRAP,
        .tMipmapMode     = PL_MIPMAP_MODE_LINEAR,
        .pcDebugName     = "2D Drawing Near Sampler"
    };
    gptDrawBackendCtx->tNearSampler = gptGfx->create_sampler(ptDevice, &tNearSamplerDesc);

    const plBindGroupPoolDesc tPoolDesc = {
        .tFlags = PL_BIND_GROUP_POOL_FLAGS_INDIVIDUAL_RESET,
        .szSamplerBindings = 10,
        .szSampledTextureBindings = 10000
    };
    gptDrawBackendCtx->ptBindGroupPool = gptGfx->create_bind_group_pool(ptDevice, &tPoolDesc);

    const plBindGroupLayoutDesc tSamplerBindGroupLayout = {
        .atSamplerBindings = {
            {.uSlot =  0, .tStages = PL_SHADER_STAGE_FRAGMENT}
        }
    };
    gptDrawBackendCtx->tSamplerBindGroupLayout = gptGfx->create_bind_group_layout(ptDevice, &tSamplerBindGroupLayout);

    const plBindGroupLayoutDesc tDrawingBindGroup = {
        .atTextureBindings = { 
            {.uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED}
        }
    };
    gptDrawBackendCtx->tTextureBindGroupLayout = gptGfx->create_bind_group_layout(ptDevice, &tDrawingBindGroup);

    {
        const plBindGroupDesc tSamplerBindGroupDesc = {
            .ptPool      = gptDrawBackendCtx->ptBindGroupPool,
            .tLayout     = gptDrawBackendCtx->tSamplerBindGroupLayout,
            .pcDebugName = "font sampler bind group"
        };
        gptDrawBackendCtx->tFontSamplerBindGroup = gptGfx->create_bind_group(ptDevice, &tSamplerBindGroupDesc);
        gptDrawBackendCtx->tCurrentSamplerBindGroup = gptDrawBackendCtx->tFontSamplerBindGroup;
        const plBindGroupUpdateSamplerData atSamplerData[] = {
            { .uSlot = 0, .tSampler = gptDrawBackendCtx->tFontSampler}
        };

        plBindGroupUpdateData tBGData0 = {
            .uSamplerCount = 1,
            .atSamplerBindings = atSamplerData,
        };
        gptGfx->update_bind_group(ptDevice, gptDrawBackendCtx->tFontSamplerBindGroup, &tBGData0);
    }

    {
        const plBindGroupDesc tSamplerBindGroupDesc = {
            .ptPool      = gptDrawBackendCtx->ptBindGroupPool,
            .tLayout     = gptDrawBackendCtx->tSamplerBindGroupLayout,
            .pcDebugName = "near sampler bind group"
        };
        gptDrawBackendCtx->tNearSamplerBindGroup = gptGfx->create_bind_group(ptDevice, &tSamplerBindGroupDesc);
        const plBindGroupUpdateSamplerData atSamplerData[] = {
            { .uSlot = 0, .tSampler = gptDrawBackendCtx->tNearSampler}
        };

        plBindGroupUpdateData tBGData0 = {
            .uSamplerCount = 1,
            .atSamplerBindings = atSamplerData,
        };
        gptGfx->update_bind_group(ptDevice, gptDrawBackendCtx->tNearSamplerBindGroup, &tBGData0);
    }
}

void
pl_cleanup_draw_backend(void)
{
    plDevice* ptDevice = gptDrawBackendCtx->ptDevice;
    for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
    {
        gptGfx->destroy_buffer(ptDevice, gptDrawBackendCtx->atBufferInfo[i].tVertexBuffer);
        gptGfx->destroy_buffer(ptDevice, gptDrawBackendCtx->at3DBufferInfo[i].tVertexBuffer);
        gptGfx->destroy_buffer(ptDevice, gptDrawBackendCtx->at3DTexturedBufferInfo[i].tVertexBuffer);
        gptGfx->destroy_buffer(ptDevice, gptDrawBackendCtx->atLineBufferInfo[i].tVertexBuffer);
        gptGfx->destroy_buffer(ptDevice, gptDrawBackendCtx->atIndexBuffer[i]);
    }

    gptGfx->cleanup_bind_group_pool(gptDrawBackendCtx->ptBindGroupPool);

    pl_sb_free(gptDrawBackendCtx->sbt3dPipelineEntries);
    pl_sb_free(gptDrawBackendCtx->sbt3dTexturedPipelineEntries);
    pl_sb_free(gptDrawBackendCtx->sbt2dPipelineEntries);
    pl_temp_allocator_free(&gptDrawBackendCtx->tTempAllocator);

    gptDraw->cleanup();
}

void
pl_new_draw_frame(void)
{

    static double* pd2dPipelineCount = NULL;
    static double* pd3dPipelineCount = NULL;

    if(!pd2dPipelineCount)
        pd2dPipelineCount = gptStats->get_counter("Draw 2D Pipelines");

    if(!pd3dPipelineCount)
        pd3dPipelineCount = gptStats->get_counter("Draw 3D Pipelines");

    *pd2dPipelineCount = pl_sb_size(gptDrawBackendCtx->sbt2dPipelineEntries);
    *pd3dPipelineCount = pl_sb_size(gptDrawBackendCtx->sbt3dPipelineEntries);

    gptDraw->new_frame();
    gptDrawBackendCtx->tCurrentDynamicDataBlock = gptGfx->allocate_dynamic_data_block(gptDrawBackendCtx->ptDevice);

    // reset buffer offsets
    for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
    {
        gptDrawBackendCtx->atBufferInfo[i].uVertexBufferOffset = 0;
        gptDrawBackendCtx->at3DBufferInfo[i].uVertexBufferOffset = 0;
        gptDrawBackendCtx->at3DTexturedBufferInfo[i].uVertexBufferOffset = 0;
        gptDrawBackendCtx->atLineBufferInfo[i].uVertexBufferOffset = 0;
        gptDrawBackendCtx->auIndexBufferOffset[i] = 0;
    }
}

bool
pl_build_font_atlas_backend(plCommandBuffer* ptCommandBuffer, dcFontAtlas* ptAtlas)
{

    gptDraw->prepare_font_atlas(ptAtlas);

    // create texture
    const plTextureDesc tFontTextureDesc = {
        .tDimensions   = {ptAtlas->tAtlasSize.x, ptAtlas->tAtlasSize.y, 1},
        .tFormat       = PL_FORMAT_R8G8B8A8_UNORM,
        .uLayers       = 1,
        .uMips         = 1,
        .tType         = PL_TEXTURE_TYPE_2D,
        .tUsage        = PL_TEXTURE_USAGE_SAMPLED,
        .pcDebugName   = "2D Drawing Font Atlas"
    };

    plDevice* ptDevice = gptDrawBackendCtx->ptDevice;

    plTexture* ptTexture = NULL;
    plTextureHandle tTexture = gptGfx->create_texture(ptDevice, &tFontTextureDesc, &ptTexture);
    uint64_t ulData = (uint64_t)tTexture.uData;
    ptAtlas->ptUserData = (void*)ulData;
    
    const plDeviceMemoryAllocation tAllocation = gptGfx->allocate_memory(ptDevice,
        ptTexture->tMemoryRequirements.ulSize,
        PL_MEMORY_FLAGS_DEVICE_LOCAL,
        ptTexture->tMemoryRequirements.uMemoryTypeBits,
        "font texture memory");

    gptGfx->bind_texture_to_memory(ptDevice, tTexture, &tAllocation);

    const plBufferDesc tBufferDesc = {
        .tUsage      = PL_BUFFER_USAGE_TRANSFER_SOURCE,
        .szByteSize  = (size_t)(ptAtlas->tAtlasSize.x * ptAtlas->tAtlasSize.y * 4),
        .pcDebugName = "font staging buffer"
    };
    plBufferHandle tStagingBuffer = pl__create_staging_buffer(&tBufferDesc, "font staging buffer", 0);
    plBuffer* ptStagingBuffer = gptGfx->get_buffer(ptDevice, tStagingBuffer);
    memcpy(ptStagingBuffer->tMemoryAllocation.pHostMapped, ptAtlas->pucPixelsAsRGBA32, tBufferDesc.szByteSize);

    // begin recording
    gptGfx->begin_command_recording(ptCommandBuffer, NULL);
    
    // begin blit pass, copy texture, end pass
    plBlitEncoder* ptEncoder = gptGfx->begin_blit_pass(ptCommandBuffer);

    gptGfx->pipeline_barrier_blit(ptEncoder, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE);

    gptGfx->set_texture_usage(ptEncoder, tTexture, PL_TEXTURE_USAGE_SAMPLED, 0);

    const plBufferImageCopy tBufferImageCopy = {
        .uImageWidth = (uint32_t)ptAtlas->tAtlasSize.x,
        .uImageHeight = (uint32_t)ptAtlas->tAtlasSize.y,
        .uImageDepth = 1,
        .uLayerCount = 1
    };

    gptGfx->copy_buffer_to_texture(ptEncoder, tStagingBuffer, tTexture, 1, &tBufferImageCopy);
    gptGfx->pipeline_barrier_blit(ptEncoder, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ);
    gptGfx->end_blit_pass(ptEncoder);

    // finish recording
    gptGfx->end_command_recording(ptCommandBuffer);

    // submit command buffer
    gptGfx->submit_command_buffer(ptCommandBuffer, NULL);
    gptGfx->wait_on_command_buffer(ptCommandBuffer);

    ptAtlas->tTexture = pl_create_bind_group_for_texture(tTexture).uData;

    gptGfx->destroy_buffer(ptDevice, tStagingBuffer);
    return true;
}

void
pl_cleanup_font_atlas_backend(dcFontAtlas* ptAtlas)
{
    if(ptAtlas == NULL)
        ptAtlas = gptDraw->get_current_font_atlas();

    uint64_t ulData = (uint64_t)ptAtlas->ptUserData;
    plTextureHandle tTexture = {.uData = (uint32_t)ulData};
    gptGfx->destroy_texture(gptDrawBackendCtx->ptDevice, tTexture);

    gptDraw->cleanup_font_atlas(ptAtlas);
}

//-----------------------------------------------------------------------------
// [SECTION] internal api implementation
//-----------------------------------------------------------------------------

static plBufferHandle
pl__create_staging_buffer(const plBufferDesc* ptDesc, const char* pcName, uint32_t uIdentifier)
{
    // for convience
    plDevice* ptDevice = gptDrawBackendCtx->ptDevice;

    // create buffer
    plBuffer* ptBuffer = NULL;
    const plBufferHandle tHandle = gptGfx->create_buffer(ptDevice, ptDesc, &ptBuffer);
    pl_temp_allocator_reset(&gptDrawBackendCtx->tTempAllocator);

    // allocate memory
    const plDeviceMemoryAllocation tAllocation = gptGfx->allocate_memory(ptDevice,
        ptBuffer->tMemoryRequirements.ulSize,
        PL_MEMORY_FLAGS_HOST_VISIBLE | PL_MEMORY_FLAGS_HOST_COHERENT,
        ptBuffer->tMemoryRequirements.uMemoryTypeBits,
        pl_temp_allocator_sprintf(&gptDrawBackendCtx->tTempAllocator, "%s: %u", pcName, uIdentifier));

    // bind memory
    gptGfx->bind_buffer_to_memory(ptDevice, tHandle, &tAllocation);
    return tHandle;
}

static const dcPipelineEntry*
pl__get_3d_pipeline(plRenderPassHandle tRenderPass, uint32_t uMSAASampleCount, dcDrawFlags tFlags, uint32_t uSubpassIndex)
{
    // check if pipeline exists
    for(uint32_t i = 0; i < pl_sb_size(gptDrawBackendCtx->sbt3dPipelineEntries); i++)
    {
        const dcPipelineEntry* ptEntry = &gptDrawBackendCtx->sbt3dPipelineEntries[i];
        if(ptEntry->tRenderPass.uIndex == tRenderPass.uIndex && ptEntry->uMSAASampleCount == uMSAASampleCount && ptEntry->tFlags == tFlags && ptEntry->uSubpassIndex == uSubpassIndex)
        {
            return ptEntry;
        }
    }

    plDevice* ptDevice = gptDrawBackendCtx->ptDevice;

    pl_sb_add(gptDrawBackendCtx->sbt3dPipelineEntries);
    dcPipelineEntry* ptEntry = &gptDrawBackendCtx->sbt3dPipelineEntries[pl_sb_size(gptDrawBackendCtx->sbt3dPipelineEntries) - 1];
    ptEntry->tFlags = tFlags;
    ptEntry->tRenderPass = tRenderPass;
    ptEntry->uMSAASampleCount = uMSAASampleCount;
    ptEntry->uSubpassIndex = uSubpassIndex;

    uint64_t ulCullMode = PL_CULL_MODE_NONE;
    if(tFlags & DC_DRAW_FLAG_CULL_FRONT)
        ulCullMode |= PL_CULL_MODE_CULL_FRONT;
    if(tFlags & DC_DRAW_FLAG_CULL_BACK)
        ulCullMode |= PL_CULL_MODE_CULL_BACK;

    {
        const plShaderDesc t3DShaderDesc = {
            .tFragmentShader = gptShader->load_glsl("dc_draw_3d.frag", "main", NULL, NULL),
            .tVertexShader   = gptShader->load_glsl("dc_draw_3d.vert", "main", NULL, NULL),
            .tGraphicsState = {
                .ulDepthWriteEnabled  = tFlags & DC_DRAW_FLAG_DEPTH_WRITE ? 1 : 0,
                .ulDepthMode          = tFlags & DC_DRAW_FLAG_DEPTH_TEST ? (tFlags & DC_DRAW_FLAG_REVERSE_Z_DEPTH ? PL_COMPARE_MODE_GREATER : PL_COMPARE_MODE_LESS) : PL_COMPARE_MODE_ALWAYS,
                .ulCullMode           = ulCullMode,
                .ulWireframe          = 0,
                .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
                .ulStencilRef         = 0xff,
                .ulStencilMask        = 0xff,
                .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
                .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
                .ulStencilOpPass      = PL_STENCIL_OP_KEEP
            },
            .atVertexBufferLayouts = {
                {
                    .uByteStride = sizeof(float) * 4,
                    .atAttributes = {
                        {.uByteOffset = 0,                 .tFormat = PL_VERTEX_FORMAT_FLOAT3},
                        {.uByteOffset = sizeof(float) * 3, .tFormat = PL_VERTEX_FORMAT_UINT},
                    }
                }
            },
            .atBlendStates = {
                {
                    .bBlendEnabled   = true,
                    .uColorWriteMask = PL_COLOR_WRITE_MASK_ALL,
                    .tSrcColorFactor = PL_BLEND_FACTOR_SRC_ALPHA,
                    .tDstColorFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .tColorOp        = PL_BLEND_OP_ADD,
                    .tSrcAlphaFactor = PL_BLEND_FACTOR_SRC_ALPHA,
                    .tDstAlphaFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .tAlphaOp        = PL_BLEND_OP_ADD
                }
            },
            .tRenderPassLayout = gptGfx->get_render_pass(ptDevice, tRenderPass)->tDesc.tLayout,
            .uSubpassIndex = uSubpassIndex,
            .tMSAASampleCount = uMSAASampleCount
        };
        ptEntry->tRegularPipeline = gptGfx->create_shader(ptDevice, &t3DShaderDesc);
        pl_temp_allocator_reset(&gptDrawBackendCtx->tTempAllocator);
    }

    {
        const plShaderDesc t3DLineShaderDesc = {
            .tFragmentShader = gptShader->load_glsl("dc_draw_3d.frag", "main", NULL, NULL),
            .tVertexShader   = gptShader->load_glsl("dc_draw_3d_line.vert", "main", NULL, NULL),
            .tGraphicsState = {
                .ulDepthWriteEnabled  = tFlags & DC_DRAW_FLAG_DEPTH_WRITE,
                .ulDepthMode          = tFlags & DC_DRAW_FLAG_DEPTH_TEST ? (tFlags & DC_DRAW_FLAG_REVERSE_Z_DEPTH ? PL_COMPARE_MODE_GREATER : PL_COMPARE_MODE_LESS) : PL_COMPARE_MODE_ALWAYS,
                .ulCullMode           = ulCullMode,
                .ulWireframe          = 0,
                .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
                .ulStencilRef         = 0xff,
                .ulStencilMask        = 0xff,
                .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
                .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
                .ulStencilOpPass      = PL_STENCIL_OP_KEEP
            },
            .atVertexBufferLayouts = {
                {
                    .uByteStride = sizeof(float) * 10,
                    .atAttributes = {
                        {.uByteOffset = 0,                 .tFormat = PL_VERTEX_FORMAT_FLOAT3},
                        {.uByteOffset = sizeof(float) * 3, .tFormat = PL_VERTEX_FORMAT_FLOAT3},
                        {.uByteOffset = sizeof(float) * 6, .tFormat = PL_VERTEX_FORMAT_FLOAT3},
                        {.uByteOffset = sizeof(float) * 9, .tFormat = PL_VERTEX_FORMAT_UINT},
                    }
                }
            },
            .atBlendStates = {
                {
                    .bBlendEnabled   = true,
                    .uColorWriteMask = PL_COLOR_WRITE_MASK_ALL,
                    .tSrcColorFactor = PL_BLEND_FACTOR_SRC_ALPHA,
                    .tDstColorFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .tColorOp        = PL_BLEND_OP_ADD,
                    .tSrcAlphaFactor = PL_BLEND_FACTOR_SRC_ALPHA,
                    .tDstAlphaFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    .tAlphaOp        = PL_BLEND_OP_ADD
                }
            },
            .tRenderPassLayout = gptGfx->get_render_pass(ptDevice, tRenderPass)->tDesc.tLayout,
            .uSubpassIndex = uSubpassIndex,
            .tMSAASampleCount = uMSAASampleCount
        };
        ptEntry->tSecondaryPipeline = gptGfx->create_shader(ptDevice, &t3DLineShaderDesc);
        pl_temp_allocator_reset(&gptDrawBackendCtx->tTempAllocator);
    }
    return ptEntry;
}

static const dcPipelineEntry*
pl__get_3d_textured_pipeline(plRenderPassHandle tRenderPass, uint32_t uMSAASampleCount, dcDrawFlags tFlags, uint32_t uSubpassIndex)
{
    // check if pipeline exists
    for(uint32_t i = 0; i < pl_sb_size(gptDrawBackendCtx->sbt3dTexturedPipelineEntries); i++)
    {
        const dcPipelineEntry* ptEntry = &gptDrawBackendCtx->sbt3dTexturedPipelineEntries[i];
        if(ptEntry->tRenderPass.uIndex == tRenderPass.uIndex && ptEntry->uMSAASampleCount == uMSAASampleCount && ptEntry->tFlags == tFlags && ptEntry->uSubpassIndex == uSubpassIndex)
        {
            return ptEntry;
        }
    }

    plDevice* ptDevice = gptDrawBackendCtx->ptDevice;

    pl_sb_add(gptDrawBackendCtx->sbt3dTexturedPipelineEntries);
    dcPipelineEntry* ptEntry = &gptDrawBackendCtx->sbt3dTexturedPipelineEntries[pl_sb_size(gptDrawBackendCtx->sbt3dTexturedPipelineEntries) - 1];
    ptEntry->tFlags = tFlags;
    ptEntry->tRenderPass = tRenderPass;
    ptEntry->uMSAASampleCount = uMSAASampleCount;
    ptEntry->uSubpassIndex = uSubpassIndex;

    uint64_t ulCullMode = PL_CULL_MODE_NONE;
    if(tFlags & DC_DRAW_FLAG_CULL_FRONT)
        ulCullMode |= PL_CULL_MODE_CULL_FRONT;
    if(tFlags & DC_DRAW_FLAG_CULL_BACK)
        ulCullMode |= PL_CULL_MODE_CULL_BACK;

    const plShaderDesc t3DTexturedShaderDesc = {
        .tFragmentShader = gptShader->load_glsl("dc_draw_3d_textured.frag", "main", NULL, NULL),
        .tVertexShader   = gptShader->load_glsl("dc_draw_3d_textured.vert", "main", NULL, NULL),
        .tGraphicsState = {
            .ulDepthWriteEnabled  = tFlags & DC_DRAW_FLAG_DEPTH_WRITE ? 1 : 0,
            .ulDepthMode          = tFlags & DC_DRAW_FLAG_DEPTH_TEST ? (tFlags & DC_DRAW_FLAG_REVERSE_Z_DEPTH ? PL_COMPARE_MODE_GREATER : PL_COMPARE_MODE_LESS) : PL_COMPARE_MODE_ALWAYS,
            .ulCullMode           = ulCullMode,
            .ulWireframe          = 0,
            .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
            .ulStencilRef         = 0xff,
            .ulStencilMask        = 0xff,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP
        },
        .atVertexBufferLayouts = {
            {
                .uByteStride = sizeof(float) * 6,  // pos3 + uv2 + color (padded)
                .atAttributes = {
                    {.uByteOffset = 0,                 .tFormat = PL_VERTEX_FORMAT_FLOAT3},
                    {.uByteOffset = sizeof(float) * 3, .tFormat = PL_VERTEX_FORMAT_FLOAT2},
                    {.uByteOffset = sizeof(float) * 5, .tFormat = PL_VERTEX_FORMAT_UINT},
                }
            }
        },
        .atBlendStates = {
            {
                .bBlendEnabled   = true,
                .uColorWriteMask = PL_COLOR_WRITE_MASK_ALL,
                .tSrcColorFactor = PL_BLEND_FACTOR_SRC_ALPHA,
                .tDstColorFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .tColorOp        = PL_BLEND_OP_ADD,
                .tSrcAlphaFactor = PL_BLEND_FACTOR_SRC_ALPHA,
                .tDstAlphaFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .tAlphaOp        = PL_BLEND_OP_ADD
            }
        },
        .atBindGroupLayouts = {
            {
                .atSamplerBindings = {
                    {.uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT}
                }
            },
            {
                .atTextureBindings = {
                    {.uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED}
                }
            }
        },
        .tRenderPassLayout = gptGfx->get_render_pass(ptDevice, tRenderPass)->tDesc.tLayout,
        .uSubpassIndex = uSubpassIndex,
        .tMSAASampleCount = uMSAASampleCount
    };
    ptEntry->tRegularPipeline = gptGfx->create_shader(ptDevice, &t3DTexturedShaderDesc);
    pl_temp_allocator_reset(&gptDrawBackendCtx->tTempAllocator);

    return ptEntry;
}

static const dcPipelineEntry*
pl__get_2d_pipeline(plRenderPassHandle tRenderPass, uint32_t uMSAASampleCount, uint32_t uSubpassIndex)
{
    // check if pipeline exists
    for(uint32_t i = 0; i < pl_sb_size(gptDrawBackendCtx->sbt2dPipelineEntries); i++)
    {
        const dcPipelineEntry* ptEntry = &gptDrawBackendCtx->sbt2dPipelineEntries[i];
        if(ptEntry->tRenderPass.uIndex == tRenderPass.uIndex && ptEntry->uMSAASampleCount == uMSAASampleCount && ptEntry->uSubpassIndex == uSubpassIndex)
        {
            return ptEntry;
        }
    }

    plDevice* ptDevice = gptDrawBackendCtx->ptDevice;

    pl_sb_add(gptDrawBackendCtx->sbt2dPipelineEntries);
    dcPipelineEntry* ptEntry = &gptDrawBackendCtx->sbt2dPipelineEntries[pl_sb_size(gptDrawBackendCtx->sbt2dPipelineEntries) - 1];
    ptEntry->tFlags = 0;
    ptEntry->tRenderPass = tRenderPass;
    ptEntry->uMSAASampleCount = uMSAASampleCount;
    ptEntry->uSubpassIndex = uSubpassIndex;

    const plShaderDesc tRegularShaderDesc = {
        .tFragmentShader  = gptShader->load_glsl("dc_draw_2d.frag", "main", NULL, NULL),
        .tVertexShader    = gptShader->load_glsl("dc_draw_2d.vert", "main", NULL, NULL),
        .tGraphicsState = {
            .ulDepthWriteEnabled  = 0,
            .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
            .ulCullMode           = PL_CULL_MODE_NONE,
            .ulWireframe          = 0,
            .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
            .ulStencilRef         = 0xff,
            .ulStencilMask        = 0xff,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP
        },
        .atVertexBufferLayouts = {
            {
                .uByteStride = sizeof(float) * 5,
                .atAttributes = {
                    {.uByteOffset = 0,                 .tFormat = PL_VERTEX_FORMAT_FLOAT2},
                    {.uByteOffset = sizeof(float) * 2, .tFormat = PL_VERTEX_FORMAT_FLOAT2},
                    {.uByteOffset = sizeof(float) * 4, .tFormat = PL_VERTEX_FORMAT_UINT},
                }
            }
        },
        .atBlendStates = {
            {
                .bBlendEnabled   = true,
                .uColorWriteMask = PL_COLOR_WRITE_MASK_ALL,
                .tSrcColorFactor = PL_BLEND_FACTOR_SRC_ALPHA,
                .tDstColorFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .tColorOp        = PL_BLEND_OP_ADD,
                .tSrcAlphaFactor = PL_BLEND_FACTOR_SRC_ALPHA,
                .tDstAlphaFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .tAlphaOp        = PL_BLEND_OP_ADD
            }
        },
        .tRenderPassLayout = gptGfx->get_render_pass(ptDevice, tRenderPass)->tDesc.tLayout,
        .uSubpassIndex = uSubpassIndex,
        .atBindGroupLayouts = {
            {
                .atSamplerBindings = {
                    {.uSlot =  0, .tStages = PL_SHADER_STAGE_FRAGMENT}
                }
            },
            {
                .atTextureBindings = {
                    {.uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED}
                }
            }
        },
        .tMSAASampleCount = uMSAASampleCount
    };
    ptEntry->tRegularPipeline = gptGfx->create_shader(ptDevice, &tRegularShaderDesc);
    pl_temp_allocator_reset(&gptDrawBackendCtx->tTempAllocator);

    const plShaderDesc tSecondaryShaderDesc = {
        .tFragmentShader  = gptShader->load_glsl("dc_draw_2d_sdf.frag", "main", NULL, NULL),
        .tVertexShader    = gptShader->load_glsl("dc_draw_2d.vert", "main", NULL, NULL),
        .tGraphicsState = {
            .ulDepthWriteEnabled  = 0,
            .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
            .ulCullMode           = PL_CULL_MODE_NONE,
            .ulWireframe          = 0,
            .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
            .ulStencilRef         = 0xff,
            .ulStencilMask        = 0xff,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP
        },
        .atVertexBufferLayouts = {
            {
                .uByteStride = sizeof(float) * 5,
                .atAttributes = {
                    {.uByteOffset = 0,                 .tFormat = PL_VERTEX_FORMAT_FLOAT2},
                    {.uByteOffset = sizeof(float) * 2, .tFormat = PL_VERTEX_FORMAT_FLOAT2},
                    {.uByteOffset = sizeof(float) * 4, .tFormat = PL_VERTEX_FORMAT_UINT},
                }
            }
        },
        .atBlendStates = {
            {
                .bBlendEnabled   = true,
                .uColorWriteMask = PL_COLOR_WRITE_MASK_ALL,
                .tSrcColorFactor = PL_BLEND_FACTOR_SRC_ALPHA,
                .tDstColorFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .tColorOp        = PL_BLEND_OP_ADD,
                .tSrcAlphaFactor = PL_BLEND_FACTOR_SRC_ALPHA,
                .tDstAlphaFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .tAlphaOp        = PL_BLEND_OP_ADD
            }
        },
        .tRenderPassLayout = gptGfx->get_render_pass(ptDevice, tRenderPass)->tDesc.tLayout,
        .uSubpassIndex = uSubpassIndex,
        .atBindGroupLayouts = {
            {
                .atSamplerBindings = {
                    {.uSlot =  0, .tStages = PL_SHADER_STAGE_FRAGMENT}
                }
            },
            {
                .atTextureBindings = {
                    {.uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED}
                }
            }
        },
        .tMSAASampleCount = uMSAASampleCount
    };
    ptEntry->tSecondaryPipeline = gptGfx->create_shader(ptDevice, &tSecondaryShaderDesc);
    pl_temp_allocator_reset(&gptDrawBackendCtx->tTempAllocator);
    return ptEntry;
}

plBindGroupHandle
pl_create_bind_group_for_texture(plTextureHandle tTexture)
{
    const plBindGroupLayoutDesc tDrawingBindGroup = {
        .atTextureBindings = { 
            {.uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED}
        }
    };
    const plBindGroupDesc tSamplerBindGroupDesc = {
        .ptPool      = gptDrawBackendCtx->ptBindGroupPool,
        .tLayout     = gptDrawBackendCtx->tTextureBindGroupLayout,
        .pcDebugName = "draw texture bind group"
    };
    plBindGroupHandle tBindGroup = gptGfx->create_bind_group(gptDrawBackendCtx->ptDevice, &tSamplerBindGroupDesc);

    const plBindGroupUpdateTextureData atBGTextureData[] = {
        {
            .tTexture = tTexture,
            .uSlot    = 0,
            .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED
        }
    };
    const plBindGroupUpdateData tBGData = {
        .uTextureCount = 1,
        .atTextureBindings = atBGTextureData
    };

    gptGfx->update_bind_group(gptDrawBackendCtx->ptDevice, tBindGroup, &tBGData);

    return tBindGroup;
}

static void
pl__use_nearest_sampler(const dcDrawList2D* ptDrawlist, const dcDrawCommand* tCmd)
{
    gptDrawBackendCtx->tCurrentSamplerBindGroup = gptDrawBackendCtx->tNearSamplerBindGroup;
}

void
pl_use_nearest_sampler(dcDrawLayer2D* ptLayer)
{
    gptDraw->add_2d_callback(ptLayer, pl__use_nearest_sampler, NULL, 0);
}

void
pl_use_linear_sampler(dcDrawLayer2D* ptLayer)
{
    gptDraw->add_2d_callback(ptLayer, dcDrawCallbackResetRenderState, NULL, 0);
}

void
pl_set_shader(dcDrawLayer2D* ptLayer, plShaderHandle* pt2dShader, plShaderHandle* ptSdfShader)
{
    // allocate shader override data that persists until submit
    dcShaderOverride* ptOverride = NULL;
    if(pt2dShader != NULL || ptSdfShader != NULL)
    {
        ptOverride = PL_ALLOC(sizeof(dcShaderOverride));
        ptOverride->pt2dShader = pt2dShader;
        ptOverride->ptSdfShader = ptSdfShader;
    }
    // insert callback into command stream (NULL userData means reset to default)
    gptDraw->add_2d_callback(ptLayer, dcDrawCallbackSetShader, ptOverride, sizeof(dcShaderOverride*));
}

void
pl_set_3d_shader(dcDrawList3D* ptDrawlist, plShaderHandle* ptSolidShader, plShaderHandle* ptTexturedShader)
{
    // allocate shader override data that persists until submit
    dcShaderOverride3D* ptOverride = NULL;
    if(ptSolidShader != NULL || ptTexturedShader != NULL)
    {
        ptOverride = PL_ALLOC(sizeof(dcShaderOverride3D));
        ptOverride->ptSolidShader = ptSolidShader;
        ptOverride->ptTexturedShader = ptTexturedShader;
    }
    // insert callback into 3D command stream (NULL userData means reset to default)
    gptDraw->add_3d_callback(ptDrawlist, dcDrawCallback3DSetShader, ptOverride, sizeof(dcShaderOverride3D));
}

void
pl_submit_2d_drawlist(dcDrawList2D* ptDrawlist, plRenderEncoder* ptEncoder, float fWidth, float fHeight, uint32_t uMSAASampleCount)
{
    gptGfx->set_depth_bias( ptEncoder, 0.0f, 0.0f, 0.0f);
    gptDraw->prepare_2d_drawlist(ptDrawlist);

    if(pl_sb_size(ptDrawlist->sbtVertexBuffer) == 0u)
        return;

    gptGfx->push_render_debug_group(ptEncoder, "2D Draw", (plVec4){0.33f, 0.02f, 0.10f, 1.0f});

    plDevice* ptDevice = gptDrawBackendCtx->ptDevice;

    const uint32_t uFrameIdx = gptGfx->get_current_frame_index();

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~vertex buffer prep~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // ensure gpu vertex buffer size is adequate
    const uint32_t uVtxBufSzNeeded = sizeof(dcDrawVertex) * pl_sb_size(ptDrawlist->sbtVertexBuffer);

    dcBufferInfo* ptBufferInfo = &gptDrawBackendCtx->atBufferInfo[uFrameIdx];

    // space left in vertex buffer
    const uint32_t uAvailableVertexBufferSpace = ptBufferInfo->uVertexBufferSize - ptBufferInfo->uVertexBufferOffset;

    // grow buffer if not enough room
    if(uVtxBufSzNeeded >= uAvailableVertexBufferSpace)
    {

        gptGfx->queue_buffer_for_deletion(ptDevice, ptBufferInfo->tVertexBuffer);

        const plBufferDesc tBufferDesc = {
            .tUsage     = PL_BUFFER_USAGE_VERTEX,
            .szByteSize = pl_max(ptBufferInfo->uVertexBufferSize * 2, uVtxBufSzNeeded + uAvailableVertexBufferSpace),
            .pcDebugName = "2D Draw Vertex Buffer"
        };
        PL_LOG_DEBUG_API_F(gptLog, uLogChannelDrawBackend, "Grow \"%s\" %u to %u frame %llu", tBufferDesc.pcDebugName, ptBufferInfo->uVertexBufferSize, (uint32_t)tBufferDesc.szByteSize, gptIO->ulFrameCount);
        ptBufferInfo->uVertexBufferSize = (uint32_t)tBufferDesc.szByteSize;
        ptBufferInfo->uVertexBufferOffset = 0;

        ptBufferInfo->tVertexBuffer = pl__create_staging_buffer(&tBufferDesc, "draw vtx buffer", uFrameIdx);
    }

    // vertex GPU data transfer
    plBuffer* ptVertexBuffer = gptGfx->get_buffer(ptDevice, ptBufferInfo->tVertexBuffer);
    char* pucMappedVertexBufferLocation = ptVertexBuffer->tMemoryAllocation.pHostMapped;
    size_t tVertexCopySize = sizeof(dcDrawVertex) * pl_sb_size(ptDrawlist->sbtVertexBuffer);
    memcpy(&pucMappedVertexBufferLocation[ptBufferInfo->uVertexBufferOffset], ptDrawlist->sbtVertexBuffer, tVertexCopySize);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~index buffer prep~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // ensure gpu index buffer size is adequate
    const uint32_t uIdxBufSzNeeded = ptDrawlist->uIndexBufferByteSize;

    // space left in index buffer
    const uint32_t uAvailableIndexBufferSpace = gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] - gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx];

    if(uIdxBufSzNeeded >= uAvailableIndexBufferSpace)
    {
        gptGfx->queue_buffer_for_deletion(ptDevice, gptDrawBackendCtx->atIndexBuffer[uFrameIdx]);

        const plBufferDesc tBufferDesc = {
            .tUsage     = PL_BUFFER_USAGE_INDEX,
            .szByteSize = pl_max(gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] * 2, uIdxBufSzNeeded + uAvailableIndexBufferSpace),
            .pcDebugName = "Draw Index Buffer"
        };
        PL_LOG_DEBUG_API_F(gptLog, uLogChannelDrawBackend, "(2D) Grow \"%s\" %u to %u frame %llu", tBufferDesc.pcDebugName, gptDrawBackendCtx->auIndexBufferSize[uFrameIdx], (uint32_t)tBufferDesc.szByteSize, gptIO->ulFrameCount);
        gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] = (uint32_t)tBufferDesc.szByteSize;

        gptDrawBackendCtx->atIndexBuffer[uFrameIdx] = pl__create_staging_buffer(&tBufferDesc, "draw idx buffer", uFrameIdx);
        gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] = 0;
    }

    // index GPU data transfer
    plBuffer* ptIndexBuffer = gptGfx->get_buffer(ptDevice, gptDrawBackendCtx->atIndexBuffer[uFrameIdx]);
    char* pucMappedIndexBufferLocation = ptIndexBuffer->tMemoryAllocation.pHostMapped;
    memcpy(&pucMappedIndexBufferLocation[gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx]], ptDrawlist->sbuIndexBuffer, uIdxBufSzNeeded);

    const int32_t iVertexOffset = ptBufferInfo->uVertexBufferOffset / sizeof(dcDrawVertex);
    const int32_t iIndexOffset = gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] / sizeof(uint32_t);

    const dcPipelineEntry* ptEntry = pl__get_2d_pipeline(gptGfx->get_encoder_render_pass(ptEncoder), uMSAASampleCount, gptGfx->get_render_encoder_subpass(ptEncoder));

    const plVec2 tClipScale = gptIOI->get_io()->tMainFramebufferScale;

    const float fScale[] = { 2.0f / fWidth, 2.0f / fHeight};

    fWidth = fWidth * tClipScale.x;
    fHeight = fHeight * tClipScale.y;
    
    // const plVec2 tClipScale = {1.0f, 1.0f};
    // const plVec2 tClipScale = ptCtx->tFrameBufferScale;
    
    const float fTranslate[] = {-1.0f, -1.0f};

    // reset custom shader state at start of submit
    gptDrawBackendCtx->bCustomShaderActive = false;
    gptDrawBackendCtx->pt2dShaderOverride = NULL;
    gptDrawBackendCtx->ptSdfShaderOverride = NULL;

    plShaderHandle tCurrentShader = ptEntry->tRegularPipeline;

    typedef struct _dcDrawDynamicData
    {
        plVec2 uScale;
        plVec2 uTranslate;
    } dcDrawDynamicData;

    plDynamicBinding tDynamicBinding = pl_allocate_dynamic_data(gptGfx, ptDevice, &gptDrawBackendCtx->tCurrentDynamicDataBlock);

    dcDrawDynamicData* ptDynamicData = (dcDrawDynamicData*)tDynamicBinding.pcData;
    ptDynamicData->uScale.x = fScale[0];
    ptDynamicData->uScale.y = fScale[1];
    ptDynamicData->uTranslate.x = -1.0f;
    ptDynamicData->uTranslate.y = -1.0f;

    bool bSdf = false;

    plRenderViewport tViewport = {
        .fWidth  = fWidth,
        .fHeight = fHeight,
        .fMaxDepth = 1.0f
    };

    gptGfx->set_viewport(ptEncoder, &tViewport);
    gptGfx->bind_vertex_buffer(ptEncoder, ptBufferInfo->tVertexBuffer);
    gptGfx->bind_shader(ptEncoder, tCurrentShader);

    const uint32_t uCmdCount = pl_sb_size(ptDrawlist->sbtDrawCommands);
    for(uint32_t i = 0u; i < uCmdCount; i++)
    {
        dcDrawCommand cmd = ptDrawlist->sbtDrawCommands[i];

        // callback (state changes like sampler switching, shader override)
        if(cmd.tUserCallback != NULL)
        {
            if(cmd.tUserCallback == dcDrawCallbackResetRenderState)
            {
                gptGfx->set_viewport(ptEncoder, &tViewport);
                gptGfx->bind_vertex_buffer(ptEncoder, ptBufferInfo->tVertexBuffer);
                gptGfx->bind_shader(ptEncoder, tCurrentShader);
                gptDrawBackendCtx->tCurrentSamplerBindGroup = gptDrawBackendCtx->tFontSamplerBindGroup;
            }
            else if(cmd.tUserCallback == dcDrawCallbackSetShader)
            {
                dcShaderOverride* ptOverride = (dcShaderOverride*)cmd.pUserCallbackData;
                if(ptOverride)
                {
                    // custom shader override - store for use in draw commands
                    gptDrawBackendCtx->pt2dShaderOverride = ptOverride->pt2dShader;
                    gptDrawBackendCtx->ptSdfShaderOverride = ptOverride->ptSdfShader;
                    gptDrawBackendCtx->bCustomShaderActive = true;

                    // bind the appropriate shader based on current bSdf state
                    plShaderHandle tShader = bSdf
                        ? (ptOverride->ptSdfShader ? *ptOverride->ptSdfShader : ptEntry->tSecondaryPipeline)
                        : (ptOverride->pt2dShader ? *ptOverride->pt2dShader : ptEntry->tRegularPipeline);
                    gptGfx->bind_shader(ptEncoder, tShader);
                    tCurrentShader = tShader;

                    // free the override data
                    PL_FREE(ptOverride);
                    ptDrawlist->sbtDrawCommands[i].pUserCallbackData = NULL;
                }
                else
                {
                    // reset to default shaders
                    gptDrawBackendCtx->pt2dShaderOverride = NULL;
                    gptDrawBackendCtx->ptSdfShaderOverride = NULL;
                    gptDrawBackendCtx->bCustomShaderActive = false;

                    // let normal bSdf logic handle the shader on next draw command
                    tCurrentShader = bSdf ? ptEntry->tSecondaryPipeline : ptEntry->tRegularPipeline;
                    gptGfx->bind_shader(ptEncoder, tCurrentShader);
                }
            }
            else
            {
                cmd.tUserCallback(ptDrawlist, &cmd);
            }
        }
        else
        {
            // switch shaders based on bSdf flag, using overrides if custom shader active
            if(cmd.bSdf && !bSdf)
            {
                plShaderHandle tShader = gptDrawBackendCtx->bCustomShaderActive && gptDrawBackendCtx->ptSdfShaderOverride
                    ? *gptDrawBackendCtx->ptSdfShaderOverride
                    : ptEntry->tSecondaryPipeline;
                gptGfx->bind_shader(ptEncoder, tShader);
                tCurrentShader = tShader;
                bSdf = true;
            }
            else if(!cmd.bSdf && bSdf)
            {
                plShaderHandle tShader = gptDrawBackendCtx->bCustomShaderActive && gptDrawBackendCtx->pt2dShaderOverride
                    ? *gptDrawBackendCtx->pt2dShaderOverride
                    : ptEntry->tRegularPipeline;
                gptGfx->bind_shader(ptEncoder, tShader);
                tCurrentShader = tShader;
                bSdf = false;
            }

            if(pl_rect_width(&cmd.tClip) == 0)
            {
                const plScissor tScissor = {
                    .uWidth = (uint32_t)(fWidth),
                    .uHeight = (uint32_t)(fHeight),
                };
                gptGfx->set_scissor_region(ptEncoder, &tScissor);
            }
            else
            {

                cmd.tClip.tMin.x = tClipScale.x * cmd.tClip.tMin.x;
                cmd.tClip.tMax.x = tClipScale.x * cmd.tClip.tMax.x;
                cmd.tClip.tMin.y = tClipScale.y * cmd.tClip.tMin.y;
                cmd.tClip.tMax.y = tClipScale.y * cmd.tClip.tMax.y;

                // clamp to viewport
                if (cmd.tClip.tMin.x < 0.0f)   { cmd.tClip.tMin.x = 0.0f; }
                if (cmd.tClip.tMin.y < 0.0f)   { cmd.tClip.tMin.y = 0.0f; }
                if (cmd.tClip.tMax.x > fWidth)  { cmd.tClip.tMax.x = (float)fWidth; }
                if (cmd.tClip.tMax.y > fHeight) { cmd.tClip.tMax.y = (float)fHeight; }
                if (cmd.tClip.tMax.x <= cmd.tClip.tMin.x || cmd.tClip.tMax.y <= cmd.tClip.tMin.y)
                    continue;

                const plScissor tScissor = {
                    .iOffsetX  = (uint32_t) (cmd.tClip.tMin.x < 0 ? 0 : cmd.tClip.tMin.x),
                    .iOffsetY  = (uint32_t) (cmd.tClip.tMin.y < 0 ? 0 : cmd.tClip.tMin.y),
                    .uWidth    = (uint32_t)pl_rect_width(&cmd.tClip),
                    .uHeight   = (uint32_t)pl_rect_height(&cmd.tClip)
                };
                gptGfx->set_scissor_region(ptEncoder, &tScissor);
            }

            // use font atlas texture as fallback when no texture specified
            plBindGroupHandle tTexture = {.uData = cmd.tTextureId};
            if(tTexture.uData == 0)
                tTexture.uData = gptDraw->get_current_font_atlas()->tTexture;
            plBindGroupHandle atBindGroups[] = {
                gptDrawBackendCtx->tCurrentSamplerBindGroup,
                tTexture
            };
            gptGfx->bind_graphics_bind_groups(ptEncoder, tCurrentShader, 0, 2, atBindGroups, 1, &tDynamicBinding);

            const plDrawIndex tDraw = {
                .tIndexBuffer   = gptDrawBackendCtx->atIndexBuffer[uFrameIdx],
                .uIndexCount    = cmd.uElementCount,
                .uIndexStart    = cmd.uIndexOffset + iIndexOffset,
                .uInstance      = 0,
                .uInstanceCount = 1,
                .uVertexStart   = iVertexOffset
            };
            gptGfx->draw_indexed(ptEncoder, 1, &tDraw);
        }
    }

    // bump vertex & index buffer offset
    ptBufferInfo->uVertexBufferOffset += uVtxBufSzNeeded;
    gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] += uIdxBufSzNeeded;

    gptGfx->pop_render_debug_group(ptEncoder);
}

void
pl_submit_3d_drawlist(dcDrawList3D* ptDrawlist, plRenderEncoder* ptEncoder, float fWidth, float fHeight, const plMat4* ptMVP, dcDrawFlags tFlags, uint32_t uMSAASampleCount)
{
    gptGfx->push_render_debug_group(ptEncoder, "3D Draw", (plVec4){0.33f, 0.02f, 0.10f, 1.0f});

    plDevice* ptDevice = gptDrawBackendCtx->ptDevice;
    const uint32_t uFrameIdx = gptGfx->get_current_frame_index();

    const dcPipelineEntry* ptEntry = pl__get_3d_pipeline(gptGfx->get_encoder_render_pass(ptEncoder), uMSAASampleCount, tFlags, gptGfx->get_render_encoder_subpass(ptEncoder));

    const float fAspectRatio = fWidth / fHeight;

    const plVec2 tClipScale = gptIOI->get_io()->tMainFramebufferScale;
    const float fScaledWidth  = fWidth * tClipScale.x;
    const float fScaledHeight = fHeight * tClipScale.y;

    const plScissor tScissor = {
        .uWidth = (uint32_t)fScaledWidth,
        .uHeight = (uint32_t)fScaledHeight
    };

    const plRenderViewport tViewport = {
        .fWidth = fScaledWidth,
        .fHeight = fScaledHeight,
        .fMaxDepth = 1.0f
    };

    gptGfx->set_scissor_region(ptEncoder, &tScissor);
    gptGfx->set_viewport(ptEncoder, &tViewport);

    // base offsets for each geometry type (GPU buffer offsets after upload)
    int32_t iSolidVertexBase = 0, iSolidIndexBase = 0;
    int32_t iLineVertexBase = 0, iLineIndexBase = 0;
    int32_t iTexturedVertexBase = 0, iTexturedIndexBase = 0;

    //=========================================================================
    // Phase 1: Upload all vertex/index data
    //=========================================================================

    // solid
    if(pl_sb_size(ptDrawlist->sbtSolidVertexBuffer) > 0)
    {
        const uint32_t uVtxBufSzNeeded = sizeof(dcDrawVertex3DSolid) * pl_sb_size(ptDrawlist->sbtSolidVertexBuffer);
        dcBufferInfo* ptBufferInfo = &gptDrawBackendCtx->at3DBufferInfo[uFrameIdx];
        const uint32_t uAvailableVertexBufferSpace = ptBufferInfo->uVertexBufferSize - ptBufferInfo->uVertexBufferOffset;
        if(uVtxBufSzNeeded >= uAvailableVertexBufferSpace)
        {
            gptGfx->queue_buffer_for_deletion(ptDevice, ptBufferInfo->tVertexBuffer);
            const plBufferDesc tBufferDesc = {
                .tUsage     = PL_BUFFER_USAGE_VERTEX,
                .szByteSize = pl_max(ptBufferInfo->uVertexBufferSize * 2, uVtxBufSzNeeded + uAvailableVertexBufferSpace),
                .pcDebugName = "3D Draw Vertex Buffer"
            };
            ptBufferInfo->uVertexBufferSize = (uint32_t)tBufferDesc.szByteSize;
            ptBufferInfo->tVertexBuffer = pl__create_staging_buffer(&tBufferDesc, "3d draw vtx buffer", uFrameIdx);
        }
        plBuffer* ptVertexBuffer = gptGfx->get_buffer(ptDevice, ptBufferInfo->tVertexBuffer);
        memcpy(&((char*)ptVertexBuffer->tMemoryAllocation.pHostMapped)[ptBufferInfo->uVertexBufferOffset],
            ptDrawlist->sbtSolidVertexBuffer, uVtxBufSzNeeded);

        const uint32_t uIdxBufSzNeeded = sizeof(uint32_t) * pl_sb_size(ptDrawlist->sbtSolidIndexBuffer);
        const uint32_t uAvailableIndexBufferSpace = gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] - gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx];
        if(uIdxBufSzNeeded >= uAvailableIndexBufferSpace)
        {
            gptGfx->queue_buffer_for_deletion(ptDevice, gptDrawBackendCtx->atIndexBuffer[uFrameIdx]);
            const plBufferDesc tBufferDesc = {
                .tUsage     = PL_BUFFER_USAGE_INDEX,
                .szByteSize = pl_max(gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] * 2, uIdxBufSzNeeded + uAvailableIndexBufferSpace),
                .pcDebugName = "Draw Index Buffer"
            };
            gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] = (uint32_t)tBufferDesc.szByteSize;
            gptDrawBackendCtx->atIndexBuffer[uFrameIdx] = pl__create_staging_buffer(&tBufferDesc, "3d draw idx buffer", uFrameIdx);
            gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] = 0;
        }
        plBuffer* ptIndexBuffer = gptGfx->get_buffer(ptDevice, gptDrawBackendCtx->atIndexBuffer[uFrameIdx]);
        memcpy(&((char*)ptIndexBuffer->tMemoryAllocation.pHostMapped)[gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx]],
            ptDrawlist->sbtSolidIndexBuffer, uIdxBufSzNeeded);

        iSolidVertexBase = ptBufferInfo->uVertexBufferOffset / sizeof(dcDrawVertex3DSolid);
        iSolidIndexBase  = gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] / sizeof(uint32_t);
        ptBufferInfo->uVertexBufferOffset += uVtxBufSzNeeded;
        gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] += uIdxBufSzNeeded;
    }

    // lines
    if(pl_sb_size(ptDrawlist->sbtLineVertexBuffer) > 0u)
    {
        const uint32_t uVtxBufSzNeeded = sizeof(dcDrawVertex3DLine) * pl_sb_size(ptDrawlist->sbtLineVertexBuffer);
        dcBufferInfo* ptBufferInfo = &gptDrawBackendCtx->atLineBufferInfo[uFrameIdx];
        const uint32_t uAvailableVertexBufferSpace = ptBufferInfo->uVertexBufferSize - ptBufferInfo->uVertexBufferOffset;
        if(uVtxBufSzNeeded >= uAvailableVertexBufferSpace)
        {
            gptGfx->queue_buffer_for_deletion(ptDevice, ptBufferInfo->tVertexBuffer);
            const plBufferDesc tBufferDesc = {
                .tUsage     = PL_BUFFER_USAGE_VERTEX,
                .szByteSize = pl_max(ptBufferInfo->uVertexBufferSize * 2, uVtxBufSzNeeded + uAvailableVertexBufferSpace),
                .pcDebugName = "3D Lines Vertex Buffer"
            };
            ptBufferInfo->uVertexBufferSize = (uint32_t)tBufferDesc.szByteSize;
            ptBufferInfo->tVertexBuffer = pl__create_staging_buffer(&tBufferDesc, "draw vtx buffer", uFrameIdx);
        }
        plBuffer* ptVertexBuffer = gptGfx->get_buffer(ptDevice, ptBufferInfo->tVertexBuffer);
        memcpy(&((char*)ptVertexBuffer->tMemoryAllocation.pHostMapped)[ptBufferInfo->uVertexBufferOffset],
            ptDrawlist->sbtLineVertexBuffer, uVtxBufSzNeeded);

        const uint32_t uIdxBufSzNeeded = sizeof(uint32_t) * pl_sb_size(ptDrawlist->sbtLineIndexBuffer);
        const uint32_t uAvailableIndexBufferSpace = gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] - gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx];
        if(uIdxBufSzNeeded >= uAvailableIndexBufferSpace)
        {
            gptGfx->queue_buffer_for_deletion(ptDevice, gptDrawBackendCtx->atIndexBuffer[uFrameIdx]);
            const plBufferDesc tBufferDesc = {
                .tUsage     = PL_BUFFER_USAGE_INDEX,
                .szByteSize = pl_max(gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] * 2, uIdxBufSzNeeded + uAvailableIndexBufferSpace),
                .pcDebugName = "Draw Index Buffer"
            };
            gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] = (uint32_t)tBufferDesc.szByteSize;
            gptDrawBackendCtx->atIndexBuffer[uFrameIdx] = pl__create_staging_buffer(&tBufferDesc, "draw idx buffer", uFrameIdx);
            gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] = 0;
        }
        plBuffer* ptIndexBuffer = gptGfx->get_buffer(ptDevice, gptDrawBackendCtx->atIndexBuffer[uFrameIdx]);
        memcpy(&((char*)ptIndexBuffer->tMemoryAllocation.pHostMapped)[gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx]],
            ptDrawlist->sbtLineIndexBuffer, uIdxBufSzNeeded);

        iLineVertexBase = ptBufferInfo->uVertexBufferOffset / sizeof(dcDrawVertex3DLine);
        iLineIndexBase  = gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] / sizeof(uint32_t);
        ptBufferInfo->uVertexBufferOffset += uVtxBufSzNeeded;
        gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] += uIdxBufSzNeeded;
    }

    // textured
    const dcPipelineEntry* ptTexturedEntry = NULL;
    if(pl_sb_size(ptDrawlist->sbtTexturedVertexBuffer) > 0u)
    {
        ptTexturedEntry = pl__get_3d_textured_pipeline(gptGfx->get_encoder_render_pass(ptEncoder), uMSAASampleCount, tFlags, gptGfx->get_render_encoder_subpass(ptEncoder));

        const uint32_t uVtxBufSzNeeded = sizeof(dcDrawVertex3DTextured) * pl_sb_size(ptDrawlist->sbtTexturedVertexBuffer);
        dcBufferInfo* ptBufferInfo = &gptDrawBackendCtx->at3DTexturedBufferInfo[uFrameIdx];
        const uint32_t uAvailableVertexBufferSpace = ptBufferInfo->uVertexBufferSize - ptBufferInfo->uVertexBufferOffset;
        if(uVtxBufSzNeeded >= uAvailableVertexBufferSpace)
        {
            gptGfx->queue_buffer_for_deletion(ptDevice, ptBufferInfo->tVertexBuffer);
            const plBufferDesc tBufferDesc = {
                .tUsage     = PL_BUFFER_USAGE_VERTEX,
                .szByteSize = pl_max(ptBufferInfo->uVertexBufferSize * 2, uVtxBufSzNeeded + uAvailableVertexBufferSpace),
                .pcDebugName = "3D Textured Vertex Buffer"
            };
            ptBufferInfo->uVertexBufferSize = (uint32_t)tBufferDesc.szByteSize;
            ptBufferInfo->tVertexBuffer = pl__create_staging_buffer(&tBufferDesc, "3d textured draw vtx buffer", uFrameIdx);
        }
        plBuffer* ptVertexBuffer = gptGfx->get_buffer(ptDevice, ptBufferInfo->tVertexBuffer);
        memcpy(&((char*)ptVertexBuffer->tMemoryAllocation.pHostMapped)[ptBufferInfo->uVertexBufferOffset],
            ptDrawlist->sbtTexturedVertexBuffer, uVtxBufSzNeeded);

        const uint32_t uIdxBufSzNeeded = sizeof(uint32_t) * pl_sb_size(ptDrawlist->sbtTexturedIndexBuffer);
        const uint32_t uAvailableIndexBufferSpace = gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] - gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx];
        if(uIdxBufSzNeeded >= uAvailableIndexBufferSpace)
        {
            gptGfx->queue_buffer_for_deletion(ptDevice, gptDrawBackendCtx->atIndexBuffer[uFrameIdx]);
            const plBufferDesc tBufferDesc = {
                .tUsage     = PL_BUFFER_USAGE_INDEX,
                .szByteSize = pl_max(gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] * 2, uIdxBufSzNeeded + uAvailableIndexBufferSpace),
                .pcDebugName = "Draw Index Buffer"
            };
            gptDrawBackendCtx->auIndexBufferSize[uFrameIdx] = (uint32_t)tBufferDesc.szByteSize;
            gptDrawBackendCtx->atIndexBuffer[uFrameIdx] = pl__create_staging_buffer(&tBufferDesc, "draw idx buffer", uFrameIdx);
            gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] = 0;
        }
        plBuffer* ptIndexBuffer = gptGfx->get_buffer(ptDevice, gptDrawBackendCtx->atIndexBuffer[uFrameIdx]);
        memcpy(&((char*)ptIndexBuffer->tMemoryAllocation.pHostMapped)[gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx]],
            ptDrawlist->sbtTexturedIndexBuffer, uIdxBufSzNeeded);

        iTexturedVertexBase = ptBufferInfo->uVertexBufferOffset / sizeof(dcDrawVertex3DTextured);
        iTexturedIndexBase  = gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] / sizeof(uint32_t);
        ptBufferInfo->uVertexBufferOffset += uVtxBufSzNeeded;
        gptDrawBackendCtx->auIndexBufferOffset[uFrameIdx] += uIdxBufSzNeeded;
    }

    //=========================================================================
    // Phase 2: Issue draw calls via command buffer
    //=========================================================================

    // allocate shared dynamic data for MVP
    plDynamicBinding tMvpDynamicData = pl_allocate_dynamic_data(gptGfx, gptDrawBackendCtx->ptDevice, &gptDrawBackendCtx->tCurrentDynamicDataBlock);
    plMat4* ptMvpDynamicData = (plMat4*)tMvpDynamicData.pcData;
    *ptMvpDynamicData = *ptMVP;

    // line dynamic data (MVP + aspect)
    typedef struct _dcLineDynamiceData
    {
        plMat4 tMVP;
        float fAspect;
        int   padding[3];
    } dcLineDynamiceData;

    plDynamicBinding tLineDynamicData = pl_allocate_dynamic_data(gptGfx, gptDrawBackendCtx->ptDevice, &gptDrawBackendCtx->tCurrentDynamicDataBlock);
    dcLineDynamiceData* ptLineDynamicData = (dcLineDynamiceData*)tLineDynamicData.pcData;
    ptLineDynamicData->tMVP = *ptMVP;
    ptLineDynamicData->fAspect = fAspectRatio;

    const uint32_t uCmdCount = pl_sb_size(ptDrawlist->sbtDrawCommands3D);
    if(uCmdCount > 0)
    {
        // reset 3D shader override state
        gptDrawBackendCtx->bCustom3DShaderActive = false;
        gptDrawBackendCtx->pt3dSolidShaderOverride = NULL;
        gptDrawBackendCtx->pt3dTexturedShaderOverride = NULL;

        for(uint32_t i = 0; i < uCmdCount; i++)
        {
            dcDrawCommand3D cmd = ptDrawlist->sbtDrawCommands3D[i];

            // callback (shader changes)
            if(cmd.tUserCallback != NULL)
            {
                if(cmd.tUserCallback == dcDrawCallback3DSetShader)
                {
                    dcShaderOverride3D* ptOverride = (dcShaderOverride3D*)cmd.pUserCallbackData;
                    if(ptOverride)
                    {
                        gptDrawBackendCtx->pt3dSolidShaderOverride = ptOverride->ptSolidShader;
                        gptDrawBackendCtx->pt3dTexturedShaderOverride = ptOverride->ptTexturedShader;
                        gptDrawBackendCtx->bCustom3DShaderActive = true;
                        PL_FREE(ptOverride);
                        ptDrawlist->sbtDrawCommands3D[i].pUserCallbackData = NULL;
                    }
                    else
                    {
                        gptDrawBackendCtx->pt3dSolidShaderOverride = NULL;
                        gptDrawBackendCtx->pt3dTexturedShaderOverride = NULL;
                        gptDrawBackendCtx->bCustom3DShaderActive = false;
                    }
                }
                continue;
            }

            if(cmd.uElementCount == 0)
                continue;

            if(cmd.eType == DC_DRAW_COMMAND_3D_SOLID)
            {
                plShaderHandle tShader = gptDrawBackendCtx->bCustom3DShaderActive && gptDrawBackendCtx->pt3dSolidShaderOverride
                    ? *gptDrawBackendCtx->pt3dSolidShaderOverride
                    : ptEntry->tRegularPipeline;

                gptGfx->bind_vertex_buffer(ptEncoder, gptDrawBackendCtx->at3DBufferInfo[uFrameIdx].tVertexBuffer);
                gptGfx->bind_shader(ptEncoder, tShader);
                gptGfx->bind_graphics_bind_groups(ptEncoder, tShader, 0, 0, NULL, 1, &tMvpDynamicData);

                const plDrawIndex tDrawIndex = {
                    .tIndexBuffer   = gptDrawBackendCtx->atIndexBuffer[uFrameIdx],
                    .uIndexCount    = cmd.uElementCount,
                    .uIndexStart    = iSolidIndexBase + cmd.uIndexOffset,
                    .uInstance      = 0,
                    .uInstanceCount = 1,
                    .uVertexStart   = iSolidVertexBase + cmd.uVertexOffset
                };
                gptGfx->draw_indexed(ptEncoder, 1, &tDrawIndex);
            }
            else if(cmd.eType == DC_DRAW_COMMAND_3D_LINE)
            {
                plShaderHandle tShader = ptEntry->tSecondaryPipeline;

                gptGfx->bind_vertex_buffer(ptEncoder, gptDrawBackendCtx->atLineBufferInfo[uFrameIdx].tVertexBuffer);
                gptGfx->bind_shader(ptEncoder, tShader);
                gptGfx->bind_graphics_bind_groups(ptEncoder, tShader, 0, 0, NULL, 1, &tLineDynamicData);

                const plDrawIndex tDrawIndex = {
                    .tIndexBuffer   = gptDrawBackendCtx->atIndexBuffer[uFrameIdx],
                    .uIndexCount    = cmd.uElementCount,
                    .uIndexStart    = iLineIndexBase + cmd.uIndexOffset,
                    .uInstance      = 0,
                    .uInstanceCount = 1,
                    .uVertexStart   = iLineVertexBase + cmd.uVertexOffset
                };
                gptGfx->draw_indexed(ptEncoder, 1, &tDrawIndex);
            }
            else if(cmd.eType == DC_DRAW_COMMAND_3D_TEXTURED && ptTexturedEntry)
            {
                plShaderHandle tShader = gptDrawBackendCtx->bCustom3DShaderActive && gptDrawBackendCtx->pt3dTexturedShaderOverride
                    ? *gptDrawBackendCtx->pt3dTexturedShaderOverride
                    : ptTexturedEntry->tRegularPipeline;

                plBindGroupHandle tTexture = {.uData = cmd.tTextureId};
                plBindGroupHandle atBindGroups[] = {
                    gptDrawBackendCtx->tFontSamplerBindGroup,
                    tTexture
                };

                gptGfx->bind_vertex_buffer(ptEncoder, gptDrawBackendCtx->at3DTexturedBufferInfo[uFrameIdx].tVertexBuffer);
                gptGfx->bind_shader(ptEncoder, tShader);
                gptGfx->bind_graphics_bind_groups(ptEncoder, tShader, 0, 2, atBindGroups, 1, &tMvpDynamicData);

                const plDrawIndex tDrawIndex = {
                    .tIndexBuffer   = gptDrawBackendCtx->atIndexBuffer[uFrameIdx],
                    .uIndexCount    = cmd.uElementCount,
                    .uIndexStart    = iTexturedIndexBase + cmd.uIndexOffset,
                    .uInstance      = 0,
                    .uInstanceCount = 1,
                    .uVertexStart   = iTexturedVertexBase + cmd.uVertexOffset
                };
                gptGfx->draw_indexed(ptEncoder, 1, &tDrawIndex);
            }
        }
    }
    else
    {
        // fallback: no command buffer (backward compatibility)
        if(pl_sb_size(ptDrawlist->sbtSolidVertexBuffer) > 0)
        {
            gptGfx->bind_vertex_buffer(ptEncoder, gptDrawBackendCtx->at3DBufferInfo[uFrameIdx].tVertexBuffer);
            gptGfx->bind_shader(ptEncoder, ptEntry->tRegularPipeline);
            gptGfx->bind_graphics_bind_groups(ptEncoder, ptEntry->tRegularPipeline, 0, 0, NULL, 1, &tMvpDynamicData);
            const plDrawIndex tDrawIndex = {
                .tIndexBuffer   = gptDrawBackendCtx->atIndexBuffer[uFrameIdx],
                .uIndexCount    = pl_sb_size(ptDrawlist->sbtSolidIndexBuffer),
                .uIndexStart    = iSolidIndexBase,
                .uInstance      = 0,
                .uInstanceCount = 1,
                .uVertexStart   = iSolidVertexBase
            };
            gptGfx->draw_indexed(ptEncoder, 1, &tDrawIndex);
        }
        if(pl_sb_size(ptDrawlist->sbtLineVertexBuffer) > 0u)
        {
            gptGfx->bind_vertex_buffer(ptEncoder, gptDrawBackendCtx->atLineBufferInfo[uFrameIdx].tVertexBuffer);
            gptGfx->bind_shader(ptEncoder, ptEntry->tSecondaryPipeline);
            gptGfx->bind_graphics_bind_groups(ptEncoder, ptEntry->tSecondaryPipeline, 0, 0, NULL, 1, &tLineDynamicData);
            const plDrawIndex tDrawIndex = {
                .tIndexBuffer   = gptDrawBackendCtx->atIndexBuffer[uFrameIdx],
                .uIndexCount    = pl_sb_size(ptDrawlist->sbtLineIndexBuffer),
                .uIndexStart    = iLineIndexBase,
                .uInstance      = 0,
                .uInstanceCount = 1,
                .uVertexStart   = iLineVertexBase
            };
            gptGfx->draw_indexed(ptEncoder, 1, &tDrawIndex);
        }
        if(pl_sb_size(ptDrawlist->sbtTexturedVertexBuffer) > 0u && ptTexturedEntry)
        {
            plBindGroupHandle tTexture = {.uData = ptDrawlist->tTexturedTexture};
            plBindGroupHandle atBindGroups[] = {
                gptDrawBackendCtx->tFontSamplerBindGroup,
                tTexture
            };
            gptGfx->bind_vertex_buffer(ptEncoder, gptDrawBackendCtx->at3DTexturedBufferInfo[uFrameIdx].tVertexBuffer);
            gptGfx->bind_shader(ptEncoder, ptTexturedEntry->tRegularPipeline);
            gptGfx->bind_graphics_bind_groups(ptEncoder, ptTexturedEntry->tRegularPipeline, 0, 2, atBindGroups, 1, &tMvpDynamicData);
            const plDrawIndex tDrawIndex = {
                .tIndexBuffer   = gptDrawBackendCtx->atIndexBuffer[uFrameIdx],
                .uIndexCount    = pl_sb_size(ptDrawlist->sbtTexturedIndexBuffer),
                .uIndexStart    = iTexturedIndexBase,
                .uInstance      = 0,
                .uInstanceCount = 1,
                .uVertexStart   = iTexturedVertexBase
            };
            gptGfx->draw_indexed(ptEncoder, 1, &tDrawIndex);
        }
    }

    const uint32_t uTextCount = pl_sb_size(ptDrawlist->sbtTextEntries);
    for(uint32_t i = 0; i < uTextCount; i++)
    {
        const dcDraw3DText* ptText = &ptDrawlist->sbtTextEntries[i];
        plVec4 tPos = pl_mul_mat4_vec4(ptMVP, (plVec4){.xyz = ptText->tP, .w = 1});
        tPos = pl_div_vec4_scalarf(tPos, tPos.w);
        if(!(tPos.z < 0.0f || tPos.z > 1.0f))
        {
            tPos.x = fWidth * 0.5f * (1.0f + tPos.x);
            tPos.y = fHeight * 0.5f * (1.0f + tPos.y);
            gptDraw->add_text(ptDrawlist->ptLayer,
                (plVec2){roundf(tPos.x + 0.5f), roundf(tPos.y + 0.5f)},
                ptText->acText,
                (dcDrawTextOptions){
                    .fSize = ptText->fSize,
                    .ptFont = ptText->ptFont,
                    .fWrap = ptText->fWrap,
                    .uColor = ptText->uColor});
        }
    }

    gptDraw->submit_2d_layer(ptDrawlist->ptLayer);
    pl_submit_2d_drawlist(ptDrawlist->pt2dDrawlist, ptEncoder, fWidth, fHeight, uMSAASampleCount);
    gptGfx->pop_render_debug_group(ptEncoder);
}

plBindGroupPool*
pl_draw_get_bind_group_pool(void)
{
    return gptDrawBackendCtx->ptBindGroupPool;
}

//-----------------------------------------------------------------------------
// [SECTION] extension loading
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_load_ext(plApiRegistryI* ptApiRegistry, bool bReload)
{

    const dcDrawBackendI tApi = {
        .initialize                    = pl_initialize_draw_backend,
        .cleanup                       = pl_cleanup_draw_backend,
        .new_frame                     = pl_new_draw_frame,
        .build_font_atlas              = pl_build_font_atlas_backend,
        .cleanup_font_atlas            = pl_cleanup_font_atlas_backend,
        .submit_2d_drawlist            = pl_submit_2d_drawlist,
        .submit_3d_drawlist            = pl_submit_3d_drawlist,
        .create_bind_group_for_texture = pl_create_bind_group_for_texture,
        .get_bind_group_pool           = pl_draw_get_bind_group_pool,
        .use_nearest_sampler           = pl_use_nearest_sampler,
        .use_linear_sampler            = pl_use_linear_sampler,
        .set_shader                    = pl_set_shader,
        .set_3d_shader                 = pl_set_3d_shader,
    };
    pl_set_api(ptApiRegistry, dcDrawBackendI, &tApi);

    gptMemory = pl_get_api_latest(ptApiRegistry, plMemoryI);
    gptStats  = pl_get_api_latest(ptApiRegistry, plStatsI);
    gptGfx    = pl_get_api_latest(ptApiRegistry, plGraphicsI);
    gptDraw   = pl_get_api_latest(ptApiRegistry, dcDrawI);
    gptShader = pl_get_api_latest(ptApiRegistry, plShaderI);
    gptLog    = pl_get_api_latest(ptApiRegistry, plLogI);
    gptIOI    = pl_get_api_latest(ptApiRegistry, plIOI);
    gptIO = gptIOI->get_io();
    const plDataRegistryI* ptDataRegistry = pl_get_api_latest(ptApiRegistry, plDataRegistryI);

    if(bReload)
    {
        gptDrawBackendCtx = ptDataRegistry->get_data("dcDrawBackendContext");
        uLogChannelDrawBackend = gptLog->get_channel_id("Draw Backend");
    }
    else  // first load
    {
        static dcDrawBackendContext tCtx = {0};
        gptDrawBackendCtx = &tCtx;
        ptDataRegistry->set_data("dcDrawBackendContext", gptDrawBackendCtx);

        plLogExtChannelInit tLogInit = {
            .tType       = PL_LOG_CHANNEL_TYPE_BUFFER | PL_LOG_CHANNEL_TYPE_CONSOLE,
            .uEntryCount = 256
        };
        uLogChannelDrawBackend = gptLog->add_channel("Draw Backend", tLogInit);
    }
}

PL_EXPORT void
pl_unload_ext(plApiRegistryI* ptApiRegistry, bool bReload)
{
    if(bReload)
        return;
        
    const dcDrawBackendI* ptApi = pl_get_api_latest(ptApiRegistry, dcDrawBackendI);
    ptApiRegistry->remove_api(ptApi);
}

//-----------------------------------------------------------------------------
// [SECTION] unity build
//-----------------------------------------------------------------------------

#ifndef PL_UNITY_BUILD

    #define PL_MEMORY_IMPLEMENTATION
    #include "pl_memory.h"
    #undef PL_MEMORY_IMPLEMENTATION

    #ifdef PL_USE_STB_SPRINTF
        #define STB_SPRINTF_IMPLEMENTATION
        #include "stb_sprintf.h"
        #undef STB_SPRINTF_IMPLEMENTATION
    #endif

#endif
