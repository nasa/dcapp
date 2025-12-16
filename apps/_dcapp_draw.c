#define _USE_MATH_DEFINES
#include <math.h>

#include "dcapp.h"

#include "../src/app/enums.h"
#include "../src/utils/string.h"

// Forward declarations
static void _draw_node_button(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_circle(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_container(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_conditional(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_image(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_line(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_panel(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_pixelstream(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_polygon(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_rectangle(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_set(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_terrain(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_text(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_window(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);

static void _draw_node_list(_AppData *app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *node_transform) {
    _NodeIndex current_node_index = node_index;
    while (current_node_index != NODE_INDEX_UNDEFINED) {
        _draw_node(app_data, current_node_index, parent_position, parent_dimensions, node_transform);
        current_node_index = _get_node(app_data, current_node_index)->next;
    }
}

static void _draw_node(_AppData *app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    if (node_index == NODE_INDEX_UNDEFINED) {
        fprintf(stderr, "DCAPP _draw_node(): attempting to draw undefined node index\n");
    }

    _Node *node = _get_node(app_data, node_index);
    switch (node->type) {
        case NODE_TYPE_BUTTON:
            _draw_node_button(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_CIRCLE:
            _draw_node_circle(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_CONTAINER:
            _draw_node_container(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_CONDITIONAL:
            _draw_node_conditional(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_IMAGE:
            _draw_node_image(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_LINE:
            _draw_node_line(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_PANEL:
            _draw_node_panel(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_PIXELSTREAM:
            _draw_node_pixelstream(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_POLYGON:
            _draw_node_polygon(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_RECTANGLE:
            _draw_node_rectangle(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_SET:
            _draw_node_set(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_TERRAIN:
            _draw_node_terrain(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_TEXT:
            _draw_node_text(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_WINDOW:
            _draw_node_window(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        default:
            break;
    }
}

static void _draw_node_button(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_dimension[2] = {
        node->button.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->button.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_virtual_dimension[2] = {
        node->button.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->button.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation       = node->button.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->button.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->button.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->button.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->button.dimension.y)->value_double : parent_dimensions->y};

    // get virtual dimensions
    float virtual_dimension[2] = {
        use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->button.virtual_dimension.x)->value_double : dimension[0],
        use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->button.virtual_dimension.y)->value_double : dimension[1]};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->button.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->button.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->button.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->button.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->button.local_align.x)->value_integer,
            node->button.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->button.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2];
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Button> draw call: %d\n", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Button> draw call: %d\n", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        // boolean check
        bool use_position[2] = {
            node->button.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->button.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        // get position
        float position[2];
        if (use_position[0]) {
            position[0] = (float)dc_app_lookup_get_value(app_data->lookup, node->button.position.x)->value_double;
        } else {
            DcAppAlignType parent_align_x = node->button.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->button.parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    position[0] = parent_dimensions->x;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() button: Invalid parent_align_x value %d\n", parent_align_x);
                    break;
            }

            // add parent offset
            position[0] += parent_position->x;
        }
        if (use_position[1]) {
            position[1] = (float)dc_app_lookup_get_value(app_data->lookup, node->button.position.y)->value_double;
        } else {
            DcAppAlignType parent_align_y = node->button.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->button.parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    position[1] = parent_dimensions->y;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() button: Invalid parent_align_y value %d\n", parent_align_y);
                    break;
            }

            // add parent offset
            position[1] += parent_position->y;
        }

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->button.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->button.pivot_local_align.x)->value_integer,
                node->button.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->button.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2];
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <Button> draw call: %d\n", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <Button> draw call: %d\n", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->button.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform scale
    {
        // compute matrix
        plMat4 scale_xform = pl_mat4_scale_xyz(dimension[0] / virtual_dimension[0], dimension[1] / virtual_dimension[1], 1.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &scale_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // determine enabled state
    DcValue *enabled_var_value = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, node->button.var_enabled)->value_index);
    DcValue *enabled_on_value  = dc_app_lookup_get_value(app_data->lookup, node->button.val_enabled_on);
    bool     is_enabled        = dc_value_is_equal(enabled_var_value, enabled_on_value);

    // determine indicator
    DcValue *indicator_var_value = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, node->button.var_indicator)->value_index);
    DcValue *indicator_on_value  = dc_app_lookup_get_value(app_data->lookup, node->button.val_indicator_on);
    bool     is_indicator_on     = dc_value_is_equal(indicator_var_value, indicator_on_value);

    // determine transition
    DcValue *target_var_value = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, node->button.var_target)->value_index);
    bool     is_transitioning = dc_value_is_not_equal(target_var_value, indicator_var_value);

    // process mouse event states
    bool is_pressed  = (app_data->frame_data.pressed_node == node_index);
    bool is_released = (app_data->frame_data.released_node == node_index);

    // process mouse events per button type
    DcValue *target_on_value  = dc_app_lookup_get_value(app_data->lookup, node->button.val_target_on);
    DcValue *target_off_value = dc_app_lookup_get_value(app_data->lookup, node->button.val_target_off);
    switch (node->button.type) {

        // toggle: flip based on current indicator state (on release)
        case DC_APP_BUTTON_TYPE_TOGGLE:
            if (is_released) {
                if (is_indicator_on) {
                    dc_value_copy(target_var_value, target_off_value);
                } else {
                    dc_value_copy(target_var_value, target_on_value);
                }
            }
            break;

        // momentary: flip on press/release
        case DC_APP_BUTTON_TYPE_MOMENTARY:
            if (is_pressed) {
                dc_value_copy(target_var_value, target_on_value);
            } else if (is_released) {
                dc_value_copy(target_var_value, target_off_value);
            }
            break;

        // standard: set to on value on release
        case DC_APP_BUTTON_TYPE_STANDARD:
            if (is_released) {
                dc_value_copy(target_var_value, target_on_value);
            }
            break;

        default:
            break;
    }

    // mouse interaction
    bool enable_mouse_events = false;
    switch (node->button.type) {
        case DC_APP_BUTTON_TYPE_MOMENTARY:
            enable_mouse_events = is_enabled;
            break;
        case DC_APP_BUTTON_TYPE_STANDARD:
        case DC_APP_BUTTON_TYPE_TOGGLE:
            enable_mouse_events = is_enabled && !is_transitioning;
            break;
        default:
            break;
    }
    if (enable_mouse_events) {
        plVec4 mouse_position = (plVec4){
            app_data->frame_data.mouse_position.x,
            app_data->frame_data.mouse_position.y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        bool inside = mouse_position.x > 0 && mouse_position.x < virtual_dimension[0] && mouse_position.y > 0 && mouse_position.y < virtual_dimension[1];

        // update global states (only if enabled)
        if (inside) {
            app_data->frame_data.next_hovered_node = node_index;

            if (app_data->frame_data.is_mouse_pressed) {
                app_data->frame_data.next_pressed_node = node_index;
            }
        }
    }

    // draw children based on state
    plVec2 child_position   = (plVec2){0.0f, 0.0f};
    plVec2 child_dimensions = (plVec2){virtual_dimension[0], virtual_dimension[1]};

    // layer 1: indicator state (base appearance)
    if (is_transitioning) {
        _draw_node_list(app_data, node->button.child_transition, &child_position, &child_dimensions, &transform);
    } else if (is_indicator_on) {
        _draw_node_list(app_data, node->button.child_indicator_on, &child_position, &child_dimensions, &transform);
    } else {
        _draw_node_list(app_data, node->button.child_indicator_off, &child_position, &child_dimensions, &transform);
    }

    // layer 2: enabled/disabled overlay
    if (is_enabled) {
        _draw_node_list(app_data, node->button.child_enabled, &child_position, &child_dimensions, &transform);
    } else {
        _draw_node_list(app_data, node->button.child_disabled, &child_position, &child_dimensions, &transform);
    }

    // layer 3: press/release feedback
    if (is_pressed) {
        _draw_node_list(app_data, node->button.child_pressed, &child_position, &child_dimensions, &transform);
    } else if (is_released) {
        _draw_node_list(app_data, node->button.child_released, &child_position, &child_dimensions, &transform);
    }
}

static void _draw_node_circle(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_radius         = node->circle.radius != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_rotation       = node->circle.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->circle.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->circle.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float radius, diameter;
    if (use_radius) {
        radius   = (float)dc_app_lookup_get_value(app_data->lookup, node->circle.radius)->value_double;
        diameter = 2 * radius;
    } else {
        diameter = fminf(parent_dimensions->x, parent_dimensions->y);
        radius   = diameter / 2;
    }

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->circle.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->circle.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->circle.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->circle.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->circle.local_align.x)->value_integer,
            node->circle.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->circle.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2];
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * diameter / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * diameter;
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * diameter / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * diameter;
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        // boolean check
        bool use_position[2] = {
            node->circle.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->circle.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        // get position
        float position[2];
        if (use_position[0]) {
            position[0] = (float)dc_app_lookup_get_value(app_data->lookup, node->circle.position.x)->value_double;
        } else {
            DcAppAlignType parent_align_x = node->circle.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->circle.parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    position[0] = parent_dimensions->x;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() circle: Invalid parent_align_x value %d\n", parent_align_x);
                    break;
            }

            // add parent offset
            position[0] += parent_position->x;
        }
        if (use_position[1]) {
            position[1] = (float)dc_app_lookup_get_value(app_data->lookup, node->circle.position.y)->value_double;
        } else {
            DcAppAlignType parent_align_y = node->circle.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->circle.parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    position[1] = parent_dimensions->y;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() circle: Invalid parent_align_y value %d\n", parent_align_y);
                    break;
            }

            // add parent offset
            position[1] += parent_position->y;
        }

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->circle.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->circle.pivot_local_align.x)->value_integer,
                node->circle.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->circle.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2];
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = diameter / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = diameter;
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <circle> draw call: %d\n", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = diameter / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = diameter;
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <circle> draw call: %d\n", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->circle.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // get points
    int    num_points = node->circle.num_segments == DC_APP_VAL_INDEX_UNDEFINED ? 40 : dc_app_lookup_get_value(app_data->lookup, node->circle.num_segments)->value_integer;
    plVec2 points[_NODE_CIRCLE_MAX_SEGMENTS];
    for (int ii = 0; ii < num_points; ii++) {
        float  angle  = ii * (2.0f * (float)M_PI / num_points);
        plVec4 point4 = (plVec4){
            radius * (1.0f + cosf(angle)),
            radius * (1.0f + sinf(angle)),
            0, 1};
        point4     = pl_mul_mat4_vec4(&transform, point4);
        points[ii] = (plVec2){point4.x, point4.y};
    }

    // draw fill
    if (node->circle.fill_enabled) {

        float fill_color[4] = {
            node->circle.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->circle.fill_color.r)->value_double,
            node->circle.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->circle.fill_color.g)->value_double,
            node->circle.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->circle.fill_color.b)->value_double,
            node->circle.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->circle.fill_color.a)->value_double,
        };
        uint32_t pl_fill_color = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
        _ext_draw->add_convex_polygon_filled(app_data->pl_layer, points, num_points, (plDrawSolidOptions){.uColor = pl_fill_color});
    }

    // draw outline
    if (node->circle.line_enabled) {
        float line_thickness = (float)dc_app_lookup_get_value(app_data->lookup, node->circle.line_width)->value_double;
        float line_color[4]  = {
            node->circle.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->circle.line_color.r)->value_double,
            node->circle.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->circle.line_color.g)->value_double,
            node->circle.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->circle.line_color.b)->value_double,
            node->circle.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->circle.line_color.a)->value_double,
        };
        uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
        _ext_draw->add_polygon(app_data->pl_layer, points, num_points, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
    }

    // mouse events
    if (node->circle.mouse_events.enabled) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            app_data->frame_data.mouse_position.x,
            app_data->frame_data.mouse_position.y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        bool inside = false;
        if (mouse_position.x > 0 && mouse_position.x < diameter && mouse_position.y > 0 && mouse_position.y < diameter) {

            // now do the actual check
            float dx      = mouse_position.x - radius;
            float dy      = mouse_position.y - radius;
            float dist_sq = dx * dx + dy * dy;
            float r_sq    = radius * radius;
            inside        = dist_sq <= r_sq;
        }

        // update global states
        if (inside) {
            app_data->frame_data.next_hovered_node = node_index;

            if (app_data->frame_data.is_mouse_pressed) {
                app_data->frame_data.next_pressed_node = node_index;
            }
        }

        // draw mouse events
        plVec2 position   = (plVec2){0.0f, 0.0f};
        plVec2 dimensions = (plVec2){diameter, diameter};
        if (app_data->frame_data.pressed_node == node_index) {
            _draw_node_list(app_data, node->circle.mouse_events.pressed, &position, &dimensions, &transform);
        } else if (app_data->frame_data.active_node == node_index) {
            _draw_node_list(app_data, node->circle.mouse_events.active, &position, &dimensions, &transform);
        } else if (app_data->frame_data.released_node == node_index) {
            _draw_node_list(app_data, node->circle.mouse_events.released, &position, &dimensions, &transform);
        } else if (app_data->frame_data.hovered_node == node_index) {
            _draw_node_list(app_data, node->circle.mouse_events.hovered, &position, &dimensions, &transform);
        } else {
            _draw_node_list(app_data, node->circle.mouse_events.inactive, &position, &dimensions, &transform);
        }
    }
}

static void _draw_node_container(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_dimension[2] = {
        node->container.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->container.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_virtual_dimension[2] = {
        node->container.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->container.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation       = node->container.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->container.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->container.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->container.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->container.dimension.y)->value_double : parent_dimensions->y};

    // get virtual dimensions
    float virtual_dimension[2] = {
        use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->container.virtual_dimension.x)->value_double : dimension[0],
        use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->container.virtual_dimension.y)->value_double : dimension[1]};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->container.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->container.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->container.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->container.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->container.local_align.x)->value_integer,
            node->container.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->container.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2];
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        // boolean check
        bool use_position[2] = {
            node->container.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->container.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        // get position
        float position[2];
        if (use_position[0]) {
            position[0] = (float)dc_app_lookup_get_value(app_data->lookup, node->container.position.x)->value_double;
        } else {
            DcAppAlignType parent_align_x = node->container.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->container.parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    position[0] = parent_dimensions->x;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() container: Invalid parent_align_x value %d\n", parent_align_x);
                    break;
            }

            // add parent offset
            position[0] += parent_position->x;
        }
        if (use_position[1]) {
            position[1] = (float)dc_app_lookup_get_value(app_data->lookup, node->container.position.y)->value_double;
        } else {
            DcAppAlignType parent_align_y = node->container.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->container.parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    position[1] = parent_dimensions->y;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() container: Invalid parent_align_y value %d\n", parent_align_y);
                    break;
            }

            // add parent offset
            position[1] += parent_position->y;
        }

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->container.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->container.pivot_local_align.x)->value_integer,
                node->container.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->container.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2];
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <container> draw call: %d\n", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <container> draw call: %d\n", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->container.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform scale
    {
        // compute matrix
        plMat4 scale_xform = pl_mat4_scale_xyz(dimension[0] / virtual_dimension[0], dimension[1] / virtual_dimension[1], 1.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &scale_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // draw children
    plVec2 virtual_dimensions_vec2 = (plVec2){virtual_dimension[0], virtual_dimension[1]};
    plVec2 position_vec2           = (plVec2){0.0f, 0.0f};
    _draw_node_list(app_data, node->container.child, &position_vec2, &virtual_dimensions_vec2, &transform);
}

static void _draw_node_conditional(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    DcValue             *val1     = dc_app_lookup_get_value(app_data->lookup, node->conditional.value1);
    DcAppConditionalType type     = (DcAppConditionalType)dc_app_lookup_get_value(app_data->lookup, node->conditional.type)->value_integer;
    bool                 use_val2 = node->conditional.value2 != DC_APP_VAL_INDEX_UNDEFINED;

    // evaluate
    bool result = false;
    if (use_val2) {
        DcValue *val2 = dc_app_lookup_get_value(app_data->lookup, node->conditional.value2);

        switch (type) {
            case DC_APP_CONDITIONAL_TYPE_EQ:
                result = dc_value_is_equal(val1, val2);
                break;
            case DC_APP_CONDITIONAL_TYPE_NE:
                result = dc_value_is_not_equal(val1, val2);
                break;
            case DC_APP_CONDITIONAL_TYPE_LT:
                result = dc_value_is_less(val1, val2);
                break;
            case DC_APP_CONDITIONAL_TYPE_GT:
                result = dc_value_is_greater(val1, val2);
                break;
            case DC_APP_CONDITIONAL_TYPE_LTE:
                result = dc_value_is_less_or_equal(val1, val2);
                break;
            case DC_APP_CONDITIONAL_TYPE_GTE:
                result = dc_value_is_greater_or_equal(val1, val2);
            default:
                fprintf(stderr, "DCApp _draw_node(): unknown conditional_type on evaluation %d for valid Val2\n", type);
                break;
        }
    } else {
        switch (type) {
            case DC_APP_CONDITIONAL_TYPE_TRUE:
                result = val1->value_boolean;
                break;
            case DC_APP_CONDITIONAL_TYPE_FALSE:
                result = !(val1->value_boolean);
                break;
            default:
                fprintf(stderr, "DCApp _draw_node(): unknown conditional_type on evaluation %d for Val1 only\n", type);
                break;
        }
    }

    // process children
    if (result) {
        _draw_node_list(app_data, node->conditional.child_true, parent_position, parent_dimensions, parent_transform);
    } else {
        _draw_node_list(app_data, node->conditional.child_false, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_image(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_dimension[2] = {
        node->image.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->image.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation       = node->image.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->image.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->image.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->image.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->image.dimension.y)->value_double : parent_dimensions->y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->image.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->image.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->image.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->image.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->image.local_align.x)->value_integer,
            node->image.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->image.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2];
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        // boolean check
        bool use_position[2] = {
            node->image.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->image.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        // get position
        float position[2];
        if (use_position[0]) {
            position[0] = (float)dc_app_lookup_get_value(app_data->lookup, node->image.position.x)->value_double;
        } else {
            DcAppAlignType parent_align_x = node->image.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->image.parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    position[0] = parent_dimensions->x;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() image: Invalid parent_align_x value %d\n", parent_align_x);
                    break;
            }

            // add parent offset
            position[0] += parent_position->x;
        }
        if (use_position[1]) {
            position[1] = (float)dc_app_lookup_get_value(app_data->lookup, node->image.position.y)->value_double;
        } else {
            DcAppAlignType parent_align_y = node->image.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->image.parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    position[1] = parent_dimensions->y;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() image: Invalid parent_align_y value %d\n", parent_align_y);
                    break;
            }

            // add parent offset
            position[1] += parent_position->y;
        }

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->image.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->image.pivot_local_align.x)->value_integer,
                node->image.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->image.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2];
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <image> draw call: %d\n", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <image> draw call: %d\n", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->image.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // PL specific fixes
    {
        // move from top-left reference to bottom-left
        plMat4 trans_pl_origin_xform = pl_mat4_translate_xyz(0, dimension[1], 0.0f);

        // flip over the y axis
        plMat4 scale_invert_y_xform = pl_mat4_scale_xyz(1.0f, -1.0f, 1.0f);

        // apply transforms
        transform = pl_mul_mat4t(&transform, &trans_pl_origin_xform);
        transform = pl_mul_mat4t(&transform, &scale_invert_y_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // compute points
    plVec4 point0_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){0.0f, 0.0f, 0.0f, 1.0f});
    plVec4 point1_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){0.0f, dimension[1], 0.0f, 1.0f});
    plVec4 point2_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){dimension[0], dimension[1], 0.0f, 1.0f});
    plVec4 point3_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){dimension[0], 0.0f, 0.0f, 1.0f});
    plVec2 point0      = (plVec2){point0_vec4.x, point0_vec4.y};
    plVec2 point1      = (plVec2){point1_vec4.x, point1_vec4.y};
    plVec2 point2      = (plVec2){point2_vec4.x, point2_vec4.y};
    plVec2 point3      = (plVec2){point3_vec4.x, point3_vec4.y};

    // draw
    plBindGroupHandle bind_group_handle = app_data->sb_textures[node->image.texture_index].bind_group_handle;
    _ext_draw->add_image_quad(app_data->pl_layer, bind_group_handle.uData, point0, point1, point2, point3);

    // mouse events
    if (node->image.mouse_events.enabled) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            app_data->frame_data.mouse_position.x,
            app_data->frame_data.mouse_position.y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        bool inside = mouse_position.x > 0 && mouse_position.x < dimension[0] && mouse_position.y > 0 && mouse_position.y < dimension[1];

        // update global states
        if (inside) {
            app_data->frame_data.next_hovered_node = node_index;

            if (app_data->frame_data.is_mouse_pressed) {
                app_data->frame_data.next_pressed_node = node_index;
            }
        }

        // draw mouse events
        plVec2 position   = (plVec2){0.0f, 0.0f};
        plVec2 dimensions = (plVec2){dimension[0], dimension[1]};
        if (app_data->frame_data.pressed_node == node_index) {
            _draw_node_list(app_data, node->image.mouse_events.pressed, &position, &dimensions, &transform);
        } else if (app_data->frame_data.active_node == node_index) {
            _draw_node_list(app_data, node->image.mouse_events.active, &position, &dimensions, &transform);
        } else if (app_data->frame_data.released_node == node_index) {
            _draw_node_list(app_data, node->image.mouse_events.released, &position, &dimensions, &transform);
        } else if (app_data->frame_data.hovered_node == node_index) {
            _draw_node_list(app_data, node->image.mouse_events.hovered, &position, &dimensions, &transform);
        } else {
            _draw_node_list(app_data, node->image.mouse_events.inactive, &position, &dimensions, &transform);
        }
    }
}

