/*
   pl_planet_ext.c
*/

/*
Index of this file:
// [SECTION] includes
// [SECTION] defines
// [SECTION] forward declarations
// [SECTION] structs
// [SECTION] global data
// [SECTION] internal helpers (preprocessing)
// [SECTION] internal helpers (rendering)
// [SECTION] public api implementation
// [SECTION] internal helpers implementation (preprocessing)
// [SECTION] internal helpers implementation (rendering)
// [SECTION] extension loading
// [SECTION] unity build
*/

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include "pl.h"
#include "pl_planet_ext.h"

// libs
#define PL_MATH_INCLUDE_FUNCTIONS
#include "pl_math.h"
#undef pl_vnsprintf
#include "pl_memory.h"
#include "pl_string.h"

// stable extensions
#include "pl_platform_ext.h"
#include "pl_image_ext.h"
#include "pl_profile_ext.h"
#include "pl_graphics_ext.h"
#include "pl_gpu_allocators_ext.h"
#include "pl_starter_ext.h"
#include "pl_shader_ext.h"
#include "pl_screen_log_ext.h"
#include "pl_draw_ext.h"
#include "pl_vfs_ext.h"
#include "pl_pak_ext.h"

// unstable extensions
#include "pl_collision_ext.h"
#include "pl_freelist_ext.h"
#include "pl_camera_ext.h"
#include "pl_planet_processor_ext.h"
#include "pl_image_ops_ext.h"
#include "pl_resource_ext.h"

// shader interop
#include "pl_shader_interop_planet.h"

//-----------------------------------------------------------------------------
// [SECTION] defines
//-----------------------------------------------------------------------------

#define PL_REQUEST_QUEUE_SIZE 100

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

// basic types
typedef struct _plPlanetResidencyNode   plPlanetResidencyNode;
typedef struct _plPlanetReplacementNode plPlanetReplacementNode;
typedef struct _plPlanetContext plPlanetContext;

//-----------------------------------------------------------------------------
// [SECTION] global data
//-----------------------------------------------------------------------------


static const plMemoryI*  gptMemory = NULL;
#define PL_ALLOC(x)      gptMemory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_REALLOC(x, y) gptMemory->tracked_realloc((x), (y), __FILE__, __LINE__)
#define PL_FREE(x)       gptMemory->tracked_realloc((x), 0, __FILE__, __LINE__)

#ifndef PL_DS_ALLOC
    #define PL_DS_ALLOC(x)                      gptMemory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
    #define PL_DS_ALLOC_INDIRECT(x, FILE, LINE) gptMemory->tracked_realloc(NULL, (x), FILE, LINE)
    #define PL_DS_FREE(x)                       gptMemory->tracked_realloc((x), 0, __FILE__, __LINE__)
#endif

// required APIs
static const plImageI*            gptImage            = NULL;
static const plFileI*             gptFile             = NULL;
static const plProfileI*          gptProfile          = NULL;
static const plGraphicsI*         gptGfx              = NULL;
static const plFreeListI*         gptFreeList         = NULL;
static const plIOI*               gptIOI              = NULL;
static const plShaderI*           gptShader           = NULL;
static const plStarterI*          gptStarter          = NULL;
static const plCollisionI*        gptCollision        = NULL;
static const plScreenLogI*        gptScreenLog        = NULL;
static const plDrawI*             gptDraw             = NULL;
static const plPlanetProcessorI* gptTerrainProcessor  = NULL;
static const plGPUAllocatorsI*   gptGpuAllocators     = NULL;
static const plImageOpsI*        gptImageOps          = NULL;
static const plVfsI*             gptVfs               = NULL;
static const plResourceI*        gptResource          = NULL;
static const plPakI*             gptPak               = NULL;


#include "pl_ds.h"

// context
static plPlanetContext* gptCtx = NULL;

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plPlanetResidencyNode
{
    plPlanetResidencyNode* ptNext;
    plPlanetResidencyNode* ptPrev;
    plPlanetChunk*         ptChunk;
    uint64_t                uFrameRequested;
    float                   fDistance;
} plPlanetResidencyNode;

typedef struct _plOBB2
{
    plVec3 tCenter;
    plVec3 tExtents;
    plVec3 atAxes[3]; // Orthonormal basis
} plOBB2;

typedef struct _plChunkFileData
{
    plPlanetChunkFile tFile;
    plPakFile*        ptPakFile;
    char              acPakFileName[256];
} plChunkFileData;

typedef struct _plPlanet
{
    plPlanetRuntimeOptions tRuntimeOptions;
    plChunkFileData* sbtChunkFiles;
    double           dRadius;

    // shaders
    plShaderHandle tShader;
    plShaderHandle tWireframeShader;
    
    plPlanetResidencyNode tRequestQueue;
    plPlanetResidencyNode atRequests[PL_REQUEST_QUEUE_SIZE];
    uint32_t*             sbuFreeRequests;

    plPlanetChunk tReplacementQueue;

    plRenderPassHandle tRenderPass;
    plTextureHandle    tOutputTexture;
    plTextureHandle    tOutputTextureDepth;
    plBindGroupHandle  tOutputTextureHandle;
    uint32_t           uOutputWidth;
    uint32_t           uOutputHeight;
    plDrawList3D*      pt3dDrawlist;

    const char* pcVertexShader;
    const char* pcFragmentShader;
} plPlanet;

typedef struct _plPlanetContext
{
    plDevice*                ptDevice;
    plRenderPassLayoutHandle tRenderPassLayout;
    plBindGroupPool*         ptBindGroupPool;
    plDynamicDataBlock       tCurrentDynamicBufferBlock;

    // gpu allocators
    plDeviceMemoryAllocatorI* tLocalDedicatedAllocator;
    plDeviceMemoryAllocatorI* tLocalBuddyAllocator;

    // samplers
    plSamplerHandle tSampler;
    plTextureHandle tDummyTexture;
    uint32_t        uDummyIndex;

    // bindless texture system
    uint32_t          uTextureIndexCount;
    plHashMap64       tTextureIndexHashmap; // texture handle <-> index
    plBindGroupHandle atBindGroups[PL_MAX_FRAMES_IN_FLIGHT];

    char* sbcScratchBuffer;
    char* sbcScratchBuffer2;
    plTempAllocator tTempAllocator;

    plBufferHandle tStagingBuffer;
    uint32_t       uStagingBufferSize;

    plBufferHandle tIndexBuffer;
    plFreeList tIndexBufferManager;
    
    plBufferHandle tVertexBuffer;
    plFreeList tVertexBufferManager;
} plPlanetContext;



//-----------------------------------------------------------------------------
// [SECTION] internal helpers (rendering)
//-----------------------------------------------------------------------------

void pl_planet_load_shaders(plPlanet* ptPlanet);

// rendering
static void pl__handle_residency (plPlanet*, plCommandBuffer*);
static void pl__request_residency(plPlanet*, plPlanetChunk*, float);
static void pl__touch_chunk(plPlanet*, plPlanetChunk*);
static void pl__make_unresident  (plPlanet*, plPlanetChunk*);
static bool pl__planet_load(plPlanet* ptPlanet, plPlanetProcessInfo* ptInfo, plPlanetTexture* ptTexture, plPlanetLoadFlags tFlags);

static void pl__render_chunk(plPlanet*, plCamera*, plRenderEncoder*, plPlanetChunk*, plPlanetChunkFile*);
static bool pl__sat_visibility_test(plCamera*, const plAABB*);

static void pl__free_chunk(plPlanet* ptPlanet , float, uint64_t);

static plTextureHandle pl__planet_create_texture(plCommandBuffer* ptCmdBuffer, const plTextureDesc* ptDesc, const char* pcName, plTextureUsage);
static plTextureHandle pl__planet_create_texture_with_data (const plTextureDesc*, const char* pcName, uint32_t uIdentifier, const void*, size_t);
static uint32_t pl__planet_get_bindless_texture_index(plTextureHandle tTexture);
static void pl__planet_return_bindless_texture_index(plTextureHandle tTexture);


//-----------------------------------------------------------------------------
// [SECTION] public api implementation
//-----------------------------------------------------------------------------

