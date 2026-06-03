
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
#include "pl_icons.h"
#include "pl_json.h"

// stable extensions
#include "pl_image_ext.h"
#include "pl_profile_ext.h"
#include "pl_log_ext.h"
#include "pl_stats_ext.h"
#include "pl_graphics_ext.h"
#include "pl_tools_ext.h"
#include "pl_draw_ext.h"
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
#include "pl_resource_ext.h"

// unstable extensions
#include "pl_camera_ext.h"
#include "pl_mesh_ext.h"

// our extensions
#include "pl_planet_ext.h"
#include "pl_planet_processor_ext.h"

// dear imgui
#include "pl_dear_imgui_ext.h"
#include "imgui.h"
#include "implot.h"

//-----------------------------------------------------------------------------
// [SECTION] global apis
//-----------------------------------------------------------------------------

const plWindowI*       gptWindows       = nullptr;
const plStatsI*        gptStats         = nullptr;
const plGraphicsI*     gptGfx           = nullptr;
const plToolsI*        gptTools         = nullptr;
const plDrawI*         gptDraw          = nullptr;
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
const plImageI*        gptImage         = nullptr;
const plThreadsI*      gptThreads       = nullptr;
const plResourceI*     gptResource      = nullptr;
const plCameraI*       gptCamera        = nullptr;

const plPlanetI*           gptPlanet          = nullptr;
const plPlanetProcessorI*  gptPlanetProcessor = nullptr;

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

