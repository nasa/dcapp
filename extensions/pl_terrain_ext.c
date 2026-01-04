/*
   pl_terrain_ext.c
*/

/*
Index of this file:
// [SECTION] includes
// [SECTION] forward declarations
// [SECTION] structs
// [SECTION] enums
// [SECTION] global data
// [SECTION] defines & macros
// [SECTION] internal helpers
// [SECTION] job system threads
// [SECTION] public api implementation
// [SECTION] internal helpers implementation
// [SECTION] extension loading
*/

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <float.h>
#include "pl.h"
#include "pl_terrain_ext.h"

// libs
#define PL_MATH_INCLUDE_FUNCTIONS
#include "pl_math.h"
#undef pl_vnsprintf
#include "pl_memory.h"
#include "pl_string.h"
#include "pl_json.h"

// extensions
#include "pl_graphics_ext.h"
#include "pl_vfs_ext.h"
#include "pl_platform_ext.h"
#include "pl_mesh_ext.h"
#include "pl_shader_ext.h"
#include "pl_image_ext.h"
#include "pl_gpu_allocators_ext.h"
#include "pl_tools_ext.h"
#include "pl_job_ext.h"
#include "pl_camera_ext.h"
#include "dc_draw_ext.h"
#include "dc_draw_backend_ext.h"
#include "pl_ui_ext.h"

// shader interop
#include "terrain_interop.h"

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

// basic types
typedef struct _plTerrainBufferChunk plTerrainBufferChunk;
typedef struct _plTerrainContext     plTerrainContext;

// enums/flags
typedef int plTerrainDirection;

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plHeightMapTile
{
    plTerrainTileFlags tFlags;

    plVec2   tWorldPos; // world position of top-left corner
    uint32_t uXOffset; // original texture offset
    uint32_t uYOffset; // original texture offset
    char     acFile[256];
    float    fMinHeight;
    float    fMaxHeight;

    // internal
    int32_t  _iXCoord;        // tile position in grid space
    int32_t  _iYCoord;        // tile position in grid space
    uint32_t _uXOffsetActual; // offset in terrain texture
    uint32_t _uYOffsetActual; // offset in terrain texture
    uint32_t _uChunkIndex;
    bool     _bEmpty;
} plHeightMapTile;

enum _plTerrainTileFlags
{
    PL_TERRAIN_TILE_FLAGS_NONE                = 0,
    PL_TERRAIN_TILE_FLAGS_ACTIVE              = 1 << 0,
    PL_TERRAIN_TILE_FLAGS_QUEUED              = 1 << 1,
    PL_TERRAIN_TILE_FLAGS_UPLOADED            = 1 << 2,
    PL_TERRAIN_TILE_FLAGS_PROCESSED           = 1 << 3,
};

typedef struct _plTerrainBufferChunk
{
    size_t   szOffset;
    uint32_t uOwnerTileIndex;
} plTerrainBufferChunk;

typedef struct _plTerrain
{
    plCamera tCamera;
    plDrawList3D* pt3dDrawlist;

    plTerrainFlags tFlags;

    // visualization options
    plVec3 tSunDirection;

    // bind groups
    plBindGroupHandle atBindGroup0[PL_MAX_FRAMES_IN_FLIGHT];
    plBindGroupHandle tMipmapBG0;
    plBindGroupHandle tPreprocessBG0;

    // height map
    uint32_t          uHeightMapResolution;
    uint32_t          uTileSize;
    float             fUnitConversion;
    float             fMetersPerTexel;
    float             fMaxElevation;
    float             fMinElevation;
    plVec2            tMinWorldPosition;
    plVec2            tMaxWorldPosition;
    uint32_t          uHorizontalTiles;
    uint32_t          uVerticalTiles;

    plBufferHandle tStagingBuffer;
    plAtomicCounter* ptQueuedCounter;

    plVec2 tCurrentExtent;
    plTerrainDirection tCurrentDirectionUpdate;
    uint32_t uCurrentXOffset;
    int32_t iCurrentXCoordMin;
    int32_t iCurrentXCoordMax;

    uint32_t uCurrentYOffset;
    int32_t iCurrentYCoordMin;
    int32_t iCurrentYCoordMax;

    uint32_t uTileCount;
    plHeightMapTile* atTiles;
    uint32_t* sbuActiveTileIndices;
    uint32_t* auFetchTileIndices;
    uint32_t* auPrefetchTileIndices;
    uint32_t  uNewActiveTileCount;
    uint32_t  uNewPrefetchTileCount;
    uint32_t  uMaxPrefetchedTiles;
    uint32_t  uMaxActiveTiles;
    uint32_t  uPrefetchRadius;

    uint32_t* sbuFreeChunks;
    plTerrainBufferChunk* atChunks;
    uint32_t  uChunkCapacity;

    // shaders
    plShaderHandle tRegularShader;
    plShaderHandle tWireframeShader;

    // compute shaders
    plComputeShaderHandle tPreProcessHeightShader;
    plComputeShaderHandle tMipMapShader;

    // textures
    plTextureHandle tRawTexture;                                     // raw heightmap
    plTextureHandle tDummyTexture;                                   // raw heightmap
    plTextureHandle tProcessedTexture;                               // processed heightmap
    plTextureHandle atActiveTexture[PL_MAX_FRAMES_IN_FLIGHT];        // actual heightmap in use (double buffered ideally)
    bool            abPendingTextureUpdate[PL_MAX_FRAMES_IN_FLIGHT]; // processing done, update active when ready

    plRenderPassHandle tRenderPass;
    plTextureHandle    tOutputTexture;
    plTextureHandle    tOutputTextureDepth;
    uint32_t           uOutputWidth;
    uint32_t           uOutputHeight;
    plBindGroupHandle  tOutputTextureHandle;
} plTerrain;
 
typedef struct _plTerrainContext
{
    bool bLoadedTextures;

    // graphics
    plDevice*                ptDevice;
    plRenderPassLayoutHandle tRenderPassLayout;
    plBindGroupPool*         ptBindGroupPool;
    plSamplerHandle          tSampler;
    plSamplerHandle          tMipSampler;
    plSamplerHandle          tMirrorSampler;

    // bind group layouts
    plBindGroupLayoutHandle tBindGroupLayout0;
    plBindGroupLayoutHandle tPreprocessBGLayout0;
    plBindGroupLayoutHandle tMipmapBGLayout0;

    // gpu allocators
    plDeviceMemoryAllocatorI* tLocalDedicatedAllocator;
    plDeviceMemoryAllocatorI* tLocalBuddyAllocator;
    plDeviceMemoryAllocatorI* tStagingDedicatedAllocator;
    plDeviceMemoryAllocatorI* tStagingBuddyAllocator;

    // CPU side data
    uint32_t  uIndexCount;
    uint32_t  uVertexCount;
    plVec3*   ptVertexBuffer;
    uint32_t* puIndexBuffer;

    // GPU side data
    plBufferHandle tIndexBuffer;
    plBufferHandle tVertexBuffer;
    plTextureHandle tDiffuseTexture;
    plTextureHandle tNoiseTexture;

    plDynamicDataBlock tCurrentDynamicDataBlock;

    // terrains
    plTerrain** sbtTerrains;

    // UI
    plUiTextFilter tFilter;
} plTerrainContext;

//-----------------------------------------------------------------------------
// [SECTION] enums
//-----------------------------------------------------------------------------

enum _plTerrainFlags
{
    PL_TERRAIN_FLAGS_NONE           = 0,
    PL_TERRAIN_FLAGS_WIREFRAME      = 1 << 0,
    PL_TERRAIN_FLAGS_TILE_STREAMING = 1 << 1,
    PL_TERRAIN_FLAGS_SHOW_ORIGIN    = 1 << 2,
    PL_TERRAIN_FLAGS_SHOW_BOUNDARY  = 1 << 3,
    PL_TERRAIN_FLAGS_SHOW_GRID      = 1 << 4,
};

enum _plTerrainDirection
{
    PL_TERRAIN_DIRECTION_NONE,
    PL_TERRAIN_DIRECTION_EAST,
    PL_TERRAIN_DIRECTION_WEST,
    PL_TERRAIN_DIRECTION_NORTH,
    PL_TERRAIN_DIRECTION_SOUTH,
    PL_TERRAIN_DIRECTION_ALL,
};

//-----------------------------------------------------------------------------
// [SECTION] global data
//-----------------------------------------------------------------------------

// required APIs
static const plGraphicsI*      gptGfx           = NULL;
static const plMemoryI*        gptMemory        = NULL;
static const plVfsI*           gptVfs           = NULL;
static const plMeshBuilderI*   gptMeshBuilder   = NULL;
static const plShaderI*        gptShader        = NULL;
static const plImageI*         gptImage         = NULL;
static const plGPUAllocatorsI* gptGpuAllocators = NULL;
static const plFileI*          gptFile          = NULL;
static const plJobI*           gptJob           = NULL;
static const plToolsI*         gptTools         = NULL;
static const plAtomicsI*       gptAtomics       = NULL;
static const plCameraI*        gptCamera        = NULL;
static const plDrawI*          gptDraw          = NULL;
static const plDrawBackendI*   gptDrawBackend   = NULL;
static const plUiI*            gptUI            = NULL;

// context
static plTerrainContext* gptCtx = NULL;

//-----------------------------------------------------------------------------
// [SECTION] defines & macros
//-----------------------------------------------------------------------------

#define PL_ALLOC(x)      gptMemory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_REALLOC(x, y) gptMemory->tracked_realloc((x), (y), __FILE__, __LINE__)
#define PL_FREE(x)       gptMemory->tracked_realloc((x), 0, __FILE__, __LINE__)

#ifndef PL_DS_ALLOC
    #define PL_DS_ALLOC(x)                      gptMemory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
    #define PL_DS_ALLOC_INDIRECT(x, FILE, LINE) gptMemory->tracked_realloc(NULL, (x), FILE, LINE)
    #define PL_DS_FREE(x)                       gptMemory->tracked_realloc((x), 0, __FILE__, __LINE__)
#endif

#include "pl_ds.h"

//-----------------------------------------------------------------------------
// [SECTION] internal helpers
//-----------------------------------------------------------------------------

static void            pl__terrain_create_shaders          (plTerrain*);
static plTextureHandle pl__terrain_load_texture            (plCommandBuffer*, const char* pcFile);
static uint32_t        pl__terrain_tile_activation_distance(plTerrain*, uint32_t uIndex);
static void            pl__terrain_return_free_chunk       (plTerrain*, uint32_t uOwnerTileIndex);
static void            pl__terrain_clear_cache             (plTerrain*, uint32_t uCount, uint32_t uRadius);
static void            pl__terrain_get_free_chunk          (plTerrain*, uint32_t uOwnerTileIndex);
static bool            pl__terrain_process_height_map_tiles(plTerrain*, plCommandBuffer* ptCmdBuffer, plVec3 tPos);
static plTextureHandle pl__terrain_create_texture          (plCommandBuffer* ptCmdBuffer, const plTextureDesc* ptDesc, const char* pcName, plTextureUsage);
static void            pl__terrain_prepare                 (plTerrain*, plCommandBuffer*);

static inline int
pl__terrain_get_tile_index_by_pos(plTerrain* ptTerrain, int iX, int iY)
{
    if(iX < 0 || iY < 0 || iX >= (int)ptTerrain->uHorizontalTiles || iY >= (int)ptTerrain->uVerticalTiles)
        return -1;
    return iX + iY * (int)ptTerrain->uHorizontalTiles;
}

static inline plHeightMapTile*
pl__terrain_get_tile_by_pos(plTerrain* ptTerrain, int iX, int iY)
{
    int iIndex = pl__terrain_get_tile_index_by_pos(ptTerrain, iX, iY);
    if(iIndex > -1)
        return &ptTerrain->atTiles[iIndex];
    return NULL;
}

void pl_terrain_tile_height_map(plTerrain* ptTerrain, plTerrainTilingInfo tInfo);

//-----------------------------------------------------------------------------
// [SECTION] job system threads
//-----------------------------------------------------------------------------

static void
pl__tile_upload_job(plInvocationData tInvoData, void* pData, void* pGroupSharedMemory)
{
    plTerrain* ptTerrain = (plTerrain*)pData;
    plHeightMapTile* ptTile = &ptTerrain->atTiles[ptTerrain->auFetchTileIndices[tInvoData.uGlobalIndex]];

    plBuffer* ptStagingBuffer = gptGfx->get_buffer(gptCtx->ptDevice, ptTerrain->tStagingBuffer);

    size_t szTileFileSize = ptTerrain->uTileSize * ptTerrain->uTileSize * sizeof(float);

    const plTerrainBufferChunk* ptChunk = &ptTerrain->atChunks[ptTile->_uChunkIndex];

    if(ptTile->_bEmpty)
        memset((uint8_t*)&ptStagingBuffer->tMemoryAllocation.pHostMapped[ptChunk->szOffset], 0, szTileFileSize);
    else
        gptFile->binary_read(ptTile->acFile, &szTileFileSize, (uint8_t*)&ptStagingBuffer->tMemoryAllocation.pHostMapped[ptChunk->szOffset]);
}

static void
pl__tile_prefetch_job(plInvocationData tInvoData, void* pData, void* pGroupSharedMemory)
{ 
    plTerrain* ptTerrain = (plTerrain*)pData;
    plHeightMapTile* ptTile = &ptTerrain->atTiles[ptTerrain->auPrefetchTileIndices[tInvoData.uGlobalIndex]];

    plBuffer* ptStagingBuffer = gptGfx->get_buffer(gptCtx->ptDevice, ptTerrain->tStagingBuffer);

    size_t szTileFileSize = ptTerrain->uTileSize * ptTerrain->uTileSize * sizeof(float);

    const plTerrainBufferChunk* ptChunk = &ptTerrain->atChunks[ptTile->_uChunkIndex];

    if(ptTile->_bEmpty)
        memset((uint8_t*)&ptStagingBuffer->tMemoryAllocation.pHostMapped[ptChunk->szOffset], 0, szTileFileSize);
    else
        gptFile->binary_read(ptTile->acFile, &szTileFileSize, (uint8_t*)&ptStagingBuffer->tMemoryAllocation.pHostMapped[ptChunk->szOffset]);
}

typedef struct _plTileJobData
{
    plTerrain* ptTerrain;
    plTerrainTilingInfo* ptTilingInfo;
} plTileJobData;

static void
pl__tile_job(plInvocationData tInvoData, void* pData, void* pGroupSharedMemory)
{
    plTileJobData* ptTileJobData = (plTileJobData*)pData;
    pl_terrain_tile_height_map(ptTileJobData->ptTerrain, ptTileJobData->ptTilingInfo[tInvoData.uGlobalIndex]);
}

//-----------------------------------------------------------------------------
// [SECTION] public api implementation
//-----------------------------------------------------------------------------