void
pl_planet_initialize(plPlanetExtInit tInit)
{
    gptCtx->ptDevice = tInit.ptDevice;

    const plRenderPassLayoutDesc tRenderPassLayoutDesc = {
        .atRenderTargets = {
            { .tFormat = PL_FORMAT_D32_FLOAT_S8_UINT, .bDepth = true },  // depth buffer
            { .tFormat = PL_FORMAT_R8G8B8A8_UNORM }, // final output
        },
        .atSubpasses = {
            {
                .uRenderTargetCount = 2,
                .auRenderTargets = {0, 1},
            }
        },
        .atSubpassDependencies = {
            {
                .uSourceSubpass         = UINT32_MAX,
                .uDestinationSubpass    = 0,
                .tSourceStageMask       = PL_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT | PL_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS | PL_PIPELINE_STAGE_LATE_FRAGMENT_TESTS | PL_PIPELINE_STAGE_COMPUTE_SHADER,
                .tDestinationStageMask  = PL_PIPELINE_STAGE_FRAGMENT_SHADER | PL_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT | PL_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS | PL_PIPELINE_STAGE_LATE_FRAGMENT_TESTS,
                .tSourceAccessMask      = PL_ACCESS_COLOR_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ,
                .tDestinationAccessMask = PL_ACCESS_SHADER_READ | PL_ACCESS_COLOR_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ,
            },
            {
                .uSourceSubpass         = 0,
                .uDestinationSubpass    = UINT32_MAX,
                .tSourceStageMask       = PL_PIPELINE_STAGE_FRAGMENT_SHADER | PL_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT | PL_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS | PL_PIPELINE_STAGE_LATE_FRAGMENT_TESTS,
                .tDestinationStageMask  = PL_PIPELINE_STAGE_FRAGMENT_SHADER | PL_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT | PL_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS | PL_PIPELINE_STAGE_LATE_FRAGMENT_TESTS | PL_PIPELINE_STAGE_COMPUTE_SHADER,
                .tSourceAccessMask      = PL_ACCESS_SHADER_READ | PL_ACCESS_COLOR_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ,
                .tDestinationAccessMask = PL_ACCESS_SHADER_READ | PL_ACCESS_COLOR_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ,
            }
        }
    };
    gptCtx->tRenderPassLayout = gptGfx->create_render_pass_layout(tInit.ptDevice, &tRenderPassLayoutDesc);

    // create bind group pool
    plBindGroupPoolDesc tBindGroupPoolDesc = {
        .tFlags                   = PL_BIND_GROUP_POOL_FLAGS_NONE,
        .szSamplerBindings        = 1,
        .szSampledTextureBindings = PL_PLANET_MAX_BINDLESS_TEXTURES * 2,
        .szStorageTextureBindings = 1,
        .szStorageBufferBindings  = 1
    };
    gptCtx->ptBindGroupPool = gptGfx->create_bind_group_pool(tInit.ptDevice, &tBindGroupPoolDesc);

    // retrieve GPU allocators
    gptCtx->tLocalDedicatedAllocator   = gptGpuAllocators->get_local_dedicated_allocator(tInit.ptDevice);
    gptCtx->tLocalBuddyAllocator       = gptGpuAllocators->get_local_buddy_allocator(tInit.ptDevice);

    const plBindGroupLayoutDesc tBindGroupLayoutDesc = {
        .atSamplerBindings = {
            { .uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT}
        },
        .atTextureBindings = {
            {.uSlot = 1, .tStages = PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED, .bNonUniformIndexing = true, .uDescriptorCount = PL_PLANET_MAX_BINDLESS_TEXTURES}
        }
    };
    plBindGroupLayoutHandle tBindGroupLayout = gptGfx->create_bind_group_layout(tInit.ptDevice, &tBindGroupLayoutDesc);



    const plSamplerDesc tSamplerDesc = {
        .tMagFilter    = PL_FILTER_LINEAR,
        .tMinFilter    = PL_FILTER_LINEAR,
        .fMinMip       = 0.0f,
        .fMaxMip       = 1.0f,
        .tVAddressMode = PL_ADDRESS_MODE_CLAMP_TO_EDGE,
        .tUAddressMode = PL_ADDRESS_MODE_CLAMP_TO_EDGE,
        .pcDebugName   = "sampler"
    };
    gptCtx->tSampler = gptGfx->create_sampler(tInit.ptDevice, &tSamplerDesc);



    const plBindGroupUpdateSamplerData tSamplerData = {
        .tSampler = gptCtx->tSampler,
        .uSlot    = 0
    };
    const plBindGroupUpdateData tBGSet0Data = {
        .uSamplerCount = 1,
        .atSamplerBindings = &tSamplerData
    };

    for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
    {
        // create global bindgroup
        const plBindGroupDesc tGlobalBindGroupDesc = {
            .ptPool      = gptCtx->ptBindGroupPool,
            .tLayout     = tBindGroupLayout,
            .pcDebugName = "global bind group"
        };
        gptCtx->atBindGroups[i] = gptGfx->create_bind_group(tInit.ptDevice, &tGlobalBindGroupDesc);

        gptGfx->update_bind_group(tInit.ptDevice, gptCtx->atBindGroups[i], &tBGSet0Data);
    }

    const plTextureDesc tDummyTextureDesc = {
        .tDimensions   = {2, 2, 1},
        .tFormat       = PL_FORMAT_R32G32B32A32_FLOAT,
        .uLayers       = 1,
        .uMips         = 1,
        .tType         = PL_TEXTURE_TYPE_2D,
        .tUsage        = PL_TEXTURE_USAGE_SAMPLED,
        .pcDebugName   = "dummy"
    };

    const float afDummyTextureData[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
    gptCtx->tDummyTexture = pl__planet_create_texture_with_data(&tDummyTextureDesc, "dummy", 0, afDummyTextureData, sizeof(afDummyTextureData));
    gptCtx->uDummyIndex = pl__planet_get_bindless_texture_index(gptCtx->tDummyTexture);

    if(tInit.uStagingBufferSize == 0) tInit.uStagingBufferSize = 268435456;
    if(tInit.uIndexBufferSize == 0)   tInit.uIndexBufferSize = 268435456;
    if(tInit.uVertexBufferSize == 0)  tInit.uVertexBufferSize = 268435456;
    gptCtx->uStagingBufferSize = tInit.uStagingBufferSize;

    gptFreeList->create(tInit.uVertexBufferSize, 256, &gptCtx->tVertexBufferManager);
    gptFreeList->create(tInit.uIndexBufferSize, 256, &gptCtx->tIndexBufferManager);

    plDevice* ptDevice = gptCtx->ptDevice;

    const plBufferDesc tVertexBufferDesc = {
        .tUsage      = PL_BUFFER_USAGE_VERTEX | PL_BUFFER_USAGE_TRANSFER_DESTINATION,
        .szByteSize  = gptCtx->tVertexBufferManager.uSize,
        .pcDebugName = "vertex buffer"
    };
    gptCtx->tVertexBuffer = gptGfx->create_buffer(ptDevice, &tVertexBufferDesc, NULL);

    // retrieve buffer to get memory allocation requirements (do not store buffer pointer)
    plBuffer* ptVertexBuffer = gptGfx->get_buffer(ptDevice, gptCtx->tVertexBuffer);

    // allocate memory for the vertex buffer
    const plDeviceMemoryAllocation tVertexBufferAllocation = gptGfx->allocate_memory(ptDevice,
        ptVertexBuffer->tMemoryRequirements.ulSize,
        PL_MEMORY_FLAGS_DEVICE_LOCAL,
        ptVertexBuffer->tMemoryRequirements.uMemoryTypeBits,
        "vertex buffer memory");

    // bind the buffer to the new memory allocation
    gptGfx->bind_buffer_to_memory(ptDevice, gptCtx->tVertexBuffer, &tVertexBufferAllocation);

    // create index buffer
    const plBufferDesc tIndexBufferDesc = {
        .tUsage      = PL_BUFFER_USAGE_INDEX | PL_BUFFER_USAGE_TRANSFER_DESTINATION,
        .szByteSize  = gptCtx->tIndexBufferManager.uSize,
        .pcDebugName = "index buffer"
    };
    gptCtx->tIndexBuffer = gptGfx->create_buffer(ptDevice, &tIndexBufferDesc, NULL);

    // retrieve buffer to get memory allocation requirements (do not store buffer pointer)
    plBuffer* ptIndexBuffer = gptGfx->get_buffer(ptDevice, gptCtx->tIndexBuffer);

    // allocate memory for the index buffer
    const plDeviceMemoryAllocation tIndexBufferAllocation = gptGfx->allocate_memory(ptDevice,
        ptIndexBuffer->tMemoryRequirements.ulSize,
        PL_MEMORY_FLAGS_DEVICE_LOCAL,
        ptIndexBuffer->tMemoryRequirements.uMemoryTypeBits,
        "index buffer memory");

    // bind the buffer to the new memory allocation
    gptGfx->bind_buffer_to_memory(ptDevice, gptCtx->tIndexBuffer, &tIndexBufferAllocation);

    // create vertex buffer
    const plBufferDesc tStagingBufferDesc = {
        .tUsage      = PL_BUFFER_USAGE_TRANSFER_SOURCE,
        .szByteSize  = gptCtx->uStagingBufferSize,
        .pcDebugName = "cdlod staging buffer"
    };
    gptCtx->tStagingBuffer = gptGfx->create_buffer(ptDevice, &tStagingBufferDesc, NULL);

    // retrieve buffer to get memory allocation requirements (do not store buffer pointer)
    plBuffer* ptStagingBuffer = gptGfx->get_buffer(ptDevice, gptCtx->tStagingBuffer);

    // allocate memory for the vertex buffer
    const plDeviceMemoryAllocation tStagingBufferAllocation = gptGfx->allocate_memory(ptDevice,
        ptStagingBuffer->tMemoryRequirements.ulSize,
        PL_MEMORY_FLAGS_HOST_VISIBLE | PL_MEMORY_FLAGS_HOST_COHERENT,
        ptStagingBuffer->tMemoryRequirements.uMemoryTypeBits,
        "staging buffer memory");

    // bind the buffer to the new memory allocation
    gptGfx->bind_buffer_to_memory(ptDevice, gptCtx->tStagingBuffer, &tStagingBufferAllocation);
}

void
pl_planet_cleanup(void)
{
    plDevice* ptDevice = gptCtx->ptDevice;

    gptGfx->destroy_buffer(ptDevice, gptCtx->tVertexBuffer);
    gptGfx->destroy_buffer(ptDevice, gptCtx->tIndexBuffer);
    gptGfx->destroy_buffer(ptDevice, gptCtx->tStagingBuffer);
    gptFreeList->cleanup(&gptCtx->tVertexBufferManager);
    gptFreeList->cleanup(&gptCtx->tIndexBufferManager);
    pl_sb_free(gptCtx->sbcScratchBuffer);
    pl_sb_free(gptCtx->sbcScratchBuffer2);
    pl_hm_free(&gptCtx->tTextureIndexHashmap);
    pl_temp_allocator_free(&gptCtx->tTempAllocator);
    gptGfx->cleanup_bind_group_pool(gptCtx->ptBindGroupPool);
    gptGfx->destroy_render_pass_layout(gptCtx->ptDevice, gptCtx->tRenderPassLayout);
    gptGpuAllocators->cleanup(gptCtx->ptDevice);
}

plPlanet*
pl_create_planet(plCommandBuffer* ptCmdBuffer, plPlanetInit tInit, plPlanetProcessInfo* ptInfo)
{
    plPlanet* ptPlanet = PL_ALLOC(sizeof(plPlanet));
    memset(ptPlanet, 0, sizeof(plPlanet));


    ptPlanet->tRuntimeOptions.fTau = 0.3f;
    ptPlanet->tRuntimeOptions.tLightDirection = (plVec3){-1.0f, -1.0f, -1.0f};

    if(tInit.pcVertexShader == NULL) tInit.pcVertexShader = "planet.vert";
    if(tInit.pcFragmentShader == NULL) tInit.pcFragmentShader = "planet.frag";

    ptPlanet->pcVertexShader = tInit.pcVertexShader;
    ptPlanet->pcFragmentShader = tInit.pcFragmentShader;
    pl_planet_load_shaders(ptPlanet);

    ptPlanet->uOutputWidth = tInit.uOutputWidth;
    ptPlanet->uOutputHeight = tInit.uOutputHeight;
    ptPlanet->dRadius = tInit.dRadius;
    
    ptPlanet->pt3dDrawlist = gptDraw->request_3d_drawlist();

    pl_sb_resize(ptPlanet->sbuFreeRequests, PL_REQUEST_QUEUE_SIZE);

    for(uint32_t i = 0; i < PL_REQUEST_QUEUE_SIZE; i++) 
    {
        ptPlanet->sbuFreeRequests[i] = i;
    }

    const plTextureDesc tOutputTextureDesc = {
        .tDimensions   = {(float)ptPlanet->uOutputWidth, (float)ptPlanet->uOutputHeight, 1},
        .tFormat       = PL_FORMAT_R8G8B8A8_UNORM,
        .uLayers       = 1,
        .uMips         = 1,
        .tType         = PL_TEXTURE_TYPE_2D,
        .tUsage        = PL_TEXTURE_USAGE_SAMPLED | PL_TEXTURE_USAGE_COLOR_ATTACHMENT,
        .pcDebugName   = "final output"
    };
    ptPlanet->tOutputTexture = pl__planet_create_texture(ptCmdBuffer, &tOutputTextureDesc, "final output", PL_TEXTURE_USAGE_SAMPLED);
    ptPlanet->tOutputTextureHandle = gptDraw->create_bind_group_for_texture(ptPlanet->tOutputTexture);

    const plTextureDesc tDepthTextureDesc = {
        .tDimensions   = {(float)ptPlanet->uOutputWidth, (float)ptPlanet->uOutputHeight, 1},
        .tFormat       = PL_FORMAT_D32_FLOAT_S8_UINT,
        .uLayers       = 1,
        .uMips         = 1,
        .tType         = PL_TEXTURE_TYPE_2D,
        .tUsage        = PL_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT,
        .pcDebugName   = "final output depth"
    };
    ptPlanet->tOutputTextureDepth = pl__planet_create_texture(ptCmdBuffer, &tDepthTextureDesc, "final output depth", PL_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT);

    plRenderPassAttachments atAttachmentSets[PL_MAX_FRAMES_IN_FLIGHT] = {0};

    for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
    {
        atAttachmentSets[i].atViewAttachments[0] = ptPlanet->tOutputTextureDepth;
        atAttachmentSets[i].atViewAttachments[1] = ptPlanet->tOutputTexture;
    }

    const plRenderPassDesc tRenderPassDesc = {
        .tLayout = gptCtx->tRenderPassLayout,
        .tDepthTarget = {
                .tLoadOp         = PL_LOAD_OP_CLEAR,
                .tStoreOp        = PL_STORE_OP_STORE,
                .tStencilLoadOp  = PL_LOAD_OP_CLEAR,
                .tStencilStoreOp = PL_STORE_OP_STORE,
                .tCurrentUsage   = PL_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT,
                .tNextUsage      = PL_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT,
                .fClearZ         = 0.0f
        },
        .atColorTargets = {
            {
                .tLoadOp       = PL_LOAD_OP_CLEAR,
                .tStoreOp      = PL_STORE_OP_STORE,
                .tCurrentUsage = PL_TEXTURE_USAGE_SAMPLED,
                .tNextUsage    = PL_TEXTURE_USAGE_SAMPLED,
                .tClearColor   = {0.0f, 0.0f, 0.0f, 1.0f}
            }
        },
        .tDimensions = {.x = (float)ptPlanet->uOutputWidth, .y = (float)ptPlanet->uOutputHeight},
        .pcDebugName = "Main"
    };
    ptPlanet->tRenderPass = gptGfx->create_render_pass(gptCtx->ptDevice, &tRenderPassDesc, atAttachmentSets);

    pl__planet_load(ptPlanet, ptInfo, tInit.atTextures[0].fMetersPerPixel > 0.0f ? &tInit.atTextures[0] : NULL, tInit.tLoadFlags);

    for(uint32_t i = 0; i < pl_sb_size(ptPlanet->sbtChunkFiles); i++)
        pl__request_residency(ptPlanet, &ptPlanet->sbtChunkFiles[i].tFile.atChunks[0], 0.0f);
    return ptPlanet;
}

void
pl_cleanup_planet(plPlanet* ptPlanet)
{
    plDevice* ptDevice = gptCtx->ptDevice;

    for(uint32_t i = 0; i < pl_sb_size(ptPlanet->sbtChunkFiles); i++)
    {
        PL_FREE(ptPlanet->sbtChunkFiles[i].tFile.atChunks);
        ptPlanet->sbtChunkFiles[i].tFile.atChunks = NULL;
        ptPlanet->sbtChunkFiles[i].tFile.uChunkCount = 0;
        ptPlanet->sbtChunkFiles[i].tFile.fMaxBaseError = 0.0f;
        ptPlanet->sbtChunkFiles[i].tFile.iTreeDepth = 0;
        if(ptPlanet->sbtChunkFiles[i].ptPakFile)
            gptPak->unload(&ptPlanet->sbtChunkFiles[i].ptPakFile);
    }

    // cleanup our resources

    gptGfx->destroy_render_pass(ptDevice, ptPlanet->tRenderPass);
    gptGfx->destroy_texture(ptDevice, ptPlanet->tOutputTexture);
    gptGfx->destroy_texture(ptDevice, ptPlanet->tOutputTextureDepth);



    pl_sb_free(ptPlanet->sbuFreeRequests);
    pl_sb_free(ptPlanet->sbtChunkFiles);
    PL_FREE(ptPlanet);
}

plBindGroupHandle
pl_get_planet_texture(plPlanet* ptPlanet)
{
    return ptPlanet->tOutputTextureHandle;
}

bool
pl_chlod_load_chunk_file(plPlanet* ptPlanet, const char* pcPath, const char* pcTexture, plPlanetLoadFlags tFlags)
{
    plChunkFileData tChunkFileData = {0};
    uint32_t uChunkFileID = pl_sb_size(ptPlanet->sbtChunkFiles);
    gptTerrainProcessor->load_chunk_file(pcPath, &tChunkFileData.tFile, uChunkFileID);

    plPakInfo tPakFileInfo = {0};

    char acFileNameOnly[256] = {0};
    pl_str_get_file_name_only(pcPath, acFileNameOnly, 256);


    bool bTextured = pcTexture != NULL;
    bool bPacking = false;
    plImageOpData tImageOpData = {0};
    if(bTextured)
    {
        sprintf(tChunkFileData.acPakFileName, "/cache/%s.pak", acFileNameOnly);
        
        if(tFlags & PL_PLANET_LOAD_FLAGS_CACHE_TEXTURES && gptVfs->does_file_exist(tChunkFileData.acPakFileName))
        {
            gptPak->load(tChunkFileData.acPakFileName, &tPakFileInfo, &tChunkFileData.ptPakFile);
        }
        else
        {
            plVfsFileHandle tPakFileHandle = gptVfs->register_file(tChunkFileData.acPakFileName, false);
            const char* pcPhysicalPath = gptVfs->get_real_path(tPakFileHandle);
            gptPak->begin_packing(pcPhysicalPath, 1, &tChunkFileData.ptPakFile);
            bPacking = true;


            // load actual data from file data
            int iImageWidth = 0;
            int iImageHeight = 0;
            int _unused;
            unsigned char* pucImageData = gptImage->load_from_file(pcTexture,  &iImageWidth, &iImageHeight, &_unused, 4);

            plImageOpInfo tImageOpInfo = {
                .uChannels = 4,
                .uStride = 4,
                .uWidth = (uint32_t)iImageWidth,
                .uHeight = (uint32_t)iImageHeight
            };
        
            gptImageOps->initialize(&tImageOpInfo, &tImageOpData);

            plImageOpInfo tImageOpInfo0 = {
                .uChannels = 4,
                .uStride = 4,
                .uWidth = (uint32_t)iImageWidth,
                .uHeight = (uint32_t)iImageHeight,
                .puData = pucImageData
            };
            gptImageOps->add(&tImageOpData, tImageOpInfo0, 0, 0);
            gptImage->free(pucImageData);
            gptImageOps->square(&tImageOpData);
            // gptImageOps->downsample(&tImageOpData, 1);
        }
    }

    for(uint32_t i = 0; i < tChunkFileData.tFile.uChunkCount; i++)
    {

        tChunkFileData.tFile.atChunks[i].uIndex = i;

        if(bPacking)
        {
            uint32_t uTopDownLevel = tChunkFileData.tFile.iTreeDepth - tChunkFileData.tFile.atChunks[i].uLevel - 1;

            plImageOpData tImageOpDataMod = {0};
            uint32_t uWidth = (uint32_t)((float)tImageOpData.uWidth / powf(2.0f, (float)uTopDownLevel));
            uint32_t uHeight = (uint32_t)((float)tImageOpData.uHeight / powf(2.0f, (float)uTopDownLevel));

            float fXRatio = (float)tChunkFileData.tFile.atChunks[i].uX / 4096.0f; // UV on original heightmap
            float fYRatio = (float)tChunkFileData.tFile.atChunks[i].uY / 4096.0f; // UV on original heightmap

            uint32_t uXOffset = (uint32_t)((float)tImageOpData.uWidth * fXRatio);  // pixels on texture map
            uint32_t uYOffset = (uint32_t)((float)tImageOpData.uHeight * fYRatio); // pixels on texture map


            if (uXOffset + uWidth > tImageOpData.uWidth)    uWidth = tImageOpData.uWidth  - uXOffset;
            if (uYOffset + uHeight > tImageOpData.uHeight)  uHeight = tImageOpData.uHeight - uYOffset;


            gptImageOps->extract(&tImageOpData,
                uXOffset,
                uYOffset,
                uWidth, uHeight, &tImageOpDataMod);

            plImageWriteInfo tImageWriteInfo = {
                .iWidth = (int)uWidth,
                .iHeight = (int)uHeight,
                .iComponents = 4,
                .iByteStride = 4 * uWidth
            };
            
            pl_sb_sprintf(gptCtx->sbcScratchBuffer2, "%u_tile_%u.png", uChunkFileID, i);
            gptImage->write(gptCtx->sbcScratchBuffer2, tImageOpDataMod.puData, &tImageWriteInfo);
            gptImageOps->cleanup(&tImageOpDataMod);
            pl_sb_sprintf(gptCtx->sbcScratchBuffer, "%s/%u_tile_%u.png", tChunkFileData.acPakFileName, uChunkFileID, i);
            gptPak->add_from_disk(tChunkFileData.ptPakFile, gptCtx->sbcScratchBuffer, gptCtx->sbcScratchBuffer2, false);
            gptFile->remove(gptCtx->sbcScratchBuffer2);
            pl_sb_reset(gptCtx->sbcScratchBuffer);
            pl_sb_reset(gptCtx->sbcScratchBuffer2);
        }
        
        FILE* ptDataFile = fopen(tChunkFileData.tFile.acFile, "rb");
        fseek(ptDataFile, (long)tChunkFileData.tFile.atChunks[i].szFileLocation, SEEK_SET);

        fseek(ptDataFile, sizeof(plVec3) * 2 + sizeof(int) * 4, SEEK_CUR);

        uint32_t uVertexCount = 0;
        fread(&uVertexCount, 1, sizeof(uint32_t), ptDataFile);
        fseek(ptDataFile, sizeof(plPlanetVertex) * uVertexCount, SEEK_CUR);

        uint32_t uIndexCount = 0;
        fread(&uIndexCount, 1, sizeof(uint32_t), ptDataFile);
        
        fclose(ptDataFile);
    }
    
    if(bPacking)
    {
        gptImageOps->cleanup(&tImageOpData);
        gptPak->end_packing(&tChunkFileData.ptPakFile);
        gptPak->load(tChunkFileData.acPakFileName, &tPakFileInfo, &tChunkFileData.ptPakFile);
    }

    pl_sb_push(ptPlanet->sbtChunkFiles, tChunkFileData);

    return true;
}

void
pl_planet_load_shaders(plPlanet* ptPlanet)
{
    if(gptGfx->is_shader_valid(gptCtx->ptDevice, ptPlanet->tShader))
    {
        gptGfx->queue_shader_for_deletion(gptCtx->ptDevice, ptPlanet->tShader);
        gptGfx->queue_shader_for_deletion(gptCtx->ptDevice, ptPlanet->tWireframeShader);
    }

    plShaderDesc tShaderDesc = {
        .tVertexShader    = gptShader->load_glsl(ptPlanet->pcVertexShader, "main", NULL, NULL),
        .tFragmentShader  = gptShader->load_glsl(ptPlanet->pcFragmentShader, "main", NULL, NULL),
        .tGraphicsState = {
            .ulDepthWriteEnabled  = 1,
            .ulDepthMode          = PL_COMPARE_MODE_GREATER_OR_EQUAL,
            .ulCullMode           = PL_CULL_MODE_CULL_BACK,
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
                .atAttributes = {
                    {.tFormat = PL_VERTEX_FORMAT_FLOAT3},
                    {.tFormat = PL_VERTEX_FORMAT_FLOAT2},
                    {.tFormat = PL_VERTEX_FORMAT_FLOAT2}
                }
            }
        },
        .atBlendStates = {
            {
                .bBlendEnabled   = false,
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
                    { .uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT}
                },
                .atTextureBindings = {
                    {.uSlot = 1, .tStages = PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED, .bNonUniformIndexing = true, .uDescriptorCount = PL_PLANET_MAX_BINDLESS_TEXTURES}
                }
            }
        },
        .tRenderPassLayout = gptCtx->tRenderPassLayout,
    };
    plDevice* ptDevice = gptCtx->ptDevice;
    ptPlanet->tShader = gptGfx->create_shader(ptDevice, &tShaderDesc);

    tShaderDesc.tGraphicsState.ulWireframe = 1;
    tShaderDesc.tGraphicsState.ulDepthWriteEnabled = 0;
    tShaderDesc.tGraphicsState.ulDepthMode = PL_COMPARE_MODE_ALWAYS;
    ptPlanet->tWireframeShader = gptGfx->create_shader(ptDevice, &tShaderDesc);
}

