#include "dcapp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The readable path is: lifecycle checks, three DrawFunction groups, then the
// small helper implementations. require_api_tables() is intentionally a flat,
// mechanical inventory so adding a public API entry creates one obvious edit.

#define REQUIRE(condition) \
    api_test_require((condition), #condition, __FILE__, __LINE__)
#define REQUIRE_API(table, member) \
    api_test_require((table) != NULL && (table)->member != NULL, #table "->" #member, __FILE__, __LINE__)

typedef struct ApiTestState {
    DcTextureId texture;
    DcVec2 texture_size;
    DcPlanetBreadcrumbsHandle breadcrumbs;
    unsigned int update_count;
    unsigned int function_call_count;
    unsigned int graphics_call_count;
    unsigned int resources_call_count;
    unsigned int planet_call_count;
    bool initialized;
    bool arguments_checked;
    bool texture_checked;
    bool planet_checked;
} ApiTestState;

static ApiTestState api_test_state;

static void api_test_require(bool condition, const char *expression, const char *file, int line);
static void require_generated_bindings(DcAppContext *app_ctx);
static void require_generated_helpers(void);
static void require_api_tables(void);
static void require_string_binding(char (*binding)[256]);
static void require_integer_binding(int *binding);
static void require_double_binding(double *binding);
static void require_boolean_binding(bool *binding);
static void check_draw_arguments(const DcDrawFuncArgs *args);
static void initialize_texture(DcAppContext *app_ctx);
static void check_planet_data_api(DcAppContext *app_ctx);

void display_init(DcAppContext *app_ctx, void **user_data) {
    REQUIRE(app_ctx != NULL);
    REQUIRE(user_data != NULL);

    api_test_state = (ApiTestState){};
    *user_data = &api_test_state;

    REQUIRE(app_ctx == dc_app_ctx);
    require_generated_bindings(app_ctx);
    require_generated_helpers();
    require_api_tables();
    initialize_texture(app_ctx);
    check_planet_data_api(app_ctx);

    api_test_state.initialized = true;
}

void display_draw(DcAppContext *app_ctx, void *user_data) {
    REQUIRE(app_ctx == dc_app_ctx);
    REQUIRE(user_data == &api_test_state);
    REQUIRE(api_test_state.initialized);
    api_test_state.update_count++;
}

void display_close(DcAppContext *app_ctx, void *user_data) {
    REQUIRE(app_ctx == dc_app_ctx);
    REQUIRE(user_data == &api_test_state);
    REQUIRE(api_test_state.initialized);
    REQUIRE(api_test_state.update_count > 0);
    REQUIRE(api_test_state.function_call_count > 0);
    REQUIRE(api_test_state.graphics_call_count > 0);
    REQUIRE(api_test_state.resources_call_count > 0);
    REQUIRE(api_test_state.planet_call_count > 0);
    REQUIRE(api_test_state.arguments_checked);
    REQUIRE(api_test_state.texture_checked);
    REQUIRE(api_test_state.planet_checked);

    fprintf(stdout, "[api-test] PASS (%u logic updates)\n", api_test_state.update_count);
    fflush(stdout);
}

void api_test_function(DcAppContext *app_ctx, void *user_data) {
    REQUIRE(app_ctx == dc_app_ctx);
    REQUIRE(user_data == &api_test_state);
    REQUIRE(api_test_state.initialized);

    api_test_state.function_call_count++;
    snprintf(*API_TEST_STATUS, sizeof(*API_TEST_STATUS), "Function callback: PASS");
}

void draw_api_test_graphics(
    DcDrawContext *draw_ctx,
    const DcDrawFuncArgs *args,
    void *user_data) {
    REQUIRE(draw_ctx != NULL);
    REQUIRE(user_data == &api_test_state);
    REQUIRE(api_test_state.initialized);
    check_draw_arguments(args);
    api_test_state.graphics_call_count++;

    const DcDrawArea *area = dc_draw->get_area(draw_ctx);
    REQUIRE(area != NULL);
    REQUIRE(area->dimensions[0] > 0.0f);
    REQUIRE(area->dimensions[1] > 0.0f);

    const DcStroke blue_stroke = {
        .color = {.r = 0.42f, .g = 0.76f, .b = 1.0f, .a = 1.0f},
        .width = 2.0f,
    };
    const DcVec4 blue_fill = {.r = 0.16f, .g = 0.38f, .b = 0.62f, .a = 0.90f};
    const DcVec2 points[] = {
        {.x = 20.0f, .y = 38.0f},
        {.x = 64.0f, .y = 112.0f},
        {.x = 108.0f, .y = 44.0f},
    };

    dc_draw->line(draw_ctx, (DcVec2){.x = 20.0f, .y = 30.0f}, (DcVec2){.x = 108.0f, .y = 30.0f}, blue_stroke);
    dc_draw->polyline(draw_ctx, points, 3, blue_stroke);
    dc_draw->polygon(draw_ctx, points, 3, blue_stroke);
    dc_draw->convex_polygon_filled(draw_ctx, points, 3, (DcVec4){.r = 0.12f, .g = 0.30f, .b = 0.48f, .a = 0.45f});
    dc_draw->quad_filled(
        draw_ctx,
        (DcVec2){.x = 130.0f, .y = 36.0f},
        (DcVec2){.x = 200.0f, .y = 44.0f},
        (DcVec2){.x = 188.0f, .y = 104.0f},
        (DcVec2){.x = 120.0f, .y = 94.0f},
        (DcVec4){.r = 0.44f, .g = 0.24f, .b = 0.62f, .a = 0.82f});
    dc_draw->circle(draw_ctx, (DcVec2){.x = 240.0f, .y = 75.0f}, 36.0f, blue_stroke);
    dc_draw->ellipse_filled(
        draw_ctx,
        (DcVec2){.x = 318.0f, .y = 76.0f},
        (DcVec2){.x = 44.0f, .y = 28.0f},
        (DcVec4){.r = 0.28f, .g = 0.56f, .b = 0.38f, .a = 0.88f});

    DcDrawResult result = {};
    dc_draw->rounded_rect_filled_ex(
        draw_ctx,
        (DcVec2){.x = 390.0f, .y = 76.0f},
        (DcVec2){.x = 64.0f, .y = 64.0f},
        10.0f,
        blue_fill,
        (DcPlacement){.local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE},
        &result);
    REQUIRE(dc_draw->container_push_area(draw_ctx, &result.area));
    dc_draw->text_ex(
        draw_ctx,
        (DcVec2){.x = 0.0f, .y = 0.0f},
        "area",
        (DcTextStyle){.color = {.r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f}, .size = 11.0f},
        dc_place_center(),
        NULL);
    dc_draw->container_pop(draw_ctx);

    DcVec2 label_size = dc_draw->text_size(
        draw_ctx,
        "generated args: PASS",
        (DcTextStyle){.size = 11.0f});
    REQUIRE(label_size.x > 0.0f);
    REQUIRE(label_size.y > 0.0f);
    dc_draw->text(
        draw_ctx,
        (DcVec2){.x = 20.0f, .y = 12.0f},
        "generated args: PASS",
        (DcTextStyle){.color = {.r = 0.64f, .g = 0.92f, .b = 0.72f, .a = 1.0f}, .size = 11.0f});

    REQUIRE(dc_draw->stencil_begin(draw_ctx));
    dc_draw->stencil_add(draw_ctx);
    dc_draw->circle_filled(draw_ctx, (DcVec2){.x = 390.0f, .y = 128.0f}, 18.0f, dc_stencil_color());
    dc_draw->stencil_remove(draw_ctx);
    dc_draw->circle_filled(draw_ctx, (DcVec2){.x = 390.0f, .y = 128.0f}, 7.0f, dc_stencil_color());
    dc_draw->stencil_draw(draw_ctx);
    dc_draw->rect_filled(
        draw_ctx,
        (DcVec2){.x = 370.0f, .y = 108.0f},
        (DcVec2){.x = 40.0f, .y = 40.0f},
        (DcVec4){.r = 0.96f, .g = 0.64f, .b = 0.24f, .a = 1.0f});
    dc_draw->stencil_end(draw_ctx);
}

void draw_api_test_resources(
    DcDrawContext *draw_ctx,
    const DcDrawFuncArgs *args,
    void *user_data) {
    (void)args;
    REQUIRE(draw_ctx != NULL);
    REQUIRE(user_data == &api_test_state);
    REQUIRE(api_test_state.initialized);
    REQUIRE(api_test_state.texture != 0);
    api_test_state.resources_call_count++;

    DcVec2 image_size = {.x = 150.0f, .y = 80.0f};
    if (api_test_state.texture_size.x > 0.0f) {
        image_size.y = image_size.x * api_test_state.texture_size.y / api_test_state.texture_size.x;
    }
    dc_draw->image(
        draw_ctx,
        api_test_state.texture,
        (DcVec2){.x = 20.0f, .y = 44.0f},
        image_size,
        (DcVec4){.r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f});
    dc_draw->image_ex(
        draw_ctx,
        api_test_state.texture,
        (DcVec2){.x = 220.0f, .y = 86.0f},
        (DcVec2){.x = 88.0f, .y = 48.0f},
        (DcVec4){.r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 0.80f},
        (DcPlacement){.local_align_x = DC_ALIGN_CENTER, .local_align_y = DC_ALIGN_MIDDLE},
        NULL);

    const char *id = "api_test_mouse_target";
    const DcVec2 polygon[] = {
        {.x = 310.0f, .y = 42.0f},
        {.x = 400.0f, .y = 42.0f},
        {.x = 382.0f, .y = 126.0f},
        {.x = 326.0f, .y = 126.0f},
    };
    const DcPlacement centered = {
        .local_align_x = DC_ALIGN_CENTER,
        .local_align_y = DC_ALIGN_MIDDLE,
    };

    dc_mouse->rect(draw_ctx, id, (DcVec2){.x = 310.0f, .y = 42.0f}, (DcVec2){.x = 90.0f, .y = 84.0f});
    dc_mouse->circle(draw_ctx, "api_test_circle", (DcVec2){.x = 355.0f, .y = 84.0f}, 32.0f);
    dc_mouse->ellipse(draw_ctx, "api_test_ellipse", (DcVec2){.x = 355.0f, .y = 84.0f}, (DcVec2){.x = 42.0f, .y = 28.0f});
    dc_mouse->polygon(draw_ctx, "api_test_polygon", polygon, 4, (DcVec2){.x = 0.0f, .y = 0.0f});
    dc_mouse->rect_ex(draw_ctx, "api_test_rect_ex", (DcVec2){.x = 355.0f, .y = 84.0f}, (DcVec2){.x = 90.0f, .y = 84.0f}, centered);
    dc_mouse->circle_ex(draw_ctx, "api_test_circle_ex", (DcVec2){.x = 355.0f, .y = 84.0f}, 32.0f, centered);
    dc_mouse->ellipse_ex(draw_ctx, "api_test_ellipse_ex", (DcVec2){.x = 355.0f, .y = 84.0f}, (DcVec2){.x = 42.0f, .y = 28.0f}, centered);
    dc_mouse->polygon_ex(draw_ctx, "api_test_polygon_ex", polygon, 4, (DcVec2){.x = 0.0f, .y = 0.0f}, centered);

    (void)dc_mouse->hovered(draw_ctx, id);
    (void)dc_mouse->pressed(draw_ctx, id);
    (void)dc_mouse->released(draw_ctx, id);
    (void)dc_mouse->active(draw_ctx, id);
    (void)dc_mouse->clicked(draw_ctx, id);
    (void)dc_mouse->down(draw_ctx);
    REQUIRE(dc_mouse->get_state(draw_ctx) != NULL);

    dc_draw->polygon(
        draw_ctx,
        polygon,
        4,
        (DcStroke){.color = {.r = 0.44f, .g = 0.92f, .b = 0.58f, .a = 1.0f}, .width = 2.0f});
    dc_draw->text(
        draw_ctx,
        (DcVec2){.x = 20.0f, .y = 18.0f},
        "texture + mouse calls: PASS",
        (DcTextStyle){.color = {.r = 0.64f, .g = 0.94f, .b = 0.72f, .a = 1.0f}, .size = 11.0f});
}

void draw_api_test_planet(
    DcDrawContext *draw_ctx,
    const DcDrawFuncArgs *args,
    void *user_data) {
    (void)args;
    REQUIRE(draw_ctx != NULL);
    REQUIRE(user_data == &api_test_state);
    REQUIRE(api_test_state.initialized);
    REQUIRE(api_test_state.planet_checked);
    api_test_state.planet_call_count++;

    dc_draw->rounded_rect_filled(
        draw_ctx,
        (DcVec2){.x = 20.0f, .y = 54.0f},
        (DcVec2){.x = 392.0f, .y = 86.0f},
        8.0f,
        (DcVec4){.r = 0.16f, .g = 0.13f, .b = 0.08f, .a = 0.95f});
    dc_draw->text(
        draw_ctx,
        (DcVec2){.x = 40.0f, .y = 104.0f},
        "20 planet API entries wired",
        (DcTextStyle){.color = {.r = 1.0f, .g = 0.88f, .b = 0.58f, .a = 1.0f}, .size = 14.0f});
    dc_draw->text(
        draw_ctx,
        (DcVec2){.x = 40.0f, .y = 76.0f},
        "cartesian breadcrumb create/update/get/clear: PASS",
        (DcTextStyle){.color = {.r = 0.76f, .g = 0.72f, .b = 0.58f, .a = 1.0f}, .size = 10.0f});
}

static void api_test_require(bool condition, const char *expression, const char *file, int line) {
    if (condition) return;

    fprintf(stderr, "[api-test] FAIL: %s (%s:%d)\n", expression, file, line);
    fflush(stderr);
    exit(EXIT_FAILURE);
}

static void require_generated_bindings(DcAppContext *app_ctx) {
    // These typed calls turn a wrong generated pointer type into a build error.
    require_string_binding(API_TEST_STRING);
    require_string_binding(API_TEST_STATUS);
    require_integer_binding(API_TEST_INTEGER);
    require_double_binding(API_TEST_DOUBLE);
    require_boolean_binding(API_TEST_BOOLEAN);
    require_double_binding(_API_TEST_LEADING_UNDERSCORE);

    REQUIRE(strcmp(*API_TEST_STRING, "generated string") == 0);
    REQUIRE(strcmp(*API_TEST_STATUS, "waiting for first frame") == 0);
    REQUIRE(*API_TEST_INTEGER == 7);
    REQUIRE(*API_TEST_DOUBLE == 2.5);
    REQUIRE(*API_TEST_BOOLEAN);
    REQUIRE(*_API_TEST_LEADING_UNDERSCORE == 9.25);

    REQUIRE(dc_get_variable("API_TEST_STRING") == API_TEST_STRING);
    REQUIRE(dc_app->get_variable(app_ctx, "API_TEST_INTEGER") == API_TEST_INTEGER);
    REQUIRE(dc_app->get_variable(app_ctx, "API_TEST_DOUBLE") == API_TEST_DOUBLE);
    REQUIRE(dc_app->get_variable(app_ctx, "API_TEST_BOOLEAN") == API_TEST_BOOLEAN);
}

static void require_generated_helpers(void) {
    const DcVec4 stencil = dc_stencil_color();
    REQUIRE(stencil.r == 0.0f);
    REQUIRE(stencil.g == 0.0f);
    REQUIRE(stencil.b == 0.0f);
    REQUIRE(stencil.a == 1.0f);

    const DcPlacement placements[] = {
        dc_place_center(),
        dc_place_left(),
        dc_place_right(),
        dc_place_top(),
        dc_place_bottom(),
        dc_place_top_left(),
        dc_place_top_right(),
        dc_place_bottom_left(),
        dc_place_bottom_right(),
    };
    for (size_t i = 0; i < sizeof(placements) / sizeof(placements[0]); i++) {
        REQUIRE(placements[i].parent_align_x != DC_ALIGN_UNDEFINED);
        REQUIRE(placements[i].parent_align_y != DC_ALIGN_UNDEFINED);
        REQUIRE(placements[i].local_align_x != DC_ALIGN_UNDEFINED);
        REQUIRE(placements[i].local_align_y != DC_ALIGN_UNDEFINED);
    }
}

static void require_api_tables(void) {
    // This is intentionally boring: every generated field is named here, so a
    // missing declaration or an unwired host entry fails in one obvious place.
    REQUIRE_API(dc_app, get_variable);

    REQUIRE_API(dc_draw, get_area);
    REQUIRE_API(dc_draw, line);
    REQUIRE_API(dc_draw, polyline);
    REQUIRE_API(dc_draw, polygon);
    REQUIRE_API(dc_draw, convex_polygon_filled);
    REQUIRE_API(dc_draw, rounded_polygon);
    REQUIRE_API(dc_draw, rounded_convex_polygon_filled);
    REQUIRE_API(dc_draw, quad);
    REQUIRE_API(dc_draw, quad_filled);
    REQUIRE_API(dc_draw, rounded_quad);
    REQUIRE_API(dc_draw, rounded_quad_filled);
    REQUIRE_API(dc_draw, image);
    REQUIRE_API(dc_draw, rect);
    REQUIRE_API(dc_draw, rect_filled);
    REQUIRE_API(dc_draw, rounded_rect);
    REQUIRE_API(dc_draw, rounded_rect_filled);
    REQUIRE_API(dc_draw, circle);
    REQUIRE_API(dc_draw, circle_filled);
    REQUIRE_API(dc_draw, ellipse);
    REQUIRE_API(dc_draw, ellipse_filled);
    REQUIRE_API(dc_draw, text_size);
    REQUIRE_API(dc_draw, text);
    REQUIRE_API(dc_draw, line_ex);
    REQUIRE_API(dc_draw, polyline_ex);
    REQUIRE_API(dc_draw, polygon_ex);
    REQUIRE_API(dc_draw, convex_polygon_filled_ex);
    REQUIRE_API(dc_draw, rounded_polygon_ex);
    REQUIRE_API(dc_draw, rounded_convex_polygon_filled_ex);
    REQUIRE_API(dc_draw, quad_ex);
    REQUIRE_API(dc_draw, quad_filled_ex);
    REQUIRE_API(dc_draw, rounded_quad_ex);
    REQUIRE_API(dc_draw, rounded_quad_filled_ex);
    REQUIRE_API(dc_draw, image_ex);
    REQUIRE_API(dc_draw, rect_ex);
    REQUIRE_API(dc_draw, rect_filled_ex);
    REQUIRE_API(dc_draw, rounded_rect_ex);
    REQUIRE_API(dc_draw, rounded_rect_filled_ex);
    REQUIRE_API(dc_draw, circle_ex);
    REQUIRE_API(dc_draw, circle_filled_ex);
    REQUIRE_API(dc_draw, ellipse_ex);
    REQUIRE_API(dc_draw, ellipse_filled_ex);
    REQUIRE_API(dc_draw, text_ex);
    REQUIRE_API(dc_draw, container_push);
    REQUIRE_API(dc_draw, container_push_ex);
    REQUIRE_API(dc_draw, container_push_area);
    REQUIRE_API(dc_draw, container_pop);
    REQUIRE_API(dc_draw, stencil_begin);
    REQUIRE_API(dc_draw, stencil_add);
    REQUIRE_API(dc_draw, stencil_remove);
    REQUIRE_API(dc_draw, stencil_draw);
    REQUIRE_API(dc_draw, stencil_end);
    REQUIRE_API(dc_draw, planet_view_geodetic);
    REQUIRE_API(dc_draw, planet_view_cartesian);
    REQUIRE_API(dc_draw, planet_container_push_geodetic);
    REQUIRE_API(dc_draw, planet_container_pop);
    REQUIRE_API(dc_draw, planet_line_local);
    REQUIRE_API(dc_draw, planet_polygon_local);
    REQUIRE_API(dc_draw, planet_convex_polygon_filled_local);
    REQUIRE_API(dc_draw, planet_sphere_geodetic);
    REQUIRE_API(dc_draw, planet_sphere_cartesian);
    REQUIRE_API(dc_draw, planet_line_geodetic);
    REQUIRE_API(dc_draw, planet_line_cartesian);
    REQUIRE_API(dc_draw, planet_polygon_geodetic);
    REQUIRE_API(dc_draw, planet_polygon_cartesian);
    REQUIRE_API(dc_draw, planet_convex_polygon_filled_geodetic);
    REQUIRE_API(dc_draw, planet_convex_polygon_filled_cartesian);
    REQUIRE_API(dc_draw, planet_ellipse_geodetic);
    REQUIRE_API(dc_draw, planet_ellipse_cartesian);
    REQUIRE_API(dc_draw, planet_ellipse_filled_geodetic);
    REQUIRE_API(dc_draw, planet_ellipse_filled_cartesian);
    REQUIRE_API(dc_draw, planet_image_geodetic);
    REQUIRE_API(dc_draw, planet_image_cartesian);
    REQUIRE_API(dc_draw, planet_text_geodetic);
    REQUIRE_API(dc_draw, planet_text_cartesian);
    REQUIRE_API(dc_draw, planet_geojson);

    REQUIRE_API(dc_mouse, rect);
    REQUIRE_API(dc_mouse, circle);
    REQUIRE_API(dc_mouse, ellipse);
    REQUIRE_API(dc_mouse, polygon);
    REQUIRE_API(dc_mouse, rect_ex);
    REQUIRE_API(dc_mouse, circle_ex);
    REQUIRE_API(dc_mouse, ellipse_ex);
    REQUIRE_API(dc_mouse, polygon_ex);
    REQUIRE_API(dc_mouse, hovered);
    REQUIRE_API(dc_mouse, pressed);
    REQUIRE_API(dc_mouse, released);
    REQUIRE_API(dc_mouse, active);
    REQUIRE_API(dc_mouse, clicked);
    REQUIRE_API(dc_mouse, down);
    REQUIRE_API(dc_mouse, get_state);

    REQUIRE_API(dc_texture, load_image);
    REQUIRE_API(dc_texture, get_size);

    REQUIRE_API(dc_planet, get_planet_by_id);
    REQUIRE_API(dc_planet, create_planet);
    REQUIRE_API(dc_planet, create_planet_with_id);
    REQUIRE_API(dc_planet, set_texture_geodetic);
    REQUIRE_API(dc_planet, set_texture_cartesian);
    REQUIRE_API(dc_planet, set_texture_projected);
    REQUIRE_API(dc_planet, set_texture_geodetic_slot);
    REQUIRE_API(dc_planet, set_texture_cartesian_slot);
    REQUIRE_API(dc_planet, set_texture_projected_slot);
    REQUIRE_API(dc_planet, clear_texture);
    REQUIRE_API(dc_planet, set_light_direction);
    REQUIRE_API(dc_planet, create_geodetic_view);
    REQUIRE_API(dc_planet, create_cartesian_view);
    REQUIRE_API(dc_planet, set_view_shaders);
    REQUIRE_API(dc_planet, load_geojson);
    REQUIRE_API(dc_planet, create_breadcrumbs);
    REQUIRE_API(dc_planet, update_breadcrumbs_geodetic);
    REQUIRE_API(dc_planet, update_breadcrumbs_cartesian);
    REQUIRE_API(dc_planet, clear_breadcrumbs);
    REQUIRE_API(dc_planet, get_breadcrumbs_points);
}

static void require_string_binding(char (*binding)[256]) {
    REQUIRE(binding != NULL);
}

static void require_integer_binding(int *binding) {
    REQUIRE(binding != NULL);
}

static void require_double_binding(double *binding) {
    REQUIRE(binding != NULL);
}

static void require_boolean_binding(bool *binding) {
    REQUIRE(binding != NULL);
}

static void check_draw_arguments(const DcDrawFuncArgs *args) {
    REQUIRE(args != NULL);
    REQUIRE(args->count == 4);
    REQUIRE(args->values != NULL);

    REQUIRE(args->values[0].type == DC_VALUE_TYPE_STRING);
    REQUIRE(strcmp(args->values[0].value_string, *API_TEST_STRING) == 0);
    REQUIRE(args->values[1].type == DC_VALUE_TYPE_INTEGER);
    REQUIRE(args->values[1].value_integer == *API_TEST_INTEGER);
    REQUIRE(args->values[2].type == DC_VALUE_TYPE_DOUBLE);
    REQUIRE(args->values[2].value_double == *API_TEST_DOUBLE);
    REQUIRE(args->values[3].type == DC_VALUE_TYPE_BOOLEAN);
    REQUIRE(args->values[3].value_boolean == *API_TEST_BOOLEAN);

    api_test_state.arguments_checked = true;
}

static void initialize_texture(DcAppContext *app_ctx) {
    api_test_state.texture = dc_texture->load_image(
        app_ctx,
        "../../assets/nasa.png",
        &api_test_state.texture_size);
    REQUIRE(api_test_state.texture != 0);
    REQUIRE(api_test_state.texture_size.x > 0.0f);
    REQUIRE(api_test_state.texture_size.y > 0.0f);

    DcVec2 queried_size = {};
    REQUIRE(dc_texture->get_size(app_ctx, api_test_state.texture, &queried_size));
    REQUIRE(queried_size.x == api_test_state.texture_size.x);
    REQUIRE(queried_size.y == api_test_state.texture_size.y);
    api_test_state.texture_checked = true;
}

static void check_planet_data_api(DcAppContext *app_ctx) {
    REQUIRE(dc_planet->get_planet_by_id(app_ctx, "api-test-missing-planet") == NULL);

    api_test_state.breadcrumbs = dc_planet->create_breadcrumbs(
        app_ctx,
        DC_PLANET_CRS_CARTESIAN,
        4,
        0.0f);
    REQUIRE(api_test_state.breadcrumbs != NULL);
    REQUIRE(dc_planet->update_breadcrumbs_cartesian(
        api_test_state.breadcrumbs,
        (DcVec3d){.x = 1.0, .y = 2.0, .z = 3.0}));
    REQUIRE(dc_planet->update_breadcrumbs_cartesian(
        api_test_state.breadcrumbs,
        (DcVec3d){.x = 4.0, .y = 5.0, .z = 6.0}));

    DcPlanetBreadcrumbsPoints points = dc_planet->get_breadcrumbs_points(api_test_state.breadcrumbs);
    REQUIRE(points.crs == DC_PLANET_CRS_CARTESIAN);
    REQUIRE(points.count == 2);
    REQUIRE(points.points != NULL);
    REQUIRE(points.points[1].x == 4.0);

    dc_planet->clear_breadcrumbs(api_test_state.breadcrumbs);
    points = dc_planet->get_breadcrumbs_points(api_test_state.breadcrumbs);
    REQUIRE(points.count == 0);
    api_test_state.planet_checked = true;
}