void
pl_terrain_initialize(plDevice* ptDevice)
{

    gptJob->initialize((plJobSystemInit){0});
    gptCtx->ptDevice = ptDevice;

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
                .uSourceSubpass = UINT32_MAX,
                .uDestinationSubpass = 0,
                .tSourceStageMask = PL_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT | PL_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS | PL_PIPELINE_STAGE_LATE_FRAGMENT_TESTS | PL_PIPELINE_STAGE_COMPUTE_SHADER,
                .tDestinationStageMask = PL_PIPELINE_STAGE_FRAGMENT_SHADER | PL_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT | PL_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS | PL_PIPELINE_STAGE_LATE_FRAGMENT_TESTS,
                .tSourceAccessMask = PL_ACCESS_COLOR_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ,
                .tDestinationAccessMask = PL_ACCESS_SHADER_READ | PL_ACCESS_COLOR_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ,
            },
            {
                .uSourceSubpass = 0,
                .uDestinationSubpass = UINT32_MAX,
                .tSourceStageMask = PL_PIPELINE_STAGE_FRAGMENT_SHADER | PL_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT | PL_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS | PL_PIPELINE_STAGE_LATE_FRAGMENT_TESTS,
                .tDestinationStageMask = PL_PIPELINE_STAGE_FRAGMENT_SHADER | PL_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT | PL_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS | PL_PIPELINE_STAGE_LATE_FRAGMENT_TESTS | PL_PIPELINE_STAGE_COMPUTE_SHADER,
                .tSourceAccessMask = PL_ACCESS_SHADER_READ | PL_ACCESS_COLOR_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ,
                .tDestinationAccessMask = PL_ACCESS_SHADER_READ | PL_ACCESS_COLOR_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE | PL_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ,
            }
        }
    };
    gptCtx->tRenderPassLayout = gptGfx->create_render_pass_layout(ptDevice, &tRenderPassLayoutDesc);


    // retrieve GPU allocators
    gptCtx->tLocalDedicatedAllocator   = gptGpuAllocators->get_local_dedicated_allocator(ptDevice);
    gptCtx->tLocalBuddyAllocator       = gptGpuAllocators->get_local_buddy_allocator(ptDevice);
    gptCtx->tStagingDedicatedAllocator = gptGpuAllocators->get_staging_uncached_allocator(ptDevice);
    gptCtx->tStagingBuddyAllocator     = gptGpuAllocators->get_staging_uncached_buddy_allocator(ptDevice);

    // create bind group pool
    plBindGroupPoolDesc tBindGroupPoolDesc = {
        .tFlags                   = PL_BIND_GROUP_POOL_FLAGS_NONE,
        .szSamplerBindings        = 10,
        .szSampledTextureBindings = 10,
        .szStorageTextureBindings = 10,
        .szStorageBufferBindings  = 10
    };
    gptCtx->ptBindGroupPool = gptGfx->create_bind_group_pool(ptDevice, &tBindGroupPoolDesc);

    // create samplers
    plSamplerDesc tSamplerDesc = {
        .tMagFilter     = PL_FILTER_LINEAR,
        .tMinFilter     = PL_FILTER_LINEAR,
        .tMipmapMode    = PL_MIPMAP_MODE_NEAREST,
        .fMaxAnisotropy = 0.0f,
        .fMinMip        = 0.0f,
        .fMaxMip        = 64.0f,
        .tVAddressMode  = PL_ADDRESS_MODE_CLAMP,
        .tUAddressMode  = PL_ADDRESS_MODE_CLAMP,
        .pcDebugName    = "sampler",
    };
    gptCtx->tSampler = gptGfx->create_sampler(ptDevice, &tSamplerDesc);

    tSamplerDesc.tMagFilter = PL_FILTER_NEAREST;
    tSamplerDesc.tMinFilter = PL_FILTER_NEAREST;
    gptCtx->tMipSampler  = gptGfx->create_sampler(ptDevice, &tSamplerDesc);

    plSamplerDesc tSamplerDesc2 = {
        .tMagFilter     = PL_FILTER_LINEAR,
        .tMinFilter     = PL_FILTER_LINEAR,
        .tMipmapMode    = PL_MIPMAP_MODE_LINEAR,
        .fMaxAnisotropy = 0.0f,
        .fMinMip        = 0.0f,
        .fMaxMip        = 64.0f,
        .tVAddressMode  = PL_ADDRESS_MODE_WRAP,
        .tUAddressMode  = PL_ADDRESS_MODE_WRAP,
        .pcDebugName    = "sampler",
    };
    gptCtx->tMirrorSampler = gptGfx->create_sampler(ptDevice, &tSamplerDesc2);

    // bind group layouts
    plBindGroupLayoutDesc tBindGroupLayout0 = {
        .atSamplerBindings = {
            {.uSlot = 0, .tStages = PL_SHADER_STAGE_VERTEX | PL_SHADER_STAGE_FRAGMENT },
            {.uSlot = 4, .tStages = PL_SHADER_STAGE_VERTEX | PL_SHADER_STAGE_FRAGMENT }
        },
        .atTextureBindings = {
            {.uSlot = 1, .tStages = PL_SHADER_STAGE_VERTEX | PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED },
            {.uSlot = 2, .tStages = PL_SHADER_STAGE_VERTEX | PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED },
            {.uSlot = 3, .tStages = PL_SHADER_STAGE_VERTEX | PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED },
        }
    };
    gptCtx->tBindGroupLayout0 = gptGfx->create_bind_group_layout(ptDevice, &tBindGroupLayout0);

    plBindGroupLayoutDesc tPreprocessBGLayout0Desc = {
        .atTextureBindings = {
            {.uSlot = 0, .tStages = PL_SHADER_STAGE_COMPUTE, .tType = PL_TEXTURE_BINDING_TYPE_STORAGE },
            {.uSlot = 1, .tStages = PL_SHADER_STAGE_COMPUTE, .tType = PL_TEXTURE_BINDING_TYPE_STORAGE },
        }
    };
    gptCtx->tPreprocessBGLayout0 = gptGfx->create_bind_group_layout(ptDevice, &tPreprocessBGLayout0Desc);

    plBindGroupLayoutDesc tMipmapBGLayout0Desc = {
        .atSamplerBindings = {
            {.uSlot = 0, .tStages = PL_SHADER_STAGE_COMPUTE }
        },
        .atTextureBindings = {
            {.uSlot = 1, .tStages = PL_SHADER_STAGE_COMPUTE, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED },
            {.uSlot = 2, .tStages = PL_SHADER_STAGE_COMPUTE, .tType = PL_TEXTURE_BINDING_TYPE_STORAGE },
        }
    };
    gptCtx->tMipmapBGLayout0 = gptGfx->create_bind_group_layout(ptDevice, &tMipmapBGLayout0Desc);

    gptCtx->tCurrentDynamicDataBlock = gptGfx->allocate_dynamic_data_block(ptDevice);
}

plBindGroupHandle
pl_get_terrain_texture(plTerrain* ptTerrain)
{
    return ptTerrain->tOutputTextureHandle;
}

plCamera*
pl_get_terrain_camera(plTerrain* ptTerrain)
{
    return &ptTerrain->tCamera;
}

void
pl_terrain_set_camera_pos(plTerrain* ptTerrain, float fX, float fY, float fZ)
{
    ptTerrain->tCamera.tPos.x = fX / ptTerrain->fUnitConversion;
    ptTerrain->tCamera.tPos.y = fY / ptTerrain->fUnitConversion;
    ptTerrain->tCamera.tPos.z = fZ / ptTerrain->fUnitConversion;
}

static inline float
pl__wrap_angle(float tTheta)
{
    static const float f2Pi = 2.0f * PL_PI;
    const float fMod = fmodf(tTheta, f2Pi);
    if (fMod > PL_PI)       return fMod - f2Pi;
    else if (fMod < -PL_PI) return fMod + f2Pi;
    return fMod;
}

void
pl_terrain_set_camera_orientation(plTerrain* ptTerrain, float fPitch, float fYaw, float fRoll)
{
    ptTerrain->tCamera.fPitch = fPitch;
    ptTerrain->tCamera.fYaw = fYaw;
    ptTerrain->tCamera.fRoll = fRoll;

    ptTerrain->tCamera.fYaw = pl__wrap_angle(ptTerrain->tCamera.fYaw);
    ptTerrain->tCamera.fPitch = pl__wrap_angle(ptTerrain->tCamera.fPitch);
}

void
pl_terrain_set_camera_aspect(plTerrain* ptTerrain, float fAspect)
{
    gptCamera->set_aspect(&ptTerrain->tCamera, fAspect);
}