static void
pl_planet_set_shaders(plPlanet* ptPlanet, const char* pcVertexShader, const char* pcFragmentShader)
{
    ptPlanet->pcVertexShader   = pcVertexShader   ? pcVertexShader   : "planet.vert";
    ptPlanet->pcFragmentShader = pcFragmentShader ? pcFragmentShader : "planet.frag";
    pl_planet_load_shaders(ptPlanet);
}

void
pl_draw_sphere(plPlanet* ptPlanet, float fLongitude, float fLatitude, float fHeight, float fSphereRadius, uint32_t uColor)
{
    fLongitude = pl_radiansf(fLongitude);
    fLatitude = pl_radiansf(fLatitude);
    
    // float fLongitude = pl_radiansf(-29.0f);
    // float fLatitude = pl_radiansf(-45.0f);
    float fRadius = (float)ptPlanet->dRadius + fHeight;

    float fX = fRadius * cosf(fLatitude) * cosf(fLongitude);
    float fY = fRadius * sinf(fLatitude);
    float fZ = fRadius * cosf(fLatitude) * sinf(fLongitude);

    plSphere tSphere = {
        .fRadius = fSphereRadius,
        .tCenter = {
            fX, fY, fZ
        }
    };

    gptDraw->add_3d_sphere_filled(ptPlanet->pt3dDrawlist, tSphere, 0, 0, (plDrawSolidOptions){.uColor = uColor});
}