static void _draw_node_line(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_rotation       = node->line.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->line.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->line.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->line.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->line.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->line.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform position
    {
        // boolean check
        bool use_position[2] = {
            node->line.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->line.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        // get position
        float position[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->line.position.x)->value_double : 0.0f,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->line.position.y)->value_double : 0.0f,
        };

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // get points, min/max
    plVec2 min_pos    = (plVec2){FLT_MAX, FLT_MAX};
    plVec2 max_pos    = (plVec2){FLT_MIN, FLT_MIN};
    int    num_points = sbcount(node->line.sb_points);
    plVec2 raw_points[_NODE_LINE_MAX_POINTS];
    plVec2 points[_NODE_LINE_MAX_POINTS];
    for (int ii = 0; ii < num_points; ii++) {

        // get raw point
        raw_points[ii] = (plVec2){
            (float)dc_app_lookup_get_value(app_data->lookup, node->line.sb_points[ii].x)->value_double,
            (float)dc_app_lookup_get_value(app_data->lookup, node->line.sb_points[ii].y)->value_double};

        // update max/min
        min_pos.x = fminf(min_pos.x, raw_points[ii].x);
        min_pos.y = fminf(min_pos.y, raw_points[ii].y);
        max_pos.x = fmaxf(max_pos.x, raw_points[ii].x);
        max_pos.y = fmaxf(max_pos.y, raw_points[ii].y);

        // transform point
        plVec4 point4 = (plVec4){
            raw_points[ii].x,
            raw_points[ii].y,
            0, 1};
        point4     = pl_mul_mat4_vec4(&transform, point4);
        points[ii] = (plVec2){point4.x, point4.y};
    }

    // draw outline
    if (node->line.line_enabled) {
        float line_thickness = (float)dc_app_lookup_get_value(app_data->lookup, node->line.line_width)->value_double;
        float line_color[4]  = {
            node->line.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->line.line_color.r)->value_double,
            node->line.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->line.line_color.g)->value_double,
            node->line.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->line.line_color.b)->value_double,
            node->line.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->line.line_color.a)->value_double,
        };
        uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
        _ext_draw->add_lines(app_data->pl_layer, points, num_points, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
    }
}