plTerrain*
pl_create_terrain(plCommandBuffer* ptCmdBuffer, plTerrainInit tInit)
{

    plDevice* ptDevice = gptCtx->ptDevice;

    if(!gptCtx->bLoadedTextures)
    {
        gptCtx->tNoiseTexture   = pl__terrain_load_texture(ptCmdBuffer, "/assets/rock.jpg");
        gptCtx->tDiffuseTexture = pl__terrain_load_texture(ptCmdBuffer, "/assets/ground.jpg");
        gptCtx->bLoadedTextures = true;
    }

    plTerrain* ptTerrain = PL_ALLOC(sizeof(plTerrain));
    memset(ptTerrain, 0, sizeof(plTerrain));
    pl_sb_push(gptCtx->sbtTerrains, ptTerrain);

    ptTerrain->pt3dDrawlist = gptDraw->request_3d_drawlist();
    ptTerrain->tCamera.tPos         = (plVec3){0.0f, 10.0f, 0.0f};
    ptTerrain->tCamera.fNearZ       = 0.01f;
    // ptTerrain->tCamera.fFarZ        = 4.75f * 2048.0f / PL_UNIT_CONVERSION;
    ptTerrain->tCamera.fFarZ        = 100000.0f;
    ptTerrain->tCamera.fFieldOfView = PL_PI_3;
    ptTerrain->tCamera.fAspectRatio = 1.0f;
    ptTerrain->tCamera.fYaw         = PL_PI_4;
    ptTerrain->tCamera.fPitch       = -0.3f;
    gptCamera->update(&ptTerrain->tCamera);

    ptTerrain->uOutputWidth            = tInit.uOutputWidth;
    ptTerrain->uOutputHeight           = tInit.uOutputHeight;
    // ptTerrain->tFlags                  = PL_TERRAIN_FLAGS_TILE_STREAMING | PL_TERRAIN_FLAGS_SHOW_ORIGIN | PL_TERRAIN_FLAGS_SHOW_BOUNDARY | PL_TERRAIN_FLAGS_SHOW_GRID;
    ptTerrain->tFlags                  = PL_TERRAIN_FLAGS_TILE_STREAMING | PL_TERRAIN_FLAGS_SHOW_BOUNDARY;
    ptTerrain->uHeightMapResolution    = 4096;
    ptTerrain->uTileSize               = 128;
    ptTerrain->fUnitConversion         = tInit.fUnitConversion;
    ptTerrain->fMetersPerTexel         = tInit.fMetersPerTexel;
    ptTerrain->fMaxElevation           = tInit.fMaxElevation;
    ptTerrain->fMinElevation           = tInit.fMinElevation;
    ptTerrain->tMaxWorldPosition       = tInit.tMaxPosition;
    ptTerrain->tMinWorldPosition       = tInit.tMinPosition;
    ptTerrain->tCurrentDirectionUpdate = PL_TERRAIN_DIRECTION_ALL;
    ptTerrain->tSunDirection           = (plVec3){-1.0f, -1.0f, -1.0f};
    pl__terrain_create_shaders(ptTerrain);

    plTextureDesc tTextureDesc = {
        .tDimensions = { (float)ptTerrain->uHeightMapResolution, (float)ptTerrain->uHeightMapResolution, 1},
        .tFormat     = PL_FORMAT_R32_FLOAT,
        .uLayers     = 1,
        .uMips       = 1,
        .tType       = PL_TEXTURE_TYPE_2D,
        .tUsage      = PL_TEXTURE_USAGE_SAMPLED | PL_TEXTURE_USAGE_STORAGE,
        .pcDebugName = "raw height map texture",
    };

    ptTerrain->tRawTexture = gptGfx->create_texture(ptDevice, &tTextureDesc, NULL);

    plTextureDesc tProcessedTextureDesc = {
        .tDimensions = { (float)ptTerrain->uHeightMapResolution, (float)ptTerrain->uHeightMapResolution, 1},
        .tFormat     = PL_FORMAT_R32G32B32A32_FLOAT,
        .uLayers     = 1,
        .uMips       = 0,
        .tType       = PL_TEXTURE_TYPE_2D,
        .tUsage      = PL_TEXTURE_USAGE_SAMPLED | PL_TEXTURE_USAGE_STORAGE,
        .pcDebugName = "process height map texture",
    };
    ptTerrain->tProcessedTexture = gptGfx->create_texture(ptDevice, &tProcessedTextureDesc, NULL);

    tProcessedTextureDesc.pcDebugName = "dummy texture";
    tProcessedTextureDesc.uMips = 1;
    ptTerrain->tDummyTexture = gptGfx->create_texture(ptDevice, &tProcessedTextureDesc, NULL);
    tProcessedTextureDesc.uMips = 0;
    tProcessedTextureDesc.pcDebugName = "active height map texture";
    for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
        ptTerrain->atActiveTexture[i] = gptGfx->create_texture(ptDevice, &tProcessedTextureDesc, NULL);

    // retrieve new texture (also could have used out param from create_texture above)
    plTexture* ptRawTexture = gptGfx->get_texture(ptDevice, ptTerrain->tRawTexture);
    plTexture* ptDummyTexture = gptGfx->get_texture(ptDevice, ptTerrain->tDummyTexture);
    plTexture* ptTexture = gptGfx->get_texture(ptDevice, ptTerrain->tProcessedTexture);

    size_t szBuddyBlockSize = gptGpuAllocators->get_buddy_block_size();

    plDeviceMemoryAllocatorI* ptAllocator = gptCtx->tLocalDedicatedAllocator;

    if(ptTexture->tMemoryRequirements.ulSize < szBuddyBlockSize)
        ptAllocator = gptCtx->tLocalBuddyAllocator;

    // allocate memory
    const plDeviceMemoryAllocation tRawTextureAllocation = ptAllocator->allocate(ptAllocator->ptInst,
        ptRawTexture->tMemoryRequirements.uMemoryTypeBits,
        ptRawTexture->tMemoryRequirements.ulSize,
        ptRawTexture->tMemoryRequirements.ulAlignment,
        "raw heightmap memory");

    const plDeviceMemoryAllocation tProcessedTextureAllocation = ptAllocator->allocate(ptAllocator->ptInst,
        ptTexture->tMemoryRequirements.uMemoryTypeBits,
        ptTexture->tMemoryRequirements.ulSize,
        ptTexture->tMemoryRequirements.ulAlignment,
        "processed heightmap memory");

    const plDeviceMemoryAllocation tDummyTextureAllocation = ptAllocator->allocate(ptAllocator->ptInst,
        ptDummyTexture->tMemoryRequirements.uMemoryTypeBits,
        ptDummyTexture->tMemoryRequirements.ulSize,
        ptDummyTexture->tMemoryRequirements.ulAlignment,
        "dummy heightmap memory");

    // bind memory
    gptGfx->bind_texture_to_memory(ptDevice, ptTerrain->tRawTexture, &tRawTextureAllocation);
    gptGfx->bind_texture_to_memory(ptDevice, ptTerrain->tProcessedTexture, &tProcessedTextureAllocation);
    gptGfx->bind_texture_to_memory(ptDevice, ptTerrain->tDummyTexture, &tDummyTextureAllocation);

    for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
    {
        const plDeviceMemoryAllocation tActiveTextureAllocation = ptAllocator->allocate(ptAllocator->ptInst,
            ptTexture->tMemoryRequirements.uMemoryTypeBits,
            ptTexture->tMemoryRequirements.ulSize,
            ptTexture->tMemoryRequirements.ulAlignment,
            "active heightmap memory");
        gptGfx->bind_texture_to_memory(ptDevice, ptTerrain->atActiveTexture[i], &tActiveTextureAllocation);
    }

    // ptTerrain->uPrefetchRadius = tInit.uPrefetchRadius == 0 ? 2 : tInit.uPrefetchRadius;
    ptTerrain->uPrefetchRadius = 2;

    const uint32_t uTilesAcross = ptTerrain->uHeightMapResolution / ptTerrain->uTileSize;
    ptTerrain->uMaxPrefetchedTiles = uTilesAcross * ptTerrain->uPrefetchRadius * 4 + ptTerrain->uPrefetchRadius * ptTerrain->uPrefetchRadius * 4;
    ptTerrain->uMaxPrefetchedTiles *= 2;
    ptTerrain->uMaxActiveTiles = uTilesAcross * uTilesAcross;

    // create staging buffer
    plBufferDesc tStagingBufferDesc = {
        .tUsage      = PL_BUFFER_USAGE_STAGING,
        .szByteSize  = (ptTerrain->uMaxPrefetchedTiles + ptTerrain->uMaxActiveTiles) * (ptTerrain->uTileSize * ptTerrain->uTileSize * sizeof(float)),
        .pcDebugName = "staging buffer"
    };

    plBuffer* ptStagingBuffer = NULL;
    ptTerrain->tStagingBuffer = gptGfx->create_buffer(ptDevice, &tStagingBufferDesc, &ptStagingBuffer);

    // allocate memory for the vertex buffer
    ptAllocator = gptCtx->tStagingDedicatedAllocator;
    const plDeviceMemoryAllocation tStagingBufferAllocation = ptAllocator->allocate(ptAllocator->ptInst,
        ptStagingBuffer->tMemoryRequirements.uMemoryTypeBits,
        ptStagingBuffer->tMemoryRequirements.ulSize,
        0,
        "staging buffer memory");
    gptGfx->bind_buffer_to_memory(ptDevice, ptTerrain->tStagingBuffer, &tStagingBufferAllocation);
    memset(ptStagingBuffer->tMemoryAllocation.pHostMapped, 0, ptStagingBuffer->tMemoryRequirements.ulSize);

    ptTerrain->auPrefetchTileIndices = PL_ALLOC(sizeof(uint32_t) * ptTerrain->uMaxPrefetchedTiles);
    memset(ptTerrain->auPrefetchTileIndices, 255, sizeof(uint32_t) * ptTerrain->uMaxPrefetchedTiles);
    ptTerrain->auFetchTileIndices = PL_ALLOC(sizeof(uint32_t) * ptTerrain->uMaxActiveTiles);
    memset(ptTerrain->auFetchTileIndices, 255, sizeof(uint32_t) * ptTerrain->uMaxActiveTiles);

    ptTerrain->uChunkCapacity = ptTerrain->uMaxPrefetchedTiles + ptTerrain->uMaxActiveTiles;
    ptTerrain->atChunks = PL_ALLOC(ptTerrain->uChunkCapacity * sizeof(plTerrainBufferChunk));
    pl_sb_resize(ptTerrain->sbuFreeChunks, ptTerrain->uChunkCapacity);
    for(uint32_t i = 0; i < ptTerrain->uChunkCapacity; i++)
    {
        ptTerrain->atChunks[i].szOffset = ptTerrain->uTileSize * ptTerrain->uTileSize * sizeof(float) * i;
        ptTerrain->atChunks[i].uOwnerTileIndex = UINT32_MAX;
        ptTerrain->sbuFreeChunks[i] = i;
    }

    // create bindgroup
    plBindGroupDesc tBindGroupDesc = {
        .tLayout = gptCtx->tBindGroupLayout0,
        .ptPool  = gptCtx->ptBindGroupPool,
        .pcDebugName = "bind group 0"
    };

    plBindGroupUpdateSamplerData atSamplerData[] = {
        {
            .tSampler = gptCtx->tSampler,
            .uSlot    = 0
        },
        {
            .tSampler = gptCtx->tMirrorSampler,
            .uSlot    = 4
        },
    };

    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdBuffer);
    gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE);
    gptGfx->set_texture_usage(ptBlit, ptTerrain->tDummyTexture, PL_TEXTURE_USAGE_STORAGE, 0);
    gptGfx->set_texture_usage(ptBlit, ptTerrain->tRawTexture, PL_TEXTURE_USAGE_STORAGE, 0);
    gptGfx->set_texture_usage(ptBlit, ptTerrain->tProcessedTexture, PL_TEXTURE_USAGE_SAMPLED, 0);
    for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
    {
        gptGfx->set_texture_usage(ptBlit, ptTerrain->atActiveTexture[i], PL_TEXTURE_USAGE_SAMPLED, 0);
    }
    gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ);
    gptGfx->end_blit_pass(ptBlit);

    for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
    {
        ptTerrain->atBindGroup0[i] = gptGfx->create_bind_group(ptDevice, &tBindGroupDesc);

        plBindGroupUpdateTextureData atTextureData[] = 
        {
            {
                .tTexture = ptTerrain->atActiveTexture[i],
                .uSlot    = 1,
                .tType    = PL_TEXTURE_BINDING_TYPE_SAMPLED
            },
            {
                .tTexture = gptCtx->tNoiseTexture,
                .uSlot    = 2,
                .tType    = PL_TEXTURE_BINDING_TYPE_SAMPLED
            },
            {
                .tTexture = gptCtx->tDiffuseTexture,
                .uSlot    = 3,
                .tType    = PL_TEXTURE_BINDING_TYPE_SAMPLED
            },
        };

        plBindGroupUpdateData tBGData = {
            .uSamplerCount     = 2,
            .atSamplerBindings = atSamplerData,
            .uTextureCount     = 3,
            .atTextureBindings = atTextureData
        };
        gptGfx->update_bind_group(ptDevice, ptTerrain->atBindGroup0[i], &tBGData);
    }

    plBindGroupDesc tPreprocessBG0Desc = {
        .tLayout = gptCtx->tPreprocessBGLayout0,
        .ptPool = gptCtx->ptBindGroupPool,
        .pcDebugName = "compute bind group 0"
    };
    ptTerrain->tPreprocessBG0 = gptGfx->create_bind_group(ptDevice, &tPreprocessBG0Desc);

    plBindGroupDesc tMipmapBG0Desc = {
        .tLayout = gptCtx->tMipmapBGLayout0,
        .ptPool = gptCtx->ptBindGroupPool,
        .pcDebugName = "compute bind group 0"
    };
    ptTerrain->tMipmapBG0 = gptGfx->create_bind_group(ptDevice, &tMipmapBG0Desc);

    plBindGroupUpdateTextureData atComputeTextureData0[2] = {
        {.uSlot = 0, .tTexture = ptTerrain->tRawTexture, .tType = PL_TEXTURE_BINDING_TYPE_STORAGE, .tCurrentUsage = PL_TEXTURE_USAGE_STORAGE},
        {.uSlot = 1, .tTexture = ptTerrain->tProcessedTexture, .tType = PL_TEXTURE_BINDING_TYPE_STORAGE, .tCurrentUsage = PL_TEXTURE_USAGE_STORAGE},
    };

    plBindGroupUpdateData tPreprocessBG0Data = {
        .uTextureCount = 2,
        .atTextureBindings = atComputeTextureData0
    };
    gptGfx->update_bind_group(ptDevice, ptTerrain->tPreprocessBG0, &tPreprocessBG0Data);
    
    plBindGroupUpdateSamplerData tSamplerData = {
        .tSampler = gptCtx->tMipSampler,
        .uSlot    = 0
    };

    plBindGroupUpdateTextureData atComputeTextureData1[2] = {
        {.uSlot = 1, .tTexture = ptTerrain->tProcessedTexture, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED, .tCurrentUsage = PL_TEXTURE_USAGE_SAMPLED},
        {.uSlot = 2, .tTexture = ptTerrain->tDummyTexture, .tType = PL_TEXTURE_BINDING_TYPE_STORAGE, .tCurrentUsage = PL_TEXTURE_USAGE_STORAGE},
    };

    plBindGroupUpdateData tMipmapBG0Data = {
        .uSamplerCount     = 1,
        .atSamplerBindings = &tSamplerData,
        .uTextureCount     = 2,
        .atTextureBindings = atComputeTextureData1
    };
    gptGfx->update_bind_group(ptDevice, ptTerrain->tMipmapBG0, &tMipmapBG0Data);

    ptTerrain->uHorizontalTiles = (uint32_t)((ptTerrain->tMaxWorldPosition.x - ptTerrain->tMinWorldPosition.x) / ((float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel));
    ptTerrain->uVerticalTiles   = (uint32_t)((ptTerrain->tMaxWorldPosition.y - ptTerrain->tMinWorldPosition.y) / ((float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel));

    if(ptTerrain->uHeightMapResolution / ptTerrain->uTileSize > ptTerrain->uHorizontalTiles)
    {
         ptTerrain->uHorizontalTiles = ptTerrain->uHeightMapResolution / ptTerrain->uTileSize;
         ptTerrain->tMaxWorldPosition.x = ptTerrain->uHorizontalTiles * (float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel + ptTerrain->tMinWorldPosition.x;
    }

    if(ptTerrain->uHeightMapResolution / ptTerrain->uTileSize > ptTerrain->uVerticalTiles)
    {
         ptTerrain->uVerticalTiles = ptTerrain->uHeightMapResolution / ptTerrain->uTileSize;
         ptTerrain->tMaxWorldPosition.y = ptTerrain->uVerticalTiles * (float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel + ptTerrain->tMinWorldPosition.y;
    }

    ptTerrain->uTileCount = ptTerrain->uHorizontalTiles * ptTerrain->uVerticalTiles;
    ptTerrain->atTiles = PL_ALLOC(ptTerrain->uTileCount * sizeof(plHeightMapTile));
    memset(ptTerrain->atTiles, 0, ptTerrain->uTileCount * sizeof(plHeightMapTile));

    // load with empty
    uint32_t uYCoord = UINT32_MAX;
    for(uint32_t i = 0; i < ptTerrain->uTileCount; i++)
    {
        plHeightMapTile* ptTile = &ptTerrain->atTiles[i];

        uint32_t uXCoord = (int)(i % ptTerrain->uHorizontalTiles);
        if(uXCoord == 0)
            uYCoord++;

        ptTile->fMaxHeight  = 0.0f;
        ptTile->fMinHeight  = 0.0f;
        ptTile->uXOffset    = uXCoord * ptTerrain->uTileSize;
        ptTile->uYOffset    = uYCoord * ptTerrain->uTileSize;
        ptTile->_iXCoord    = (int)uXCoord * (int)ptTerrain->uTileSize;
        ptTile->_iYCoord    = (int)uYCoord * (int)ptTerrain->uTileSize;
        ptTile->tWorldPos.x = (float)(ptTile->_iXCoord) * ptTerrain->fMetersPerTexel + ptTerrain->tMinWorldPosition.x;
        ptTile->tWorldPos.y = (float)(ptTile->_iYCoord) * ptTerrain->fMetersPerTexel + ptTerrain->tMinWorldPosition.y;
        ptTile->_iXCoord    = (int32_t)((ptTile->tWorldPos.x - ptTerrain->tMinWorldPosition.x) / ((float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel));
        ptTile->_iYCoord    = (int32_t)((ptTile->tWorldPos.y - ptTerrain->tMinWorldPosition.y) / ((float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel));
    }

    const plTextureDesc tOutputTextureDesc = {
        .tDimensions   = {(float)ptTerrain->uOutputWidth, (float)ptTerrain->uOutputHeight, 1},
        .tFormat       = PL_FORMAT_R8G8B8A8_UNORM,
        .uLayers       = 1,
        .uMips         = 1,
        .tType         = PL_TEXTURE_TYPE_2D,
        .tUsage        = PL_TEXTURE_USAGE_SAMPLED | PL_TEXTURE_USAGE_COLOR_ATTACHMENT,
        .pcDebugName   = "final output"
    };

    const plTextureDesc tDepthTextureDesc = {
        .tDimensions   = {(float)ptTerrain->uOutputWidth, (float)ptTerrain->uOutputHeight, 1},
        .tFormat       = PL_FORMAT_D32_FLOAT_S8_UINT,
        .uLayers       = 1,
        .uMips         = 1,
        .tType         = PL_TEXTURE_TYPE_2D,
        .tUsage        = PL_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT,
        .pcDebugName   = "final output depth"
    };

    ptTerrain->tOutputTexture = pl__terrain_create_texture(ptCmdBuffer, &tOutputTextureDesc, "final output", PL_TEXTURE_USAGE_SAMPLED);
    ptTerrain->tOutputTextureDepth = pl__terrain_create_texture(ptCmdBuffer, &tDepthTextureDesc, "final output depth", PL_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT);
    ptTerrain->tOutputTextureHandle = gptDrawBackend->create_bind_group_for_texture(ptTerrain->tOutputTexture);


    plRenderPassAttachments atAttachmentSets[PL_MAX_FRAMES_IN_FLIGHT] = {0};

    for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
    {
        atAttachmentSets[i].atViewAttachments[0] = ptTerrain->tOutputTextureDepth;
        atAttachmentSets[i].atViewAttachments[1] = ptTerrain->tOutputTexture;
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
                .fClearZ         = 1.0f
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
        .tDimensions = {.x = (float)ptTerrain->uOutputWidth, .y = (float)ptTerrain->uOutputHeight},
        .pcDebugName = "Main"
    };
    ptTerrain->tRenderPass = gptGfx->create_render_pass(ptDevice, &tRenderPassDesc, atAttachmentSets);

    return ptTerrain;
}

void
pl_terrain_cleanup(void)
{
    plDevice* ptDevice = gptCtx->ptDevice;
    gptGfx->flush_device(ptDevice);
    for(uint32_t i = 0; i < pl_sb_size(gptCtx->sbtTerrains); i++)
    {
        plTerrain* ptTerrain = gptCtx->sbtTerrains[i];
        gptGfx->destroy_shader(ptDevice, ptTerrain->tRegularShader);
        gptGfx->destroy_shader(ptDevice, ptTerrain->tWireframeShader);
        gptGfx->destroy_compute_shader(ptDevice, ptTerrain->tPreProcessHeightShader);
        gptGfx->destroy_compute_shader(ptDevice, ptTerrain->tMipMapShader);
        gptGfx->destroy_texture(ptDevice, ptTerrain->tProcessedTexture);
        for(uint32_t j = 0; j < gptGfx->get_frames_in_flight(); j++)
        {
            gptGfx->destroy_bind_group(ptDevice, ptTerrain->atBindGroup0[j]);
            gptGfx->destroy_texture(ptDevice, ptTerrain->atActiveTexture[j]);
        }
        gptDraw->return_3d_drawlist(ptTerrain->pt3dDrawlist);
        gptGfx->destroy_texture(ptDevice, ptTerrain->tDummyTexture);
        gptGfx->destroy_texture(ptDevice, ptTerrain->tRawTexture);
        gptGfx->destroy_bind_group(ptDevice, ptTerrain->tMipmapBG0);
        pl_sb_free(ptTerrain->sbuActiveTileIndices);
        pl_sb_free(ptTerrain->sbuFreeChunks);
        PL_FREE(ptTerrain->auPrefetchTileIndices);
        PL_FREE(ptTerrain->auFetchTileIndices);
        PL_FREE(ptTerrain->atChunks);
        PL_FREE(ptTerrain->atTiles);
        PL_FREE(ptTerrain);
    }
    pl_sb_free(gptCtx->sbtTerrains);

    gptGfx->destroy_buffer(ptDevice, gptCtx->tIndexBuffer);
    gptGfx->destroy_buffer(ptDevice, gptCtx->tVertexBuffer);

    gptGfx->destroy_texture(ptDevice, gptCtx->tNoiseTexture);
    gptGfx->destroy_texture(ptDevice, gptCtx->tDiffuseTexture);
    gptGfx->destroy_sampler(ptDevice, gptCtx->tSampler);
    gptGfx->destroy_sampler(ptDevice, gptCtx->tMirrorSampler);
    
    gptGfx->destroy_bind_group_layout(ptDevice, gptCtx->tBindGroupLayout0);
    gptGfx->destroy_bind_group_layout(ptDevice, gptCtx->tPreprocessBGLayout0);
    gptGfx->destroy_bind_group_layout(ptDevice, gptCtx->tMipmapBGLayout0);
    gptGfx->cleanup_bind_group_pool(gptCtx->ptBindGroupPool);

    gptGpuAllocators->cleanup(ptDevice);
}

void
pl_terrain_tile_height_map(plTerrain* ptTerrain, plTerrainTilingInfo tInfo)
{

    size_t szImageFileSize = gptVfs->get_file_size_str(tInfo.pcFile);
    plVfsFileHandle tSpriteSheet = gptVfs->open_file(tInfo.pcFile, PL_VFS_FILE_MODE_READ);
    gptVfs->read_file(tSpriteSheet, NULL, &szImageFileSize);
    unsigned char* pucBuffer = (unsigned char*)PL_ALLOC(szImageFileSize);
    gptVfs->read_file(tSpriteSheet, pucBuffer, &szImageFileSize);
    gptVfs->close_file(tSpriteSheet);

    uint32_t uHorizontalTileCount = 0;
    uint32_t uVerticalTileCount = 0;
    plImageInfo tImageInfo = {0};
    gptImage->get_info(pucBuffer, (int)szImageFileSize, &tImageInfo);


    const uint32_t uXAlignmentOffset = (uint32_t)(tInfo.tOrigin.x - ptTerrain->tMinWorldPosition.x) % ptTerrain->uTileSize;
    const uint32_t uYAlignmentOffset = (uint32_t)(tInfo.tOrigin.y - ptTerrain->tMinWorldPosition.y) % ptTerrain->uTileSize;

    uHorizontalTileCount = (int)ceilf((float)((uint32_t)tImageInfo.iWidth + uXAlignmentOffset) / (float)ptTerrain->uTileSize);
    uVerticalTileCount   = (int)ceilf((float)((uint32_t)tImageInfo.iHeight + uYAlignmentOffset) / (float)ptTerrain->uTileSize);


    float* pufImageData = NULL;
    unsigned char* pucImageData = NULL;
    uint8_t* puBuffer = NULL;

    char pcFileNameOnlyBuffer[256] = {0};
    pl_str_get_file_name_only(tInfo.pcFile, pcFileNameOnlyBuffer, 256);

    plTempAllocator tAllocator = {0};
    char* pcFileName = pl_temp_allocator_sprintf(&tAllocator, "cache/tile_%s_%u.txt", pcFileNameOnlyBuffer, ptTerrain->uTileSize);

    // load actual data from file data
    int iImageWidth = 0;
    int iImageHeight = 0;
        
    if(gptFile->exists(pcFileName))
    {
        plImageInfo tImageInfo2 = {0};
        gptImage->get_info(pucBuffer, (int)szImageFileSize, &tImageInfo2);
        iImageWidth = tImageInfo2.iWidth;
        iImageHeight = tImageInfo2.iHeight;
    }
    else
    {
        int _unused;
        pufImageData = gptImage->load_hdr(pucBuffer, (int)szImageFileSize, &iImageWidth, &iImageHeight, &_unused, 1);
        pucImageData = (unsigned char*)pufImageData;


        // TODO (JHH): actually store required info
        int iDummy = 7;
        gptFile->binary_write(pcFileName, sizeof(int), (uint8_t*)&iDummy);

        puBuffer = PL_ALLOC(ptTerrain->uTileSize * ptTerrain->uTileSize * sizeof(float));
        memset(puBuffer, 0, ptTerrain->uTileSize * ptTerrain->uTileSize * sizeof(float));
    }
    pl_temp_allocator_reset(&tAllocator);
    PL_FREE(pucBuffer);

    const uint32_t uFileSize = ptTerrain->uTileSize * ptTerrain->uTileSize * sizeof(float);

    for(uint32_t j = 0; j < uVerticalTileCount; j++)
    {
        uint32_t uActiveTileHeight = ptTerrain->uTileSize;
        uint32_t uYStart = 0;
        uint32_t uYSrcStart = 0;

        if(j == 0)
        {
            uYStart = uYAlignmentOffset;
        }
        else if(j == uVerticalTileCount - 1)
        {
            uActiveTileHeight = (uint32_t)iImageHeight - ptTerrain->uTileSize * j + uYAlignmentOffset;
            uYSrcStart = uYAlignmentOffset;
        }
        else
        {
            uYSrcStart = uYAlignmentOffset;
        }

        for(uint32_t i = 0; i < uHorizontalTileCount; i++)
        {
            uint32_t uTileIndex = i + j * uHorizontalTileCount;
            uint32_t uStartX = 0;
            char* pcCacheFileName = pl_temp_allocator_sprintf(&tAllocator, "cache/tile_%s_%u_%u_%u.hdr", pcFileNameOnlyBuffer, ptTerrain->uTileSize, i, j);

            if(!gptFile->exists(pcCacheFileName))
            {

                memset(puBuffer, 0, uFileSize);

                uint32_t uActiveTileWidth = ptTerrain->uTileSize;
                uint32_t uXSrcStart = 0;
                if(i == 0)
                {
                    uActiveTileWidth = ptTerrain->uTileSize - uXAlignmentOffset;
                    uStartX = uXAlignmentOffset;
                }
                else if(i == uHorizontalTileCount - 1)
                {
                    uActiveTileWidth = (uint32_t)iImageWidth - ptTerrain->uTileSize * i + uXAlignmentOffset;
                    uXSrcStart = uXAlignmentOffset;
                }
                else
                {
                    uXSrcStart = uXAlignmentOffset;
                }

                {

                    uint32_t uFileOffset = (ptTerrain->uTileSize * j - uYSrcStart) * iImageWidth * sizeof(float);
                    uFileOffset += (ptTerrain->uTileSize * i - uXSrcStart) * sizeof(float);

                    unsigned char* pcStart = &pucImageData[uFileOffset];

                    for(uint32_t k = uYStart; k < uActiveTileHeight; k++)
                    {
                        uint32_t uDest = k * ptTerrain->uTileSize * sizeof(float) + uStartX * sizeof(float);
                        memcpy(&puBuffer[uDest], &pcStart[(k - uYStart) * iImageWidth * sizeof(float)], uActiveTileWidth * sizeof(float));
                    }
                }

                gptFile->binary_write(pcCacheFileName, ptTerrain->uTileSize * ptTerrain->uTileSize * sizeof(float), puBuffer);
            }

            plHeightMapTile tTile = {0};
            tTile.fMinHeight   = tInfo.fMinHeight;
            tTile.fMaxHeight   = tInfo.fMaxHeight;
            tTile._bEmpty      = false;
            tTile.uXOffset     = i * ptTerrain->uTileSize;
            tTile.uYOffset     = j * ptTerrain->uTileSize;
            tTile.tWorldPos    = tInfo.tOrigin;
            tTile.tWorldPos.x -= uXAlignmentOffset;
            tTile.tWorldPos.y -= uYAlignmentOffset;
            tTile.tWorldPos.x += tTile.uXOffset * ptTerrain->fMetersPerTexel;
            tTile.tWorldPos.y += tTile.uYOffset * ptTerrain->fMetersPerTexel;
            strncpy(tTile.acFile, pcFileName, 256);

            tTile._iXCoord = (int32_t)((tTile.tWorldPos.x - ptTerrain->tMinWorldPosition.x) / ((float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel));
            tTile._iYCoord = (int32_t)((tTile.tWorldPos.y - ptTerrain->tMinWorldPosition.y) / ((float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel));

            int iIndex = pl__terrain_get_tile_index_by_pos(ptTerrain, tTile._iXCoord, tTile._iYCoord);

            PL_ASSERT(iIndex > -1);
            ptTerrain->atTiles[iIndex] = tTile;

            pl_temp_allocator_reset(&tAllocator);
        }
    }
    pl_temp_allocator_free(&tAllocator);
    
    if(pufImageData)
        gptImage->free(pufImageData);
    if(puBuffer)
    {
        PL_FREE(puBuffer);
    }
}

plTerrain*
pl_create_terrain_from_file(plCommandBuffer* ptCmdBuffer, const char* pcFile)
{
    if(!gptVfs->does_file_exist(pcFile))
        return NULL;

    size_t szImageFileSize = gptVfs->get_file_size_str(pcFile);
    plVfsFileHandle tShaderJson = gptVfs->open_file(pcFile, PL_VFS_FILE_MODE_READ);
    gptVfs->read_file(tShaderJson, NULL, &szImageFileSize);
    char* pucBuffer = (char*)PL_ALLOC(szImageFileSize + 1);
    memset(pucBuffer, 0, szImageFileSize + 1);
    gptVfs->read_file(tShaderJson, pucBuffer, &szImageFileSize);
    gptVfs->close_file(tShaderJson);

    plJsonObject* ptRootJsonObject = NULL;
    pl_load_json(pucBuffer, &ptRootJsonObject);

    plTerrainInit tInit = {0};

    plJsonObject* ptInfo = pl_json_member(ptRootJsonObject, "info");

    tInit.uOutputWidth = pl_json_uint_member(ptInfo, "uOutputWidth", 256);
    tInit.uOutputHeight = pl_json_uint_member(ptInfo, "uOutputHeight", 256);
    tInit.fUnitConversion = pl_json_float_member(ptInfo, "fUnitConversion", 100.0f);
    tInit.fMetersPerTexel = pl_json_float_member(ptInfo, "fMetersPerTexel", 1.0f);
    tInit.fMinElevation = pl_json_float_member(ptInfo, "fMinElevation", 0.0f);
    tInit.fMaxElevation = pl_json_float_member(ptInfo, "fMaxElevation", 1.0f);
    tInit.fMaxElevation = pl_json_float_member(ptInfo, "fMaxElevation", 1.0f);
    pl_json_float_array_member(ptInfo, "tMinPosition", tInit.tMinPosition.d, NULL);
    pl_json_float_array_member(ptInfo, "tMaxPosition", tInit.tMaxPosition.d, NULL);


    plTerrain* ptTerrain = pl_create_terrain(ptCmdBuffer, tInit);

    uint32_t uSourceCount = 0;
    plJsonObject* ptSources = pl_json_array_member(ptRootJsonObject, "sources", &uSourceCount);
    plTerrainTilingInfo* ptTilingInfo = PL_ALLOC(sizeof(plTerrainTilingInfo) * uSourceCount);
    memset(ptTilingInfo, 0, sizeof(plTerrainTilingInfo) * uSourceCount);
    for(uint32_t i = 0; i < uSourceCount; i++)
    {
        plJsonObject* ptSource = pl_json_member_by_index(ptSources, i);

        ptTilingInfo[i].fMaxHeight = pl_json_float_member(ptSource, "fMaxHeight", 1.0f);
        ptTilingInfo[i].fMinHeight = pl_json_float_member(ptSource, "fMinHeight", 0.0f);

        pl_json_string_member(ptSource, "pcFile", ptTilingInfo[i].pcFile, 64);
        pl_json_float_array_member(ptSource, "tOrigin", ptTilingInfo[i].tOrigin.d, NULL);
        ptTilingInfo[i].tOrigin = pl_add_vec2(ptTilingInfo[i].tOrigin, tInit.tMinPosition);

        // preregister with vfs since vfs isn't thread safe
        plVfsFileHandle tFileHandle = gptVfs->open_file(ptTilingInfo[i].pcFile, PL_VFS_FILE_MODE_READ);
        gptVfs->close_file(tFileHandle);
    }


    plTileJobData tJobData = {
        .ptTerrain = ptTerrain,
        .ptTilingInfo = ptTilingInfo
    };

    plJobDesc tJobDesc = {
        .task  = pl__tile_job,
        .pData = &tJobData
    };
    plAtomicCounter* ptCounter = NULL;
    gptJob->dispatch_batch(uSourceCount, 0, tJobDesc, &ptCounter);
    gptJob->wait_for_counter(ptCounter);
    PL_FREE(ptTilingInfo);

    // for(uint32_t i = 0; i < uSourceCount; i++)
    // {
    //     pl_terrain_tile_height_map(ptTerrain, ptTilingInfo[i]);
    // }

    pl_unload_json(&ptRootJsonObject);
    PL_FREE(pucBuffer);

    return ptTerrain;
}

void
pl_terrain_render(plTerrain* ptTerrain, plCommandBuffer* ptCmdBuffer)
{
    gptCamera->update(&ptTerrain->tCamera);
    pl__terrain_prepare(ptTerrain, ptCmdBuffer);

    const plMat4 tMVP = pl_mul_mat4(&ptTerrain->tCamera.tProjMat, &ptTerrain->tCamera.tViewMat);
    plVec3 tPos = ptTerrain->tCamera.tPos;

    plRenderEncoder* ptEncoder = gptGfx->begin_render_pass(ptCmdBuffer, ptTerrain->tRenderPass, NULL);

    // check how many tile boundaries we've cross (needed for streaming new tiles)
    float fAddressOriginX = (float)ptTerrain->uTileSize * floorf(ptTerrain->fUnitConversion * tPos.x / ((float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel)) / (float)ptTerrain->uHeightMapResolution;
    float fAddressOriginY = (float)ptTerrain->uTileSize * floorf(ptTerrain->fUnitConversion * tPos.z / ((float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel)) / (float)ptTerrain->uHeightMapResolution;

    uint32_t uDirectionUpdateTileCount = 0;
    if(ptTerrain->tCurrentDirectionUpdate != PL_TERRAIN_DIRECTION_ALL) // first load or teleport
    {
        if(ptTerrain->tCurrentExtent.x < fAddressOriginX)
        {
            ptTerrain->tCurrentDirectionUpdate = PL_TERRAIN_DIRECTION_EAST;
            uDirectionUpdateTileCount = (uint32_t)fabsf((float)ptTerrain->uHeightMapResolution * (fAddressOriginX - ptTerrain->tCurrentExtent.x) / (float)ptTerrain->uTileSize);
            ptTerrain->tCurrentExtent.x = fAddressOriginX;
        }
        else if(ptTerrain->tCurrentExtent.x > fAddressOriginX)
        {
            ptTerrain->tCurrentDirectionUpdate = PL_TERRAIN_DIRECTION_WEST;
            uDirectionUpdateTileCount = (uint32_t)fabsf((float)ptTerrain->uHeightMapResolution * (fAddressOriginX - ptTerrain->tCurrentExtent.x) / (float)ptTerrain->uTileSize);
            ptTerrain->tCurrentExtent.x = fAddressOriginX;
        }
        else if(ptTerrain->tCurrentExtent.y > fAddressOriginY)
        {
            ptTerrain->tCurrentDirectionUpdate = PL_TERRAIN_DIRECTION_NORTH;
            uDirectionUpdateTileCount = (uint32_t)fabsf((float)ptTerrain->uHeightMapResolution * (fAddressOriginY - ptTerrain->tCurrentExtent.y) / (float)ptTerrain->uTileSize);
            ptTerrain->tCurrentExtent.y = fAddressOriginY;
        }
        else if(ptTerrain->tCurrentExtent.y < fAddressOriginY)
        {
            ptTerrain->tCurrentDirectionUpdate = PL_TERRAIN_DIRECTION_SOUTH;
            uDirectionUpdateTileCount = (uint32_t)fabsf((float)ptTerrain->uHeightMapResolution * (fAddressOriginY - ptTerrain->tCurrentExtent.y) / (float)ptTerrain->uTileSize);
            ptTerrain->tCurrentExtent.y = fAddressOriginY;
        }
        if(uDirectionUpdateTileCount > 1)
        {
            for(uint32_t i = 0; i < ptTerrain->uTileCount; i++)
            {
                plHeightMapTile* ptTile = &ptTerrain->atTiles[i];
                ptTile->tFlags &= PL_TERRAIN_TILE_FLAGS_NONE;
            }
            ptTerrain->uNewActiveTileCount = 0;
            ptTerrain->uNewPrefetchTileCount = 0;


            pl_sb_resize(ptTerrain->sbuFreeChunks, ptTerrain->uChunkCapacity);
            for(uint32_t i = 0; i < ptTerrain->uChunkCapacity; i++)
            {
                ptTerrain->sbuFreeChunks[i] = i;
            }
            ptTerrain->tCurrentDirectionUpdate = PL_TERRAIN_DIRECTION_ALL;
        }
    }
    else
    {
        ptTerrain->tCurrentExtent.x = fAddressOriginX;
        ptTerrain->tCurrentExtent.y = fAddressOriginY;
    }

    plDevice* ptDevice = gptCtx->ptDevice;

    gptGfx->bind_shader(ptEncoder, ptTerrain->tFlags & PL_TERRAIN_FLAGS_WIREFRAME ? ptTerrain->tWireframeShader : ptTerrain->tRegularShader);
    gptGfx->bind_vertex_buffer(ptEncoder, gptCtx->tVertexBuffer);

    plDynamicBinding tDynamicBinding = pl_allocate_dynamic_data(gptGfx, ptDevice, &gptCtx->tCurrentDynamicDataBlock);
    plTerrainDynamicData* ptDynamicData = (plTerrainDynamicData*)tDynamicBinding.pcData;
    ptDynamicData->tPos.xyz                   = tPos;
    ptDynamicData->tCameraViewProjection      = tMVP;
    ptDynamicData->fMetersPerHeightFieldTexel = ptTerrain->fMetersPerTexel;
    ptDynamicData->fScale                     = 1.0f;
    ptDynamicData->fGlobalMaxHeight           = ptTerrain->fMaxElevation;
    ptDynamicData->fGlobalMinHeight           = ptTerrain->fMinElevation;
    ptDynamicData->fUnitConversion            = ptTerrain->fUnitConversion;
    ptDynamicData->tSunDirection.xyz          = ptTerrain->tSunDirection;
    ptDynamicData->fXUVOffset                 = ptTerrain->tCurrentExtent.x;
    ptDynamicData->fYUVOffset                 = ptTerrain->tCurrentExtent.y;

    gptGfx->bind_graphics_bind_groups(ptEncoder, ptTerrain->tRegularShader, 0, 1, &ptTerrain->atBindGroup0[gptGfx->get_current_frame_index()], 1, &tDynamicBinding);

    plDrawIndex tDraw = {
        .uInstanceCount = 1,
        .uIndexCount    = gptCtx->uIndexCount,
        .tIndexBuffer   = gptCtx->tIndexBuffer,
    };

    gptGfx->draw_indexed(ptEncoder, 1, &tDraw);

    if(ptTerrain->tFlags & PL_TERRAIN_FLAGS_SHOW_ORIGIN)
    {
        plMat4 tOrigin = pl_identity_mat4();
        gptDraw->add_3d_transform(ptTerrain->pt3dDrawlist, &tOrigin, 1000.0f, (plDrawLineOptions){.fThickness = 0.1f});
    }

    if(ptTerrain->tFlags & PL_TERRAIN_FLAGS_SHOW_BOUNDARY)
    {
        plVec3 tOriginOrigin = {
            0.5f * (ptTerrain->tMaxWorldPosition.x + ptTerrain->tMinWorldPosition.x) / ptTerrain->fUnitConversion,
            0.5f * (ptTerrain->fMaxElevation + ptTerrain->fMinElevation) / ptTerrain->fUnitConversion,
            0.5f * (ptTerrain->tMaxWorldPosition.y + ptTerrain->tMinWorldPosition.y) / ptTerrain->fUnitConversion
        };

        plVec3 tOrigin = tOriginOrigin;
        tOrigin.z = ptTerrain->tMaxWorldPosition.y / ptTerrain->fUnitConversion;
        gptDraw->add_3d_plane_xy_filled(ptTerrain->pt3dDrawlist, tOrigin, 
            (ptTerrain->tMaxWorldPosition.x - ptTerrain->tMinWorldPosition.x) / ptTerrain->fUnitConversion,
            (ptTerrain->fMaxElevation - ptTerrain->fMinElevation) / ptTerrain->fUnitConversion,
            (plDrawSolidOptions){.uColor = PL_COLOR_32_RGBA(0.5f, 0.0f, 0.0f, 0.95f)}
        );

        tOrigin.z = ptTerrain->tMinWorldPosition.y / ptTerrain->fUnitConversion;
        gptDraw->add_3d_plane_xy_filled(ptTerrain->pt3dDrawlist, tOrigin, 
            (ptTerrain->tMaxWorldPosition.x - ptTerrain->tMinWorldPosition.x) / ptTerrain->fUnitConversion,
            (ptTerrain->fMaxElevation - ptTerrain->fMinElevation) / ptTerrain->fUnitConversion,
            (plDrawSolidOptions){.uColor = PL_COLOR_32_RGBA(0.5f, 0.0f, 0.0f, 0.95f)}
        );

        tOrigin = tOriginOrigin;
        tOrigin.x = ptTerrain->tMaxWorldPosition.x / ptTerrain->fUnitConversion;
        gptDraw->add_3d_plane_yz_filled(ptTerrain->pt3dDrawlist, tOrigin, 
            (ptTerrain->tMaxWorldPosition.y - ptTerrain->tMinWorldPosition.y) / ptTerrain->fUnitConversion,
            (ptTerrain->fMaxElevation - ptTerrain->fMinElevation) / ptTerrain->fUnitConversion,            
            (plDrawSolidOptions){.uColor = PL_COLOR_32_RGBA(0.5f, 0.0f, 0.0f, 0.95f)}
        );

        tOrigin.x = ptTerrain->tMinWorldPosition.x / ptTerrain->fUnitConversion;
        gptDraw->add_3d_plane_yz_filled(ptTerrain->pt3dDrawlist, tOrigin, 
            (ptTerrain->tMaxWorldPosition.y - ptTerrain->tMinWorldPosition.y) / ptTerrain->fUnitConversion,
            (ptTerrain->fMaxElevation - ptTerrain->fMinElevation) / ptTerrain->fUnitConversion,   
            (plDrawSolidOptions){.uColor = PL_COLOR_32_RGBA(0.5f, 0.0f, 0.0f, 0.95f)}
        );
    }

    if(ptTerrain->tFlags & PL_TERRAIN_FLAGS_SHOW_GRID)
    {
        plVec3 tP0 = {ptTerrain->tMinWorldPosition.x / ptTerrain->fUnitConversion, ptTerrain->fMaxElevation / ptTerrain->fUnitConversion, ptTerrain->tMinWorldPosition.y / ptTerrain->fUnitConversion};
        plVec3 tP1 = {ptTerrain->tMaxWorldPosition.x / ptTerrain->fUnitConversion, ptTerrain->fMaxElevation / ptTerrain->fUnitConversion, ptTerrain->tMinWorldPosition.y / ptTerrain->fUnitConversion};
        const float fInc = (float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel / ptTerrain->fUnitConversion;
        for(uint32_t i = 0; i < ptTerrain->uHorizontalTiles; i++)
        {
            tP0.z += fInc;
            tP1.z += fInc;
            gptDraw->add_3d_line(ptTerrain->pt3dDrawlist, tP0, tP1,   
                (plDrawLineOptions){.uColor = PL_COLOR_32_GREEN, .fThickness = 0.1f});
        }

        tP0 = (plVec3){ptTerrain->tMinWorldPosition.x / ptTerrain->fUnitConversion, ptTerrain->fMaxElevation / ptTerrain->fUnitConversion, ptTerrain->tMinWorldPosition.y / ptTerrain->fUnitConversion};
        tP1 = (plVec3){ptTerrain->tMinWorldPosition.x / ptTerrain->fUnitConversion, ptTerrain->fMaxElevation / ptTerrain->fUnitConversion, ptTerrain->tMaxWorldPosition.y / ptTerrain->fUnitConversion};
        for(uint32_t i = 0; i < ptTerrain->uHorizontalTiles; i++)
        {
            tP0.x += fInc;
            tP1.x += fInc;
            gptDraw->add_3d_line(ptTerrain->pt3dDrawlist, tP0, tP1,   
                (plDrawLineOptions){.uColor = PL_COLOR_32_GREEN, .fThickness = 0.1f});
        }
    }

    static uint32_t uSelectedTile = UINT32_MAX;
    if(gptUI->begin_window("Terrain", NULL, 0))
    {
        

        if(gptUI->input_text_hint("Tiles", "Filter (inc,-exc)", gptCtx->tFilter.acInputBuffer, 256, 0))
        {
            gptUI->text_filter_build(&gptCtx->tFilter);
        }

        if(gptUI->begin_child("Tile List", 0, 0))
        {

            if(gptUI->text_filter_active(&gptCtx->tFilter))
            {
                for(uint32_t i = 0; i < ptTerrain->uTileCount; i++)
                {
                    bool bSelected = uSelectedTile == i;
                    if(gptUI->selectable(ptTerrain->atTiles[i].acFile, &bSelected, 0))
                    {
                        if(bSelected)
                            uSelectedTile = i;
                        else
                            uSelectedTile = UINT32_MAX;
                    }
                }
            }
            else
            {
                plUiClipper tClipper = {ptTerrain->uTileCount};
                while(gptUI->step_clipper(&tClipper))
                {
                    for(uint32_t i = tClipper.uDisplayStart; i < tClipper.uDisplayEnd; i++)
                    {
                        bool bSelected = uSelectedTile == i;
                        if(gptUI->selectable(ptTerrain->atTiles[i].acFile, &bSelected, 0))
                        {
                            if(bSelected)
                                uSelectedTile = i;
                            else
                                uSelectedTile = UINT32_MAX;
                        }
                    }
                }
            }
            gptUI->end_child();
        }

        gptUI->end_window();
    }

    if(uSelectedTile != UINT32_MAX)
    {
        plHeightMapTile* ptTile = &ptTerrain->atTiles[uSelectedTile];
        if(gptUI->begin_window("Selected Tile", NULL, 0))
        {
            
            gptUI->text("File: %s", ptTile->acFile);
            gptUI->text("Source Texture Offset: %u, %u", ptTile->uXOffset, ptTile->uYOffset);
            gptUI->text("Dest Texture Offset: %u, %u", ptTile->_uXOffsetActual, ptTile->_uYOffsetActual);
            gptUI->text("Grid Coord: %d, %d", ptTile->_iXCoord, ptTile->_iYCoord);
            gptUI->text("World Pos: %g, 0.0, %g", ptTile->tWorldPos.x, ptTile->tWorldPos.y);
            gptUI->end_window();
        }

        plVec3 tOrigin = {
            (ptTile->tWorldPos.x + (float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel * 0.5f) / ptTerrain->fUnitConversion, 
            ptTerrain->fMaxElevation / ptTerrain->fUnitConversion,
            (ptTile->tWorldPos.y + (float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel * 0.5f) / ptTerrain->fUnitConversion, 
        };

        gptDraw->add_3d_plane_xz_filled(ptTerrain->pt3dDrawlist, tOrigin, 
            (float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel / ptTerrain->fUnitConversion,
            (float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel / ptTerrain->fUnitConversion,   
            (plDrawSolidOptions){.uColor = PL_COLOR_32_RGBA(0.0f, 0.5f, 0.0f, 0.5f)});
    }

    // submit 3d drawlist
    gptDrawBackend->submit_3d_drawlist(ptTerrain->pt3dDrawlist,
        ptEncoder,
        (float)ptTerrain->uOutputWidth,
        (float)ptTerrain->uOutputHeight,
        &tMVP,
        PL_DRAW_FLAG_DEPTH_TEST | PL_DRAW_FLAG_DEPTH_WRITE,
        PL_SAMPLE_COUNT_1);


    gptGfx->end_render_pass(ptEncoder);
}



void
pl_terrain_load_mesh(plCommandBuffer* ptCmdBuffer, const char* pcFile, uint32_t uMeshLevels, uint32_t uMeshBaseLodExtentTexels)
{
    plDevice* ptDevice = gptCtx->ptDevice;

    if(gptVfs->does_file_exist(pcFile))
    {
        plVfsFileHandle tFileHandle = gptVfs->open_file(pcFile, PL_VFS_FILE_MODE_READ);
        size_t szFileSize = 0;
        gptVfs->read_file(tFileHandle, NULL, &szFileSize);
        void* pBuffer = PL_ALLOC(szFileSize);
        memset(pBuffer, 0, szFileSize);
        gptVfs->read_file(tFileHandle, pBuffer, &szFileSize);

        char* pcBuffer = (char*)pBuffer;

        gptCtx->uIndexCount = *(uint32_t*)pcBuffer;
        gptCtx->uVertexCount = *(uint32_t*)&pcBuffer[sizeof(uint32_t)];
        gptCtx->puIndexBuffer = (uint32_t*)PL_ALLOC(sizeof(uint32_t) * gptCtx->uIndexCount);
        gptCtx->ptVertexBuffer = (plVec3*)PL_ALLOC(sizeof(plVec3) * gptCtx->uVertexCount);

        memcpy(gptCtx->puIndexBuffer, (uint32_t*)&pcBuffer[sizeof(uint32_t) * 2], sizeof(uint32_t) * gptCtx->uIndexCount);
        memcpy(gptCtx->ptVertexBuffer, &pcBuffer[sizeof(uint32_t) * (gptCtx->uIndexCount + 2)], sizeof(plVec3) * gptCtx->uVertexCount);
        gptVfs->close_file(tFileHandle);
        PL_FREE(pBuffer);
    }
    else
    {
        plMeshBuilderOptions tOptions = {0};
        plMeshBuilder* ptBuilder = gptMeshBuilder->create(tOptions);

        int numMeshLODLevels = (int)uMeshLevels;
        int meshBaseLODExtentHeightfieldTexels = (int)uMeshBaseLodExtentTexels;

        // Store LOD in the Y coordinate, store XZ on the texel grid resolution
        for (int level = 0; level < numMeshLODLevels; ++level)
        {
            printf("Level: %d\n", level);
            const int step = (1 << level);
            const int prevStep = pl_max(0, (1 << (level - 1)));
            const int halfStep = prevStep;

            const int g = meshBaseLODExtentHeightfieldTexels / 2;
            const float L = (float)level;

            // Move up one grid level; used when stitching
            const plVec3 nextLevel = {0, 1, 0};

            // Pad by one element to hide the gap to the next level
            const int pad = 1;
            const int radius = step * (g + pad);
            for (int z = -radius; z < radius; z += step)
            {
                for (int x = -radius; x < radius; x += step)
                {
                    if (pl_max(abs(x + halfStep), abs(z + halfStep)) >= g * prevStep)
                    {
                        // Cleared the cutout from the previous level. Tessellate the
                        // square.

                        //   A-----B-----C
                        //   | \   |   / |
                        //   |   \ | /   |
                        //   D-----E-----F
                        //   |   / | \   |
                        //   | /   |   \ |
                        //   G-----H-----I

                        const plVec3 A = {(float)x, L, (float)z};
                        const plVec3 C = {(float)(x + step), L, A.z};
                        const plVec3 G = {A.x, L, (float)(z + step)};
                        const plVec3 I = {C.x, L, G.z};

                        const plVec3 B = pl_mul_vec3_scalarf(pl_add_vec3(A, C), 0.5f);
                        const plVec3 D = pl_mul_vec3_scalarf(pl_add_vec3(A, G), 0.5f);
                        const plVec3 F = pl_mul_vec3_scalarf(pl_add_vec3(C, I), 0.5f);
                        const plVec3 H = pl_mul_vec3_scalarf(pl_add_vec3(G, I), 0.5f);

                        const plVec3 E = pl_mul_vec3_scalarf(pl_add_vec3(A, I), 0.5f);

                        // Stitch the border into the next level

                        if (x == -radius)
                        {
                            //   A-----B-----C
                            //   | \   |   / |
                            //   |   \ | /   |
                            //   |     E-----F
                            //   |   / | \   |
                            //   | /   |   \ |
                            //   G-----H-----I
                            gptMeshBuilder->add_triangle(ptBuilder, E, A, G);
                        }
                        else
                        {
                            gptMeshBuilder->add_triangle(ptBuilder, E, A, D);
                            gptMeshBuilder->add_triangle(ptBuilder, E, D, G);
                        }

                        if (z == radius - 1)
                        {
                            gptMeshBuilder->add_triangle(ptBuilder, E, G, I);
                        }
                        else
                        {
                            gptMeshBuilder->add_triangle(ptBuilder, E, G, H);
                            gptMeshBuilder->add_triangle(ptBuilder, E, H, I);
                        }

                        if (x == radius - 1)
                        {
                            gptMeshBuilder->add_triangle(ptBuilder, E, I, C);
                        }
                        else
                        {
                            gptMeshBuilder->add_triangle(ptBuilder, E, I, F);
                            gptMeshBuilder->add_triangle(ptBuilder, E, F, C);
                        }

                        if(z == -radius)
                        {
                            gptMeshBuilder->add_triangle(ptBuilder, E, C, A);
                        }
                        else
                        {
                            gptMeshBuilder->add_triangle(ptBuilder, E, C, B);
                            gptMeshBuilder->add_triangle(ptBuilder, E, B, A);
                        }
                    }
                }
            }
        }

        gptMeshBuilder->commit(ptBuilder, NULL, NULL, &gptCtx->uIndexCount, &gptCtx->uVertexCount);
        gptCtx->puIndexBuffer = (uint32_t*)PL_ALLOC(sizeof(uint32_t) * gptCtx->uIndexCount);
        gptCtx->ptVertexBuffer = (plVec3*)PL_ALLOC(sizeof(plVec3) * gptCtx->uVertexCount);
        gptMeshBuilder->commit(ptBuilder, gptCtx->puIndexBuffer, gptCtx->ptVertexBuffer, &gptCtx->uIndexCount, &gptCtx->uVertexCount);
        gptMeshBuilder->cleanup(ptBuilder);

        plVfsFileHandle tFileHandle = gptVfs->open_file(pcFile, PL_VFS_FILE_MODE_WRITE);
        gptVfs->write_file(tFileHandle, &gptCtx->uIndexCount, sizeof(uint32_t));
        gptVfs->write_file(tFileHandle, &gptCtx->uVertexCount, sizeof(uint32_t));
        gptVfs->write_file(tFileHandle, gptCtx->puIndexBuffer, sizeof(uint32_t) * gptCtx->uIndexCount);
        gptVfs->write_file(tFileHandle, gptCtx->ptVertexBuffer, sizeof(plVec3) * gptCtx->uVertexCount);
        gptVfs->close_file(tFileHandle);
    }

    plBufferDesc tVertexBufferDesc = {
        .tUsage      = PL_BUFFER_USAGE_VERTEX,
        .szByteSize  = sizeof(plVec3) * gptCtx->uVertexCount,
        .pcDebugName = "clipmap vertex buffer",
    };

    plBufferDesc tIndexBufferDesc = {
        .tUsage      = PL_BUFFER_USAGE_INDEX,
        .szByteSize  = sizeof(uint32_t) * gptCtx->uIndexCount,
        .pcDebugName = "clipmap index buffer",
    };

    gptCtx->tVertexBuffer = gptGfx->create_buffer(ptDevice, &tVertexBufferDesc, NULL);
    gptCtx->tIndexBuffer = gptGfx->create_buffer(ptDevice, &tIndexBufferDesc, NULL);

    plBuffer* ptIndexBuffer = gptGfx->get_buffer(ptDevice, gptCtx->tIndexBuffer);

    size_t szBuddyBlockSize = gptGpuAllocators->get_buddy_block_size();

    plDeviceMemoryAllocatorI* ptAllocator = gptCtx->tLocalDedicatedAllocator;

    if(ptIndexBuffer->tMemoryRequirements.ulSize >= szBuddyBlockSize)
        ptAllocator = gptCtx->tLocalBuddyAllocator;

    const plDeviceMemoryAllocation tIndexMemory = ptAllocator->allocate(ptAllocator->ptInst,
        ptIndexBuffer->tMemoryRequirements.uMemoryTypeBits,
        ptIndexBuffer->tMemoryRequirements.ulSize,
        ptIndexBuffer->tMemoryRequirements.ulAlignment,
        "clipmap index memory");

    plBuffer* ptVertexBuffer = gptGfx->get_buffer(ptDevice, gptCtx->tVertexBuffer);

    ptAllocator = gptCtx->tLocalDedicatedAllocator;

    if(ptVertexBuffer->tMemoryRequirements.ulSize >= szBuddyBlockSize)
        ptAllocator = gptCtx->tLocalBuddyAllocator;

    const plDeviceMemoryAllocation tVertexMemory = ptAllocator->allocate(ptAllocator->ptInst,
        ptVertexBuffer->tMemoryRequirements.uMemoryTypeBits,
        ptVertexBuffer->tMemoryRequirements.ulSize,
        ptVertexBuffer->tMemoryRequirements.ulAlignment,
        "clipmap vertex memory");

    gptGfx->bind_buffer_to_memory(ptDevice, gptCtx->tIndexBuffer, &tIndexMemory);
    gptGfx->bind_buffer_to_memory(ptDevice, gptCtx->tVertexBuffer, &tVertexMemory);

    size_t szMaxBufferSize = tVertexBufferDesc.szByteSize + tIndexBufferDesc.szByteSize;

    // create staging buffer
    plBufferDesc tStagingBufferDesc = {
        .tUsage      = PL_BUFFER_USAGE_STAGING,
        .szByteSize  = szMaxBufferSize,
        .pcDebugName = "staging buffer"
    };

    plBuffer* ptStagingBuffer = NULL;
    plBufferHandle tStagingBuffer = gptGfx->create_buffer(ptDevice, &tStagingBufferDesc, &ptStagingBuffer);

    // allocate memory for the vertex buffer
    ptAllocator = gptCtx->tStagingDedicatedAllocator;
    const plDeviceMemoryAllocation tStagingBufferAllocation = ptAllocator->allocate(ptAllocator->ptInst,
        ptStagingBuffer->tMemoryRequirements.uMemoryTypeBits,
        ptStagingBuffer->tMemoryRequirements.ulSize,
        0,
        "staging buffer memory");

    // bind the buffer to the new memory allocation
    gptGfx->bind_buffer_to_memory(ptDevice, tStagingBuffer, &tStagingBufferAllocation);

    memcpy(tStagingBufferAllocation.pHostMapped, gptCtx->puIndexBuffer, tIndexBufferDesc.szByteSize);
    PL_FREE(gptCtx->puIndexBuffer);
    gptCtx->puIndexBuffer = NULL;
    memcpy(&tStagingBufferAllocation.pHostMapped[tIndexBufferDesc.szByteSize], gptCtx->ptVertexBuffer, tVertexBufferDesc.szByteSize);
    PL_FREE(gptCtx->ptVertexBuffer);
    gptCtx->ptVertexBuffer = NULL;

    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdBuffer);
    gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE);
    gptGfx->copy_buffer(ptBlit, tStagingBuffer, gptCtx->tIndexBuffer, 0, 0, tIndexBufferDesc.szByteSize);
    gptGfx->copy_buffer(ptBlit, tStagingBuffer, gptCtx->tVertexBuffer, (uint32_t)tIndexBufferDesc.szByteSize, 0, tVertexBufferDesc.szByteSize);
    gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ);
    gptGfx->end_blit_pass(ptBlit);

    gptGfx->queue_buffer_for_deletion(gptCtx->ptDevice, tStagingBuffer);
}

//-----------------------------------------------------------------------------
// [SECTION] internal helpers implementation
//-----------------------------------------------------------------------------

static void
pl__terrain_prepare(plTerrain* ptTerrain, plCommandBuffer* ptCmdBuffer)
{

    plVec3 tPos = ptTerrain->tCamera.tPos;
    plDevice* ptDevice = gptCtx->ptDevice;
    gptCtx->tCurrentDynamicDataBlock = gptGfx->allocate_dynamic_data_block(ptDevice);

    // without blocking, check if pretching is complete
    if(ptTerrain->ptQueuedCounter)
    {
        int64_t iJobsLeft = gptAtomics->atomic_load(ptTerrain->ptQueuedCounter);
        if(iJobsLeft == 0)
        {
            gptJob->wait_for_counter(ptTerrain->ptQueuedCounter);
            ptTerrain->ptQueuedCounter = NULL;
            for(uint32_t i = 0; i < ptTerrain->uNewPrefetchTileCount; i++)
            {
                plHeightMapTile* ptTile = &ptTerrain->atTiles[ptTerrain->auPrefetchTileIndices[i]];
                ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_UPLOADED;
                ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_QUEUED;
            }
            ptTerrain->uNewPrefetchTileCount = 0;
        }
    }

    // determine required tiles
    if(ptTerrain->tCurrentDirectionUpdate != PL_TERRAIN_DIRECTION_NONE)
    {
        if(ptTerrain->ptQueuedCounter)
        {
            gptJob->wait_for_counter(ptTerrain->ptQueuedCounter);
            ptTerrain->ptQueuedCounter = NULL;
            for(uint32_t i = 0; i < ptTerrain->uNewPrefetchTileCount; i++)
            {
                plHeightMapTile* ptTile = &ptTerrain->atTiles[ptTerrain->auPrefetchTileIndices[i]];
                ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_UPLOADED;
                ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_QUEUED;
            }
            ptTerrain->uNewPrefetchTileCount = 0;
        }

        // sets active tiles & decides final tile location
        bool bPendingTextureProcess = pl__terrain_process_height_map_tiles(ptTerrain, ptCmdBuffer, tPos);

        if(bPendingTextureProcess) // load new tiles & process them
        {
            const uint32_t uActiveTileCount = pl_sb_size(ptTerrain->sbuActiveTileIndices);

            // add new tile data to raw texture
            {

                plBuffer* ptStagingBuffer = gptGfx->get_buffer(ptDevice, ptTerrain->tStagingBuffer);

                // size_t szTileFileSize = ptTerrain->uTileSize * ptTerrain->uTileSize * 4 * sizeof(float);

                // find active tiles that require updates (newly visible tiles)
            
                plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdBuffer);
                gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE);

                if(ptTerrain->uNewActiveTileCount > 0)
                {
                    plJobDesc tJobDesc = {
                        .task  = pl__tile_upload_job,
                        .pData = ptTerrain
                    };
                    plAtomicCounter* ptCounter = NULL;
                    gptJob->dispatch_batch(ptTerrain->uNewActiveTileCount, 0, tJobDesc, &ptCounter);
                    gptJob->wait_for_counter(ptCounter);

                    for(uint32_t i = 0; i < ptTerrain->uNewActiveTileCount; i++)
                    {
                        plHeightMapTile* ptTile = &ptTerrain->atTiles[ptTerrain->auFetchTileIndices[i]];
                        ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_UPLOADED;
                        ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_QUEUED;
                    }

                    ptTerrain->uNewActiveTileCount = 0;
            }

                
                for(uint32_t i = 0; i < uActiveTileCount; i++)
                {
                    plHeightMapTile* ptTile = &ptTerrain->atTiles[ptTerrain->sbuActiveTileIndices[i]];
                    if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_PROCESSED)) // otherwise, its already in the texture
                    {
                        // ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_PROCESSED;

                        const plTerrainBufferChunk* ptChunk = &ptTerrain->atChunks[ptTile->_uChunkIndex];
     
                        // copy staging buffer contents to raw texture
                        plBufferImageCopy tBufferImageCopy = {
                            .uImageWidth        = ptTerrain->uTileSize,
                            .uImageHeight       = ptTerrain->uTileSize,
                            .uImageDepth        = 1,
                            .iImageOffsetX      = ptTile->_uXOffsetActual,
                            .iImageOffsetY      = ptTile->_uYOffsetActual,
                            .uLayerCount        = 1,
                            .szBufferOffset     = ptChunk->szOffset,
                            .tCurrentImageUsage = PL_TEXTURE_USAGE_STORAGE,
                        };
                        gptGfx->copy_buffer_to_texture(ptBlit, ptTerrain->tStagingBuffer, ptTerrain->tRawTexture, 1, &tBufferImageCopy);
                    }
                }
                

                gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ);
                gptGfx->end_blit_pass(ptBlit);
                
                // let individual frames know they need to update
                // active texture (but we must preprocess first in the
                // next step)
                for(uint32_t i = 0; i < gptGfx->get_frames_in_flight(); i++)
                    ptTerrain->abPendingTextureUpdate[i] = true;

                // set processed texture usage to starage since it was
                // last used for sampling
                ptBlit = gptGfx->begin_blit_pass(ptCmdBuffer);
                gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_COMPUTE_SHADER, PL_ACCESS_SHADER_READ | PL_ACCESS_SHADER_WRITE, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_READ | PL_ACCESS_TRANSFER_WRITE);
                gptGfx->set_texture_usage(ptBlit, ptTerrain->tProcessedTexture, PL_TEXTURE_USAGE_STORAGE, PL_TEXTURE_USAGE_SAMPLED);
                gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_READ | PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_COMPUTE_SHADER, PL_ACCESS_SHADER_READ | PL_ACCESS_SHADER_WRITE);
                gptGfx->end_blit_pass(ptBlit);
            }

            // height map processing
            {

                // preprocess heightmap (i.e. calculate normals)


                plComputeEncoder* ptComputeEncoder = gptGfx->begin_compute_pass(ptCmdBuffer, NULL);
                gptGfx->bind_compute_shader(ptComputeEncoder, ptTerrain->tPreProcessHeightShader);
                

                for(uint32_t i = 0; i < uActiveTileCount; i++)
                {
                    plHeightMapTile* ptTile = &ptTerrain->atTiles[ptTerrain->sbuActiveTileIndices[i]];
                    if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_PROCESSED))
                    {

                        ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_PROCESSED;

                        plDynamicBinding tDynamicBinding = pl_allocate_dynamic_data(gptGfx, ptDevice, &gptCtx->tCurrentDynamicDataBlock);
                        plTerrainDynamicData* ptDynamicData = (plTerrainDynamicData*)tDynamicBinding.pcData;
                        ptDynamicData->fMetersPerHeightFieldTexel = ptTerrain->fMetersPerTexel;
                        ptDynamicData->fMaxHeight = ptTile->fMaxHeight;
                        ptDynamicData->fMinHeight = ptTile->fMinHeight;
                        ptDynamicData->fGlobalMaxHeight = ptTerrain->fMaxElevation;
                        ptDynamicData->fGlobalMinHeight = ptTerrain->fMinElevation;
                        ptDynamicData->iXOffset = (int)ptTile->_uXOffsetActual;
                        ptDynamicData->iYOffset = (int)ptTile->_uYOffsetActual;
                        ptDynamicData->iTileSize = (int)ptTerrain->uTileSize;

                        gptGfx->bind_compute_bind_groups(ptComputeEncoder, ptTerrain->tPreProcessHeightShader, 0, 1, &ptTerrain->tPreprocessBG0, 1, &tDynamicBinding);

                        plDispatch tDispatch = {
                            .uGroupCountX     = ptTerrain->uTileSize / 8,
                            .uGroupCountY     = ptTerrain->uTileSize / 8,
                            .uGroupCountZ     = 1,
                            .uThreadPerGroupX = 8,
                            .uThreadPerGroupY = 8,
                            .uThreadPerGroupZ = 1,
                        };

                        gptGfx->dispatch(ptComputeEncoder, 1, &tDispatch);
                    }
                }


                gptGfx->end_compute_pass(ptComputeEncoder);

                // set processed texture usage back to sampled
                plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdBuffer);
                gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE);
                gptGfx->set_texture_usage(ptBlit, ptTerrain->tProcessedTexture, PL_TEXTURE_USAGE_SAMPLED, PL_TEXTURE_USAGE_STORAGE);
                gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ);
                gptGfx->end_blit_pass(ptBlit);
            }

            // generate mip maps
            {
                plTexture* ptProcessedTexture = gptGfx->get_texture(ptDevice, ptTerrain->tProcessedTexture);
                for(uint32_t i = 1; i < ptProcessedTexture->tDesc.uMips; i++)
                {
                    plDynamicBinding tDynamicBinding = pl_allocate_dynamic_data(gptGfx, ptDevice, &gptCtx->tCurrentDynamicDataBlock);
                    int* piSourceLevel = (int*)tDynamicBinding.pcData;
                    *piSourceLevel = (int)i - 1;

                    // process single mip level
                    plComputeEncoder* ptComputeEncoder = gptGfx->begin_compute_pass(ptCmdBuffer, NULL);
                    gptGfx->bind_compute_shader(ptComputeEncoder, ptTerrain->tMipMapShader);
                    gptGfx->bind_compute_bind_groups(ptComputeEncoder, ptTerrain->tMipMapShader, 0, 1, &ptTerrain->tMipmapBG0, 1, &tDynamicBinding);
                    plDispatch tDispatch = {
                        .uGroupCountX     = (int)ptTerrain->uHeightMapResolution / (8 * (1 << (int)i)),
                        .uGroupCountY     = (int)ptTerrain->uHeightMapResolution / (8 * (1 << (int)i)),
                        .uGroupCountZ     = 1,
                        .uThreadPerGroupX = 8,
                        .uThreadPerGroupY = 8,
                        .uThreadPerGroupZ = 1,
                    };

                    gptGfx->dispatch(ptComputeEncoder, 1, &tDispatch);
                    gptGfx->end_compute_pass(ptComputeEncoder);

                    // copy processed mip level 
                    plImageCopy tImageCopy = {
                        .uSourceExtentX             = (int)ptTerrain->uHeightMapResolution / (1 << (int)i),
                        .uSourceExtentY             = (int)ptTerrain->uHeightMapResolution / (1 << (int)i),
                        .uSourceExtentZ             = 1,
                        .uSourceMipLevel            = 0,
                        .uSourceBaseArrayLayer      = 0,
                        .uSourceLayerCount          = 1,
                        .tSourceImageUsage          = PL_TEXTURE_USAGE_STORAGE,
                        .uDestinationMipLevel       = i,
                        .uDestinationBaseArrayLayer = 0,
                        .uDestinationLayerCount     = 1,
                        .tDestinationImageUsage     = PL_TEXTURE_USAGE_SAMPLED,
                    };

                    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdBuffer);
                    gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_COMPUTE_SHADER, PL_ACCESS_SHADER_READ | PL_ACCESS_SHADER_WRITE, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_READ | PL_ACCESS_TRANSFER_WRITE);
                    gptGfx->copy_texture(ptBlit, ptTerrain->tDummyTexture, ptTerrain->tProcessedTexture, 1, &tImageCopy);
                    gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_READ | PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_COMPUTE_SHADER, PL_ACCESS_SHADER_READ | PL_ACCESS_SHADER_WRITE);
                    gptGfx->end_blit_pass(ptBlit);
                }
            }
        }
    }

    // update active texture with new tile data
    if(ptTerrain->abPendingTextureUpdate[gptGfx->get_current_frame_index()])
    {

        gptGfx->pipeline_barrier(ptCmdBuffer,
            PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE | PL_ACCESS_TRANSFER_READ,
            PL_PIPELINE_STAGE_TRANSFER,  PL_ACCESS_TRANSFER_WRITE | PL_ACCESS_TRANSFER_READ);

        plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdBuffer);

        plTexture* ptTexture2 = gptGfx->get_texture(ptDevice, ptTerrain->atActiveTexture[gptGfx->get_current_frame_index()]);
        for(uint32_t i = 0; i < ptTexture2->tDesc.uMips; i++)
        {
            plImageCopy tImageCopy = {
                .uSourceExtentX             = (int)ptTerrain->uHeightMapResolution / (1 << (int)i),
                .uSourceExtentY             = (int)ptTerrain->uHeightMapResolution / (1 << (int)i),
                .uSourceExtentZ             = 1,
                .uSourceMipLevel            = i,
                .uSourceBaseArrayLayer      = 0,
                .uSourceLayerCount          = 1,
                .tSourceImageUsage          = PL_TEXTURE_USAGE_SAMPLED,
                .uDestinationMipLevel       = i,
                .uDestinationBaseArrayLayer = 0,
                .uDestinationLayerCount     = 1,
                .tDestinationImageUsage     = PL_TEXTURE_USAGE_SAMPLED,
            };

            gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_COMPUTE_SHADER, PL_ACCESS_SHADER_READ | PL_ACCESS_SHADER_WRITE, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_READ | PL_ACCESS_TRANSFER_WRITE);
            gptGfx->copy_texture(ptBlit, ptTerrain->tProcessedTexture, ptTerrain->atActiveTexture[gptGfx->get_current_frame_index()], 1, &tImageCopy);
            gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_READ | PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_COMPUTE_SHADER, PL_ACCESS_SHADER_READ | PL_ACCESS_SHADER_WRITE);
            
        }
        gptGfx->end_blit_pass(ptBlit);

        ptTerrain->abPendingTextureUpdate[gptGfx->get_current_frame_index()] = false;
    }

    gptGfx->pipeline_barrier(ptCmdBuffer,
        PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER,
        PL_ACCESS_SHADER_WRITE | PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_WRITE | PL_ACCESS_TRANSFER_READ,
        PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_FRAGMENT_SHADER,
        PL_ACCESS_SHADER_READ
    );
}