void
pl_prepare_planet(plPlanet* ptPlanet, plCommandBuffer* ptCmdBuffer)
{
    if(ptPlanet->tRequestQueue.ptNext)
    {
        gptScreenLog->add_message_ex(294, 10.0, PL_COLOR_32_YELLOW, 1.0f, "Stream Active");
        pl__handle_residency(ptPlanet, ptCmdBuffer);
    }
    else
    {
        gptScreenLog->add_message_ex(294, 10.0, PL_COLOR_32_RED, 1.0f, "Stream Inactive");
    }
}

void
pl_render_planet(plPlanet* ptPlanet, plCamera* ptCamera, plCommandBuffer* ptCmdBuffer)
{


    // pl_prepare_planet(ptPlanet, ptCmdBuffer);

    plDevice* ptDevice = gptCtx->ptDevice;
    gptCtx->tCurrentDynamicBufferBlock = gptGfx->allocate_dynamic_data_block(ptDevice);

    plRenderEncoder* ptEncoder = gptGfx->begin_render_pass(ptCmdBuffer, ptPlanet->tRenderPass, NULL);

    plRenderViewport tViewport = {
        .fWidth = (float)ptPlanet->uOutputWidth,
        .fHeight = (float)ptPlanet->uOutputHeight,
        .fMinDepth = 0.0f,
        .fMaxDepth = 1.0f
    };
    gptGfx->set_viewport(ptEncoder, &tViewport);

    plScissor tScissor = {
        .uWidth  = ptPlanet->uOutputWidth,
        .uHeight = ptPlanet->uOutputHeight
    };
    gptGfx->set_scissor_region(ptEncoder, &tScissor);
    // gptGfx->set_depth_bias(ptEncoder, -1.25f, 0.0f, -10.75f);
    gptGfx->set_depth_bias(ptEncoder, 0.0f, 0.0f, 0.0f);

    gptScreenLog->add_message_ex(193, 10.0, PL_COLOR_32_GREEN, 1.0f, "Index Buffer Usage:  %0.2f %%", 100.0f * (float)gptCtx->tIndexBufferManager.uUsedSpace / (float)gptCtx->tIndexBufferManager.uSize);
    gptScreenLog->add_message_ex(194, 10.0, PL_COLOR_32_GREEN, 1.0f, "Vertex Buffer Usage:  %0.2f %%", 100.0f * (float)gptCtx->tVertexBufferManager.uUsedSpace / (float)gptCtx->tVertexBufferManager.uSize);

    for(uint32_t i = 0; i < pl_sb_size(ptPlanet->sbtChunkFiles); i++)
        pl__render_chunk(ptPlanet, ptCamera, ptEncoder, &ptPlanet->sbtChunkFiles[i].tFile.atChunks[0], &ptPlanet->sbtChunkFiles[i].tFile);
    
    const plMat4 tMVP = pl_mul_mat4(&ptCamera->tProjMat, &ptCamera->tViewMat);

    if(ptPlanet->tRuntimeOptions.tFlags & PL_PLANET_FLAGS_SHOW_ORIGIN)
    {
        const plMat4 tOrigin = pl_identity_mat4();
        gptDraw->add_3d_transform(ptPlanet->pt3dDrawlist, &tOrigin, 1737400.0f * 1.2f, (plDrawLineOptions){0, 1000.0f});
    }

    gptDraw->submit_3d_drawlist(ptPlanet->pt3dDrawlist,
        ptEncoder,
        (float)ptPlanet->uOutputWidth,
        (float)ptPlanet->uOutputHeight,
        &tMVP,
        PL_DRAW_FLAG_DEPTH_TEST | PL_DRAW_FLAG_DEPTH_WRITE | PL_DRAW_FLAG_REVERSE_Z_DEPTH,
        PL_SAMPLE_COUNT_1);

    gptGfx->end_render_pass(ptEncoder);

}

void
pl_planet_set_runtime_options(plPlanet* ptPlanet, plPlanetRuntimeOptions tOptions)
{
    ptPlanet->tRuntimeOptions = tOptions;
}

plPlanetRuntimeOptions
pl_planet_get_runtime_options(plPlanet* ptPlanet)
{
    return ptPlanet->tRuntimeOptions;
}

//-----------------------------------------------------------------------------
// [SECTION] internal api implementation
//-----------------------------------------------------------------------------

static void
pl__unload_children(plPlanet* ptPlanet, plPlanetChunk* ptChunk)
{
    if(ptChunk->aptChildren[0] == NULL)
        return;
    if(ptChunk->aptChildren[0]->ptIndexHole) pl__make_unresident(ptPlanet, ptChunk->aptChildren[0]);
    if(ptChunk->aptChildren[1]->ptIndexHole) pl__make_unresident(ptPlanet, ptChunk->aptChildren[1]);
    if(ptChunk->aptChildren[2]->ptIndexHole) pl__make_unresident(ptPlanet, ptChunk->aptChildren[2]);
    if(ptChunk->aptChildren[3]->ptIndexHole) pl__make_unresident(ptPlanet, ptChunk->aptChildren[3]);    
}

bool
pl__all_children_resident(plPlanetChunk* ptChunk)
{
    if(ptChunk->aptChildren[0] == NULL)
        return false;
    if(ptChunk->aptChildren[0]->ptIndexHole == NULL) return false;
    if(ptChunk->aptChildren[1]->ptIndexHole == NULL) return false;
    if(ptChunk->aptChildren[2]->ptIndexHole == NULL) return false;
    if(ptChunk->aptChildren[3]->ptIndexHole == NULL) return false;
    return true;
}

void
pl__remove_from_replacement_queue(plPlanet* ptPlanet, plPlanetChunk* ptChunk)
{
    plPlanetChunk* ptCurrentRequest = ptPlanet->tReplacementQueue.ptNext;

    plPlanetChunk* ptExistingRequest = NULL;
    plPlanetChunk* ptLastRequest = NULL;

    while(ptCurrentRequest)
    {
        if(ptCurrentRequest == ptChunk)
        {
            ptExistingRequest = ptCurrentRequest;
            break;
        }
        ptCurrentRequest = ptCurrentRequest->ptNext;
    }

    if(ptExistingRequest)
    {

        // remove node
        if(ptExistingRequest->ptPrev)
            ptExistingRequest->ptPrev->ptNext = ptExistingRequest->ptNext;
        else
            ptPlanet->tReplacementQueue.ptNext = ptExistingRequest->ptNext;

        if(ptExistingRequest->ptNext)
            ptExistingRequest->ptNext->ptPrev = ptExistingRequest->ptPrev;
    }
}

static void
pl__free_chunk(plPlanet* ptPlanet, float fDistance, uint64_t uIndexCount)
{
    const uint64_t uCurrentFrame = gptIOI->get_io()->ulFrameCount;

    // find last used item
    plPlanetChunk* ptCurrentRequest = ptPlanet->tReplacementQueue.ptNext;

    if(ptCurrentRequest == NULL)
        return;

    while(ptCurrentRequest->ptNext)
        ptCurrentRequest = ptCurrentRequest->ptNext;

    plPlanetChunk* ptEndRequest = ptCurrentRequest;

    bool bChunkFreed = false;
    while(ptCurrentRequest->ptPrev)
    {
        if(ptCurrentRequest->uIndexCount > uIndexCount)
        {
            pl__make_unresident(ptPlanet, ptCurrentRequest);
            bChunkFreed = true;
            break;
        }
        ptCurrentRequest = ptCurrentRequest->ptPrev;
    }

    ptCurrentRequest = ptEndRequest;
    if(!bChunkFreed)
    {
        while(ptCurrentRequest->ptPrev)
        {
            if(uCurrentFrame - ptCurrentRequest->uLastFrameUsed > 30)
            {
                plPlanetChunk* ptChunkToRemove = ptCurrentRequest;
                ptCurrentRequest = ptCurrentRequest->ptPrev;
                pl__make_unresident(ptPlanet, ptChunkToRemove);
                bChunkFreed = true;
                // break;
            }
            else
                ptCurrentRequest = ptCurrentRequest->ptPrev;
        }
    }

    
    // if(uCurrentFrame - ptCurrentRequest->uLastFrameUsed > 1)
    // {
    //     pl__make_unresident(ptPlanet, ptCurrentRequest);
    //     bChunkFreed = true;
    // }
    
    // find further distance chunk to release
    // if(!bChunkFreed)
    // {
    //     ptCurrentRequest = ptPlanet->tReplacementQueue.ptNext;
    //     plPlanetChunk* ptFurthestChunk = ptCurrentRequest;

    //     while(ptCurrentRequest)
    //     {

    //         if(ptCurrentRequest->uIndexCount >= ptFurthestChunk->uIndexCount)
    //         {
    //             ptFurthestChunk = ptCurrentRequest;

    //         }
    //         ptCurrentRequest = ptCurrentRequest->ptNext;
    //     }

    //     if(ptFurthestChunk->uIndexCount > uIndexCount)
    //     {
    //         pl__make_unresident(ptPlanet, ptFurthestChunk);
    //         bChunkFreed = true;
    //     }
    // }

    if(!bChunkFreed)
        printf("Couldn't free chunks\n");
}

void
pl__remove_from_residency_queue(plPlanet* ptPlanet, plPlanetChunk* ptChunk)
{
    plPlanetResidencyNode* ptCurrentRequest = ptPlanet->tRequestQueue.ptNext;

    plPlanetResidencyNode* ptExistingRequest = NULL;
    plPlanetResidencyNode* ptLastRequest = NULL;

    while(ptCurrentRequest)
    {
        if(ptCurrentRequest->ptChunk == ptChunk)
        {
            ptExistingRequest = ptCurrentRequest;
            break;
        }
        ptCurrentRequest = ptCurrentRequest->ptNext;
    }

    if(ptExistingRequest)
    {

        // remove node
        if(ptExistingRequest->ptPrev)
            ptExistingRequest->ptPrev->ptNext = ptExistingRequest->ptNext;
        else
            ptPlanet->tRequestQueue.ptNext = ptExistingRequest->ptNext;

        if(ptExistingRequest->ptNext)
            ptExistingRequest->ptNext->ptPrev = ptExistingRequest->ptPrev;

        uint32_t uIndex = (uint32_t)(ptExistingRequest - &ptPlanet->atRequests[0]);
        pl_sb_push(ptPlanet->sbuFreeRequests, uIndex);
    }
}

static void
pl__make_unresident(plPlanet* ptPlanet, plPlanetChunk* ptChunk)
{
    pl__remove_from_residency_queue(ptPlanet, ptChunk);
    pl__remove_from_replacement_queue(ptPlanet, ptChunk);
    if(ptChunk->ptIndexHole && ptChunk->uIndexCount > 0)
    {

        if(ptPlanet->sbtChunkFiles[ptChunk->uFileID].ptPakFile)
        {
            pl_sb_sprintf(gptCtx->sbcScratchBuffer, "%s/%u_tile_%u.png", ptPlanet->sbtChunkFiles[ptChunk->uFileID].acPakFileName, ptChunk->uFileID, ptChunk->uIndex);
            plResourceHandle tTextureResource = gptResource->load_ex(
                gptCtx->sbcScratchBuffer,
                PL_RESOURCE_LOAD_FLAG_NO_CACHING, NULL, 0,
                ptPlanet->sbtChunkFiles[ptChunk->uFileID].acPakFileName, 0); 
            plTextureHandle tTexture = gptResource->get_texture(tTextureResource);
            pl__planet_return_bindless_texture_index(tTexture);
            ptChunk->uTextureIndex = 0;
            pl_sb_reset(gptCtx->sbcScratchBuffer);
            gptResource->evict(tTextureResource);
        }

        if(ptChunk->ptIndexHole)
            gptFreeList->return_node(&gptCtx->tIndexBufferManager, ptChunk->ptIndexHole);
        if(ptChunk->ptVertexHole)
            gptFreeList->return_node(&gptCtx->tVertexBufferManager, ptChunk->ptVertexHole);
        ptChunk->uIndexCount = 0;
        ptChunk->uLastFrameUsed = 0;
        ptChunk->ptIndexHole = NULL;
        ptChunk->ptVertexHole = NULL;
        ptChunk->ptVertexHole = NULL;
        if(ptChunk->aptChildren[0] != NULL)
        {
            pl__make_unresident(ptPlanet, ptChunk->aptChildren[0]);
            pl__make_unresident(ptPlanet, ptChunk->aptChildren[1]);
            pl__make_unresident(ptPlanet, ptChunk->aptChildren[2]);
            pl__make_unresident(ptPlanet, ptChunk->aptChildren[3]);
        }
    }
}