typedef struct _plAppData
{
    plWindow* ptWindow;

    // UI options
    bool bShowImGuiDemo;
    bool bShowImPlotDemo;
    bool bVSync;

    double dHeight;
    double dLongitude;
    double dLatitude;
    float  fPitchMod;
    float  fYawMod;

    // 3d drawing
    plCamera tCamera0;

    plPlanet* ptPlanet0;
    plPlanetView* ptPlanetView;
    // plPlanet* ptPlanet1;
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

    ptAppData->bVSync = true;

    // retrieve extension registry
    const plExtensionRegistryI* ptExtensionRegistry = pl_get_api_latest(ptApiRegistry, plExtensionRegistryI);

    // load extensions
    // ptExtensionRegistry->add_path("../../pl-terrain/out");
    ptExtensionRegistry->load("pl_unity_ext", NULL, NULL, true);
    ptExtensionRegistry->load("pl_planet_ext", NULL, NULL, true);
    ptExtensionRegistry->load("pl_planet_processor_ext", NULL, NULL, true);
    ptExtensionRegistry->load("pl_platform_ext", "pl_load_platform_ext", "pl_unload_platform_ext", false);
    ptExtensionRegistry->load("pl_dear_imgui_ext", "pl_load_dear_imgui_ext", "pl_unload_dear_imgui_ext", false);
    
    // load required apis
    pl__load_apis(ptApiRegistry);

    gptVfs->mount_directory("/shaders-terrain", "../../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    gptVfs->mount_directory("/assets", "../../assets", PL_VFS_MOUNT_FLAGS_NONE);
    gptVfs->mount_directory("/tiles", "../../data", PL_VFS_MOUNT_FLAGS_NONE);
    gptVfs->mount_directory("/cache", "../cache", PL_VFS_MOUNT_FLAGS_NONE);
    gptVfs->mount_directory("/shaders", "../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    gptVfs->mount_directory("/shader-temp", "../shader-temp", PL_VFS_MOUNT_FLAGS_NONE);
    gptFile->create_directory("../shader-temp");
    gptFile->create_directory("../cache");

    // create window
    plWindowDesc tWindowDesc = {
        PL_WINDOW_FLAG_NONE,
        "Terrain Sandbox",
        1000,
        1000,
        200,
        200
    };
    gptWindows->create(tWindowDesc, &ptAppData->ptWindow);
    gptWindows->show(ptAppData->ptWindow);

    // setup starter extension
    plStarterInit tStarterInit = {};
    tStarterInit.tFlags   = PL_STARTER_FLAGS_ALL_EXTENSIONS;
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

    // create camera
    ptAppData->tCamera0 = {};
    // ptAppData->tCamera.tPos         = {0.0f, -1738400.0f * 1.2f, 10.0f};
    ptAppData->tCamera0.tPosDouble   = {1062766.250, -1965753.0, 1243953.0};
    ptAppData->tCamera0.fNearZ       = 10.0f;
    ptAppData->tCamera0.fFarZ        = 1737400.0f * 3.0f;
    ptAppData->tCamera0.fFieldOfView = PL_PI_3;
    ptAppData->tCamera0.fAspectRatio = 1.0f;
    ptAppData->tCamera0.fYaw         = PL_PI + PL_PI_4;
    ptAppData->tCamera0.fPitch       = PL_PI_4;
    ptAppData->tCamera0.tType        = PL_CAMERA_TYPE_PERSPECTIVE_REVERSE_Z;
    gptCamera->update(&ptAppData->tCamera0);

    plResourceManagerInit tResourceManagerInit = {};
    tResourceManagerInit.ptDevice = gptStarter->get_device();
    gptResource->initialize(tResourceManagerInit);

    // initializes out Dear ImGui backend (similar to the draw backend extension)
    gptDearImGui->initialize(gptStarter->get_device(), gptStarter->get_swapchain(), gptStarter->get_render_pass());

    ImGui::GetIO().ConfigFlags &= ~ImGuiBackendFlags_PlatformHasViewports;

    // same process for implot as imgui
    ImPlot::SetCurrentContext((ImPlotContext*)ptDataRegistry->get_data("implot"));

    plPlanetExtInit tPlanetExtInit = {};
    tPlanetExtInit.ptDevice = gptStarter->get_device();
    gptPlanet->initialize(tPlanetExtInit);

    plPlanetInit tPlanetInit = {};
    tPlanetInit.dRadius = 1737400.0;
    tPlanetInit.tLoadFlags = PL_PLANET_LOAD_FLAGS_DEBUG | PL_PLANET_LOAD_FLAGS_CACHE_TEXTURES;

    plCommandBuffer* ptCmdBuffer = gptStarter->get_temporary_command_buffer();

    {
        plPlanetProcessTileInfo atTiles[64] = {};
        plPlanetProcessInfo tPlanetInfo = {};
        tPlanetInfo.tFlags = PL_PLANET_PROCESSING_FLAGS_DOUBLE_PRECISION;
        tPlanetInfo.tGeodeticModel.tDatum = PL_DATUM_SPHERE;
        tPlanetInfo.tGeodeticModel.sphere.dRadius = 1737400.0;
        tPlanetInfo.tProjection.tType = PL_PROJECTION_POLAR_STEREOGRAPHIC;
        tPlanetInfo.tProjection.tPolarStereo.dLatitudeOfOrigin = 0.0;
        tPlanetInfo.tProjection.tPolarStereo.dLongitudeOfOrigin = 0.0; // central meridian (radians)
        tPlanetInfo.tProjection.tPolarStereo.dScaleFactor = 1.0;
        tPlanetInfo.tProjection.tPolarStereo.dFalseEasting = 0.0;
        tPlanetInfo.tProjection.tPolarStereo.dFalseNorthing = 0.0;
        tPlanetInfo.dMetersPerPixel = 100.0;
        tPlanetInfo.uHorizontalTiles = 8;
        tPlanetInfo.uVerticalTiles = 8;
        tPlanetInfo.uSize = 4096;
        tPlanetInfo.uTileCount = 64;
        tPlanetInfo.atTiles = atTiles;

        for(uint32_t i = 0; i < 8; i++)
        {
            for(uint32_t j = 0; j < 8; j++)
            {
                uint32_t uTileIndex = i + j * 8;
                plPlanetProcessTileInfo tInfo = {};
                atTiles[uTileIndex].iTreeDepth      = 6;
                atTiles[uTileIndex].dMaxHeight      = 14052.0;
                atTiles[uTileIndex].dMinHeight      = -18256.0;
                atTiles[uTileIndex].dMaxBaseError   = 15.0;
                atTiles[uTileIndex].dOriginX = -1440000.0 + (double)i * 409600.0 + 409600.0 * 0.5;
                atTiles[uTileIndex].dOriginY = -1440000.0 + (double)j * 409600.0 + 409600.0 * 0.5;

                sprintf(atTiles[uTileIndex].acOutputFile, "/tiles/moon_%u_%u.chu", i, j);
                sprintf(atTiles[uTileIndex].acHeightMapFile, "/tiles/moon_%u_%u.png", i, j);
            }
        }

        gptPlanetProcessor->process(&tPlanetInfo);

        
        ptAppData->ptPlanet0 = gptPlanet->create_planet(ptCmdBuffer, tPlanetInit, &tPlanetInfo);
        
    }

    plPlanetViewInit tViewInit = {};
    tViewInit.uOutputWidth = 1024;
    tViewInit.uOutputHeight = 1024;
    ptAppData->ptPlanetView = gptPlanet->create_view(ptAppData->ptPlanet0, ptCmdBuffer, tViewInit);

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

    gptPlanet->cleanup_planet(ptAppData->ptPlanet0);
    gptPlanet->cleanup();
    gptResource->cleanup();
    gptDearImGui->cleanup();
    gptShader->cleanup();
    gptStarter->cleanup();
    gptWindows->destroy(ptAppData->ptWindow);
    free(ptAppData);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_resize
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_resize(plWindow* ptWindow, plAppData* ptAppData)
{
    gptStarter->resize();
    // plIO* ptIO = gptIO->get_io();
    // gptCamera->set_aspect(&ptAppData->tCamera, ptIO->tMainViewportSize.x / ptIO->tMainViewportSize.y);
    // ptAppData->tCamera.fAspectRatio = ptIO->tMainViewportSize.x / ptIO->tMainViewportSize.y;
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_update(plAppData* ptAppData)
{
    plIO* ptIO = gptIO->get_io();

    if(!gptStarter->begin_frame())
        return;

    gptDearImGui->new_frame(gptStarter->get_device(), gptStarter->get_render_pass());

    static uint32_t uActiveCamera = 0;

    plCamera* ptCamera = &ptAppData->tCamera0;

    static float fCameraTravelSpeed = 100000.0f;

    if((!ImGui::GetIO().WantCaptureKeyboard || !ImGui::GetIO().WantCaptureMouse) && !gptUI->wants_mouse_capture())
    {

        
        static const float fCameraRotationSpeed = 0.005f;

        if(gptIO->is_key_pressed(PL_KEY_MINUS, false)) fCameraTravelSpeed /= 10.0f;
        if(gptIO->is_key_pressed(PL_KEY_EQUAL, false)) fCameraTravelSpeed *= 10.0f;


        // camera space
        if(gptIO->is_key_down(PL_KEY_W)) gptCamera->translate(ptCamera,  0.0f,  0.0f,  fCameraTravelSpeed * ptIO->fDeltaTime);
        if(gptIO->is_key_down(PL_KEY_S)) gptCamera->translate(ptCamera,  0.0f,  0.0f, -fCameraTravelSpeed* ptIO->fDeltaTime);
        if(gptIO->is_key_down(PL_KEY_A)) gptCamera->translate(ptCamera, -fCameraTravelSpeed * ptIO->fDeltaTime,  0.0f,  0.0f);
        if(gptIO->is_key_down(PL_KEY_D)) gptCamera->translate(ptCamera,  fCameraTravelSpeed * ptIO->fDeltaTime,  0.0f,  0.0f);

        // world space
        if(gptIO->is_key_down(PL_KEY_F)) { gptCamera->translate(ptCamera,  0.0f, -fCameraTravelSpeed * ptIO->fDeltaTime,  0.0f); }
        if(gptIO->is_key_down(PL_KEY_R)) { gptCamera->translate(ptCamera,  0.0f,  fCameraTravelSpeed * ptIO->fDeltaTime,  0.0f); }
        if(gptIO->is_key_down(PL_KEY_O))
        {
            ptCamera->fRoll += 0.1f;
            ptCamera->fRoll = fmodf(ptCamera->fRoll, PL_PI * 2.0f);
        }

        ptAppData->dLongitude = atan(ptCamera->tPos.x / ptCamera->tPos.z);
        ptAppData->dLatitude = asin(ptCamera->tPos.y / pl_length_vec3(ptCamera->tPos));
        ptAppData->dHeight = pl_length_vec3(ptCamera->tPos) - 1737400.0 * 10000.0;


        if(gptIO->is_mouse_dragging(PL_MOUSE_BUTTON_LEFT, 1.0f))
        {
            const plVec2 tMouseDelta = gptIO->get_mouse_drag_delta(PL_MOUSE_BUTTON_LEFT, 1.0f);
            gptCamera->rotate(ptCamera,  -tMouseDelta.y * fCameraRotationSpeed,  -tMouseDelta.x * fCameraRotationSpeed);
            // gptIO->reset_mouse_drag_delta(PL_MOUSE_BUTTON_LEFT);
        }
    }
    gptCamera->update(ptCamera);

    static plPlanetTexture tTexture = {
        "/assets/hazard.png",
        800.0f,
        0.0f,
        0.0f
    };

    static bool bShowSphere = false;

    if(gptUI->begin_window("Debug", NULL, 0))
    {
        gptUI->layout_dynamic(0, 1);
        plPlanetRuntimeOptions tMainOptions = gptPlanet->get_runtime_options(ptAppData->ptPlanet0);
        plPlanetViewRuntimeOptions tOptions = gptPlanet->get_view_runtime_options(ptAppData->ptPlanetView);
        bool bShowDebug = tOptions.tFlags & PL_PLANET_FLAGS_SHOW_LEVELS;
        bool bWireframe = tOptions.tFlags & PL_PLANET_FLAGS_WIREFRAME;
        bool bShowOrigin = tOptions.tFlags & PL_PLANET_FLAGS_SHOW_ORIGIN;
        bool bShowChunks = tOptions.tFlags & PL_PLANET_FLAGS_SHOW_CHUNKS;
        bool bFlat = tOptions.tFlags & PL_PLANET_FLAGS_FLATTEN;
        if(gptUI->button("Toggle Camera"))
        {
            if(uActiveCamera == 0) uActiveCamera = 1;
            else uActiveCamera = 0;
        }
        gptUI->slider_float("Tau", &tOptions.fTau, 0.1f, 10.0f, 0);
        gptUI->slider_float("Hazard Map Strength", &tOptions.fHazardMapStrength, 0.0f, 1.0f, 0);
        gptUI->slider_float("Light Direction X", &tMainOptions.tLightDirection.x, -1.0f, 1.0f, 0);
        gptUI->slider_float("Light Direction Y", &tMainOptions.tLightDirection.y, -1.0f, 1.0f, 0);
        gptUI->slider_float("Light Direction Z", &tMainOptions.tLightDirection.z, -1.0f, 1.0f, 0);
        tMainOptions.tLightDirection = pl_norm_vec3(tMainOptions.tLightDirection);

        gptUI->checkbox("Show Sphere", &bShowSphere);
        if(gptUI->checkbox("Show Origin", &bShowOrigin))
        {
            if(bShowOrigin)
                tOptions.tFlags |= PL_PLANET_FLAGS_SHOW_ORIGIN;
            else
                tOptions.tFlags &= ~PL_PLANET_FLAGS_SHOW_ORIGIN;
        }

        if(gptUI->checkbox("Show Chunks", &bShowChunks))
        {
            if(bShowChunks)
                tOptions.tFlags |= PL_PLANET_FLAGS_SHOW_CHUNKS;
            else
                tOptions.tFlags &= ~PL_PLANET_FLAGS_SHOW_CHUNKS;
        }

        if(gptUI->checkbox("Wireframe", &bWireframe))
        {
            if(bWireframe)
                tOptions.tFlags |= PL_PLANET_FLAGS_WIREFRAME;
            else
                tOptions.tFlags &= ~PL_PLANET_FLAGS_WIREFRAME;
        }

        if(gptUI->checkbox("Levels", &bShowDebug))
        {
            if(bShowDebug)
                tOptions.tFlags |= PL_PLANET_FLAGS_SHOW_LEVELS;
            else
                tOptions.tFlags &= ~PL_PLANET_FLAGS_SHOW_LEVELS;
        }

        if(gptUI->checkbox("Flatten", &bFlat))
        {
            if(bFlat)
                tOptions.tFlags |= PL_PLANET_FLAGS_FLATTEN;
            else
                tOptions.tFlags &= ~PL_PLANET_FLAGS_FLATTEN;
        }

        // gptUI->slider_float("Tau", &ptAppData->fTau, 0.0f, 10.0f, 0);
        if(gptUI->button("Reload Shaders"))
        {
            gptPlanet->reload_shaders(ptAppData->ptPlanetView);
        }

        float fOriginX = (float)tTexture.dOriginX;
        float fOriginY = (float)tTexture.dOriginY;
        if(gptUI->input_float("dOriginX", &fOriginX, NULL, 0))
        {
            tTexture.dOriginX = (double)fOriginX;
        }
        if(gptUI->input_float("dOriginY", &fOriginY, NULL, 0))
        {
            tTexture.dOriginY = (double)fOriginY;
        }
        gptUI->input_float("fMetersPerPixel", &tTexture.fMetersPerPixel, NULL, 0);

        if(gptUI->button("Update Hazard"))
        {
            gptPlanet->set_texture(ptAppData->ptPlanet0, &tTexture, 0);
        }

        if(gptUI->button("Remove Hazard"))
        {
            gptPlanet->set_texture(ptAppData->ptPlanet0, NULL, 0);
        }

        gptPlanet->set_runtime_options(ptAppData->ptPlanet0, tMainOptions);
        gptPlanet->set_view_runtime_options(ptAppData->ptPlanetView, tOptions);
        gptUI->end_window();
    }

    // if(bShowSphere)
    //     gptPlanet->draw_sphere(ptAppData->ptPlanetView, tTexture.fLongitude, tTexture.fLatitude, 0.0f, 5000.0f, PL_COLOR_32_RED);
    gptPlanet->draw_sphere(ptAppData->ptPlanetView, 0.0f, -45.0f, 0.0f, 10000.0f, PL_COLOR_32_CYAN);
    gptPlanet->draw_sphere(ptAppData->ptPlanetView, 90.0f, -45.0f, 0.0f, 10000.0f, PL_COLOR_32_WHITE);
    gptPlanet->draw_sphere(ptAppData->ptPlanetView, 180.0f, -45.0f, 0.0f, 10000.0f, PL_COLOR_32_YELLOW);
    gptPlanet->draw_sphere(ptAppData->ptPlanetView, 270.0f, -45.0f, 0.0f, 10000.0f, PL_COLOR_32_GREEN);

    gptScreenLog->add_message_ex(186, 10.0, PL_COLOR_32_GREEN, 1.0f, "FPS: %0.0f", ptIO->fFrameRate);
    gptScreenLog->add_message_ex(187, 10.0, PL_COLOR_32_GREEN, 1.0f, "Pos: %0.3f, %0.3f, %0.3f", ptCamera->tPos.x, ptCamera->tPos.y, ptCamera->tPos.z);
    
    gptScreenLog->add_message_ex(189, 10.0, PL_COLOR_32_GREEN, 1.0f, "%0.0f meters / second", fCameraTravelSpeed);
    gptScreenLog->add_message_ex(190, 10.0, PL_COLOR_32_GREEN, 1.0f, "Yaw:   %0.0f", pl_degreesf(ptCamera->fYaw));
    gptScreenLog->add_message_ex(191, 10.0, PL_COLOR_32_GREEN, 1.0f, "Pitch: %0.0f", pl_degreesf(ptCamera->fPitch));
    gptScreenLog->add_message_ex(192, 10.0, PL_COLOR_32_GREEN, 1.0f, "Roll:  %0.0f", pl_degreesf(ptCamera->fRoll));
    
    gptScreenLog->add_message_ex(194, 10.0, PL_COLOR_32_GREEN, 1.0f, "Longitude:  %0.0f", ptAppData->dLongitude * 57.29577951);
    gptScreenLog->add_message_ex(195, 10.0, PL_COLOR_32_GREEN, 1.0f, "Latitude:  %0.0f", ptAppData->dLatitude * 57.29577951);
    gptScreenLog->add_message_ex(193, 10.0, PL_COLOR_32_GREEN, 1.0f, "Height:  %0.0f", ptAppData->dHeight);

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

    // gptPlanet->draw_sphere(ptAppData->ptPlanet, 45.0f, -45.0f, 1000.0f, 10000.0f, PL_COLOR_32_CYAN);
    // gptPlanet->draw_sphere(ptAppData->ptPlanet0, 63.6156921f, -60.7322617f, 10000.0f, 10000.0f, PL_COLOR_32_CYAN);


    plCommandBuffer* ptCmdBuffer = gptStarter->get_temporary_command_buffer();
    PL_PROFILE_BEGIN_SAMPLE_API(gptProfile, 0, "prepare terrain");
    gptPlanet->prepare(ptAppData->ptPlanet0,  ptCmdBuffer);
    PL_PROFILE_END_SAMPLE_API(gptProfile, 0);
    gptStarter->submit_temporary_command_buffer(ptCmdBuffer);

    ptCmdBuffer = gptStarter->get_command_buffer();
    PL_PROFILE_BEGIN_SAMPLE_API(gptProfile, 0, "terrain");
    gptPlanet->render_view(ptAppData->ptPlanetView, ptCamera, ptCmdBuffer);
    PL_PROFILE_END_SAMPLE_API(gptProfile, 0);
    gptStarter->submit_command_buffer(ptCmdBuffer);
    
    if(ImGui::Begin("View 0"))
    {
        ImVec2 tContextSize = ImGui::GetContentRegionAvail();
        gptCamera->set_aspect(ptCamera, tContextSize.x / tContextSize.y);

        ImTextureID tTexture = gptDearImGui->get_texture_id_from_bindgroup(gptStarter->get_device(), gptPlanet->get_view_texture(ptAppData->ptPlanetView));
        ImGui::Image(tTexture, tContextSize);
    }
    ImGui::End();

    plRenderEncoder* ptEncoder = gptStarter->begin_main_pass();
    

    plDevice* ptDevice = gptStarter->get_device();

    gptDearImGui->render(ptEncoder, gptGfx->get_encoder_command_buffer(ptEncoder));
    gptStarter->end_main_pass();
    gptStarter->end_frame(); 

    if(gptIO->is_mouse_dragging(PL_MOUSE_BUTTON_LEFT, 1.0f))
    {

        gptIO->reset_mouse_drag_delta(PL_MOUSE_BUTTON_LEFT);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] helper implementations
//-----------------------------------------------------------------------------

void
pl__load_apis(plApiRegistryI* ptApiRegistry)
{
    gptWindows         = pl_get_api_latest(ptApiRegistry, plWindowI);
    gptStats           = pl_get_api_latest(ptApiRegistry, plStatsI);
    gptGfx             = pl_get_api_latest(ptApiRegistry, plGraphicsI);
    gptTools           = pl_get_api_latest(ptApiRegistry, plToolsI);
    gptDraw            = pl_get_api_latest(ptApiRegistry, plDrawI);
    gptUI              = pl_get_api_latest(ptApiRegistry, plUiI);
    gptIO              = pl_get_api_latest(ptApiRegistry, plIOI);
    gptShader          = pl_get_api_latest(ptApiRegistry, plShaderI);
    gptMemory          = pl_get_api_latest(ptApiRegistry, plMemoryI);
    gptNetwork         = pl_get_api_latest(ptApiRegistry, plNetworkI);
    gptString          = pl_get_api_latest(ptApiRegistry, plStringInternI);
    gptProfile         = pl_get_api_latest(ptApiRegistry, plProfileI);
    gptFile            = pl_get_api_latest(ptApiRegistry, plFileI);
    gptConsole         = pl_get_api_latest(ptApiRegistry, plConsoleI);
    gptScreenLog       = pl_get_api_latest(ptApiRegistry, plScreenLogI);
    gptConfig          = pl_get_api_latest(ptApiRegistry, plConfigI);
    gptStarter         = pl_get_api_latest(ptApiRegistry, plStarterI);
    gptDearImGui       = pl_get_api_latest(ptApiRegistry, plDearImGuiI);
    gptDateTime        = pl_get_api_latest(ptApiRegistry, plDateTimeI);
    gptVfs             = pl_get_api_latest(ptApiRegistry, plVfsI);
    gptPak             = pl_get_api_latest(ptApiRegistry, plPakI);
    gptDateTime        = pl_get_api_latest(ptApiRegistry, plDateTimeI);
    gptCompress        = pl_get_api_latest(ptApiRegistry, plCompressI);
    gptImage           = pl_get_api_latest(ptApiRegistry, plImageI);
    gptPlanet          = pl_get_api_latest(ptApiRegistry, plPlanetI);
    gptPlanetProcessor = pl_get_api_latest(ptApiRegistry, plPlanetProcessorI);
    gptThreads         = pl_get_api_latest(ptApiRegistry, plThreadsI);
    gptResource        = pl_get_api_latest(ptApiRegistry, plResourceI);
    gptCamera          = pl_get_api_latest(ptApiRegistry, plCameraI);
}