static void
pl__terrain_create_shaders(plTerrain* ptTerrain)
{
    plShaderDesc tTerrainShaderDesc = {
        .tVertexShader     = gptShader->load_glsl("terrain.vert", "main", NULL, NULL),
        .tPixelShader      = gptShader->load_glsl("terrain.frag", "main", NULL, NULL),
        .tRenderPassLayout = gptCtx->tRenderPassLayout,
        .tMSAASampleCount  = PL_SAMPLE_COUNT_1,
        .tGraphicsState = {
            .ulDepthWriteEnabled  = 1,
            .ulDepthMode          = PL_COMPARE_MODE_LESS,
            .ulCullMode           = PL_CULL_MODE_NONE,
            .ulWireframe          = 0,
            .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
            .ulStencilRef         = 0xff,
            .ulStencilMask        = 0xff,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP,
        },
        .atVertexBufferLayouts = {
            {
                .uByteStride = sizeof(float) * 3,
                .atAttributes = {
                    {.tFormat = PL_VERTEX_FORMAT_FLOAT3 }
                }
            }
        },
        .atBlendStates = {
            {.bBlendEnabled = false, .uColorWriteMask = PL_COLOR_WRITE_MASK_ALL}
        },
        .atBindGroupLayouts = {
            {
                .atSamplerBindings = {
                    {.uSlot = 0, .tStages = PL_SHADER_STAGE_VERTEX | PL_SHADER_STAGE_FRAGMENT },
                    {.uSlot = 4, .tStages = PL_SHADER_STAGE_VERTEX | PL_SHADER_STAGE_FRAGMENT },
                },
                .atTextureBindings = {
                    {.uSlot = 1, .tStages = PL_SHADER_STAGE_VERTEX | PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED },
                    {.uSlot = 2, .tStages = PL_SHADER_STAGE_VERTEX | PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED },
                    {.uSlot = 3, .tStages = PL_SHADER_STAGE_VERTEX | PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED },
                }
            }
        }
    };

    ptTerrain->tRegularShader = gptGfx->create_shader(gptCtx->ptDevice, &tTerrainShaderDesc);
    tTerrainShaderDesc.tGraphicsState.ulWireframe = 1;
    ptTerrain->tWireframeShader = gptGfx->create_shader(gptCtx->ptDevice, &tTerrainShaderDesc);

    const plComputeShaderDesc tPreProcessHeightShaderDesc = {
        .tShader = gptShader->load_glsl("heightfield.comp", "main", NULL, NULL),
        .atBindGroupLayouts = {
            {
                .atTextureBindings = {
                    {.uSlot = 0, .tStages = PL_SHADER_STAGE_COMPUTE, .tType = PL_TEXTURE_BINDING_TYPE_STORAGE},
                    {.uSlot = 1, .tStages = PL_SHADER_STAGE_COMPUTE, .tType = PL_TEXTURE_BINDING_TYPE_STORAGE},
                }
            }
        }
    };
    
    const plComputeShaderDesc tMipMapShaderDesc = {
        .tShader = gptShader->load_glsl("mipmap.comp", "main", NULL, NULL),
        .atBindGroupLayouts = {
            {
                .atSamplerBindings = {
                    {.uSlot = 0, .tStages = PL_SHADER_STAGE_COMPUTE}
                },
                .atTextureBindings = {
                    {.uSlot = 1, .tStages = PL_SHADER_STAGE_COMPUTE, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED},
                    {.uSlot = 2, .tStages = PL_SHADER_STAGE_COMPUTE, .tType = PL_TEXTURE_BINDING_TYPE_STORAGE},
                }
            }
        }
    };

    ptTerrain->tPreProcessHeightShader = gptGfx->create_compute_shader(gptCtx->ptDevice, &tPreProcessHeightShaderDesc);
    ptTerrain->tMipMapShader = gptGfx->create_compute_shader(gptCtx->ptDevice, &tMipMapShaderDesc);
}