static void _draw_node_panel(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_virtual_dimension[2] = {
        node->panel.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->panel.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};

    // get virtual dimensions
    float virtual_dimension[2] = {
        use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->panel.virtual_dimension.x)->value_double : parent_dimensions->x,
        use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->panel.virtual_dimension.y)->value_double : parent_dimensions->y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform scale
    {
        // compute matrix
        plMat4 scale_xform = pl_mat4_scale_xyz(parent_dimensions->x / virtual_dimension[0], parent_dimensions->y / virtual_dimension[1], 1.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &scale_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // draw children
    plVec2 virtual_dimensions_vec2 = (plVec2){virtual_dimension[0], virtual_dimension[1]};
    plVec2 position_vec2           = (plVec2){0.0f, 0.0f};
    _draw_node_list(app_data, node->panel.child, &position_vec2, &virtual_dimensions_vec2, &transform);
}

static void _draw_node_pixelstream(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // get texture handler
    _Texture   texture    = app_data->sb_textures[node->pixelstream.texture_index];
    plDevice  *pl_device  = _ext_starter->get_device();
    plTexture *pl_texture = _ext_gfx->get_texture(pl_device, texture.texture_handle);

    // switch over types to get the raw image data in the buffer
    switch (node->pixelstream.type) {
        case DC_APP_PIXELSTREAM_TYPE_MJPEG: {

            // don't show anything if disconnected
            if (!dc_ps_mjpeg_server_is_connected(node->pixelstream.mjpeg.handle)) {
                // TODO show disconnected picture here instead
                // (or maybe not....I think it's better to let the user display whatever they feel)
                break;
            }

            // get latest frame data here
            if (dc_ps_mjpeg_server_has_new_data(node->pixelstream.mjpeg.handle)) {

                // get raw data from server
                size_t jpeg_size;
                dc_ps_mjpeg_get_server_data(node->pixelstream.mjpeg.handle, node->pixelstream.mjpeg.raw_jpeg, node->pixelstream.mjpeg.raw_jpeg_size, &jpeg_size);

                // free prior image data
                free(node->pixelstream.frame);

                // convert to raw image format
                int channels;
                node->pixelstream.frame = _ext_image->load(node->pixelstream.mjpeg.raw_jpeg, (int)jpeg_size, &node->pixelstream.frame_width, &node->pixelstream.frame_height, &channels, 4);

                // check size
                if (node->pixelstream.frame_height * node->pixelstream.frame_width > _NODE_PIXELSTREAM_MAX_WIDTH * _NODE_PIXELSTREAM_MAX_HEIGHT) {
                    fprintf(stderr, "DCApp draw_node() pixelstream: max image dimensions exceeded\n");
                }

                break;
            }
        }
        case DC_APP_PIXELSTREAM_TYPE_DYNAMIC_FILE: {
            // TODO implement
            break;
        }
        default:
            fprintf(stderr, "DCApp _draw_node(): unknown pixelstram type of %d\n", node->pixelstream.type);
            break;
    }

    // don't draw anything if no data
    if (node->pixelstream.frame == NULL) {
        return;
    }

    // copy to GPU image
    {
        // get device
        plDevice *device = _ext_starter->get_device();

        // set the initial pl_texture usage (this is a no-op in metal but does layout transition for vulkan)
        plBlitEncoder *encoder = _ext_starter->get_blit_encoder();
        _ext_gfx->set_texture_usage(encoder, texture.texture_handle, PL_TEXTURE_USAGE_SAMPLED, 0);

        // copy memory to mapped staging buffer
        plBuffer *staging_buffer = _ext_gfx->get_buffer(device, app_data->pl_staging_buffer_handle);
        memcpy(staging_buffer->tMemoryAllocation.pHostMapped, node->pixelstream.frame, node->pixelstream.frame_width * node->pixelstream.frame_height * 4);

        // copy staging buffer to image
        plBufferImageCopy buffer_image_copy;
        memset(&buffer_image_copy, 0, sizeof(plBufferImageCopy));
        buffer_image_copy.uImageWidth    = (uint32_t)node->pixelstream.frame_width;
        buffer_image_copy.uImageHeight   = (uint32_t)node->pixelstream.frame_height;
        buffer_image_copy.uImageDepth    = 1;
        buffer_image_copy.uLayerCount    = 1;
        buffer_image_copy.szBufferOffset = 0;
        _ext_gfx->copy_buffer_to_texture(encoder, app_data->pl_staging_buffer_handle, texture.texture_handle, 1, &buffer_image_copy);

        // return encoder
        _ext_starter->return_blit_encoder(encoder);
    }

    // boolean checks
    bool use_dimension[2] = {
        node->pixelstream.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->pixelstream.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation       = node->pixelstream.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->pixelstream.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->pixelstream.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.dimension.y)->value_double : parent_dimensions->y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->pixelstream.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.local_align.x)->value_integer,
            node->pixelstream.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2];
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        // boolean check
        bool use_position[2] = {
            node->pixelstream.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->pixelstream.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        // get position
        float position[2];
        if (use_position[0]) {
            position[0] = (float)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.position.x)->value_double;
        } else {
            DcAppAlignType parent_align_x = node->pixelstream.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->pixelstream.parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    position[0] = parent_dimensions->x;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() pixelstream: Invalid parent_align_x value %d\n", parent_align_x);
                    break;
            }

            // add parent offset
            position[0] += parent_position->x;
        }
        if (use_position[1]) {
            position[1] = (float)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.position.y)->value_double;
        } else {
            DcAppAlignType parent_align_y = node->pixelstream.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->pixelstream.parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    position[1] = parent_dimensions->y;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() pixelstream: Invalid parent_align_y value %d\n", parent_align_y);
                    break;
            }

            // add parent offset
            position[1] += parent_position->y;
        }

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->pixelstream.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.pivot_local_align.x)->value_integer,
                node->pixelstream.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2];
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <pixelstream> draw call: %d\n", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <pixelstream> draw call: %d\n", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // PL specific fixes
    {
        // move from top-left reference to bottom-left
        plMat4 trans_pl_origin_xform = pl_mat4_translate_xyz(0, dimension[1], 0.0f);

        // flip over the y axis
        plMat4 scale_invert_y_xform = pl_mat4_scale_xyz(1.0f, -1.0f, 1.0f);

        // apply transforms
        transform = pl_mul_mat4t(&transform, &trans_pl_origin_xform);
        transform = pl_mul_mat4t(&transform, &scale_invert_y_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // compute UV coordinates
    plVec3 pl_texture_dimensions = pl_texture->tDesc.tDimensions;
    plVec2 min_uv                = {
        0.0f,
        0.0f};
    plVec2 max_uv = {
        ((float)node->pixelstream.frame_width) / pl_texture_dimensions.x,
        ((float)node->pixelstream.frame_height) / pl_texture_dimensions.y};
    plVec2 uv0 = (plVec2){min_uv.x, min_uv.y};
    plVec2 uv1 = (plVec2){min_uv.x, max_uv.y};
    plVec2 uv2 = (plVec2){max_uv.x, max_uv.y};
    plVec2 uv3 = (plVec2){max_uv.x, min_uv.y};

    // compute points
    plVec4 point0_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){0.0f, 0.0f, 0.0f, 1.0f});
    plVec4 point1_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){0.0f, dimension[1], 0.0f, 1.0f});
    plVec4 point2_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){dimension[0], dimension[1], 0.0f, 1.0f});
    plVec4 point3_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){dimension[0], 0.0f, 0.0f, 1.0f});
    plVec2 point0      = (plVec2){point0_vec4.x, point0_vec4.y};
    plVec2 point1      = (plVec2){point1_vec4.x, point1_vec4.y};
    plVec2 point2      = (plVec2){point2_vec4.x, point2_vec4.y};
    plVec2 point3      = (plVec2){point3_vec4.x, point3_vec4.y};

    // draw
    plBindGroupHandle bind_group_handle = app_data->sb_textures[node->image.texture_index].bind_group_handle;
    _ext_draw->add_image_quad_ex(app_data->pl_layer, bind_group_handle.uData, point0, point1, point2, point3, uv0, uv1, uv2, uv3, 0xFFFFFFFF);

    // mouse events
    if (node->pixelstream.mouse_events.enabled) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            app_data->frame_data.mouse_position.x,
            app_data->frame_data.mouse_position.y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        bool inside = mouse_position.x > 0 && mouse_position.x < dimension[0] && mouse_position.y > 0 && mouse_position.y < dimension[1];

        // update global states
        if (inside) {
            app_data->frame_data.next_hovered_node = node_index;

            if (app_data->frame_data.is_mouse_pressed) {
                app_data->frame_data.next_pressed_node = node_index;
            }
        }

        // draw mouse events
        plVec2 position   = (plVec2){0.0f, 0.0f};
        plVec2 dimensions = (plVec2){dimension[0], dimension[1]};
        if (app_data->frame_data.pressed_node == node_index) {
            _draw_node_list(app_data, node->pixelstream.mouse_events.pressed, &position, &dimensions, &transform);
        } else if (app_data->frame_data.active_node == node_index) {
            _draw_node_list(app_data, node->pixelstream.mouse_events.active, &position, &dimensions, &transform);
        } else if (app_data->frame_data.released_node == node_index) {
            _draw_node_list(app_data, node->pixelstream.mouse_events.released, &position, &dimensions, &transform);
        } else if (app_data->frame_data.hovered_node == node_index) {
            _draw_node_list(app_data, node->pixelstream.mouse_events.hovered, &position, &dimensions, &transform);
        } else {
            _draw_node_list(app_data, node->pixelstream.mouse_events.inactive, &position, &dimensions, &transform);
        }
    }
}

