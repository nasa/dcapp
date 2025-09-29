
//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdlib.h> // malloc, free
#include <stdio.h>  // printf
#include <string.h> // memset

// pilot light
#include "pl.h"
#include "pl_memory.h"
#include "pl_string.h"
#define PL_MATH_INCLUDE_FUNCTIONS
#include "pl_math.h"
#include "../pilotlight/editor/pl_icons.h"
#include "pl_json.h"

// stable extensions
#include "pl_image_ext.h"
#include "pl_profile_ext.h"
#include "pl_log_ext.h"
#include "pl_stats_ext.h"
#include "pl_graphics_ext.h"
#include "pl_tools_ext.h"
#include "pl_job_ext.h"
#include "pl_draw_ext.h"
#include "pl_draw_backend_ext.h"
#include "pl_ui_ext.h"
#include "pl_shader_ext.h"
#include "pl_string_intern_ext.h"
#include "pl_platform_ext.h"
#include "pl_console_ext.h"
#include "pl_screen_log_ext.h"
#include "pl_starter_ext.h"
#include "pl_pak_ext.h"
#include "pl_datetime_ext.h"
#include "pl_config_ext.h"
#include "pl_vfs_ext.h"
#include "pl_compress_ext.h"
#include "pl_image_ext.h"

// unstable extensions
#include "pl_camera_ext.h"
#include "pl_mesh_ext.h"

// our extensions
#include "pl_terrain_ext.h"

// dear imgui
#include "pl_dear_imgui_ext.h"
#include "../pilotlight/dependencies/imgui/imgui.h"
#include "../pilotlight/dependencies/imgui/implot.h"

//-----------------------------------------------------------------------------
// [SECTION] defines
//-----------------------------------------------------------------------------

#define PL_UNIT_CONVERSION 100.0f

//-----------------------------------------------------------------------------
// [SECTION] global apis
//-----------------------------------------------------------------------------

const plWindowI*       gptWindows       = nullptr;
const plStatsI*        gptStats         = nullptr;
const plGraphicsI*     gptGfx           = nullptr;
const plToolsI*        gptTools         = nullptr;
const plJobI*          gptJobs          = nullptr;
const plDrawI*         gptDraw          = nullptr;
const plDrawBackendI*  gptDrawBackend   = nullptr;
const plUiI*           gptUI            = nullptr;
const plIOI*           gptIO            = nullptr;
const plShaderI*       gptShader        = nullptr;
const plMemoryI*       gptMemory        = nullptr;
const plNetworkI*      gptNetwork       = nullptr;
const plStringInternI* gptString        = nullptr;
const plProfileI*      gptProfile       = nullptr;
const plFileI*         gptFile          = nullptr;
const plConsoleI*      gptConsole       = nullptr;
const plScreenLogI*    gptScreenLog     = nullptr;
const plConfigI*       gptConfig        = nullptr;
const plStarterI*      gptStarter       = nullptr;
const plVfsI*          gptVfs           = nullptr;
const plPakI*          gptPak           = nullptr;
const plDateTimeI*     gptDateTime      = nullptr;
const plCompressI*     gptCompress      = nullptr;
const plDearImGuiI*    gptDearImGui     = nullptr;
const plCameraI*       gptCamera        = nullptr;
const plImageI*        gptImage         = nullptr;
const plTerrainI*      gptTerrain       = nullptr;

#define PL_ALLOC(x)      gptMemory->tracked_realloc(nullptr, (x), __FILE__, __LINE__)
#define PL_REALLOC(x, y) gptMemory->tracked_realloc((x), (y), __FILE__, __LINE__)
#define PL_FREE(x)       gptMemory->tracked_realloc((x), 0, __FILE__, __LINE__)

#define PL_DS_ALLOC(x)                      gptMemory->tracked_realloc(nullptr, (x), __FILE__, __LINE__)
#define PL_DS_ALLOC_INDIRECT(x, FILE, LINE) gptMemory->tracked_realloc(nullptr, (x), FILE, LINE)
#define PL_DS_FREE(x)                       gptMemory->tracked_realloc((x), 0, __FILE__, __LINE__)
#include "pl_ds.h"

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plUploadJob
{
    uint32_t i;
    uint32_t j;
    uint32_t uOffset;
} plUploadJob;

typedef struct _plAppData
{
    plWindow* ptWindow;

    // UI options
    bool bShowImGuiDemo;
    bool bShowImPlotDemo;

    plTerrain* ptTerrain;
} plAppData;

//-----------------------------------------------------------------------------
// [SECTION] helpers
//-----------------------------------------------------------------------------