static plTextureHandle
pl__terrain_load_texture(plCommandBuffer* ptCmdBuffer, const char* pcFile)
{
    plDevice* ptDevice = gptCtx->ptDevice;

    size_t szImageFileSize = gptVfs->get_file_size_str(pcFile);
    plVfsFileHandle tSpriteSheet = gptVfs->open_file(pcFile, PL_VFS_FILE_MODE_READ);
    gptVfs->read_file(tSpriteSheet, NULL, &szImageFileSize);
    unsigned char* pucBuffer = (unsigned char*)PL_ALLOC(szImageFileSize);
    gptVfs->read_file(tSpriteSheet, pucBuffer, &szImageFileSize);
    gptVfs->close_file(tSpriteSheet);

    // load actual data from file data
    int iImageWidth = 0;
    int iImageHeight = 0;
    int _unused;
    unsigned char* pucImageData = gptImage->load(pucBuffer, (int)szImageFileSize, &iImageWidth, &iImageHeight, &_unused, 4);
    PL_FREE(pucBuffer);

    // create texture
    plTextureDesc tTextureDesc = {
        .tDimensions = { (float)iImageWidth, (float)iImageHeight, 1},
        .tFormat     = PL_FORMAT_R8G8B8A8_UNORM,
        .uLayers     = 1,
        .uMips       = 0,
        .tType       = PL_TEXTURE_TYPE_2D,
        .tUsage      = PL_TEXTURE_USAGE_SAMPLED,
        .pcDebugName = "noise texture",
    };

    plTextureHandle tTexture = gptGfx->create_texture(ptDevice, &tTextureDesc, NULL);
    
    // retrieve new texture (also could have used out param from create_texture above)
    plTexture* ptTexture = gptGfx->get_texture(ptDevice, tTexture);

    size_t szBuddyBlockSize = gptGpuAllocators->get_buddy_block_size();

    plDeviceMemoryAllocatorI* ptAllocator = gptCtx->tLocalDedicatedAllocator;

    if(ptTexture->tMemoryRequirements.ulSize >= szBuddyBlockSize)
        ptAllocator = gptCtx->tLocalBuddyAllocator;

    // allocate memory
    const plDeviceMemoryAllocation tRawTextureAllocation = ptAllocator->allocate(ptAllocator->ptInst,
        ptTexture->tMemoryRequirements.uMemoryTypeBits,
        ptTexture->tMemoryRequirements.ulSize,
        ptTexture->tMemoryRequirements.ulAlignment,
        "noise texture memory");

    // bind memory
    gptGfx->bind_texture_to_memory(ptDevice, tTexture, &tRawTextureAllocation);

    // create staging buffer
    plBufferDesc tStagingBufferDesc = {
        .tUsage      = PL_BUFFER_USAGE_STAGING,
        .szByteSize  = iImageWidth * iImageHeight * 4,
        .pcDebugName = "staging buffer"
    };

    plBuffer* ptStagingBuffer = NULL;
    plBufferHandle tStagingBuffer = gptGfx->create_buffer(ptDevice, &tStagingBufferDesc, &ptStagingBuffer);

    // allocate memory for the vertex buffer
    ptAllocator = gptCtx->tStagingDedicatedAllocator;
    const plDeviceMemoryAllocation tStagingBufferAllocation = ptAllocator->allocate(ptAllocator->ptInst,
        ptStagingBuffer->tMemoryRequirements.uMemoryTypeBits,
        ptStagingBuffer->tMemoryRequirements.ulSize,
        0,
        "staging buffer memory");

    // bind the buffer to the new memory allocation
    gptGfx->bind_buffer_to_memory(ptDevice, tStagingBuffer, &tStagingBufferAllocation);

    // set the initial texture usage (this is a no-op in metal but does layout transition for vulkan)
    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdBuffer);
    gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE);
    gptGfx->set_texture_usage(ptBlit, tTexture, PL_TEXTURE_USAGE_SAMPLED, 0);

    // copy memory to mapped staging buffer
    memcpy(ptStagingBuffer->tMemoryAllocation.pHostMapped, pucImageData, iImageWidth * iImageHeight * 4);
    gptImage->free(pucImageData);

    plBufferImageCopy tBufferImageCopy = {
        .uImageWidth        = (uint32_t)iImageWidth,
        .uImageHeight       = (uint32_t)iImageHeight,
        .uImageDepth        = 1,
        .uLayerCount        = 1,
        .szBufferOffset     = 0,
        .tCurrentImageUsage = PL_TEXTURE_USAGE_SAMPLED,
    };

    gptGfx->copy_buffer_to_texture(ptBlit, tStagingBuffer, tTexture, 1, &tBufferImageCopy);
    gptGfx->generate_mipmaps(ptBlit, tTexture);
    gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_COMPUTE_SHADER | PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ);
    gptGfx->end_blit_pass(ptBlit);
    gptGfx->queue_buffer_for_deletion(ptDevice, tStagingBuffer);
    return tTexture;
}