static void _draw_node_polygon(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_rotation       = node->polygon.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->polygon.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->polygon.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->polygon.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform position
    {
        // boolean check
        bool use_position[2] = {
            node->polygon.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->polygon.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        // get position
        float position[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->line.position.x)->value_double : 0.0f,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->line.position.y)->value_double : 0.0f,
        };

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // get points, min/max
    plVec2 min_pos    = (plVec2){FLT_MAX, FLT_MAX};
    plVec2 max_pos    = (plVec2){FLT_MIN, FLT_MIN};
    int    num_points = sbcount(node->polygon.sb_points);
    plVec2 raw_points[_NODE_POLYGON_MAX_POINTS];
    plVec2 points[_NODE_POLYGON_MAX_POINTS];
    for (int ii = 0; ii < num_points; ii++) {

        // get raw point
        raw_points[ii] = (plVec2){
            (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.sb_points[ii].x)->value_double,
            (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.sb_points[ii].y)->value_double};

        // update max/min
        min_pos.x = fminf(min_pos.x, raw_points[ii].x);
        min_pos.y = fminf(min_pos.y, raw_points[ii].y);
        max_pos.x = fmaxf(max_pos.x, raw_points[ii].x);
        max_pos.y = fmaxf(max_pos.y, raw_points[ii].y);

        // transform point
        plVec4 point4 = (plVec4){
            raw_points[ii].x,
            raw_points[ii].y,
            0, 1};
        point4     = pl_mul_mat4_vec4(&transform, point4);
        points[ii] = (plVec2){point4.x, point4.y};
    }

    // draw fill
    if (node->polygon.fill_enabled) {

        float fill_color[4] = {
            node->polygon.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.fill_color.r)->value_double,
            node->polygon.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.fill_color.g)->value_double,
            node->polygon.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.fill_color.b)->value_double,
            node->polygon.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.fill_color.a)->value_double,
        };
        uint32_t pl_fill_color = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
        _ext_draw->add_convex_polygon_filled(app_data->pl_layer, points, num_points, (plDrawSolidOptions){.uColor = pl_fill_color});
    }

    // draw outline
    if (node->polygon.line_enabled) {
        float line_thickness = (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.line_width)->value_double;
        float line_color[4]  = {
            node->polygon.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.line_color.r)->value_double,
            node->polygon.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.line_color.g)->value_double,
            node->polygon.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.line_color.b)->value_double,
            node->polygon.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.line_color.a)->value_double,
        };
        uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
        _ext_draw->add_polygon(app_data->pl_layer, points, num_points, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
    }

    // mouse events
    if (node->polygon.mouse_events.enabled) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            app_data->frame_data.mouse_position.x,
            app_data->frame_data.mouse_position.y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        // first do the simple check to make sure it's even within the bounds (for performance)
        bool inside = false;
        if (mouse_position.x > min_pos.x && mouse_position.x < max_pos.x && mouse_position.y > min_pos.y && mouse_position.y < max_pos.y) {

            // now do the actual check
            for (int ii = 0, jj = num_points - 1; ii < num_points; jj = ii++) {
                double xi = raw_points[ii].x, yi = raw_points[ii].y;
                double xj = raw_points[jj].x, yj = raw_points[jj].y;

                bool intersect = ((yi > mouse_position.y) != (yj > mouse_position.y)) && (mouse_position.x < (xj - xi) * (mouse_position.y - yi) / (yj - yi + 1e-12) + xi);
                if (intersect) {
                    inside = !inside;
                }
            }
        }

        // update global states
        if (inside) {
            app_data->frame_data.next_hovered_node = node_index;

            if (app_data->frame_data.is_mouse_pressed) {
                app_data->frame_data.next_pressed_node = node_index;
            }
        }

        // draw mouse events
        plVec2 position   = (plVec2){min_pos.x, min_pos.y};
        plVec2 dimensions = (plVec2){max_pos.x - min_pos.x, max_pos.y - min_pos.y};
        if (app_data->frame_data.pressed_node == node_index) {
            _draw_node_list(app_data, node->polygon.mouse_events.pressed, &position, &dimensions, &transform);
        } else if (app_data->frame_data.active_node == node_index) {
            _draw_node_list(app_data, node->polygon.mouse_events.active, &position, &dimensions, &transform);
        } else if (app_data->frame_data.released_node == node_index) {
            _draw_node_list(app_data, node->polygon.mouse_events.released, &position, &dimensions, &transform);
        } else if (app_data->frame_data.hovered_node == node_index) {
            _draw_node_list(app_data, node->polygon.mouse_events.hovered, &position, &dimensions, &transform);
        } else {
            _draw_node_list(app_data, node->polygon.mouse_events.inactive, &position, &dimensions, &transform);
        }
    }
}