static void
pl__handle_residency(plPlanet* ptPlanet, plCommandBuffer* ptCommandBuffer)
{
    const uint64_t uCurrentFrame = gptIOI->get_io()->ulFrameCount;

    plPlanetResidencyNode* ptCurrentRequest = ptPlanet->tRequestQueue.ptNext;
    // ptCurrentRequest->uFrameRequested

    if(ptCurrentRequest)
    {

        plPlanetChunk* ptChunk = ptCurrentRequest->ptChunk;

        FILE* ptDataFile = fopen(ptPlanet->sbtChunkFiles[ptChunk->uFileID].tFile.acFile, "rb");
        fseek(ptDataFile, (long)ptChunk->szFileLocation, SEEK_SET);

        fseek(ptDataFile, sizeof(plVec3) * 2 + sizeof(int) * 4, SEEK_CUR);

        uint32_t uVertexCount = 0;
        fread(&uVertexCount, 1, sizeof(uint32_t), ptDataFile);

        plVec3* ptVertices = PL_ALLOC(uVertexCount * sizeof(plPlanetVertex));
        fread(ptVertices, 1, sizeof(plPlanetVertex) * uVertexCount, ptDataFile);

        uint32_t uIndexCount = 0;
        fread(&uIndexCount, 1, sizeof(uint32_t), ptDataFile);
        
        uint32_t* ptIndices = PL_ALLOC(uIndexCount * sizeof(uint32_t));
        fread(ptIndices, 1, sizeof(uint32_t) * uIndexCount, ptDataFile);

        const uint32_t uVertexBufferSizeBytes = uVertexCount * sizeof(plPlanetVertex);
        const uint32_t uIndexBufferSizeBytes = uIndexCount * sizeof(uint32_t);

        const uint32_t uIndexStageOffset = 0;
        const uint32_t uVertexStageOffset = uIndexBufferSizeBytes;

        // update chunk offsets
        plFreeListNode* ptIndexHole = gptFreeList->get_node(&gptCtx->tIndexBufferManager, uIndexCount * sizeof(uint32_t));
        plFreeListNode* ptVertexHole = gptFreeList->get_node(&gptCtx->tVertexBufferManager, uVertexCount * sizeof(plPlanetVertex));

        if(ptIndexHole == NULL || ptVertexHole == NULL)
        {
            if(ptIndexHole) gptFreeList->return_node(&gptCtx->tIndexBufferManager, ptIndexHole);
            if(ptVertexHole) gptFreeList->return_node(&gptCtx->tVertexBufferManager, ptVertexHole);
            pl__free_chunk(ptPlanet, ptCurrentRequest->fDistance, uIndexCount);
            ptIndexHole = gptFreeList->get_node(&gptCtx->tIndexBufferManager, uIndexCount * sizeof(uint32_t));
            ptVertexHole = gptFreeList->get_node(&gptCtx->tVertexBufferManager, uVertexCount * sizeof(plPlanetVertex));
        }

        if(ptIndexHole == NULL || ptVertexHole == NULL)
        {
            if(ptIndexHole) gptFreeList->return_node(&gptCtx->tIndexBufferManager, ptIndexHole);
            if(ptVertexHole) gptFreeList->return_node(&gptCtx->tVertexBufferManager, ptVertexHole);
            pl__free_chunk(ptPlanet, ptCurrentRequest->fDistance, uIndexCount);
            PL_FREE(ptVertices);
            PL_FREE(ptIndices);
            fclose(ptDataFile);
            printf("No Memory\n");
            return;
        }

        ptChunk->uIndexCount = uIndexCount;
        ptChunk->ptIndexHole = ptIndexHole;
        ptChunk->ptVertexHole = ptVertexHole;

        if(ptPlanet->sbtChunkFiles[ptChunk->uFileID].ptPakFile)
        {
            pl_sb_sprintf(gptCtx->sbcScratchBuffer, "%s/%u_tile_%u.png", ptPlanet->sbtChunkFiles[ptChunk->uFileID].acPakFileName, ptChunk->uFileID, ptChunk->uIndex);
            plResourceHandle tTextureResource = gptResource->load_ex(
                gptCtx->sbcScratchBuffer,
                PL_RESOURCE_LOAD_FLAG_NO_CACHING, NULL, 0,
                ptPlanet->sbtChunkFiles[ptChunk->uFileID].acPakFileName, 0); 
            gptResource->make_resident(tTextureResource);
            plTextureHandle tTexture = gptResource->get_texture(tTextureResource);
            ptChunk->uTextureIndex = pl__planet_get_bindless_texture_index(tTexture);
            
            pl_sb_reset(gptCtx->sbcScratchBuffer);
        }

        // update buffer offsets

        plDevice* ptDevice = gptCtx->ptDevice;
        plBuffer* ptStagingBuffer = gptGfx->get_buffer(ptDevice, gptCtx->tStagingBuffer);

        void* ptIndexStageDest = &ptStagingBuffer->tMemoryAllocation.pHostMapped[uIndexStageOffset];
        void* ptVertexStageDest = &ptStagingBuffer->tMemoryAllocation.pHostMapped[uVertexStageOffset];

        // copy memory to mapped staging buffer
        memcpy(ptIndexStageDest, ptIndices, uIndexBufferSizeBytes);
        memcpy(ptVertexStageDest, ptVertices, uVertexBufferSizeBytes);

        // destination offsets
        const uint64_t uIndexFinalOffset = ptChunk->ptIndexHole->uOffset;
        const uint64_t uVertexFinalOffset = ptChunk->ptVertexHole->uOffset;
        
        // begin blit pass, copy buffer, end pass
        // NOTE: we are using the starter extension to get a blit encoder, later examples we will
        //       handle this ourselves
        plBlitEncoder* ptEncoder = gptGfx->begin_blit_pass(ptCommandBuffer);
        gptGfx->pipeline_barrier_blit(ptEncoder, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE);

        gptGfx->copy_buffer(ptEncoder, gptCtx->tStagingBuffer, gptCtx->tIndexBuffer, uIndexStageOffset, uIndexFinalOffset, uIndexBufferSizeBytes);
        gptGfx->copy_buffer(ptEncoder, gptCtx->tStagingBuffer, gptCtx->tVertexBuffer, uVertexStageOffset, uVertexFinalOffset, uVertexBufferSizeBytes);
        
        gptGfx->pipeline_barrier_blit(ptEncoder, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ);
        gptGfx->end_blit_pass(ptEncoder);
        // gptGfx->queue_buffer_for_deletion(ptDevice, tStagingBuffer);

        PL_FREE(ptVertices);
        PL_FREE(ptIndices);

        fclose(ptDataFile);

        pl__remove_from_residency_queue(ptPlanet, ptCurrentRequest->ptChunk);
    }
}

static void
pl__touch_chunk(plPlanet* ptPlanet, plPlanetChunk* ptChunk)
{
    if(ptChunk == NULL)
        return;

  
    ptChunk->uLastFrameUsed = gptIOI->get_io()->ulFrameCount;

    plPlanetChunk* ptCurrentRequest = ptPlanet->tReplacementQueue.ptNext;

    plPlanetChunk* ptExistingRequest = NULL;

    while(ptCurrentRequest)
    {
        if(ptCurrentRequest == ptChunk)
        {
            ptExistingRequest = ptCurrentRequest;
            break;
        }
        ptCurrentRequest = ptCurrentRequest->ptNext;
    }

    if(ptExistingRequest)
    {

        // remove node
        if(ptExistingRequest->ptPrev)
            ptExistingRequest->ptPrev->ptNext = ptExistingRequest->ptNext;

        if(ptExistingRequest->ptNext)
            ptExistingRequest->ptNext->ptPrev = ptExistingRequest->ptPrev;   
    }
    else
    {
        ptExistingRequest = ptChunk;
    }

    // place node at beginning
    ptExistingRequest->ptPrev = NULL;
    if(ptExistingRequest != ptPlanet->tReplacementQueue.ptNext)
        ptExistingRequest->ptNext = ptPlanet->tReplacementQueue.ptNext;
    ptPlanet->tReplacementQueue.ptNext = ptExistingRequest;
    if(ptExistingRequest->ptNext)
        ptExistingRequest->ptNext->ptPrev = ptExistingRequest;

    ptExistingRequest->uLastFrameUsed = gptIOI->get_io()->ulFrameCount;
    // ptExistingRequest->fDistance = fDistance;
    // ptExistingRequest->ptChunk = ptChunk;
}

static void
pl__request_residency(plPlanet* ptPlanet, plPlanetChunk* ptChunk, float fDistance)
{
    if(ptChunk == NULL)
        return;

    if(ptChunk->ptIndexHole == NULL)
    {
        plPlanetResidencyNode* ptCurrentRequest = ptPlanet->tRequestQueue.ptNext;

        plPlanetResidencyNode* ptExistingRequest = NULL;
        plPlanetResidencyNode* ptLastRequest = NULL;

        while(ptCurrentRequest)
        {
            if(ptCurrentRequest->ptChunk == ptChunk)
            {
                ptExistingRequest = ptCurrentRequest;
                break;
            }
            if(ptCurrentRequest->ptNext == NULL)
                ptLastRequest = ptCurrentRequest;
            ptCurrentRequest = ptCurrentRequest->ptNext;
        }

        if(ptExistingRequest)
        {

            // remove node
            if(ptExistingRequest->ptPrev)
                ptExistingRequest->ptPrev->ptNext = ptExistingRequest->ptNext;

            if(ptExistingRequest->ptNext)
                ptExistingRequest->ptNext->ptPrev = ptExistingRequest->ptPrev;    
        }
        else
        {
            if(pl_sb_size(ptPlanet->sbuFreeRequests) > 0)
            {
                uint32_t uNewIndex = pl_sb_pop(ptPlanet->sbuFreeRequests);
                ptExistingRequest = &ptPlanet->atRequests[uNewIndex];

            }
            else if(ptLastRequest)
            {
                if(ptLastRequest->ptPrev)
                    ptLastRequest->ptPrev->ptNext = NULL;
                ptExistingRequest = ptLastRequest; 
            }
            else
            {
                PL_ASSERT(false);
            }
        }

        // place node at beginning
        ptExistingRequest->ptPrev = NULL;
        if(ptExistingRequest != ptPlanet->tRequestQueue.ptNext)
            ptExistingRequest->ptNext = ptPlanet->tRequestQueue.ptNext;
        ptPlanet->tRequestQueue.ptNext = ptExistingRequest;
        if(ptExistingRequest->ptNext)
            ptExistingRequest->ptNext->ptPrev = ptExistingRequest;

        ptExistingRequest->uFrameRequested = gptIOI->get_io()->ulFrameCount;
        ptExistingRequest->fDistance = fDistance;
        ptExistingRequest->ptChunk = ptChunk;
    }
}