static plTextureHandle
pl__terrain_create_texture(plCommandBuffer* ptCmdBuffer, const plTextureDesc* ptDesc, const char* pcName, plTextureUsage tInitialUsage)
{
    // for convience
   plDevice* ptDevice = gptCtx->ptDevice;
 
    // create texture
    plTempAllocator tTempAllocator = {0};
    plTexture* ptTexture = NULL;
    const plTextureHandle tHandle = gptGfx->create_texture(ptDevice, ptDesc, &ptTexture);
    pl_temp_allocator_reset(&tTempAllocator);

    // choose allocator
    plDeviceMemoryAllocatorI* ptAllocator = gptCtx->tLocalBuddyAllocator;
    if(ptTexture->tMemoryRequirements.ulSize > gptGpuAllocators->get_buddy_block_size())
        ptAllocator = gptCtx->tLocalDedicatedAllocator;

    // allocate memory
    const plDeviceMemoryAllocation tAllocation = ptAllocator->allocate(ptAllocator->ptInst, 
        ptTexture->tMemoryRequirements.uMemoryTypeBits,
        ptTexture->tMemoryRequirements.ulSize,
        ptTexture->tMemoryRequirements.ulAlignment,
        pl_temp_allocator_sprintf(&tTempAllocator, "texture alloc %s", pcName));

    // bind memory
    gptGfx->bind_texture_to_memory(ptDevice, tHandle, &tAllocation);
    pl_temp_allocator_free(&tTempAllocator);


    // set the initial texture usage (this is a no-op in metal but does layout transition for vulkan)
    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdBuffer);
    gptGfx->set_texture_usage(ptBlit, tHandle, tInitialUsage, 0);
    gptGfx->end_blit_pass(ptBlit);
    return tHandle;
}