void pl__load_apis(plApiRegistryI*);

//-----------------------------------------------------------------------------
// [SECTION] pl_app_load
//-----------------------------------------------------------------------------

PL_EXPORT void*
pl_app_load(plApiRegistryI* ptApiRegistry, plAppData* ptAppData)
{
    // NOTE: on first load, "pAppData" will be NULL but on reloads
    //       it will be the value returned from this function

    // retrieve the data registry API, this is the API used for sharing data
    // between extensions & the runtime
    const plDataRegistryI* ptDataRegistry = pl_get_api_latest(ptApiRegistry, plDataRegistryI);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~dear imgui context stuff~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Dear ImGui uses globals for it's content & memory allocators. If you are
    // crossing a dll/so boundary, you must set these. The experimental backend
    // provides these to the data registry with the names below, so they must
    // be retrieved and set.

    // retrieve/set imgui context
    ImGuiContext* ptImguiContext = (ImGuiContext*)ptDataRegistry->get_data("imgui");
    ImGui::SetCurrentContext(ptImguiContext);

    // retrieve/set imgui allocator functions
    ImGuiMemAllocFunc p_alloc_func = (ImGuiMemAllocFunc)ptDataRegistry->get_data("imgui allocate");
    ImGuiMemFreeFunc p_free_func = (ImGuiMemFreeFunc)ptDataRegistry->get_data("imgui free");
    ImGui::SetAllocatorFunctions(p_alloc_func, p_free_func, nullptr);

    // if "ptAppData" is a valid pointer, then this function is being called
    // during a hot reload.
    if(ptAppData)
    {
        pl__load_apis(ptApiRegistry);
        gptScreenLog->add_message_ex(0, 15.0, PL_COLOR_32_MAGENTA, 1.5f, "%s", "App Hot Reloaded");
        ImPlot::SetCurrentContext((ImPlotContext*)ptDataRegistry->get_data("implot"));
        return ptAppData;
    }

    // this path is taken only during first load, so we
    // allocate app memory here
    ptAppData = (plAppData*)malloc(sizeof(plAppData));
    memset(ptAppData, 0, sizeof(plAppData));

    // retrieve extension registry
    const plExtensionRegistryI* ptExtensionRegistry = pl_get_api_latest(ptApiRegistry, plExtensionRegistryI);

    // load extensions
    ptExtensionRegistry->add_path("../../pl-terrain/out");
    ptExtensionRegistry->load("pl_unity_ext", NULL, NULL, true);
    ptExtensionRegistry->load("pl_terrain_ext", NULL, NULL, true);
    ptExtensionRegistry->load("pl_platform_ext", NULL, NULL, false);
    ptExtensionRegistry->load("pl_dear_imgui_ext", NULL, NULL, false);
    
    // load required apis
    pl__load_apis(ptApiRegistry);

    gptVfs->mount_directory("/shaders-terrain", "../../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    gptVfs->mount_directory("/assets", "../../data", PL_VFS_MOUNT_FLAGS_NONE);
    gptVfs->mount_directory("/cache", "cache", PL_VFS_MOUNT_FLAGS_NONE);
    gptVfs->mount_directory("/shaders", "../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    gptVfs->mount_directory("/shader-temp", "../shader-temp", PL_VFS_MOUNT_FLAGS_NONE);
    gptFile->create_directory("../shader-temp");
    gptFile->create_directory("cache");


    plJobSystemInit tJobInit = {};
    tJobInit.uThreadCount = 0;
    gptJobs->initialize(tJobInit);

    // create window
    plWindowDesc tWindowDesc = {
        PL_WINDOW_FLAG_NONE,
        "Terrain Sandbox",
        500,
        500,
        200,
        200
    };
    gptWindows->create(tWindowDesc, &ptAppData->ptWindow);
    gptWindows->show(ptAppData->ptWindow);

    // setup starter extension
    plStarterInit tStarterInit = {};
    tStarterInit.tFlags   = PL_STARTER_FLAGS_ALL_EXTENSIONS | PL_STARTER_FLAGS_DEPTH_BUFFER | PL_STARTER_FLAGS_MSAA;
    tStarterInit.ptWindow = ptAppData->ptWindow;
    tStarterInit.tFlags &= ~PL_STARTER_FLAGS_SHADER_EXT;
    gptStarter->initialize(tStarterInit);

    // initialize shader compiler
    static plShaderOptions tDefaultShaderOptions = {};
    tDefaultShaderOptions.apcIncludeDirectories[0] = "/shaders/";
    tDefaultShaderOptions.apcIncludeDirectories[1] = "/shaders-terrain/";
    tDefaultShaderOptions.apcDirectories[0] = "/shaders/";
    tDefaultShaderOptions.apcDirectories[1] = "/shaders-terrain/";
    tDefaultShaderOptions.pcCacheOutputDirectory = "/shader-temp/";
    tDefaultShaderOptions.tFlags = PL_SHADER_FLAGS_AUTO_OUTPUT | PL_SHADER_FLAGS_INCLUDE_DEBUG | PL_SHADER_FLAGS_ALWAYS_COMPILE;
    gptShader->initialize(&tDefaultShaderOptions);

    // wraps up (i.e. builds font atlas)
    gptStarter->finalize();

    // initializes out Dear ImGui backend (similar to the draw backend extension)
    gptDearImGui->initialize(gptStarter->get_device(), gptStarter->get_swapchain(), gptStarter->get_render_pass());

    ImGui::GetIO().ConfigFlags &= ~ImGuiBackendFlags_PlatformHasViewports;

    // same process for implot as imgui
    ImPlot::SetCurrentContext((ImPlotContext*)ptDataRegistry->get_data("implot"));


    gptTerrain->initialize(gptStarter->get_device());
    
    plCommandBuffer* ptCmdBuffer = gptStarter->get_temporary_command_buffer();
    gptTerrain->load_mesh(ptCmdBuffer, "/assets/terrain.bin", 7, 128);



    plTerrainInit tTerrainInit = {};
    tTerrainInit.fUnitConversion = PL_UNIT_CONVERSION;
    tTerrainInit.uHeightMapResolution = 2048 * 2;
    tTerrainInit.uTileSize = 128 * 2;
    tTerrainInit.uPrefetchRadius = 2;
    tTerrainInit.fMetersPerTexel = 5.0f;
    tTerrainInit.tMinPosition = {-40960.0f * tTerrainInit.fMetersPerTexel * 0.5f, -40960.0f * tTerrainInit.fMetersPerTexel * 0.5f};
    tTerrainInit.tMaxPosition = { 40960.0f * tTerrainInit.fMetersPerTexel * 0.5f, 40960.0f * tTerrainInit.fMetersPerTexel * 0.5f};
    tTerrainInit.tFlags = PL_TERRAIN_FLAGS_TILE_STREAMING;
    tTerrainInit.fMaxElevation = 2713.087f;
    tTerrainInit.fMinElevation = -4380.518f;
    tTerrainInit.uOutputWidth = 1000;
    tTerrainInit.uOutputHeight = 1000;
    
    ptAppData->ptTerrain = gptTerrain->create_terrain(ptCmdBuffer, tTerrainInit);

    for(uint32_t i = 0; i < 10; i++)
    {
        for(uint32_t j = 0; j < 10; j++)
        {
            plTerrainTilingInfo tTilingInfo = {};
            tTilingInfo.fMaxHeight = 2713.087f;
            tTilingInfo.fMinHeight = -4380.518f;
            sprintf(tTilingInfo.pcFile, "/assets/moon_%u_%u.png", i, j);
            tTilingInfo.tOrigin = {
                tTerrainInit.tMinPosition.x + (float)i * 4096.0f * tTerrainInit.fMetersPerTexel,
                tTerrainInit.tMinPosition.y + (float)j * 4096.0f * tTerrainInit.fMetersPerTexel
            };
            gptTerrain->tile_height_map(ptAppData->ptTerrain, tTilingInfo);
        }
    }

    gptStarter->submit_temporary_command_buffer(ptCmdBuffer);
    return ptAppData;
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_shutdown
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_shutdown(plAppData* ptAppData)
{
    plDevice* ptDevice = gptStarter->get_device();
    gptGfx->flush_device(ptDevice);

    
    gptTerrain->cleanup();
    gptDearImGui->cleanup();
    gptStarter->cleanup();
    gptWindows->destroy(ptAppData->ptWindow);
    gptJobs->cleanup();
    free(ptAppData);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_resize
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_resize(plWindow* ptWindow, plAppData* ptAppData)
{
    gptStarter->resize();
    plIO* ptIO = gptIO->get_io();
    // gptCamera->set_aspect(&ptAppData->tCamera, ptIO->tMainViewportSize.x / ptIO->tMainViewportSize.y);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_update(plAppData* ptAppData)
{

    if(!gptStarter->begin_frame())
        return;

    gptDearImGui->new_frame(gptStarter->get_device(), gptStarter->get_render_pass());

    plIO* ptIO = gptIO->get_io();

    plCamera* ptCamera = gptTerrain->get_terrain_camera(ptAppData->ptTerrain);

    if(!ImGui::GetIO().WantCaptureKeyboard || !ImGui::GetIO().WantCaptureMouse)
    {

        float fCameraTravelSpeed = 10.0f / PL_UNIT_CONVERSION;
        static const float fCameraRotationSpeed = 0.005f;

        if(gptIO->is_key_down(PL_KEY_MOD_SHIFT) && gptIO->is_key_down(PL_KEY_MOD_CTRL))
            fCameraTravelSpeed = 10000.0f / PL_UNIT_CONVERSION;

        else if(gptIO->is_key_down(PL_KEY_MOD_SHIFT))
            fCameraTravelSpeed = 100.0f / PL_UNIT_CONVERSION;

        else if(gptIO->is_key_down(PL_KEY_MOD_CTRL))
            fCameraTravelSpeed = 1000.0f / PL_UNIT_CONVERSION;

        // camera space
        if(gptIO->is_key_down(PL_KEY_W)) gptCamera->translate(ptCamera,  0.0f,  0.0f,  fCameraTravelSpeed * ptIO->fDeltaTime);
        if(gptIO->is_key_down(PL_KEY_S)) gptCamera->translate(ptCamera,  0.0f,  0.0f, -fCameraTravelSpeed* ptIO->fDeltaTime);
        if(gptIO->is_key_down(PL_KEY_A)) gptCamera->translate(ptCamera, -fCameraTravelSpeed * ptIO->fDeltaTime,  0.0f,  0.0f);
        if(gptIO->is_key_down(PL_KEY_D)) gptCamera->translate(ptCamera,  fCameraTravelSpeed * ptIO->fDeltaTime,  0.0f,  0.0f);

        // world space
        if(gptIO->is_key_down(PL_KEY_F)) { gptCamera->translate(ptCamera,  0.0f, -fCameraTravelSpeed * ptIO->fDeltaTime,  0.0f); }
        if(gptIO->is_key_down(PL_KEY_R)) { gptCamera->translate(ptCamera,  0.0f,  fCameraTravelSpeed * ptIO->fDeltaTime,  0.0f); }

        if(gptIO->is_mouse_dragging(PL_MOUSE_BUTTON_LEFT, 1.0f))
        {
            const plVec2 tMouseDelta = gptIO->get_mouse_drag_delta(PL_MOUSE_BUTTON_LEFT, 1.0f);
            gptCamera->rotate(ptCamera,  -tMouseDelta.y * fCameraRotationSpeed,  -tMouseDelta.x * fCameraRotationSpeed);
            gptIO->reset_mouse_drag_delta(PL_MOUSE_BUTTON_LEFT);
        }
        gptCamera->update(ptCamera);
    }

    if(gptIO->is_key_pressed(PL_KEY_P, false))
        gptTerrain->reload_shaders(ptAppData->ptTerrain);

    gptScreenLog->add_message_ex(186, 10.0, PL_COLOR_32_GREEN, 1.0f, "FPS: %0.1f", ptIO->fFrameRate);
    gptScreenLog->add_message_ex(187, 10.0, PL_COLOR_32_GREEN, 1.0f, "Pos: %0.3f, %0.3f, %0.3f", ptCamera->tPos.x, ptCamera->tPos.y, ptCamera->tPos.z);
    gptScreenLog->add_message_ex(188, 10.0, PL_COLOR_32_GREEN, 1.0f, "Pos: %0.1f, %0.1f, %0.1f", ptCamera->tPos.x  * PL_UNIT_CONVERSION, ptCamera->tPos.y  * PL_UNIT_CONVERSION, ptCamera->tPos.z  * PL_UNIT_CONVERSION);
    
    
    if(gptIO->is_key_down(PL_KEY_MOD_SHIFT) && gptIO->is_key_down(PL_KEY_MOD_CTRL))
        gptScreenLog->add_message_ex(189, 10.0, PL_COLOR_32_GREEN, 1.0f, "%s", "10000 meters / second");
    else if(gptIO->is_key_down(PL_KEY_MOD_SHIFT))
        gptScreenLog->add_message_ex(189, 10.0, PL_COLOR_32_GREEN, 1.0f, "%s", "100 meters / second");
    else if(gptIO->is_key_down(PL_KEY_MOD_CTRL))
        gptScreenLog->add_message_ex(189, 10.0, PL_COLOR_32_GREEN, 1.0f, "%s", "1000 meters / second");
    else
        gptScreenLog->add_message_ex(189, 10.0, PL_COLOR_32_GREEN, 1.0f, "%s", "10 meters / second");


    ImGui::DockSpaceOverViewport(0, 0, ImGuiDockNodeFlags_PassthruCentralNode);
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("File", false)) ImGui::EndMenu();
        if(ImGui::BeginMenu("Edit", false)) ImGui::EndMenu();
        if(ImGui::BeginMenu("Tools", true))
        {
            ImGui::MenuItem("Dear ImGui Demo", nullptr, &ptAppData->bShowImGuiDemo);
            ImGui::MenuItem("ImPlot Demo", nullptr, &ptAppData->bShowImPlotDemo);
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Help", false)) ImGui::EndMenu();
        ImGui::EndMainMenuBar();
    }

    if(ptAppData->bShowImPlotDemo)
        ImPlot::ShowDemoWindow(&ptAppData->bShowImPlotDemo);
    if(ptAppData->bShowImGuiDemo)
        ImGui::ShowDemoWindow(&ptAppData->bShowImGuiDemo);

    // start main pass & return the encoder being used
    
    plCommandBuffer* ptCmdBuffer = gptStarter->get_command_buffer();
    gptTerrain->render(ptAppData->ptTerrain, ptCmdBuffer);
    gptStarter->submit_command_buffer(ptCmdBuffer);

    if(ImGui::Begin("View 1"))
    {
        ImVec2 tContextSize = ImGui::GetContentRegionAvail();
        gptCamera->set_aspect(ptCamera, tContextSize.x / tContextSize.y);

        ImTextureID tTexture = gptDearImGui->get_texture_id_from_bindgroup(gptStarter->get_device(), gptTerrain->get_terrain_texture(ptAppData->ptTerrain));
        ImGui::Image(tTexture, tContextSize);
    }
    ImGui::End();

    plRenderEncoder* ptEncoder = gptStarter->begin_main_pass();
    gptDearImGui->render(ptEncoder, gptGfx->get_encoder_command_buffer(ptEncoder));
    gptStarter->end_main_pass();
    gptStarter->end_frame(); 
}

//-----------------------------------------------------------------------------
// [SECTION] helper implementations
//-----------------------------------------------------------------------------

void
pl__load_apis(plApiRegistryI* ptApiRegistry)
{
    gptWindows       = pl_get_api_latest(ptApiRegistry, plWindowI);
    gptStats         = pl_get_api_latest(ptApiRegistry, plStatsI);
    gptGfx           = pl_get_api_latest(ptApiRegistry, plGraphicsI);
    gptTools         = pl_get_api_latest(ptApiRegistry, plToolsI);
    gptJobs          = pl_get_api_latest(ptApiRegistry, plJobI);
    gptDraw          = pl_get_api_latest(ptApiRegistry, plDrawI);
    gptDrawBackend   = pl_get_api_latest(ptApiRegistry, plDrawBackendI);
    gptUI            = pl_get_api_latest(ptApiRegistry, plUiI);
    gptIO            = pl_get_api_latest(ptApiRegistry, plIOI);
    gptShader        = pl_get_api_latest(ptApiRegistry, plShaderI);
    gptMemory        = pl_get_api_latest(ptApiRegistry, plMemoryI);
    gptNetwork       = pl_get_api_latest(ptApiRegistry, plNetworkI);
    gptString        = pl_get_api_latest(ptApiRegistry, plStringInternI);
    gptProfile       = pl_get_api_latest(ptApiRegistry, plProfileI);
    gptFile          = pl_get_api_latest(ptApiRegistry, plFileI);
    gptConsole       = pl_get_api_latest(ptApiRegistry, plConsoleI);
    gptScreenLog     = pl_get_api_latest(ptApiRegistry, plScreenLogI);
    gptConfig        = pl_get_api_latest(ptApiRegistry, plConfigI);
    gptStarter       = pl_get_api_latest(ptApiRegistry, plStarterI);
    gptDearImGui     = pl_get_api_latest(ptApiRegistry, plDearImGuiI);
    gptDateTime      = pl_get_api_latest(ptApiRegistry, plDateTimeI);
    gptVfs           = pl_get_api_latest(ptApiRegistry, plVfsI);
    gptPak           = pl_get_api_latest(ptApiRegistry, plPakI);
    gptDateTime      = pl_get_api_latest(ptApiRegistry, plDateTimeI);
    gptCompress      = pl_get_api_latest(ptApiRegistry, plCompressI);
    gptCamera        = pl_get_api_latest(ptApiRegistry, plCameraI);
    gptImage         = pl_get_api_latest(ptApiRegistry, plImageI);
    gptTerrain       = pl_get_api_latest(ptApiRegistry, plTerrainI);
}