static void
pl__render_chunk(plPlanet* ptPlanet, plCamera* ptCamera , plRenderEncoder* ptEncoder, plPlanetChunk* ptChunk, plPlanetChunkFile* ptFile)
{
    PL_ASSERT(ptChunk != NULL);

    plAABB tAABB = {
        .tMin = ptChunk->tMinBound,
        .tMax = ptChunk->tMaxBound
    };

    if(!pl__sat_visibility_test(ptCamera, &tAABB))
        return;

    plVec3 tClosestPoint = gptCollision->point_closest_point_aabb(ptCamera->tPos, tAABB);
    float fDistance = fabsf(pl_length_vec3(pl_sub_vec3(tClosestPoint, ptCamera->tPos)));

    pl__request_residency(ptPlanet, ptChunk, fDistance);

    if(ptChunk->ptIndexHole == NULL)
        return;
    
    float fViewportWidth = gptIOI->get_io()->tMainViewportSize.x;
    float fHorizontalFieldOfView = ptCamera->fFieldOfView * ptCamera->fAspectRatio;

    float fK = fViewportWidth / (2.0f * tanf(0.5f * fHorizontalFieldOfView));

    float fGeometricError = ptFile->fMaxBaseError * (float)ptChunk->uLevel;


    float fRho = fGeometricError * fK / fDistance;

    if(!pl__all_children_resident(ptChunk) || fRho <= ptPlanet->tRuntimeOptions.fTau)
    {

        const plMat4 tMVP = pl_mul_mat4(&ptCamera->tProjMat, &ptCamera->tViewMat);
        plDevice* ptDevice = gptCtx->ptDevice;
        plDynamicBinding tDynamicBinding = pl_allocate_dynamic_data(gptGfx, ptDevice, &gptCtx->tCurrentDynamicBufferBlock);
        plGpuDynPlanetData* ptDynamic = (plGpuDynPlanetData*)tDynamicBinding.pcData;
        ptDynamic->tMvp = tMVP;
        ptDynamic->iLevel = (int)ptChunk->uLevel;
        ptDynamic->tFlags = ptPlanet->tRuntimeOptions.tFlags;
        ptDynamic->uTextureIndex = ptChunk->uTextureIndex;
        ptDynamic->tLightDirection = ptPlanet->tRuntimeOptions.tLightDirection;

        // submit nonindexed draw using basic API
        plShaderHandle tShader = (ptPlanet->tRuntimeOptions.tFlags & PL_PLANET_FLAGS_WIREFRAME) ? ptPlanet->tWireframeShader : ptPlanet->tShader;
        gptGfx->bind_shader(ptEncoder, tShader);
        gptGfx->bind_vertex_buffer(ptEncoder, gptCtx->tVertexBuffer);
        gptGfx->bind_graphics_bind_groups(ptEncoder, tShader, 0, 1, &gptCtx->atBindGroups[gptGfx->get_current_frame_index()], 1, &tDynamicBinding);

        const plDrawIndex tDraw = {
            .uInstanceCount = 1,
            .uIndexCount    = ptChunk->uIndexCount,
            .uVertexStart   = (uint32_t)ptChunk->ptVertexHole->uOffset / sizeof(plPlanetVertex),
            .uIndexStart    = (uint32_t)ptChunk->ptIndexHole->uOffset / sizeof(uint32_t),
            .tIndexBuffer   = gptCtx->tIndexBuffer
        };
        gptGfx->draw_indexed(ptEncoder, 1, &tDraw);


        pl__touch_chunk(ptPlanet, ptChunk);
        if(fRho > ptPlanet->tRuntimeOptions.fTau) // we actually want children
        {
            for(uint32_t i = 0; i < 4; i++)
            {
                if(ptChunk->aptChildren[i] == NULL)
                    break;

                plAABB tChildAABB = {
                    .tMin = ptChunk->aptChildren[i]->tMinBound,
                    .tMax = ptChunk->aptChildren[i]->tMaxBound
                };
                plVec3 tClosestChildPoint = gptCollision->point_closest_point_aabb(ptCamera->tPos, tChildAABB);
                float fChildDistance = fabsf(pl_length_vec3(pl_sub_vec3(tClosestChildPoint, ptCamera->tPos)));
                pl__request_residency(ptPlanet, ptChunk->aptChildren[i], fChildDistance);
            }
        }
        else // we actually want this chunk, not children
        {
            pl__unload_children(ptPlanet, ptChunk);
        }
    }
    else
    {
        for(uint32_t i = 0; i < 4; i++)
            pl__render_chunk(ptPlanet, ptCamera, ptEncoder, ptChunk->aptChildren[i], ptFile);
    }
}

static bool
pl__sat_visibility_test(plCamera* ptCamera, const plAABB* ptAABB)
{
    const float fTanFov = tanf(0.5f * ptCamera->fFieldOfView);

    const float fZNear = ptCamera->fNearZ;
    const float fZFar = ptCamera->fFarZ;

    // half width, half height
    const float fXNear = ptCamera->fAspectRatio * ptCamera->fNearZ * fTanFov;
    const float fYNear = ptCamera->fNearZ * fTanFov;

    // consider four adjacent corners of the AABB
    plVec3 atCorners[] = {
        {ptAABB->tMin.x, ptAABB->tMin.y, ptAABB->tMin.z},
        {ptAABB->tMax.x, ptAABB->tMin.y, ptAABB->tMin.z},
        {ptAABB->tMin.x, ptAABB->tMax.y, ptAABB->tMin.z},
        {ptAABB->tMin.x, ptAABB->tMin.y, ptAABB->tMax.z},
    };

    // transform corners
    for (size_t i = 0; i < 4; i++)
        atCorners[i] = pl_mul_mat4_vec3(&ptCamera->tViewMat, atCorners[i]);

    // Use transformed atCorners to calculate center, axes and extents

    plOBB2 tObb = {
        .atAxes = {
            pl_sub_vec3(atCorners[1], atCorners[0]),
            pl_sub_vec3(atCorners[2], atCorners[0]),
            pl_sub_vec3(atCorners[3], atCorners[0])
        },
    };

    tObb.tCenter = pl_add_vec3(atCorners[0], pl_mul_vec3_scalarf((pl_add_vec3(tObb.atAxes[0], pl_add_vec3(tObb.atAxes[1], tObb.atAxes[2]))), 0.5f));
    tObb.tExtents = (plVec3){ pl_length_vec3(tObb.atAxes[0]), pl_length_vec3(tObb.atAxes[1]), pl_length_vec3(tObb.atAxes[2]) };

    // normalize
    tObb.atAxes[0] = pl_div_vec3_scalarf(tObb.atAxes[0], tObb.tExtents.x);
    tObb.atAxes[1] = pl_div_vec3_scalarf(tObb.atAxes[1], tObb.tExtents.y);
    tObb.atAxes[2] = pl_div_vec3_scalarf(tObb.atAxes[2], tObb.tExtents.z);
    tObb.tExtents = pl_mul_vec3_scalarf(tObb.tExtents, 0.5f);

    // axis along frustum
    {
        // Projected center of our OBB
        const float fMoC = tObb.tCenter.z;

        // Projected size of OBB
        float fRadius = 0.0f;
        for (size_t i = 0; i < 3; i++)
            fRadius += fabsf(tObb.atAxes[i].z) * tObb.tExtents.d[i];

        const float fObbMin = fMoC - fRadius;
        const float fObbMax = fMoC + fRadius;

        if (fObbMin > fZFar || fObbMax < fZNear)
            return false;
    }


    // other normals of frustum
    {
        const plVec3 atM[] = {
            { fZNear, 0.0f, fXNear }, // Left Plane
            { -fZNear, 0.0f, fXNear }, // Right plane
            { 0.0, -fZNear, fYNear }, // Top plane
            { 0.0, fZNear, fYNear }, // Bottom plane
        };
        for (size_t m = 0; m < 4; m++)
        {
            const float fMoX = fabsf(atM[m].x);
            const float fMoY = fabsf(atM[m].y);
            const float fMoZ = atM[m].z;
            const float fMoC = pl_dot_vec3(atM[m], tObb.tCenter);

            float fObbRadius = 0.0f;
            for (size_t i = 0; i < 3; i++)
                fObbRadius += fabsf(pl_dot_vec3(atM[m], tObb.atAxes[i])) * tObb.tExtents.d[i];

            const float fObbMin = fMoC - fObbRadius;
            const float fObbMax = fMoC + fObbRadius;

            const float fP = fXNear * fMoX + fYNear * fMoY;

            float fTau0 = fZNear * fMoZ - fP;
            float fTau1 = fZNear * fMoZ + fP;

            if (fTau0 < 0.0f)
                fTau0 *= fZFar / fZNear;

            if (fTau1 > 0.0f)
                fTau1 *= fZFar / fZNear;

            if (fObbMin > fTau1 || fObbMax < fTau0)
                return false;
        }
    }

    // OBB axes
    {
        for (size_t m = 0; m < 3; m++)
        {
            const plVec3* ptM = &tObb.atAxes[m];
            const float fMoX = fabsf(ptM->x);
            const float fMoY = fabsf(ptM->y);
            const float fMoZ = ptM->z;
            const float fMoC = pl_dot_vec3(*ptM, tObb.tCenter);

            const float fObbRadius = tObb.tExtents.d[m];

            const float fObbMin = fMoC - fObbRadius;
            const float fObbMax = fMoC + fObbRadius;

            // frustum projection
            const float fP = fXNear * fMoX + fYNear * fMoY;
            float fTau0 = fZNear * fMoZ - fP;
            float fTau1 = fZNear * fMoZ + fP;

            if (fTau0 < 0.0f)
                fTau0 *= fZFar / fZNear;

            if (fTau1 > 0.0f)
                fTau1 *= fZFar / fZNear;

            if (fObbMin > fTau1 || fObbMax < fTau0)
                return false;
        }
    }

    // cross products between the edges
    // first R x A_i
    {
        for (size_t m = 0; m < 3; m++)
        {
            const plVec3 tM = { 0.0f, -tObb.atAxes[m].z, tObb.atAxes[m].y };
            const float fMoX = 0.0f;
            const float fMoY = fabsf(tM.y);
            const float fMoZ = tM.z;
            const float fMoC = tM.y * tObb.tCenter.y + tM.z * tObb.tCenter.z;

            float fObbRadius = 0.0f;
            for (size_t i = 0; i < 3; i++)
                fObbRadius += fabsf(pl_dot_vec3(tM, tObb.atAxes[i])) * tObb.tExtents.d[i];

            const float fObbMin = fMoC - fObbRadius;
            const float fObbMax = fMoC + fObbRadius;

            // frustum projection
            const float fP = fXNear * fMoX + fYNear * fMoY;
            float fTau0 = fZNear * fMoZ - fP;
            float fTau1 = fZNear * fMoZ + fP;

            if (fTau0 < 0.0f)
                fTau0 *= fZFar / fZNear;

            if (fTau1 > 0.0f)
                fTau1 *= fZFar / fZNear;

            if (fObbMin > fTau1 || fObbMax < fTau0)
                return false;
        }
    }

    // U x A_i
    {
        for (size_t m = 0; m < 3; m++)
        {
            const plVec3 tM = { tObb.atAxes[m].z, 0.0f, -tObb.atAxes[m].x };
            const float fMoX = fabsf(tM.x);
            const float fMoY = 0.0f;
            const float fMoZ = tM.z;
            const float fMoC = tM.x * tObb.tCenter.x + tM.z * tObb.tCenter.z;

            float fObbRadius = 0.0f;
            for (size_t i = 0; i < 3; i++)
                fObbRadius += fabsf(pl_dot_vec3(tM, tObb.atAxes[i])) * tObb.tExtents.d[i];

            const float fObbMin = fMoC - fObbRadius;
            const float fObbMax = fMoC + fObbRadius;

            // frustum projection
            const float fP = fXNear * fMoX + fYNear * fMoY;
            float fTau0 = fZNear * fMoZ - fP;
            float fTau1 = fZNear * fMoZ + fP;

            if (fTau0 < 0.0f)
                fTau0 *= fZFar / fZNear;

            if (fTau1 > 0.0f)
                fTau1 *= fZFar / fZNear;

            if (fObbMin > fTau1 || fObbMax < fTau0)
                return false;
        }
    }

    // frustum Edges X Ai
    {
        for (size_t obb_edge_idx = 0; obb_edge_idx < 3; obb_edge_idx++)
        {
            const plVec3 atM[] = {
                pl_cross_vec3((plVec3){-fXNear, 0.0f, fZNear}, tObb.atAxes[obb_edge_idx]), // Left Plane
                pl_cross_vec3((plVec3){ fXNear, 0.0f, fZNear }, tObb.atAxes[obb_edge_idx]), // Right plane
                pl_cross_vec3((plVec3){ 0.0f, fYNear, fZNear }, tObb.atAxes[obb_edge_idx]), // Top plane
                pl_cross_vec3((plVec3){ 0.0, -fYNear, fZNear }, tObb.atAxes[obb_edge_idx]) // Bottom plane
            };

            for (size_t m = 0; m < 4; m++)
            {
                const float fMoX = fabsf(atM[m].x);
                const float fMoY = fabsf(atM[m].y);
                const float fMoZ = atM[m].z;

                const float fEpsilon = 1e-4f;
                if (fMoX < fEpsilon && fMoY < fEpsilon && fabsf(fMoZ) < fEpsilon) continue;

                const float fMoC = pl_dot_vec3(atM[m], tObb.tCenter);

                float fObbRadius = 0.0f;
                for (size_t i = 0; i < 3; i++)
                    fObbRadius += fabsf(pl_dot_vec3(atM[m], tObb.atAxes[i])) * tObb.tExtents.d[i];

                const float fObbMin = fMoC - fObbRadius;
                const float fObbMax = fMoC + fObbRadius;

                // frustum projection
                const float fP = fXNear * fMoX + fYNear * fMoY;
                float fTau0 = fZNear * fMoZ - fP;
                float fTau1 = fZNear * fMoZ + fP;

                if (fTau0 < 0.0f)
                    fTau0 *= fZFar / fZNear;

                if (fTau1 > 0.0f)
                    fTau1 *= fZFar / fZNear;

                if (fObbMin > fTau1 || fObbMax < fTau0)
                    return false;
            }
        }
    }

    // no intersections detected
    return true;
}

