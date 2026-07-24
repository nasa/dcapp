#define DCAPP_LOGIC_EXTERN
#include "dcapp.h"

#include <string.h>

// Compile representative uses of the generated public contract. Function-table
// fields are compared automatically with the host headers by
// scripts/check-logic-api.sh, so this fixture intentionally does not duplicate
// their layouts or enumerate every public enum and struct member.
static void check_public_contract(DcInit *init) {
    DcVec2 position;
    DcVec3 point;

    DcPlanetCreateInfo planet_info;
    DcPlanetViewOptions view_options;
    DcPlanetGeojsonStyle geojson_style;
    memset(&position, 0, sizeof(position));
    memset(&point, 0, sizeof(point));
    memset(&planet_info, 0, sizeof(planet_info));
    memset(&view_options, 0, sizeof(view_options));
    memset(&geojson_style, 0, sizeof(geojson_style));

    DcTextureId texture = init->texture->load_image(init->app_ctx, "", &position);
    init->texture->get_size(init->app_ctx, texture, &position);

    DcPlanetHandle planet = init->planet->create_planet(init->app_ctx, planet_info);
    DcPlanetViewHandle view = init->planet->create_cartesian_view(init->app_ctx, planet, 1, 1);
    init->planet->set_view_shaders(view, NULL, NULL);
    init->planet->set_light_direction(planet, point);

    (void)view_options;
    (void)geojson_style;
}

DCAPP_LOGIC_EXPORT void abi_function(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;
}

DCAPP_LOGIC_EXPORT void abi_draw_function(
    DcDrawContext *draw_ctx,
    const DcDrawFuncArgs *args,
    void *user_data) {
    (void)draw_ctx;
    (void)args;
    (void)user_data;
}

void check_generated_logic_api(DcInit *init) {
    check_public_contract(init);
}