static uint32_t
pl__terrain_tile_activation_distance(plTerrain* ptTerrain, uint32_t uIndex)
{
    plHeightMapTile* ptTile = &ptTerrain->atTiles[uIndex];

    bool bWithinX = ptTile->_iXCoord >= ptTerrain->iCurrentXCoordMin && ptTile->_iXCoord <= ptTerrain->iCurrentXCoordMax;
    bool bWithinY = ptTile->_iYCoord >= ptTerrain->iCurrentYCoordMin && ptTile->_iYCoord <= ptTerrain->iCurrentYCoordMax;

    if(bWithinX && bWithinY)
        return 0;

    uint32_t uMinDistanceX = 0;
    if(ptTile->_iXCoord > ptTerrain->iCurrentXCoordMax)
        uMinDistanceX = ptTile->_iXCoord - ptTerrain->iCurrentXCoordMax;
    else if(ptTile->_iXCoord < ptTerrain->iCurrentXCoordMin)
        uMinDistanceX = ptTerrain->iCurrentXCoordMin - ptTile->_iXCoord;

    uint32_t uMinDistanceY = 0;
    if(ptTile->_iYCoord > ptTerrain->iCurrentYCoordMax)
        uMinDistanceY = ptTile->_iYCoord - ptTerrain->iCurrentYCoordMax;
    else if(ptTile->_iYCoord < ptTerrain->iCurrentYCoordMin)
        uMinDistanceY = ptTerrain->iCurrentYCoordMin - ptTile->_iYCoord;

    return pl_maxu(uMinDistanceX, uMinDistanceY);
}

static void
pl__terrain_return_free_chunk(plTerrain* ptTerrain, uint32_t uOwnerTileIndex)
{
    plHeightMapTile* ptTile = &ptTerrain->atTiles[uOwnerTileIndex];
    uint32_t uChunkIndex = ptTile->_uChunkIndex;

    ptTile->tFlags = PL_TERRAIN_TILE_FLAGS_NONE;
    ptTile->_uChunkIndex = UINT32_MAX;
    ptTerrain->atChunks[uChunkIndex].uOwnerTileIndex = UINT32_MAX;
    pl_sb_push(ptTerrain->sbuFreeChunks, uChunkIndex);
}

static void
pl__terrain_clear_cache(plTerrain* ptTerrain, uint32_t uCount, uint32_t uRadius)
{
    for(uint32_t i = 0; i < ptTerrain->uChunkCapacity; i++)
    {
        uint32_t uTileIndex = ptTerrain->atChunks[i].uOwnerTileIndex;
        if(uTileIndex != UINT32_MAX)
        {
            if(!(ptTerrain->atTiles[uTileIndex].tFlags & PL_TERRAIN_TILE_FLAGS_ACTIVE))
            {
                if(!(ptTerrain->atTiles[uTileIndex].tFlags & PL_TERRAIN_TILE_FLAGS_QUEUED))
                {
                    uint32_t uActivationDistance = pl__terrain_tile_activation_distance(ptTerrain, uTileIndex) + 1;
                    if(uActivationDistance >= uRadius)
                    {
                        pl__terrain_return_free_chunk(ptTerrain, uTileIndex);
                        uCount--;
                        if(uCount == 0)
                            return;
                    }
                }
            }
        }
    }
}

static void
pl__terrain_get_free_chunk(plTerrain* ptTerrain, uint32_t uOwnerTileIndex)
{
    if(pl_sb_size(ptTerrain->sbuFreeChunks) > 0)
    {
        uint32_t uChunkIndex = pl_sb_pop(ptTerrain->sbuFreeChunks);
        ptTerrain->atChunks[uChunkIndex].uOwnerTileIndex = uOwnerTileIndex;
        ptTerrain->atTiles[uOwnerTileIndex]._uChunkIndex = uChunkIndex;
        return;
    }

    pl__terrain_clear_cache(ptTerrain, UINT32_MAX, ptTerrain->uPrefetchRadius);

    if(pl_sb_size(ptTerrain->sbuFreeChunks) > 0)
    {
        uint32_t uChunkIndex = pl_sb_pop(ptTerrain->sbuFreeChunks);
        ptTerrain->atChunks[uChunkIndex].uOwnerTileIndex = uOwnerTileIndex;
        ptTerrain->atTiles[uOwnerTileIndex]._uChunkIndex = uChunkIndex;
        return;
    }
    
    ptTerrain->atTiles[uOwnerTileIndex]._uChunkIndex = UINT32_MAX;
}