static bool
pl__planet_load(plPlanet* ptPlanet, plPlanetProcessInfo* ptInfo, plPlanetTexture* ptTexture, plPlanetLoadFlags tFlags)
{

    plVec2 tTopLeftGlobal = {0};
    plVec2 tBottomRightGlobal = {0};
    bool* abActiveTextureTiles = PL_ALLOC(sizeof(bool) * ptInfo->uTileCount);
    memset(abActiveTextureTiles, 0, sizeof(bool) * ptInfo->uTileCount);

    {
        float fLatitude = pl_radiansf(ptInfo->atTiles[0].fLatitude);
        float fLongitude = pl_radiansf(ptInfo->atTiles[0].fLongitude);
        float fR = 2.0f * ptInfo->fRadius * tanf(PL_PI_4 - 0.5f * fLatitude);
        float fX = fR * sinf(fLongitude);
        float fY = fR * cosf(fLongitude);
        tTopLeftGlobal.x = fX - 0.5f * (float)ptInfo->uSize * ptInfo->fMetersPerPixel;
        tTopLeftGlobal.y = fY - 0.5f * (float)ptInfo->uSize * ptInfo->fMetersPerPixel;
    }

    {
        float fLatitude = pl_radiansf(ptInfo->atTiles[ptInfo->uTileCount - 1].fLatitude);
        float fLongitude = pl_radiansf(ptInfo->atTiles[ptInfo->uTileCount - 1].fLongitude);
        float fR = 2.0f * ptInfo->fRadius * tanf(PL_PI_4 - 0.5f * fLatitude);
        float fX = fR * sinf(fLongitude);
        float fY = fR * cosf(fLongitude);
        tBottomRightGlobal.x = fX - 0.5f * (float)ptInfo->uSize * ptInfo->fMetersPerPixel;
        tBottomRightGlobal.y = fY - 0.5f * (float)ptInfo->uSize * ptInfo->fMetersPerPixel;
    }

    if(ptTexture)
    {
        float fLatitude = pl_radiansf(ptTexture->fLatitude);
        float fLongitude = pl_radiansf(ptTexture->fLongitude);
        float fR = 2.0f * ptInfo->fRadius * tanf(PL_PI_4 - 0.5f * fLatitude);
        float fX = fR * sinf(fLongitude);
        float fY = fR * cosf(fLongitude);

        plImageInfo tImageInfo = {0};
        gptImage->get_info_from_file(ptTexture->pcPath, &tImageInfo);

        plVec2 tTopLeft = {
            .x = fX - 0.5f * (float)tImageInfo.iWidth * ptTexture->fMetersPerPixel,
            .y = fY - 0.5f * (float)tImageInfo.iHeight * ptTexture->fMetersPerPixel,
        };

        plVec2 tBottomRight = {
            .x = fX + 0.5f * (float)tImageInfo.iWidth * ptTexture->fMetersPerPixel,
            .y = fY + 0.5f * (float)tImageInfo.iHeight * ptTexture->fMetersPerPixel,
        };

        uint32_t uTopLeftXIndex = (uint32_t)floorf((tTopLeft.x - tTopLeftGlobal.x) / ((float)ptInfo->uSize * ptInfo->fMetersPerPixel));
        uint32_t uTopLeftYIndex = (uint32_t)floorf((tTopLeft.y - tTopLeftGlobal.y) / ((float)ptInfo->uSize * ptInfo->fMetersPerPixel));

        uint32_t uBottomRightXIndex = (uint32_t)floorf((tBottomRight.x - tTopLeftGlobal.x) / ((float)ptInfo->uSize * ptInfo->fMetersPerPixel));
        uint32_t uBottomRightYIndex = (uint32_t)floorf((tBottomRight.y - tTopLeftGlobal.y) / ((float)ptInfo->uSize * ptInfo->fMetersPerPixel));

        plVec2 tTopLeftLocal = {0};
        plVec2 tBottomRightLocal = {0};

        {
            fLatitude = pl_radiansf(ptInfo->atTiles[uTopLeftXIndex + uTopLeftYIndex * ptInfo->uHorizontalTiles].fLatitude);
            fLongitude = pl_radiansf(ptInfo->atTiles[uTopLeftXIndex + uTopLeftYIndex * ptInfo->uHorizontalTiles].fLongitude);
            fR = 2.0f * ptInfo->fRadius * tanf(PL_PI_4 - 0.5f * fLatitude);
            fX = fR * sinf(fLongitude);
            fY = fR * cosf(fLongitude);
            tTopLeftLocal.x = fX - 0.5f * (float)ptInfo->uSize * ptInfo->fMetersPerPixel;
            tTopLeftLocal.y = fY - 0.5f * (float)ptInfo->uSize * ptInfo->fMetersPerPixel;
        }
        {
            fLatitude = pl_radiansf(ptInfo->atTiles[uBottomRightXIndex + uBottomRightYIndex * ptInfo->uHorizontalTiles].fLatitude);
            fLongitude = pl_radiansf(ptInfo->atTiles[uBottomRightXIndex + uBottomRightYIndex * ptInfo->uHorizontalTiles].fLongitude);
            fR = 2.0f * ptInfo->fRadius * tanf(PL_PI_4 - 0.5f * fLatitude);
            fX = fR * sinf(fLongitude);
            fY = fR * cosf(fLongitude);
            tBottomRightLocal.x = fX + 0.5f * (float)ptInfo->uSize * ptInfo->fMetersPerPixel;
            tBottomRightLocal.y = fY + 0.5f * (float)ptInfo->uSize * ptInfo->fMetersPerPixel;
        }

        int iImageWidth = 0;
        int iImageHeight = 0;
        int _unused;
        unsigned char* pucImageData = gptImage->load_from_file(ptTexture->pcPath, &iImageWidth, &iImageHeight, &_unused, 4);

        uint32_t uHorizontalExtent = uBottomRightXIndex - uTopLeftXIndex + 1;
        uint32_t uVerticalExtent = uBottomRightYIndex - uTopLeftYIndex + 1;

        plImageOpInfo tFullInfo = {
            .uWidth =  (uint32_t)((tBottomRightLocal.x - tTopLeftLocal.x) / ptTexture->fMetersPerPixel),
            .uHeight = (uint32_t)((tBottomRightLocal.y - tTopLeftLocal.y) / ptTexture->fMetersPerPixel),
            .uChannels = 4,
            .uStride = 4
        };
        plImageOpData tFullData = {0};
        gptImageOps->initialize(&tFullInfo, &tFullData);
        gptImageOps->square(&tFullData);


        plImageOpData tOriginalDataMod = {0};
        {
            plImageOpInfo tOriginalInfo = {
                .uWidth = (uint32_t)iImageWidth,
                .uHeight = (uint32_t)iImageHeight,
                .uChannels = 4,
                .uStride = 4
            };

            gptImageOps->initialize(&tOriginalInfo, &tOriginalDataMod); 
            tOriginalInfo.puData = pucImageData;
            gptImageOps->add(&tOriginalDataMod, tOriginalInfo, 0, 0);
            gptImage->free(pucImageData);
        }
        
        float fDistanceX = tTopLeft.x - tTopLeftLocal.x;
        float fDistanceY = tTopLeft.y - tTopLeftLocal.y;

        float fEffectiveMetersPerPixelX = (tBottomRightLocal.x - tTopLeftLocal.x) / tFullInfo.uWidth;
        float fEffectiveMetersPerPixelY = (tBottomRightLocal.y - tTopLeftLocal.y) / tFullInfo.uHeight;

        uint32_t uXOffsetIndex = (uint32_t)(fDistanceX / fEffectiveMetersPerPixelX);
        uint32_t uYOffsetIndex = (uint32_t)(fDistanceY / fEffectiveMetersPerPixelY);

        uint32_t uXInc = tFullInfo.uWidth / uHorizontalExtent;
        uint32_t uYInc = tFullInfo.uHeight / uVerticalExtent;


        plImageOpInfo tOriginalInfo2 = {
            .uWidth = tOriginalDataMod.uWidth,
            .uHeight = tOriginalDataMod.uHeight,
            .uChannels = 4,
            .uStride = 4,
            .puData = tOriginalDataMod.puData
        };
        gptImageOps->add(&tFullData, tOriginalInfo2, uXOffsetIndex, uYOffsetIndex);

        if(tFlags & PL_PLANET_LOAD_FLAGS_DEBUG)
        {
            plImageWriteInfo tWriteInfo2 = {
                .iWidth = (int)tFullData.uWidth,
                .iHeight = (int)tFullData.uHeight,
                .iComponents = 4,
                .iByteStride = (int)(tFullData.uWidth * 4)
            };
            gptImage->write( "hazard_prep_final.png", tFullData.puData, &tWriteInfo2);
        }


        for(uint32_t i = 0; i < uHorizontalExtent; i++)
        {
            for(uint32_t j = 0; j < uVerticalExtent; j++)
            {
                plImageOpData tImageData = {0};
                gptImageOps->extract(&tFullData, i * uXInc, j * uYInc, uXInc, uYInc, &tImageData);

                

                plImageWriteInfo tWriteInfo = {
                    .iWidth = (int)uXInc,
                    .iHeight = (int)uYInc,
                    .iComponents = 4,
                    .iByteStride = (int)(uXInc * 4)
                };

                char acNameBuffer[128] = {0};
                sprintf(acNameBuffer, "hazard_prep_%u_%u.png", i + uTopLeftXIndex, j + uTopLeftYIndex);

                abActiveTextureTiles[i + uTopLeftXIndex + (j + uTopLeftYIndex) * ptInfo->uHorizontalTiles] = true;

                gptImage->write(acNameBuffer, tImageData.puData, &tWriteInfo);
                gptImageOps->cleanup(&tImageData);
            }
        }
        gptImageOps->cleanup(&tOriginalDataMod);
        gptImageOps->cleanup(&tFullData);

    }

    for(uint32_t k = 0; k < ptInfo->uTileCount; k++)
    {
        uint32_t i = k % ptInfo->uHorizontalTiles;
        uint32_t j = (k - i) / ptInfo->uVerticalTiles;
        char acNameBuffer[128] = {0};
        sprintf(acNameBuffer, "hazard_prep_%u_%u.png", i, j);
        const char* pcHazard = NULL;
        if(abActiveTextureTiles[k])
            pcHazard = acNameBuffer;
        pl_chlod_load_chunk_file(ptPlanet, ptInfo->atTiles[k].acOutputFile, pcHazard, tFlags);
        if(!(tFlags & PL_PLANET_LOAD_FLAGS_DEBUG) && abActiveTextureTiles[k])
        {
            gptVfs->delete_file(gptVfs->register_file(acNameBuffer, true));
        }
    }
    PL_FREE(abActiveTextureTiles);
    return true;
}