static void _draw_node_rectangle(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_dimension[2] = {
        node->rectangle.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->rectangle.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation       = node->rectangle.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->rectangle.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->rectangle.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.dimension.y)->value_double : parent_dimensions->y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->rectangle.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->rectangle.local_align.x)->value_integer,
            node->rectangle.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->rectangle.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2];
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        // boolean check
        bool use_position[2] = {
            node->rectangle.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->rectangle.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        // get position
        float position[2];
        if (use_position[0]) {
            position[0] = (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.position.x)->value_double;
        } else {
            DcAppAlignType parent_align_x = node->rectangle.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->rectangle.parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    position[0] = parent_dimensions->x;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() rectangle: Invalid parent_align_x value %d\n", parent_align_x);
                    break;
            }

            // add parent offset
            position[0] += parent_position->x;
        }
        if (use_position[1]) {
            position[1] = (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.position.y)->value_double;
        } else {
            DcAppAlignType parent_align_y = node->rectangle.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->rectangle.parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    position[1] = parent_dimensions->y;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() rectangle: Invalid parent_align_y value %d\n", parent_align_y);
                    break;
            }

            // add parent offset
            position[1] += parent_position->y;
        }

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->rectangle.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->rectangle.pivot_local_align.x)->value_integer,
                node->rectangle.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->rectangle.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2];
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <rectangle> draw call: %d\n", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <rectangle> draw call: %d\n", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // get points
    plVec2 raw_points[4] = {
        (plVec2){0.0f, 0.0f},
        (plVec2){dimension[0], 0.0f},
        (plVec2){dimension[0], dimension[1]},
        (plVec2){0.0f, dimension[1]}};

    // transform points
    plVec2 points[4];
    for (int ii = 0; ii < 4; ii++) {
        plVec4 point4 = (plVec4){
            raw_points[ii].x,
            raw_points[ii].y,
            0, 1};
        point4     = pl_mul_mat4_vec4(&transform, point4);
        points[ii] = (plVec2){point4.x, point4.y};
    }

    // draw fill
    if (node->rectangle.fill_enabled) {

        float fill_color[4] = {
            node->rectangle.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.fill_color.r)->value_double,
            node->rectangle.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.fill_color.g)->value_double,
            node->rectangle.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.fill_color.b)->value_double,
            node->rectangle.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.fill_color.a)->value_double,
        };
        uint32_t pl_fill_color = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
        _ext_draw->add_convex_polygon_filled(app_data->pl_layer, points, 4, (plDrawSolidOptions){.uColor = pl_fill_color});
    }

    // draw outline
    if (node->rectangle.line_enabled) {
        float line_thickness = (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.line_width)->value_double;
        float line_color[4]  = {
            node->rectangle.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.line_color.r)->value_double,
            node->rectangle.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.line_color.g)->value_double,
            node->rectangle.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.line_color.b)->value_double,
            node->rectangle.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.line_color.a)->value_double,
        };
        uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
        _ext_draw->add_polygon(app_data->pl_layer, points, 4, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
    }

    // mouse events
    if (node->rectangle.mouse_events.enabled) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            app_data->frame_data.mouse_position.x,
            app_data->frame_data.mouse_position.y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        bool inside = mouse_position.x > 0 && mouse_position.x < dimension[0] && mouse_position.y > 0 && mouse_position.y < dimension[1];

        // update global states
        if (inside) {
            app_data->frame_data.next_hovered_node = node_index;

            if (app_data->frame_data.is_mouse_pressed) {
                app_data->frame_data.next_pressed_node = node_index;
            }
        }

        // draw mouse events
        plVec2 position   = (plVec2){0.0f, 0.0f};
        plVec2 dimensions = (plVec2){dimension[0], dimension[1]};
        if (app_data->frame_data.pressed_node == node_index) {
            _draw_node_list(app_data, node->rectangle.mouse_events.pressed, &position, &dimensions, &transform);
        } else if (app_data->frame_data.active_node == node_index) {
            _draw_node_list(app_data, node->rectangle.mouse_events.active, &position, &dimensions, &transform);
        } else if (app_data->frame_data.released_node == node_index) {
            _draw_node_list(app_data, node->rectangle.mouse_events.released, &position, &dimensions, &transform);
        } else if (app_data->frame_data.hovered_node == node_index) {
            _draw_node_list(app_data, node->rectangle.mouse_events.hovered, &position, &dimensions, &transform);
        } else {
            _draw_node_list(app_data, node->rectangle.mouse_events.inactive, &position, &dimensions, &transform);
        }
    }
}