static bool
pl__terrain_process_height_map_tiles(plTerrain* ptTerrain, plCommandBuffer* ptCmdBuffer, plVec3 tPos)
{
    plDevice* ptDevice = gptCtx->ptDevice;
    
    const uint32_t uTilesAcross = ptTerrain->uHeightMapResolution / ptTerrain->uTileSize;
    bool bTextureNeedUpdate = false;
    
    ptTerrain->uNewActiveTileCount = 0;
    ptTerrain->uNewPrefetchTileCount = 0;

    if(ptTerrain->tCurrentDirectionUpdate == PL_TERRAIN_DIRECTION_EAST)
    {

        ptTerrain->uNewActiveTileCount = 0;
        ptTerrain->iCurrentXCoordMax++;
        ptTerrain->iCurrentXCoordMin++;

        pl_sb_reset(ptTerrain->sbuActiveTileIndices);

        // unmark old tiles
        {
            int x = ptTerrain->iCurrentXCoordMin - 1;
            for(int y = ptTerrain->iCurrentYCoordMin; y <= ptTerrain->iCurrentYCoordMax; y++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {
                    ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_ACTIVE;
                    ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_PROCESSED;
                    if(!(ptTerrain->tFlags & PL_TERRAIN_FLAGS_TILE_STREAMING))
                    {
                        pl__terrain_return_free_chunk(ptTerrain, (uint32_t)pl__terrain_get_tile_index_by_pos(ptTerrain, x, y));
                        ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_UPLOADED;
                    }
                }
            }
        }

        // mark new tiles
        for(int y = ptTerrain->iCurrentYCoordMin; y <= ptTerrain->iCurrentYCoordMax; y++)
        {
            for(int x = ptTerrain->iCurrentXCoordMin; x <= ptTerrain->iCurrentXCoordMax; x++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {
                    int iIndex = pl__terrain_get_tile_index_by_pos(ptTerrain, x, y);
                    if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_ACTIVE)) // not uploaded
                    {
                        bTextureNeedUpdate = true;
                        ptTile->_uXOffsetActual = ptTerrain->uCurrentXOffset * ptTerrain->uTileSize;
                        uint32_t uNewIndex = (ptTile->_iYCoord - ptTerrain->iCurrentYCoordMin) + ptTerrain->uCurrentYOffset;
                        ptTile->_uYOffsetActual = uNewIndex % uTilesAcross;
                        ptTile->_uYOffsetActual *= ptTerrain->uTileSize;
                        
                        if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_UPLOADED))
                        {
                            pl__terrain_get_free_chunk(ptTerrain, (uint32_t)iIndex);
                            ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_QUEUED;
                            ptTerrain->auFetchTileIndices[ptTerrain->uNewActiveTileCount++] = iIndex;
                        }
                    }

                    ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_ACTIVE;
                    pl_sb_push(ptTerrain->sbuActiveTileIndices,  iIndex);
                }
            }
        }
        ptTerrain->uCurrentXOffset = (ptTerrain->uCurrentXOffset + 1) % uTilesAcross;
    }
    else if(ptTerrain->tCurrentDirectionUpdate == PL_TERRAIN_DIRECTION_WEST)
    {
        ptTerrain->uNewActiveTileCount = 0;

        ptTerrain->iCurrentXCoordMax--;
        ptTerrain->iCurrentXCoordMin--;
        
        pl_sb_reset(ptTerrain->sbuActiveTileIndices);
        ptTerrain->uCurrentXOffset = (ptTerrain->uCurrentXOffset - 1) % uTilesAcross;

        // unmark old tiles
        {
            int x = ptTerrain->iCurrentXCoordMax + 1;
            for(int y = ptTerrain->iCurrentYCoordMin; y <= ptTerrain->iCurrentYCoordMax; y++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {
                    ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_ACTIVE;
                    ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_PROCESSED;
                    if(!(ptTerrain->tFlags & PL_TERRAIN_FLAGS_TILE_STREAMING))
                    {
                        pl__terrain_return_free_chunk(ptTerrain, (uint32_t)pl__terrain_get_tile_index_by_pos(ptTerrain, x, y));
                        ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_UPLOADED;
                    }
                }
            }
        }

        // mark new tiles
        for(int y = ptTerrain->iCurrentYCoordMin; y <= ptTerrain->iCurrentYCoordMax; y++)
        {
            for(int x = ptTerrain->iCurrentXCoordMin; x <= ptTerrain->iCurrentXCoordMax; x++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {
                    int iIndex = pl__terrain_get_tile_index_by_pos(ptTerrain, x, y);
                    if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_ACTIVE))
                    {
                        bTextureNeedUpdate = true;
                        
                        if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_UPLOADED))
                        {
                            pl__terrain_get_free_chunk(ptTerrain, (uint32_t)iIndex);
                            ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_QUEUED;
                            ptTerrain->auFetchTileIndices[ptTerrain->uNewActiveTileCount++] = iIndex;
                        }
                        ptTile->_uXOffsetActual = ptTerrain->uCurrentXOffset * ptTerrain->uTileSize;
                        uint32_t uNewIndex = (ptTile->_iYCoord - ptTerrain->iCurrentYCoordMin) + ptTerrain->uCurrentYOffset;
                        ptTile->_uYOffsetActual = uNewIndex % uTilesAcross;
                        ptTile->_uYOffsetActual *= ptTerrain->uTileSize;

                    }
                    ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_ACTIVE;
                    pl_sb_push(ptTerrain->sbuActiveTileIndices,  iIndex);
                }
            }
        }
    }
    else if(ptTerrain->tCurrentDirectionUpdate == PL_TERRAIN_DIRECTION_NORTH)
    {

        ptTerrain->uNewActiveTileCount = 0;

        ptTerrain->iCurrentYCoordMax--;
        ptTerrain->iCurrentYCoordMin--;

        pl_sb_reset(ptTerrain->sbuActiveTileIndices);
        ptTerrain->uCurrentYOffset = (ptTerrain->uCurrentYOffset - 1) % uTilesAcross;
        

        // unmark old tiles
        {
            int y = ptTerrain->iCurrentYCoordMax + 1;
            for(int x = ptTerrain->iCurrentXCoordMin; x <= ptTerrain->iCurrentXCoordMax; x++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {
                    ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_ACTIVE;
                    ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_PROCESSED;
                    if(!(ptTerrain->tFlags & PL_TERRAIN_FLAGS_TILE_STREAMING))
                    {
                        pl__terrain_return_free_chunk(ptTerrain, (uint32_t)pl__terrain_get_tile_index_by_pos(ptTerrain, x, y));
                        ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_UPLOADED;
                    }
                }
            }
        }

        // mark new tiles
        for(int y = ptTerrain->iCurrentYCoordMin; y <= ptTerrain->iCurrentYCoordMax; y++)
        {
            for(int x = ptTerrain->iCurrentXCoordMin; x <= ptTerrain->iCurrentXCoordMax; x++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {
                    int iIndex = pl__terrain_get_tile_index_by_pos(ptTerrain, x, y);
                    if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_ACTIVE))
                    {
                        bTextureNeedUpdate = true;
                        if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_UPLOADED))
                        {
                            pl__terrain_get_free_chunk(ptTerrain, (uint32_t)iIndex);
                            ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_QUEUED;
                            ptTerrain->auFetchTileIndices[ptTerrain->uNewActiveTileCount++] = iIndex;
                        }
                        uint32_t uNewIndex = (ptTile->_iXCoord - ptTerrain->iCurrentXCoordMin) + ptTerrain->uCurrentXOffset;
                        ptTile->_uXOffsetActual = uNewIndex % uTilesAcross;
                        ptTile->_uXOffsetActual *= ptTerrain->uTileSize;
                        ptTile->_uYOffsetActual = ptTerrain->uCurrentYOffset * ptTerrain->uTileSize;

                    }
                    ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_ACTIVE;
                    pl_sb_push(ptTerrain->sbuActiveTileIndices,  iIndex);
                }
            }
        }
    }
    else if(ptTerrain->tCurrentDirectionUpdate == PL_TERRAIN_DIRECTION_SOUTH)
    {

        ptTerrain->uNewActiveTileCount = 0;
        ptTerrain->iCurrentYCoordMax++;
        ptTerrain->iCurrentYCoordMin++;

        pl_sb_reset(ptTerrain->sbuActiveTileIndices);

        // unmark old tiles
        {
            int y = ptTerrain->iCurrentYCoordMin - 1;
            for(int x = ptTerrain->iCurrentXCoordMin; x <= ptTerrain->iCurrentXCoordMax; x++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {
                    ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_ACTIVE;
                    ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_PROCESSED;
                    if(!(ptTerrain->tFlags & PL_TERRAIN_FLAGS_TILE_STREAMING))
                    {
                        pl__terrain_return_free_chunk(ptTerrain, (uint32_t)pl__terrain_get_tile_index_by_pos(ptTerrain, x, y));
                        ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_UPLOADED;
                    }
                }
            }
        }

        // mark new tiles
        for(int y = ptTerrain->iCurrentYCoordMin; y <= ptTerrain->iCurrentYCoordMax; y++)
        {
            for(int x = ptTerrain->iCurrentXCoordMin; x <= ptTerrain->iCurrentXCoordMax; x++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {
                    int iIndex = pl__terrain_get_tile_index_by_pos(ptTerrain, x, y);
                    if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_ACTIVE))
                    {
                        bTextureNeedUpdate = true;
                        if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_UPLOADED))
                        {
                            pl__terrain_get_free_chunk(ptTerrain, (uint32_t)iIndex);
                            ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_QUEUED;
                            ptTerrain->auFetchTileIndices[ptTerrain->uNewActiveTileCount++] = iIndex;
                        }
                        uint32_t uNewIndex = (ptTile->_iXCoord - ptTerrain->iCurrentXCoordMin) + ptTerrain->uCurrentXOffset;
                        ptTile->_uXOffsetActual = uNewIndex % uTilesAcross;
                        ptTile->_uXOffsetActual *= ptTerrain->uTileSize;
                        ptTile->_uYOffsetActual = ptTerrain->uCurrentYOffset * ptTerrain->uTileSize;
                    }
                    ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_ACTIVE;
                    pl_sb_push(ptTerrain->sbuActiveTileIndices,  iIndex);
                }
            }
        }
        ptTerrain->uCurrentYOffset = (ptTerrain->uCurrentYOffset + 1) % uTilesAcross;
    }
    else if(ptTerrain->tCurrentDirectionUpdate == PL_TERRAIN_DIRECTION_ALL)
    {

        float fRadius = (float)(ptTerrain->uHeightMapResolution / 2) * ptTerrain->fMetersPerTexel;
        const int iRadiusInTiles = (int)((ptTerrain->uHeightMapResolution / 2) / ptTerrain->uTileSize);
        plVec2 tCameraPos = {tPos.x * ptTerrain->fUnitConversion, tPos.z * ptTerrain->fUnitConversion};
        float fIncrement = (float)ptTerrain->uTileSize * ptTerrain->fMetersPerTexel;
        tCameraPos.x = roundf(tCameraPos.x * (1.0f / fIncrement)) * fIncrement;
        tCameraPos.y = roundf(tCameraPos.y * (1.0f / fIncrement)) * fIncrement;

        bTextureNeedUpdate = true;
        int iCameraXTile = (int)(((float)tCameraPos.x - ptTerrain->tMinWorldPosition.x) / fIncrement);
        int iCameraYTile = (int)(((float)tCameraPos.y - ptTerrain->tMinWorldPosition.y) / fIncrement);
        ptTerrain->iCurrentXCoordMin = iCameraXTile - iRadiusInTiles;
        ptTerrain->iCurrentXCoordMax = iCameraXTile + iRadiusInTiles - 1;
        ptTerrain->iCurrentYCoordMin = iCameraYTile - iRadiusInTiles;
        ptTerrain->iCurrentYCoordMax = iCameraYTile + iRadiusInTiles -1;
        ptTerrain->uCurrentXOffset = 0;
        ptTerrain->uCurrentYOffset = 0;

        int iCurrentXCoordMin = INT32_MAX;
        int iCurrentYCoordMin = INT32_MAX;

        for(uint32_t i = 0; i < ptTerrain->uNewActiveTileCount; i++)
        {
            plHeightMapTile* ptTile = &ptTerrain->atTiles[ptTerrain->sbuActiveTileIndices[i]];
            ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_ACTIVE;
        }
        pl_sb_reset(ptTerrain->sbuActiveTileIndices);

        for(uint32_t i = 0; i < ptTerrain->uTileCount; i++)
        {
            plHeightMapTile* ptTile = &ptTerrain->atTiles[i];
            ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_ACTIVE;

            float fXDistance = tCameraPos.x - ptTile->tWorldPos.x - 0.1f;
            float fYDistance = tCameraPos.y - ptTile->tWorldPos.y - 0.1f;

            if(fabsf(fXDistance) > fRadius || fabsf(fYDistance) > fRadius)
            {
                ptTile->tFlags &= ~PL_TERRAIN_TILE_FLAGS_ACTIVE;
            }
            else
            {
                pl__terrain_get_free_chunk(ptTerrain, i);
                ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_QUEUED;
                if(ptTile->_iXCoord < iCurrentXCoordMin)
                    iCurrentXCoordMin = ptTile->_iXCoord;
                if(ptTile->_iYCoord < iCurrentYCoordMin)
                    iCurrentYCoordMin = ptTile->_iYCoord;
                
                pl_sb_push(ptTerrain->sbuActiveTileIndices, i);
                ptTerrain->auFetchTileIndices[ptTerrain->uNewActiveTileCount++] = i;
            }
        }

        // sort tiles by y, then x
        uint32_t uActiveTileCount = pl_sb_size(ptTerrain->sbuActiveTileIndices);

        // by x
        bool bSwapped = false;
        for(uint32_t i = 0; i < uActiveTileCount - 1; i++)
        {
            bSwapped = false;
            for(uint32_t j = 0; j < uActiveTileCount - i - 1; j++)
            {
                uint32_t uA = ptTerrain->sbuActiveTileIndices[j];
                uint32_t uB = ptTerrain->sbuActiveTileIndices[j + 1];
                if(ptTerrain->atTiles[uA].tWorldPos.x > ptTerrain->atTiles[uB].tWorldPos.x)
                {
                    ptTerrain->sbuActiveTileIndices[j] = uB;
                    ptTerrain->sbuActiveTileIndices[j + 1] = uA;
                    bSwapped = true;
                }
            }

            if(!bSwapped)
                break;
        }

        // by y
        for(uint32_t i = 0; i < uActiveTileCount - 1; i++)
        {
            bSwapped = false;
            for(uint32_t j = 0; j < uActiveTileCount - i - 1; j++)
            {
                uint32_t uA = ptTerrain->sbuActiveTileIndices[j];
                uint32_t uB = ptTerrain->sbuActiveTileIndices[j + 1];
                if(ptTerrain->atTiles[uA].tWorldPos.y > ptTerrain->atTiles[uB].tWorldPos.y)
                {
                    ptTerrain->sbuActiveTileIndices[j] = uB;
                    ptTerrain->sbuActiveTileIndices[j + 1] = uA;
                    bSwapped = true;
                }
            }

            if(!bSwapped)
                break;
        }

        // place into atlas
        const uint32_t uXDiff = (uint32_t)(iCurrentXCoordMin - ptTerrain->iCurrentXCoordMin) * ptTerrain->uTileSize;
        const uint32_t uYDiff = (uint32_t)(iCurrentYCoordMin - ptTerrain->iCurrentYCoordMin) * ptTerrain->uTileSize;
        uint32_t uXOffset = uXDiff;
        uint32_t uYOffset = uYDiff;
        for(uint32_t i = 0; i < uActiveTileCount; i++)
        {
            uint32_t uIndex = ptTerrain->sbuActiveTileIndices[i];
            plHeightMapTile* ptTile = &ptTerrain->atTiles[uIndex];
            ptTile->_uXOffsetActual = uXOffset;
            ptTile->_uYOffsetActual = uYOffset;
            
            uXOffset = uXOffset + ptTerrain->uTileSize;
            if(uXOffset >= ptTerrain->uHeightMapResolution)
            {
                uXOffset = uXDiff;
                uYOffset += ptTerrain->uTileSize;
                uYOffset = uYOffset % ptTerrain->uHeightMapResolution;
            }
        }
        
    }

    if(ptTerrain->tFlags & PL_TERRAIN_FLAGS_TILE_STREAMING)
    {

        int iLeftX = ptTerrain->iCurrentXCoordMin - (int)ptTerrain->uPrefetchRadius;
        int iTopY = ptTerrain->iCurrentYCoordMin - (int)ptTerrain->uPrefetchRadius;
        int iFullXLength = ptTerrain->iCurrentXCoordMax - ptTerrain->iCurrentXCoordMin + 2 * (int)ptTerrain->uPrefetchRadius;
        int iFullYLength = ptTerrain->iCurrentYCoordMax - ptTerrain->iCurrentYCoordMin + 2 * (int)ptTerrain->uPrefetchRadius;
        int iBottomY = iTopY + iFullYLength;
        int iRightX = iLeftX + iFullXLength;

        for(int y = iTopY; y <= iBottomY; y++)
        {
            // left
            for(int x = iLeftX; x < iLeftX + (int)ptTerrain->uPrefetchRadius; x++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {

                    if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_UPLOADED) && !(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_QUEUED))
                    {
                        int iIndex = pl__terrain_get_tile_index_by_pos(ptTerrain, x, y);
                        ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_QUEUED;
                        pl__terrain_get_free_chunk(ptTerrain, (uint32_t)iIndex);
                        ptTerrain->auPrefetchTileIndices[ptTerrain->uNewPrefetchTileCount++] = iIndex;
                    }

                }
            }

            // right
            for(int x = iRightX - (int)ptTerrain->uPrefetchRadius + 1; x <= iRightX; x++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {

                    if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_UPLOADED) && !(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_QUEUED))
                    {
                        int iIndex = pl__terrain_get_tile_index_by_pos(ptTerrain, x, y);
                        ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_QUEUED;
                        pl__terrain_get_free_chunk(ptTerrain, (uint32_t)iIndex);
                        ptTerrain->auPrefetchTileIndices[ptTerrain->uNewPrefetchTileCount++] = iIndex;
                    }

                }
            }
        }

        for(int x = iLeftX; x <= iLeftX + iFullXLength; x++)
        {
            // top
            for(int y = iTopY; y <= iTopY + (int)ptTerrain->uPrefetchRadius; y++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {

                    if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_UPLOADED) && !(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_QUEUED))
                    {
                        int iIndex = pl__terrain_get_tile_index_by_pos(ptTerrain, x, y);
                        ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_QUEUED;
                        pl__terrain_get_free_chunk(ptTerrain, (uint32_t)iIndex);
                        ptTerrain->auPrefetchTileIndices[ptTerrain->uNewPrefetchTileCount++] = iIndex;
                    }

                }
            }

            // bottom
            for(int y = iBottomY - (int)ptTerrain->uPrefetchRadius + 1; y <= iBottomY; y++)
            {
                plHeightMapTile* ptTile = pl__terrain_get_tile_by_pos(ptTerrain, x, y);
                if(ptTile)
                {

                    if(!(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_UPLOADED) && !(ptTile->tFlags & PL_TERRAIN_TILE_FLAGS_QUEUED))
                    {
                        int iIndex = pl__terrain_get_tile_index_by_pos(ptTerrain, x, y);
                        ptTile->tFlags |= PL_TERRAIN_TILE_FLAGS_QUEUED;
                        pl__terrain_get_free_chunk(ptTerrain, (uint32_t)iIndex);
                        ptTerrain->auPrefetchTileIndices[ptTerrain->uNewPrefetchTileCount++] = iIndex;
                    }

                }
            }
        }
    }

    ptTerrain->tCurrentDirectionUpdate = PL_TERRAIN_DIRECTION_NONE;

    PL_ASSERT(ptTerrain->ptQueuedCounter == NULL);
    if(ptTerrain->uNewPrefetchTileCount > 0)
    {
        pl__terrain_clear_cache(ptTerrain, ptTerrain->uMaxPrefetchedTiles / 2, ptTerrain->uPrefetchRadius * 2);
        plJobDesc tJobDesc = {
            .task = pl__tile_prefetch_job,
            .pData = ptTerrain
        };
        gptJob->dispatch_batch(ptTerrain->uNewPrefetchTileCount, 0, tJobDesc, &ptTerrain->ptQueuedCounter);
    }
    return bTextureNeedUpdate;
}

// void
// pl_terrain_reset_tiles(plTerrain* ptTerrain)
// {
//     ptTerrain->tCurrentDirectionUpdate = PL_TERRAIN_DIRECTION_ALL;
// }

// void
// pl_terrain_clear_cache(plTerrain* ptTerrain)
// {
//     pl__terrain_clear_cache(ptTerrain, UINT32_MAX, 0);
// }

// void
// pl_terrain_set_flags(plTerrain* ptTerrain, plTerrainFlags tFlags)
// {
//     ptTerrain->tFlags = tFlags;
// }

// plTerrainFlags
// pl_terrain_get_flags(plTerrain* ptTerrain)
// {
//     return ptTerrain->tFlags;
// }

void
pl_terrain_reload_shaders(plTerrain* ptTerrain)
{
    plDevice* ptDevice = gptCtx->ptDevice;
    gptGfx->queue_shader_for_deletion(ptDevice, ptTerrain->tRegularShader);
    gptGfx->queue_shader_for_deletion(ptDevice, ptTerrain->tWireframeShader);  
    gptGfx->queue_compute_shader_for_deletion(ptDevice, ptTerrain->tPreProcessHeightShader);  
    gptGfx->queue_compute_shader_for_deletion(ptDevice, ptTerrain->tMipMapShader);
    pl__terrain_create_shaders(ptTerrain);
}

//-----------------------------------------------------------------------------
// [SECTION] extension loading
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_load_ext(plApiRegistryI* ptApiRegistry, bool bReload)
{
    const plTerrainI tApi = {
        .initialize               = pl_terrain_initialize,
        .cleanup                  = pl_terrain_cleanup,
        .load_mesh                = pl_terrain_load_mesh,
        .render                   = pl_terrain_render,
        .tile_height_map          = pl_terrain_tile_height_map,
        .create_terrain           = pl_create_terrain,
        .create_terrain_from_file = pl_create_terrain_from_file,
        .get_terrain_texture      = pl_get_terrain_texture,
        .get_terrain_camera       = pl_get_terrain_camera,
        .set_camera_pos           = pl_terrain_set_camera_pos,
        .set_camera_orientation   = pl_terrain_set_camera_orientation,
        .set_camera_aspect        = pl_terrain_set_camera_aspect,
        .reload_shaders           = pl_terrain_reload_shaders,
    };
    pl_set_api(ptApiRegistry, plTerrainI, &tApi);

    gptMemory        = pl_get_api_latest(ptApiRegistry, plMemoryI);
    gptGfx           = pl_get_api_latest(ptApiRegistry, plGraphicsI);
    gptVfs           = pl_get_api_latest(ptApiRegistry, plVfsI);
    gptMeshBuilder   = pl_get_api_latest(ptApiRegistry, plMeshBuilderI);
    gptShader        = pl_get_api_latest(ptApiRegistry, plShaderI);
    gptImage         = pl_get_api_latest(ptApiRegistry, plImageI);
    gptGpuAllocators = pl_get_api_latest(ptApiRegistry, plGPUAllocatorsI);
    gptFile          = pl_get_api_latest(ptApiRegistry, plFileI);
    gptJob           = pl_get_api_latest(ptApiRegistry, plJobI);
    gptTools         = pl_get_api_latest(ptApiRegistry, plToolsI);
    gptAtomics       = pl_get_api_latest(ptApiRegistry, plAtomicsI);
    gptCamera        = pl_get_api_latest(ptApiRegistry, plCameraI);
    gptDraw          = pl_get_api_latest(ptApiRegistry, plDrawI);
    gptDrawBackend   = pl_get_api_latest(ptApiRegistry, plDrawBackendI);
    gptUI            = pl_get_api_latest(ptApiRegistry, plUiI);

    const plDataRegistryI* ptDataRegistry = pl_get_api_latest(ptApiRegistry, plDataRegistryI);

    if(bReload)
    {
        gptCtx = ptDataRegistry->get_data("plTerrainContext");
    }
    else
    {
        static plTerrainContext tCtx = {0};
        gptCtx = &tCtx;
        ptDataRegistry->set_data("plTerrainContext", gptCtx);
    }
}

PL_EXPORT void
pl_unload_ext(plApiRegistryI* ptApiRegistry, bool bReload)
{

    if(bReload)
        return;

    const plTerrainI* ptApi = pl_get_api_latest(ptApiRegistry, plTerrainI);
    ptApiRegistry->remove_api(ptApi);
}

#define PL_MEMORY_IMPLEMENTATION
#include "pl_memory.h"

#define PL_STRING_IMPLEMENTATION
#include "pl_string.h"

#define PL_JSON_IMPLEMENTATION
#include "pl_json.h"