static plTextureHandle
pl__planet_create_texture(plCommandBuffer* ptCmdBuffer, const plTextureDesc* ptDesc, const char* pcName, plTextureUsage tInitialUsage)
{
    // for convience
   plDevice* ptDevice = gptCtx->ptDevice;
 
    // create texture
    plTexture* ptTexture = NULL;
    const plTextureHandle tHandle = gptGfx->create_texture(ptDevice, ptDesc, &ptTexture);
    pl_temp_allocator_reset(&gptCtx->tTempAllocator);

    // choose allocator
    plDeviceMemoryAllocatorI* ptAllocator = gptCtx->tLocalBuddyAllocator;
    if(ptTexture->tMemoryRequirements.ulSize > gptGpuAllocators->get_buddy_block_size())
        ptAllocator = gptCtx->tLocalDedicatedAllocator;

    // allocate memory
    const plDeviceMemoryAllocation tAllocation = ptAllocator->allocate(ptAllocator->ptInst, 
        ptTexture->tMemoryRequirements.uMemoryTypeBits,
        ptTexture->tMemoryRequirements.ulSize,
        ptTexture->tMemoryRequirements.ulAlignment,
        pl_temp_allocator_sprintf(&gptCtx->tTempAllocator, "texture alloc %s", pcName));

    // bind memory
    gptGfx->bind_texture_to_memory(ptDevice, tHandle, &tAllocation);
    pl_temp_allocator_reset(&gptCtx->tTempAllocator);


    // set the initial texture usage (this is a no-op in metal but does layout transition for vulkan)
    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdBuffer);
    gptGfx->set_texture_usage(ptBlit, tHandle, tInitialUsage, 0);
    gptGfx->end_blit_pass(ptBlit);
    return tHandle;
}

static plTextureHandle
pl__planet_create_texture_with_data(const plTextureDesc* ptDesc, const char* pcName, uint32_t uIdentifier, const void* pData, size_t szSize)
{
    // for convience
    plDevice* ptDevice = gptCtx->ptDevice;
    plCommandPool* ptCmdPool = gptStarter->get_current_command_pool();
 
    // create texture
    plTexture* ptTexture = NULL;
    const plTextureHandle tHandle = gptGfx->create_texture(ptDevice, ptDesc, &ptTexture);
    pl_temp_allocator_reset(&gptCtx->tTempAllocator);

    // choose allocator
    plDeviceMemoryAllocatorI* ptAllocator = gptCtx->tLocalBuddyAllocator;
    if(ptTexture->tMemoryRequirements.ulSize > gptGpuAllocators->get_buddy_block_size())
        ptAllocator = gptCtx->tLocalDedicatedAllocator;

    // allocate memory
    const plDeviceMemoryAllocation tAllocation = ptAllocator->allocate(ptAllocator->ptInst, 
        ptTexture->tMemoryRequirements.uMemoryTypeBits,
        ptTexture->tMemoryRequirements.ulSize,
        ptTexture->tMemoryRequirements.ulAlignment,
        pl_temp_allocator_sprintf(&gptCtx->tTempAllocator, "texture alloc %s: %u", pcName, uIdentifier));

    // bind memory
    gptGfx->bind_texture_to_memory(ptDevice, tHandle, &tAllocation);
    pl_temp_allocator_reset(&gptCtx->tTempAllocator);

    plCommandBuffer* ptCommandBuffer = gptGfx->request_command_buffer(ptCmdPool, "create texture 2");
    gptGfx->begin_command_recording(ptCommandBuffer, NULL);
    plBlitEncoder* ptBlitEncoder = gptGfx->begin_blit_pass(ptCommandBuffer);
    gptGfx->pipeline_barrier_blit(ptBlitEncoder, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE);
    gptGfx->set_texture_usage(ptBlitEncoder, tHandle, PL_TEXTURE_USAGE_SAMPLED, 0);


    // if data is presented, upload using staging buffer
    if(pData)
    {
        PL_ASSERT(ptDesc->uLayers == 1); // this is for simple textures right now

        // create staging buffer
        const plBufferDesc tStagingBufferDesc = {
            .tUsage      = PL_BUFFER_USAGE_TRANSFER_SOURCE,
            .szByteSize  = szSize,
            .pcDebugName = "temp staging buffer"
        };
        plBuffer* ptBuffer = NULL;
        plBufferHandle tStagingBuffer = gptGfx->create_buffer(ptDevice, &tStagingBufferDesc, &ptBuffer);

        // allocate memory for the vertex buffer
        const plDeviceMemoryAllocation tStagingBufferAllocation = gptGfx->allocate_memory(ptDevice,
            ptBuffer->tMemoryRequirements.ulSize,
            PL_MEMORY_FLAGS_HOST_VISIBLE | PL_MEMORY_FLAGS_HOST_COHERENT,
            ptBuffer->tMemoryRequirements.uMemoryTypeBits,
            "temp staging memory");

        gptGfx->bind_buffer_to_memory(ptDevice, tStagingBuffer, &tStagingBufferAllocation);
        memcpy(ptBuffer->tMemoryAllocation.pHostMapped, pData, szSize);
        
        const plBufferImageCopy tBufferImageCopy = {
            .uImageWidth = (uint32_t)ptDesc->tDimensions.x,
            .uImageHeight = (uint32_t)ptDesc->tDimensions.y,
            .uImageDepth = 1,
            .uLayerCount = 1
        };

        gptGfx->copy_buffer_to_texture(ptBlitEncoder, tStagingBuffer, tHandle, 1, &tBufferImageCopy);
        gptGfx->generate_mipmaps(ptBlitEncoder, tHandle);
        gptGfx->queue_buffer_for_deletion(ptDevice, tStagingBuffer);
    }

    gptGfx->pipeline_barrier_blit(ptBlitEncoder, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ);
    gptGfx->end_blit_pass(ptBlitEncoder);
    gptGfx->end_command_recording(ptCommandBuffer);
    gptGfx->submit_command_buffer(ptCommandBuffer, NULL);
    gptGfx->wait_on_command_buffer(ptCommandBuffer);
    gptGfx->return_command_buffer(ptCommandBuffer);
    return tHandle;
}

static void
pl__planet_return_bindless_texture_index(plTextureHandle tTexture)
{
    uint64_t uIndex = 0;
    if(pl_hm_has_key_ex(&gptCtx->tTextureIndexHashmap, tTexture.uData, &uIndex))
    {
        pl_hm_remove(&gptCtx->tTextureIndexHashmap, tTexture.uData);
    }
}

static uint32_t
pl__planet_get_bindless_texture_index(plTextureHandle tTexture)
{

    uint64_t uIndex = 0;
    if(pl_hm_has_key_ex(&gptCtx->tTextureIndexHashmap, tTexture.uData, &uIndex))
        return (uint32_t)uIndex;

    uint64_t ulValue = pl_hm_get_free_index(&gptCtx->tTextureIndexHashmap);
    if(ulValue == PL_DS_HASH_INVALID)
    {
        PL_ASSERT(gptCtx->uTextureIndexCount < PL_PLANET_MAX_BINDLESS_TEXTURES);
        ulValue = gptCtx->uTextureIndexCount++;

        // TODO: handle when greater than 4096
    }
    pl_hm_insert(&gptCtx->tTextureIndexHashmap, tTexture.uData, ulValue);
    
    const plBindGroupUpdateTextureData tGlobalTextureData[] = {
        {
            .tTexture = tTexture,
            .uSlot    = 1,
            .uIndex   = (uint32_t)ulValue,
            .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED
        },
    };

    plBindGroupUpdateData tGlobalBindGroupData = {
        .uTextureCount = 1,
        .atTextureBindings = tGlobalTextureData
    };

    for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
        gptGfx->update_bind_group(gptCtx->ptDevice, gptCtx->atBindGroups[i], &tGlobalBindGroupData);

    return (uint32_t)ulValue;
}

//-----------------------------------------------------------------------------
// [SECTION] extension loading
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_load_ext(plApiRegistryI* ptApiRegistry, bool bReload)
{
    const plPlanetI tApi = {
        .initialize          = pl_planet_initialize,
        .cleanup             = pl_planet_cleanup,
        .create_planet       = pl_create_planet,
        .cleanup_planet      = pl_cleanup_planet,
        .render              = pl_render_planet,
        .prepare             = pl_prepare_planet,
        .reload_shaders      = pl_planet_load_shaders,
        .set_shaders         = pl_planet_set_shaders,
        .set_runtime_options = pl_planet_set_runtime_options,
        .get_runtime_options = pl_planet_get_runtime_options,
        .get_texture         = pl_get_planet_texture,
        .draw_sphere         = pl_draw_sphere,
    };
    pl_set_api(ptApiRegistry, plPlanetI, &tApi);

    gptMemory           = pl_get_api_latest(ptApiRegistry, plMemoryI);
    gptImage            = pl_get_api_latest(ptApiRegistry, plImageI);
    gptFile             = pl_get_api_latest(ptApiRegistry, plFileI);
    gptProfile          = pl_get_api_latest(ptApiRegistry, plProfileI);
    gptGfx              = pl_get_api_latest(ptApiRegistry, plGraphicsI);
    gptFreeList         = pl_get_api_latest(ptApiRegistry, plFreeListI);
    gptIOI              = pl_get_api_latest(ptApiRegistry, plIOI);
    gptStarter          = pl_get_api_latest(ptApiRegistry, plStarterI);
    gptShader           = pl_get_api_latest(ptApiRegistry, plShaderI);
    gptCollision        = pl_get_api_latest(ptApiRegistry, plCollisionI);
    gptScreenLog        = pl_get_api_latest(ptApiRegistry, plScreenLogI);
    gptDraw             = pl_get_api_latest(ptApiRegistry, plDrawI);
    gptTerrainProcessor = pl_get_api_latest(ptApiRegistry, plPlanetProcessorI);
    gptGpuAllocators    = pl_get_api_latest(ptApiRegistry, plGPUAllocatorsI);
    gptImageOps         = pl_get_api_latest(ptApiRegistry, plImageOpsI);
    gptVfs              = pl_get_api_latest(ptApiRegistry, plVfsI);
    gptResource         = pl_get_api_latest(ptApiRegistry, plResourceI);
    gptPak              = pl_get_api_latest(ptApiRegistry, plPakI);

    const plDataRegistryI* ptDataRegistry = pl_get_api_latest(ptApiRegistry, plDataRegistryI);

    if(bReload)
    {
        gptCtx = ptDataRegistry->get_data("plPlanetContext");
    }
    else
    {
        static plPlanetContext tCtx = {0};
        gptCtx = &tCtx;
        ptDataRegistry->set_data("plPlanetContext", gptCtx);
    }
}

PL_EXPORT void
pl_unload_ext(plApiRegistryI* ptApiRegistry, bool bReload)
{

    if(bReload)
        return;

    const plPlanetI* ptApi = pl_get_api_latest(ptApiRegistry, plPlanetI);
    ptApiRegistry->remove_api(ptApi);
}

//-----------------------------------------------------------------------------
// [SECTION] unity build
//-----------------------------------------------------------------------------

#define PL_MEMORY_IMPLEMENTATION
#include "pl_memory.h"

#define PL_STRING_IMPLEMENTATION
#include "pl_string.h"