static void _draw_node_set(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    DcValue *var_value = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, node->set.var_index)->value_index);
    DcValue *op_value  = dc_app_lookup_get_value(app_data->lookup, node->set.operand);

    // get operation
    DcAppSetType operation = node->set.operation == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_SET_TYPE_UNDEFINED : (DcAppSetType)(dc_app_lookup_get_value(app_data->lookup, node->set.operation)->value_integer);

    // apply operation
    switch (operation) {
        case DC_APP_SET_TYPE_UNDEFINED:
        case DC_APP_SET_TYPE_EQUAL:
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    var_value->value_string = op_value->value_string;
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean = op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_ADD:
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING: {
                    int num_chars = DC_VALUE_STRING_BUFFER_SIZE - (int)strlen(var_value->value_string) - 1;
                    strncat(var_value->value_string, op_value->value_string, num_chars);
                    break;
                }
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer += op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double += op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean += op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;
        case DC_APP_SET_TYPE_SUBTRACT:
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer -= op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double -= op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean -= op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;
        case DC_APP_SET_TYPE_MULTIPLY:
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer *= op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double *= op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean *= op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;
        case DC_APP_SET_TYPE_DIVIDE:
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer /= op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double /= op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean /= op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;
        default:
            fprintf(stderr, "Invalid <Set> Operator value of enum %d\n", operation);
            break;
    }

    // refresh variable
    dc_value_refresh(var_value);
}

static void _draw_node_terrain(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_dimension[2] = {
        node->terrain.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->terrain.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation       = node->terrain.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->terrain.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->terrain.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->terrain.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->terrain.dimension.y)->value_double : parent_dimensions->y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->terrain.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->terrain.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->terrain.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->terrain.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->terrain.local_align.x)->value_integer,
            node->terrain.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->terrain.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2];
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * dimension[0] / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * dimension[0];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * dimension[1] / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * dimension[1];
                break;
            default:
                fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        // boolean check
        bool use_position[2] = {
            node->terrain.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->terrain.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        // get position
        float position[2];
        if (use_position[0]) {
            position[0] = (float)dc_app_lookup_get_value(app_data->lookup, node->terrain.position.x)->value_double;
        } else {
            DcAppAlignType parent_align_x = node->terrain.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->terrain.parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    position[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    position[0] = parent_dimensions->x;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() terrain: Invalid parent_align_x value %d\n", parent_align_x);
                    break;
            }

            // add parent offset
            position[0] += parent_position->x;
        }
        if (use_position[1]) {
            position[1] = (float)dc_app_lookup_get_value(app_data->lookup, node->terrain.position.y)->value_double;
        } else {
            DcAppAlignType parent_align_y = node->terrain.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->terrain.parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    position[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    position[1] = parent_dimensions->y;
                    break;
                default:
                    fprintf(stderr, "DCAPP _draw_node() terrain: Invalid parent_align_y value %d\n", parent_align_y);
                    break;
            }

            // add parent offset
            position[1] += parent_position->y;
        }

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->terrain.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->terrain.pivot_local_align.x)->value_integer,
                node->terrain.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->terrain.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2];
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = dimension[0] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = dimension[0];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <terrain> draw call: %d\n", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = dimension[1] / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = dimension[1];
                    break;
                default:
                    fprintf(stderr, "Unknown pivot alignment in <terrain> draw call: %d\n", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->terrain.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // convert to 3D matrix
    plMat3 transform3 = (plMat3){0};
    transform3.x11    = transform.x11;
    transform3.x12    = transform.x12;
    transform3.x13    = transform.x14;
    transform3.x21    = transform.x21;
    transform3.x22    = transform.x22;
    transform3.x23    = transform.x24;
    transform3.x31    = transform.x31;
    transform3.x32    = transform.x32;
    transform3.x33    = transform.x33;

    // update text options
    // text_options.tTransform = transform3;

    // draw
    // ext_draw->add_text(app_data->pl_layer, (plVec2){0, 0}, sb_text, text_options);
}

static void _draw_node_text(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // expand text
    static char *sb_text = NULL;
    sbclear(sb_text);
    for (int ii = 0; ii < sbcount(node->text.sb_vals); ii++) {

        // filler
        char *filler = &(node->text.sb_fillers[node->text.sb_filler_indices[ii]]);
        sbpushn(sb_text, filler, (int)strlen(filler));

        // value
        DcValueType format_type = node->text.sb_format_types[ii];
        char       *format      = &(node->text.sb_formats[node->text.sb_format_indices[ii]]);
        DcValue    *val         = dc_app_lookup_get_value(app_data->lookup, node->text.sb_vals[ii]);
        static char val_str[256]; // assume text won't be that long..
        switch (format_type) {
            case DC_VALUE_TYPE_STRING:
                snprintf(val_str, sizeof(val_str), format, val->value_string);
                break;
            case DC_VALUE_TYPE_INTEGER:
                snprintf(val_str, sizeof(val_str), format, val->value_integer);
                break;
            case DC_VALUE_TYPE_DOUBLE:
                snprintf(val_str, sizeof(val_str), format, val->value_double);
                break;
            case DC_VALUE_TYPE_BOOLEAN:
                snprintf(val_str, sizeof(val_str), format, val->value_boolean);
                break;
            default:
                fprintf(stderr, "Unknown value type for text: %d\n", format_type);
        }
        sbpushn(sb_text, val_str, (int)strlen(val_str));
    }

    // ending filler
    char *filler = &(node->text.sb_fillers[node->text.sb_filler_indices[sbcount(node->text.sb_vals)]]);
    sbpushn(sb_text, filler, (int)strlen(filler));
    sbpush(sb_text, '\0');

    // get text substrings per newline
    size_t subtext_indices[_NODE_TEXT_MAX_LINES];
    size_t num_lines;
    dc_utils_split_string_inplace(sb_text, "\n", subtext_indices, _NODE_TEXT_MAX_LINES, &num_lines);

    // setup text options
    plDrawTextOptions text_options = {0};
    text_options.ptFont            = app_data->pl_vera_sdf_font;
    float fill_color[4]            = {
        node->text.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->text.fill_color.r)->value_double,
        node->text.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->text.fill_color.g)->value_double,
        node->text.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->text.fill_color.b)->value_double,
        node->text.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->text.fill_color.a)->value_double,
    };
    text_options.uColor = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
    text_options.fSize  = node->text.size == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->text.size)->value_double;

    // get each strings size
    plVec2 dimensions[_NODE_TEXT_MAX_LINES];
    plVec2 total_dimensions = {0.0f, 0.0f};
    for (int ii = 0; ii < num_lines; ii++) {
        dimensions[ii] = _ext_draw->calculate_text_size(&sb_text[subtext_indices[ii]], text_options);

        // overwrite the y dimension with the size
        dimensions[ii].y = text_options.fSize;

        // compute totals
        total_dimensions.x = fmaxf(total_dimensions.x, dimensions[ii].x);
        total_dimensions.y += dimensions[ii].y;
    }

    // boolean checks
    bool use_rotation       = node->text.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->text.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->text.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

    // iterate over each string
    // TODO this has some redundant transforms.....clean this up!
    for (int ii = 0; ii < num_lines; ii++) {

        // transform
        plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

        // xform rotation (around a point)
        {
            if (use_rotation && use_pivot_position) {

                // get pivot XY, rotation
                float pivot_position[2] = {
                    (float)dc_app_lookup_get_value(app_data->lookup, node->text.pivot_position.x)->value_double,
                    (float)dc_app_lookup_get_value(app_data->lookup, node->text.pivot_position.y)->value_double};
                float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->text.rotation)->value_double);

                // compute matrices
                plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
                plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
                plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

                // apply transform
                transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
                transform = pl_mul_mat4t(&transform, &rotate_xform);
                transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
            }
        }

        // xform local alignment
        {
            // get alignment
            DcAppAlignType local_aligns[2] = {
                node->text.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->text.local_align.x)->value_integer,
                node->text.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->text.local_align.y)->value_integer};

            // compute offsets
            float trans_align_offsets[2];
            switch (local_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    trans_align_offsets[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    trans_align_offsets[0] = -1 * dimensions[ii].x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    trans_align_offsets[0] = -1 * dimensions[ii].x;
                    break;
                default:
                    fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[0]);
                    break;
            }
            switch (local_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    trans_align_offsets[1] = 0;
                    trans_align_offsets[1] += 0.0f * dimensions[ii].y * (num_lines - 1);
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    trans_align_offsets[1] = -1 * dimensions[ii].y / 2;
                    trans_align_offsets[1] -= 0.5f * dimensions[ii].y * (num_lines - 1);
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    trans_align_offsets[1] = -1 * dimensions[ii].y;
                    trans_align_offsets[1] -= 1.0f * dimensions[ii].y * (num_lines - 1);
                    break;
                default:
                    fprintf(stderr, "Unknown alignment in <Text> draw call: %d\n", local_aligns[1]);
                    break;
            }

            trans_align_offsets[1] -= dimensions[ii].y * ii;

            // compute matrix
            plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
        }

        // xform position
        {
            // boolean check
            bool use_position[2] = {
                node->text.position.x != DC_APP_VAL_INDEX_UNDEFINED,
                node->text.position.y != DC_APP_VAL_INDEX_UNDEFINED};

            // get position
            float position[2];
            if (use_position[0]) {
                position[0] = (float)dc_app_lookup_get_value(app_data->lookup, node->text.position.x)->value_double;
            } else {
                DcAppAlignType parent_align_x = node->text.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->text.parent_align.x)->value_integer;
                switch (parent_align_x) {
                    case DC_APP_ALIGN_TYPE_UNDEFINED:
                    case DC_APP_ALIGN_TYPE_LEFT:
                        position[0] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_CENTER:
                        position[0] = parent_dimensions->x / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_RIGHT:
                        position[0] = parent_dimensions->x;
                        break;
                    default:
                        fprintf(stderr, "DCAPP _draw_node() text: Invalid parent_align_x value %d\n", parent_align_x);
                        break;
                }

                // add parent offset
                position[0] += parent_position->x;
            }
            if (use_position[1]) {
                position[1] = (float)dc_app_lookup_get_value(app_data->lookup, node->text.position.y)->value_double;
            } else {
                DcAppAlignType parent_align_y = node->text.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->text.parent_align.y)->value_integer;
                switch (parent_align_y) {
                    case DC_APP_ALIGN_TYPE_UNDEFINED:
                    case DC_APP_ALIGN_TYPE_BOTTOM:
                        position[1] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_MIDDLE:
                        position[1] = parent_dimensions->y / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_TOP:
                        position[1] = parent_dimensions->y;
                        break;
                    default:
                        fprintf(stderr, "DCAPP _draw_node() text: Invalid parent_align_y value %d\n", parent_align_y);
                        break;
                }

                // add parent offset
                position[1] += parent_position->y;
            }

            // compute matrix
            plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_position_xform);
        }

        // xform local rotation
        {
            if (use_rotation && !use_pivot_position) {

                // get alignment
                DcAppAlignType local_pivot_aligns[2] = {
                    node->text.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->text.pivot_local_align.x)->value_integer,
                    node->text.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->text.pivot_local_align.y)->value_integer};

                // get pivot XY, rotation
                float pivot_position[2];
                switch (local_pivot_aligns[0]) {
                    case DC_APP_ALIGN_TYPE_UNDEFINED:
                    case DC_APP_ALIGN_TYPE_LEFT:
                        pivot_position[0] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_CENTER:
                        pivot_position[0] = total_dimensions.x / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_RIGHT:
                        pivot_position[0] = total_dimensions.x;
                        break;
                    default:
                        fprintf(stderr, "Unknown pivot alignment in <text> draw call: %d\n", local_pivot_aligns[0]);
                        break;
                }
                switch (local_pivot_aligns[1]) {
                    case DC_APP_ALIGN_TYPE_UNDEFINED:
                    case DC_APP_ALIGN_TYPE_BOTTOM:
                        pivot_position[1] = 0;
                        break;
                    case DC_APP_ALIGN_TYPE_MIDDLE:
                        pivot_position[1] = total_dimensions.y / 2;
                        break;
                    case DC_APP_ALIGN_TYPE_TOP:
                        pivot_position[1] = total_dimensions.y;
                        break;
                    default:
                        fprintf(stderr, "Unknown pivot alignment in <text> draw call: %d\n", local_pivot_aligns[1]);
                        break;
                }
                float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->text.rotation)->value_double);

                // compute matrices
                plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
                plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
                plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

                // apply transform
                transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
                transform = pl_mul_mat4t(&transform, &rotate_xform);
                transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
            }
        }

        // PL specific fixes
        {
            // move from top-left reference to bottom-left
            plMat4 trans_pl_origin_xform = pl_mat4_translate_xyz(0, total_dimensions.y, 0.0f);

            // flip over the y axis
            plMat4 scale_invert_y_xform = pl_mat4_scale_xyz(1.0f, -1.0f, 1.0f);

            // apply transforms
            transform = pl_mul_mat4t(&transform, &trans_pl_origin_xform);
            transform = pl_mul_mat4t(&transform, &scale_invert_y_xform);
        }

        // parent transform
        transform = pl_mul_mat4t(parent_transform, &transform);

        // convert to 3D matrix
        plMat3 transform3 = (plMat3){0};
        transform3.x11    = transform.x11;
        transform3.x12    = transform.x12;
        transform3.x13    = transform.x14;
        transform3.x21    = transform.x21;
        transform3.x22    = transform.x22;
        transform3.x23    = transform.x24;
        transform3.x31    = transform.x31;
        transform3.x32    = transform.x32;
        transform3.x33    = transform.x33;

        // update text options
        text_options.tTransform = transform3;

        // draw
        _ext_draw->add_text(app_data->pl_layer, (plVec2){0, 0}, &sb_text[subtext_indices[ii]], text_options);
    }
}

static void _draw_node_window(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // TODO move this code to only the resize() function

    // current dimensions
    uint32_t dimensionX, dimensionY;
    _ext_windows->get_size(app_data->pl_window, &dimensionX, &dimensionY);

    // boolean checks
    bool use_virtual_dimension[2] = {
        node->panel.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->panel.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};

    // all transform parameters
    float dimension[2]         = {(float)dimensionX, (float)dimensionY};
    float virtual_dimension[2] = {
        use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->window.virtual_dimension.x)->value_double : dimension[0],
        use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->window.virtual_dimension.y)->value_double : dimension[1]};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // PL xform translate Y from negative to positive range
    {
        // compute matrix
        plMat4 trans_pl_matrix = pl_mat4_translate_xyz(0.0f, dimension[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_pl_matrix);
    }

    // PL xform flip y axis
    {
        // compute matrix
        plMat4 scale_pl_matrix = pl_mat4_scale_xyz(1.0f, -1.0f, 1.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &scale_pl_matrix);
    }

    // xform scale from virtual to real dimensions,
    {
        // compute matrix
        plMat4 scale_matrix = pl_mat4_scale_xyz(dimension[0] / virtual_dimension[0], dimension[1] / virtual_dimension[1], 1.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &scale_matrix);
    }

    // draw children
    plVec2 position_vec2           = (plVec2){0.0f, 0.0f};
    plVec2 virtual_dimensions_vec2 = (plVec2){virtual_dimension[0], virtual_dimension[1]};
    _draw_node_list(app_data, node->window.child, &position_vec2, &virtual_dimensions_vec2, &transform);
}

#include "_dcapp_utils.c"
