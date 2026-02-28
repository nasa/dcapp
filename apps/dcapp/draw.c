#define _USE_MATH_DEFINES
#include <math.h>

#include "dcapp.h"

#include "app/enums.h"
#include "utils/log.h"
#include "utils/math.h"
#include "utils/string.h"
#include "utils/time.h"

// Forward declarations
static void _draw_node_blink(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_button(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_arc(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_conditional(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_container(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_ellipse(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_function(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_image(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_line(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_panel(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_pixelstream(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_polygon(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_rectangle(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_mouse_motion(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_set(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_sphere(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_button_enabled(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_button_disabled(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_button_indicator_on(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_button_indicator_off(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_button_transition(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_mouse_pressed(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_mouse_released(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_mouse_active(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_mouse_inactive(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_mouse_hovered(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_if_true(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_state_if_false(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_stencil(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_planet_view(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_text(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);
static void _draw_node_window(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform);

// deferred set operations
static bool _apply_set_operation(_AppData *app_data, DcAppVarIndex var_index, DcValue *var_value, DcValue *op_value, DcAppSetType operation);
static void _flush_deferred_sets(_AppData *app_data);

// draw batch utils
static void           _draw_batch_reset(_AppData *app_data);
static plDrawLayer2D *_draw_batch_get_2d(_AppData *app_data);
static plDrawList3D  *_draw_batch_get_3d(_AppData *app_data);

static void _draw_node_list(_AppData *app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *node_transform) {
    _NodeIndex current_node_index = node_index;
    while (current_node_index != NODE_INDEX_UNDEFINED) {
        _draw_node(app_data, current_node_index, parent_position, parent_dimensions, node_transform);
        current_node_index = _get_node(app_data, current_node_index)->next;
    }
}

static void _draw_node(_AppData *app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    if (node_index == NODE_INDEX_UNDEFINED) {
        DC_LOG_ERROR("Draw", "Attempting to draw undefined node index");
    }

    _Node *node = _get_node(app_data, node_index);
    switch (node->type) {
        case NODE_TYPE_ARC:
            _draw_node_arc(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_BLINK:
            _draw_node_blink(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_BUTTON:
            _draw_node_button(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_CONDITIONAL:
            _draw_node_conditional(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_CONTAINER:
            _draw_node_container(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_ELLIPSE:
            _draw_node_ellipse(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_FUNCTION:
            _draw_node_function(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
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

        case NODE_TYPE_MOUSE_MOTION:
            _draw_node_mouse_motion(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_SET:
            _draw_node_set(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_SPHERE:
            _draw_node_sphere(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_STENCIL:
            _draw_node_stencil(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_PLANET_VIEW:
            _draw_node_planet_view(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_TEXT:
            _draw_node_text(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        case NODE_TYPE_WINDOW:
            _draw_node_window(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        // state conditional nodes (check parent's state_flags)
        case NODE_TYPE_STATE_BUTTON_ENABLED:
            _draw_node_state_button_enabled(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;
        case NODE_TYPE_STATE_BUTTON_DISABLED:
            _draw_node_state_button_disabled(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;
        case NODE_TYPE_STATE_BUTTON_INDICATOR_ON:
            _draw_node_state_button_indicator_on(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;
        case NODE_TYPE_STATE_BUTTON_INDICATOR_OFF:
            _draw_node_state_button_indicator_off(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;
        case NODE_TYPE_STATE_BUTTON_TRANSITION:
            _draw_node_state_button_transition(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;
        case NODE_TYPE_STATE_MOUSE_PRESSED:
            _draw_node_state_mouse_pressed(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;
        case NODE_TYPE_STATE_MOUSE_RELEASED:
            _draw_node_state_mouse_released(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;
        case NODE_TYPE_STATE_MOUSE_ACTIVE:
            _draw_node_state_mouse_active(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;
        case NODE_TYPE_STATE_MOUSE_INACTIVE:
            _draw_node_state_mouse_inactive(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;
        case NODE_TYPE_STATE_MOUSE_HOVERED:
            _draw_node_state_mouse_hovered(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;
        case NODE_TYPE_STATE_IF_TRUE:
            _draw_node_state_if_true(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;
        case NODE_TYPE_STATE_IF_FALSE:
            _draw_node_state_if_false(app_data, node_index, node, parent_position, parent_dimensions, parent_transform);
            break;

        default:
            break;
    }
}

static void _draw_node_blink(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    double current_time = dc_utils_time_get();
    double frequency    = dc_app_lookup_get_value(app_data->lookup, node->blink.frequency)->value_double;
    double duty_cycle   = dc_app_lookup_get_value(app_data->lookup, node->blink.duty_cycle)->value_double;
    double duration     = dc_app_lookup_get_value(app_data->lookup, node->blink.duration)->value_double;

    // check trigger variable for any change
    if (node->blink.fire_blink != DC_APP_VAL_INDEX_UNDEFINED) {
        DcValue *val = dc_app_lookup_get_value(app_data->lookup, node->blink.fire_blink);
        if (!dc_value_is_equal(val, &node->blink.last_fire_blink_value)) {
            if (duration <= 0.0) {
                // indefinite: toggle (0 <-> 1)
                node->blink.remaining_duration = 1.0 - node->blink.remaining_duration;
            } else {
                // timed: restart
                node->blink.remaining_duration = duration;
            }
            node->blink.last_fire_blink_value = *val;
        }
    }

    // calculate blink state
    bool blink_state = true;
    if (node->blink.remaining_duration > 0.0) {
        double period  = (frequency > 0.0) ? (1.0 / frequency) : 1.0;
        double on_time = period * duty_cycle;
        blink_state = (fmod(current_time, period) <= on_time);

        // countdown for timed blinks
        if (duration > 0.0) {
            node->blink.remaining_duration -= (current_time - node->blink.last_frame_time);
            if (node->blink.remaining_duration <= 0.0) {
                blink_state = true;
            }
        }
    }

    if (blink_state) {
        _draw_node_list(app_data, node->blink.child, parent_position, parent_dimensions, parent_transform);
    }

    node->blink.last_frame_time = current_time;
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
    bool use_pivot_parent_align = (node->button.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->button.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

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
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->button.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->button.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->button.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->button.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2];
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
                case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
                case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
                default: DC_LOG_WARN("Button", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
                case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
                case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
                default: DC_LOG_WARN("Button", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->button.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

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
                DC_LOG_WARN("Button", "Unknown X alignment: %d", local_aligns[0]);
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
                DC_LOG_WARN("Button", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->button.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->button.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float anchor[2] = {0, 0};
        DcAppAlignType parent_align_x = node->button.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->button.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Button", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->button.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->button.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Button", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->button.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->button.position.y)->value_double : 0};

        // apply negate
        if (node->button.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->button.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->button.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->button.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        // compute matrix
        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

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
                    DC_LOG_WARN("Button", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
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
                    DC_LOG_WARN("Button", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
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
    bool is_enabled = true; // default to enabled
    if (node->button.var_enabled != DC_APP_VAR_INDEX_UNDEFINED) {
        DcValue *enabled_var_value = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, node->button.var_enabled)->value_index);
        DcValue *enabled_on_value  = dc_app_lookup_get_value(app_data->lookup, node->button.val_enabled_on);
        is_enabled = dc_value_is_equal(enabled_var_value, enabled_on_value);
    }

    // determine indicator
    DcValue *indicator_var_value = NULL;
    bool     is_indicator_on     = false; // default to off
    if (node->button.var_indicator != DC_APP_VAR_INDEX_UNDEFINED) {
        indicator_var_value      = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, node->button.var_indicator)->value_index);
        DcValue *indicator_on_value = dc_app_lookup_get_value(app_data->lookup, node->button.val_indicator_on);
        is_indicator_on = dc_value_is_equal(indicator_var_value, indicator_on_value);
    }

    // determine transition
    DcValue *target_var_value = NULL;
    bool     is_transitioning = false; // default to not transitioning
    if (node->button.var_target != DC_APP_VAR_INDEX_UNDEFINED) {
        target_var_value = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, node->button.var_target)->value_index);
        if (indicator_var_value) {
            is_transitioning = dc_value_is_not_equal(target_var_value, indicator_var_value);
        }
    }

    // process mouse event states
    bool is_pressed  = (app_data->frame_data.pressed_node == node_index);
    bool is_released = (app_data->frame_data.released_node == node_index);

    // process mouse events per button type (only if target variable is defined)
    if (target_var_value) {
        DcValue *target_on_value  = dc_app_lookup_get_value(app_data->lookup, node->button.val_target_on);
        DcValue *target_off_value = dc_app_lookup_get_value(app_data->lookup, node->button.val_target_off);
        switch (node->button.type) {

            // toggle: flip based on current indicator state (on release)
            case DC_APP_BUTTON_TYPE_TOGGLE:
                if (is_pressed) {
                    if (is_indicator_on) {
                        *target_var_value = *target_off_value;
                    } else {
                        *target_var_value = *target_on_value;
                    }
                }
                break;

            // momentary: flip on press/release
            case DC_APP_BUTTON_TYPE_MOMENTARY:
                if (is_pressed) {
                    *target_var_value = *target_on_value;
                } else if (is_released) {
                    *target_var_value = *target_off_value;
                }
                break;

            // standard: set to on value on release
            case DC_APP_BUTTON_TYPE_STANDARD:
                if (is_pressed) {
                    *target_var_value = *target_on_value;
                }
                break;

            default:
                break;
        }
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

    // set state flags for conditional children to check
    node->button.state_flags = NODE_STATE_FLAG_NONE;
    if (is_enabled) {
        node->button.state_flags |= NODE_STATE_FLAG_ENABLED;
    }
    if (is_indicator_on) {
        node->button.state_flags |= NODE_STATE_FLAG_INDICATOR_ON;
    }
    if (is_transitioning) {
        node->button.state_flags |= NODE_STATE_FLAG_TRANSITIONING;
    }
    if (is_pressed) {
        node->button.state_flags |= NODE_STATE_FLAG_PRESSED;
    }
    if (app_data->frame_data.active_node == node_index) {
        node->button.state_flags |= NODE_STATE_FLAG_ACTIVE;
    }
    if (app_data->frame_data.hovered_node == node_index) {
        node->button.state_flags |= NODE_STATE_FLAG_HOVERED;
    }
    if (is_released) {
        node->button.state_flags |= NODE_STATE_FLAG_RELEASED;
    }

    // draw children (includes state conditional nodes that check state_flags)
    plVec2 child_position   = (plVec2){0.0f, 0.0f};
    plVec2 child_dimensions = (plVec2){virtual_dimension[0], virtual_dimension[1]};
    _draw_node_list(app_data, node->button.child, &child_position, &child_dimensions, &transform);
}

// Helper to get parent's state_flags (returns NODE_STATE_FLAG_NONE if parent doesn't have state_flags)
static uint32_t _get_parent_state_flags(_AppData *app_data, _Node *node) {
    _Node *parent_node = _get_node(app_data, node->parent);
    if (!parent_node) return NODE_STATE_FLAG_NONE;

    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
            return parent_node->button.state_flags;
        case NODE_TYPE_CONTAINER:
            return parent_node->container.state_flags;
        case NODE_TYPE_ELLIPSE:
            return parent_node->ellipse.state_flags;
        case NODE_TYPE_IMAGE:
            return parent_node->image.state_flags;
        case NODE_TYPE_PIXELSTREAM:
            return parent_node->pixelstream.state_flags;
        case NODE_TYPE_POLYGON:
            return parent_node->polygon.state_flags;
        case NODE_TYPE_RECTANGLE:
            return parent_node->rectangle.state_flags;
        case NODE_TYPE_CONDITIONAL:
            return parent_node->conditional.state_flags;
        default:
            return NODE_STATE_FLAG_NONE;
    }
}

static void _draw_node_state_button_enabled(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    if (flags & NODE_STATE_FLAG_ENABLED) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_state_button_disabled(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    if (!(flags & NODE_STATE_FLAG_ENABLED)) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_state_button_indicator_on(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    if (!(flags & NODE_STATE_FLAG_TRANSITIONING) && (flags & NODE_STATE_FLAG_INDICATOR_ON)) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_state_button_indicator_off(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    if (!(flags & NODE_STATE_FLAG_TRANSITIONING) && !(flags & NODE_STATE_FLAG_INDICATOR_ON)) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_state_button_transition(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    if (flags & NODE_STATE_FLAG_TRANSITIONING) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_state_mouse_pressed(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    if (flags & NODE_STATE_FLAG_PRESSED) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_state_mouse_released(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    if (flags & NODE_STATE_FLAG_RELEASED) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_state_mouse_active(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    if (flags & NODE_STATE_FLAG_ACTIVE) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_state_mouse_inactive(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    // Inactive when not active, not hovered, not pressed
    if (!(flags & NODE_STATE_FLAG_ACTIVE) && !(flags & NODE_STATE_FLAG_HOVERED) && !(flags & NODE_STATE_FLAG_PRESSED)) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_state_mouse_hovered(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    if (flags & NODE_STATE_FLAG_HOVERED) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_state_if_true(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    if (flags & NODE_STATE_FLAG_TRUE) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

static void _draw_node_state_if_false(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    uint32_t flags = _get_parent_state_flags(app_data, node);
    if (flags & NODE_STATE_FLAG_FALSE) {
        _draw_node_list(app_data, node->state_event.child, parent_position, parent_dimensions, parent_transform);
    }
}

#define _NODE_ARC_MAX_SEGMENTS 200
static void _draw_node_arc(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_radius         = node->arc.radius != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_rotation       = node->arc.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->arc.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->arc.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->arc.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->arc.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get radius
    float radius;
    if (use_radius) {
        radius = (float)dc_app_lookup_get_value(app_data->lookup, node->arc.radius)->value_double;
    } else {
        radius = fminf(parent_dimensions->x, parent_dimensions->y) / 2;
    }

    // get angle span and rotation
    float angle_span = (float)dc_app_lookup_get_value(app_data->lookup, node->arc.angle)->value_double;
    float arc_rotation = use_rotation ? (float)dc_app_lookup_get_value(app_data->lookup, node->arc.rotation)->value_double : 0.0f;

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    if (use_rotation && use_pivot_position) {
        float pivot_position[2] = {
            (float)dc_app_lookup_get_value(app_data->lookup, node->arc.pivot_position.x)->value_double,
            (float)dc_app_lookup_get_value(app_data->lookup, node->arc.pivot_position.y)->value_double};
        float rotation = pl_radiansf(arc_rotation);

        plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
        plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
        plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

        transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
        transform = pl_mul_mat4t(&transform, &rotate_xform);
        transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
    } else if (use_rotation && use_pivot_parent_align) {

        DcAppAlignType parent_pivot_aligns[2] = {
            node->arc.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->arc.pivot_parent_align.x)->value_integer
                : DC_APP_ALIGN_TYPE_UNDEFINED,
            node->arc.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->arc.pivot_parent_align.y)->value_integer
                : DC_APP_ALIGN_TYPE_UNDEFINED};

        float pivot_position[2];
        switch (parent_pivot_aligns[0]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
            case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
            case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
            default: DC_LOG_WARN("Arc", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
        }
        switch (parent_pivot_aligns[1]) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
            case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
            case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
            default: DC_LOG_WARN("Arc", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
        }
        float rotation = pl_radiansf(arc_rotation);

        plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
        plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
        plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

        transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
        transform = pl_mul_mat4t(&transform, &rotate_xform);
        transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
    }

    // xform local alignment
    {
        float diameter = 2 * radius;

        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->arc.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->arc.local_align.x)->value_integer,
            node->arc.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->arc.local_align.y)->value_integer};

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
                DC_LOG_WARN("Arc", "Unknown X alignment: %d", local_aligns[0]);
                trans_align_offsets[0] = -1 * diameter / 2;
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
                DC_LOG_WARN("Arc", "Unknown Y alignment: %d", local_aligns[1]);
                trans_align_offsets[1] = -1 * diameter / 2;
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->arc.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->arc.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float anchor[2] = {0, 0};
        DcAppAlignType parent_align_x = node->arc.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->arc.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                anchor[0] = 0;
                break;
        }
        DcAppAlignType parent_align_y = node->arc.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->arc.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                anchor[1] = 0;
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->arc.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->arc.position.y)->value_double : 0};

        // apply negate
        if (node->arc.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->arc.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->arc.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->arc.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {
        float diameter = 2 * radius;

        // get alignment
        DcAppAlignType local_pivot_aligns[2] = {
            node->arc.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->arc.pivot_local_align.x)->value_integer,
            node->arc.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->arc.pivot_local_align.y)->value_integer};

        // get pivot XY
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
                DC_LOG_WARN("Arc", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                pivot_position[0] = diameter / 2;
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
                DC_LOG_WARN("Arc", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                pivot_position[1] = diameter / 2;
                break;
        }
        float rotation = pl_radiansf(arc_rotation);

        // compute matrices
        plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
        plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
        plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
        transform = pl_mul_mat4t(&transform, &rotate_xform);
        transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // calculate arc points
    // Corner-based: points range from (0,0) to (diameter, diameter), center at (radius, radius)
    // Arc starts at 3 o'clock (standard math convention), rotation handled by transform matrix
    // LocalAlign (default=CENTER) shifts by -radius to center at position
    int num_segments = node->arc.num_segments == DC_APP_VAL_INDEX_UNDEFINED ?
        (int)(angle_span / 3.0f) + 2 : // roughly 1 segment per 3 degrees
        (int)dc_app_lookup_get_value(app_data->lookup, node->arc.num_segments)->value_double;
    if (num_segments < 2) num_segments = 2;
    if (num_segments > _NODE_ARC_MAX_SEGMENTS - 1) num_segments = _NODE_ARC_MAX_SEGMENTS - 1;

    // Convert to radians
    float span_rad = pl_radiansf(angle_span);

    // Generate arc points (start-based)
    // Arc starts at 3 o'clock (standard math convention), rotation handled by transform matrix
    plVec2 points[_NODE_ARC_MAX_SEGMENTS];
    int num_arc_points = num_segments + 1; // segments + 1 = number of vertices on arc

    for (int ii = 0; ii <= num_segments; ii++) {
        float t = (float)ii / (float)num_segments; // 0 to 1
        float angle = t * span_rad; // from 0 to span
        float final_angle = angle;

        // Corner-based: center at (radius, radius), points from 0 to diameter
        plVec4 point4 = (plVec4){
            radius * (1.0f + cosf(final_angle)),
            radius * (1.0f + sinf(final_angle)),
            0, 1};
        point4 = pl_mul_mat4_vec4(&transform, point4);
        points[ii] = (plVec2){point4.x, point4.y};
    }

    // draw arc line
    float line_thickness = node->arc.line_width == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->arc.line_width)->value_double;
    float line_color[4] = {
        node->arc.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->arc.line_color.r)->value_double,
        node->arc.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->arc.line_color.g)->value_double,
        node->arc.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->arc.line_color.b)->value_double,
        node->arc.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->arc.line_color.a)->value_double,
    };
    uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
    _ext_draw->add_lines(_draw_batch_get_2d(app_data), points, num_arc_points, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
}

static void _draw_node_ellipse(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_radius_x       = node->ellipse.radius_x != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_radius_y       = node->ellipse.radius_y != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_rotation       = node->ellipse.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->ellipse.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->ellipse.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->ellipse.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->ellipse.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float radius_x, radius_y, diameter_x, diameter_y;
    if (use_radius_x) {
        radius_x   = (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.radius_x)->value_double;
        diameter_x = 2 * radius_x;
    } else {
        diameter_x = parent_dimensions->x;
        radius_x   = diameter_x / 2;
    }
    if (use_radius_y) {
        radius_y   = (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.radius_y)->value_double;
        diameter_y = 2 * radius_y;
    } else {
        diameter_y = parent_dimensions->y;
        radius_y   = diameter_y / 2;
    }

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            // get pivot XY, rotation
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.rotation)->value_double);

            // compute matrices
            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            // apply transform
            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->ellipse.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->ellipse.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->ellipse.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->ellipse.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2];
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
                case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
                case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
                default: DC_LOG_WARN("Ellipse", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
                case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
                case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
                default: DC_LOG_WARN("Ellipse", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        // get alignment
        DcAppAlignType local_aligns[2] = {
            node->ellipse.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->ellipse.local_align.x)->value_integer,
            node->ellipse.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->ellipse.local_align.y)->value_integer};

        // compute offsets
        float trans_align_offsets[2];
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = -1 * diameter_x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -1 * diameter_x;
                break;
            default:
                DC_LOG_WARN("Ellipse", "Unknown X alignment: %d", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = -1 * diameter_y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -1 * diameter_y;
                break;
            default:
                DC_LOG_WARN("Ellipse", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->ellipse.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->ellipse.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float anchor[2] = {0, 0};
        DcAppAlignType parent_align_x = node->ellipse.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->ellipse.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Ellipse", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->ellipse.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->ellipse.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Ellipse", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.position.y)->value_double : 0};

        // apply negate
        if (node->ellipse.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->ellipse.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->ellipse.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->ellipse.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

            // get alignment
            DcAppAlignType local_pivot_aligns[2] = {
                node->ellipse.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->ellipse.pivot_local_align.x)->value_integer,
                node->ellipse.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->ellipse.pivot_local_align.y)->value_integer};

            // get pivot XY, rotation
            float pivot_position[2];
            switch (local_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_LEFT:
                    pivot_position[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_CENTER:
                    pivot_position[0] = diameter_x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    pivot_position[0] = diameter_x;
                    break;
                default:
                    DC_LOG_WARN("Ellipse", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
                    break;
            }
            switch (local_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    pivot_position[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    pivot_position[1] = diameter_y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    pivot_position[1] = diameter_y;
                    break;
                default:
                    DC_LOG_WARN("Ellipse", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.rotation)->value_double);

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

    // check if this is a pie/wedge (angle specified and != 360)
    bool use_angle = node->ellipse.angle != DC_APP_VAL_INDEX_UNDEFINED;
    float angle_span = use_angle ? (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.angle)->value_double : 360.0f;
    bool is_pie = use_angle && !dc_utils_float_equals(angle_span, 360.0f, 0.001f);

    // get number of segments
    int num_segments = node->ellipse.num_segments == DC_APP_VAL_INDEX_UNDEFINED ?
        (is_pie ? (int)(angle_span / 3.0f) + 2 : 40) :
        dc_app_lookup_get_value(app_data->lookup, node->ellipse.num_segments)->value_integer;
    if (num_segments < 2) num_segments = 2;
    if (num_segments > _NODE_ELLIPSE_MAX_SEGMENTS - 2) num_segments = _NODE_ELLIPSE_MAX_SEGMENTS - 2;

    // generate points
    plVec2 points[_NODE_ELLIPSE_MAX_SEGMENTS];
    int num_points = 0;

    if (is_pie) {
        // Pie/wedge mode: center point + arc points
        // Center point goes first for proper convex polygon winding
        plVec4 center4 = (plVec4){radius_x, radius_y, 0, 1};
        center4 = pl_mul_mat4_vec4(&transform, center4);
        points[0] = (plVec2){center4.x, center4.y};
        num_points = 1;

        // Generate arc points (start-based)
        // Wedge starts at 3 o'clock (standard math convention), rotation handled by transform matrix
        float span_rad = pl_radiansf(angle_span);
        int num_arc_points = num_segments + 1;

        for (int ii = 0; ii <= num_segments; ii++) {
            float t = (float)ii / (float)num_segments;
            float angle = t * span_rad; // from 0 to span
            float final_angle = angle;

            plVec4 point4 = (plVec4){
                radius_x * (1.0f + cosf(final_angle)),
                radius_y * (1.0f + sinf(final_angle)),
                0, 1};
            point4 = pl_mul_mat4_vec4(&transform, point4);
            points[num_points++] = (plVec2){point4.x, point4.y};
        }
    } else {
        // Full ellipse mode
        for (int ii = 0; ii < num_segments; ii++) {
            float angle = ii * (2.0f * (float)M_PI / num_segments);
            plVec4 point4 = (plVec4){
                radius_x * (1.0f + cosf(angle)),
                radius_y * (1.0f + sinf(angle)),
                0, 1};
            point4 = pl_mul_mat4_vec4(&transform, point4);
            points[ii] = (plVec2){point4.x, point4.y};
        }
        num_points = num_segments;
    }

    // draw fill
    if (node->ellipse.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) {
        float fill_color[4] = {
            node->ellipse.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.fill_color.r)->value_double,
            node->ellipse.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.fill_color.g)->value_double,
            node->ellipse.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.fill_color.b)->value_double,
            node->ellipse.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.fill_color.a)->value_double,
        };
        uint32_t pl_fill_color = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);

        if (is_pie && num_points >= 3) {
            // Use triangle fan for pie/wedge (works for any angle, including > 180 degrees)
            // points[0] = center, points[1..num_points-1] = arc points
            // Note: using static to avoid large stack allocation that can cause issues
            // with stack guard pages during rapid redraws (e.g., window resize)
            int num_triangles = num_points - 2;
            if (num_triangles > 0) {
                static plVec2 triangle_points[_NODE_ELLIPSE_MAX_SEGMENTS * 3];
                int tri_idx = 0;
                for (int ii = 0; ii < num_triangles; ii++) {
                    triangle_points[tri_idx++] = points[0];       // center
                    triangle_points[tri_idx++] = points[ii + 1];  // arc point i
                    triangle_points[tri_idx++] = points[ii + 2];  // arc point i+1
                }
                _ext_draw->add_triangles_filled(_draw_batch_get_2d(app_data), triangle_points, num_triangles * 3, (plDrawSolidOptions){.uColor = pl_fill_color});
            }
        } else if (!is_pie) {
            // Full ellipse - use convex polygon fill
            _ext_draw->add_convex_polygon_filled(_draw_batch_get_2d(app_data), points, num_points, (plDrawSolidOptions){.uColor = pl_fill_color});
        }
    }

    // draw outline
    if (node->ellipse.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) {
        float line_thickness = node->ellipse.line_width == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.line_width)->value_double;
        float line_color[4]  = {
            node->ellipse.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.line_color.r)->value_double,
            node->ellipse.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.line_color.g)->value_double,
            node->ellipse.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.line_color.b)->value_double,
            node->ellipse.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->ellipse.line_color.a)->value_double,
        };
        uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
        _ext_draw->add_polygon(_draw_batch_get_2d(app_data), points, num_points, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
    }

    // mouse events
    if (node->ellipse.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            app_data->frame_data.mouse_position.x,
            app_data->frame_data.mouse_position.y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in (ellipse equation: (x/a)^2 + (y/b)^2 <= 1)
        bool inside = false;
        if (mouse_position.x > 0 && mouse_position.x < diameter_x && mouse_position.y > 0 && mouse_position.y < diameter_y) {

            // now do the actual check using ellipse equation
            float dx = mouse_position.x - radius_x;
            float dy = mouse_position.y - radius_y;
            float normalized_dist = (dx * dx) / (radius_x * radius_x) + (dy * dy) / (radius_y * radius_y);
            inside = normalized_dist <= 1.0f;
        }

        // update global states
        if (inside) {
            app_data->frame_data.next_hovered_node = node_index;

            if (app_data->frame_data.is_mouse_pressed) {
                app_data->frame_data.next_pressed_node = node_index;
            }
        }

        // set state flags for children to check
        node->ellipse.state_flags = NODE_STATE_FLAG_NONE;
        if (app_data->frame_data.pressed_node == node_index) {
            node->ellipse.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (app_data->frame_data.active_node == node_index) {
            node->ellipse.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (app_data->frame_data.released_node == node_index) {
            node->ellipse.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (app_data->frame_data.hovered_node == node_index) {
            node->ellipse.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 position   = (plVec2){0.0f, 0.0f};
    plVec2 dimensions = (plVec2){diameter_x, diameter_y};
    _draw_node_list(app_data, node->ellipse.child, &position, &dimensions, &transform);
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
    bool use_pivot_parent_align = (node->container.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->container.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

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
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->container.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->container.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->container.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->container.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2];
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
                case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
                case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
                default: DC_LOG_WARN("Container", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
                case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
                case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
                default: DC_LOG_WARN("Container", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->container.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

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
                DC_LOG_WARN("Text", "Unknown X alignment: %d", local_aligns[0]);
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
                DC_LOG_WARN("Text", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->container.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->container.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float anchor[2] = {0, 0};
        DcAppAlignType parent_align_x = node->container.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->container.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Container", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->container.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->container.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Container", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->container.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->container.position.y)->value_double : 0};

        // apply negate
        if (node->container.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->container.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->container.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->container.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

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
                    DC_LOG_WARN("Container", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
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
                    DC_LOG_WARN("Container", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
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

    // mouse events
    if (node->container.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

        // process mouse position
        plVec4 mouse_position = (plVec4){
            app_data->frame_data.mouse_position.x,
            app_data->frame_data.mouse_position.y,
            0, 1};
        plMat4 transform_inverse = pl_mat4t_invert(&transform);
        mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

        // check whether mouse is over/in
        bool inside = mouse_position.x > 0 && mouse_position.x < virtual_dimension[0] && mouse_position.y > 0 && mouse_position.y < virtual_dimension[1];

        // update global states
        if (inside) {
            app_data->frame_data.next_hovered_node = node_index;

            if (app_data->frame_data.is_mouse_pressed) {
                app_data->frame_data.next_pressed_node = node_index;
            }
        }

        // set state flags for children to check
        node->container.state_flags = NODE_STATE_FLAG_NONE;
        if (app_data->frame_data.pressed_node == node_index) {
            node->container.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (app_data->frame_data.active_node == node_index) {
            node->container.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (app_data->frame_data.released_node == node_index) {
            node->container.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (app_data->frame_data.hovered_node == node_index) {
            node->container.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 virtual_dimensions_vec2 = (plVec2){virtual_dimension[0], virtual_dimension[1]};
    plVec2 position_vec2           = (plVec2){0.0f, 0.0f};
    _draw_node_list(app_data, node->container.child, &position_vec2, &virtual_dimensions_vec2, &transform);
}

static void _draw_node_conditional(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    DcValue             *val1     = dc_app_lookup_get_value(app_data->lookup, node->conditional.value1);
    if (!val1) {
        // DC_LOG_WARN("If", "Value1 is undefined, skipping conditional");
        return;
    }
    DcAppConditionalType type     = (node->conditional.type == DC_APP_VAL_INDEX_UNDEFINED)
                                        ? DC_APP_CONDITIONAL_TYPE_TRUE
                                        : (DcAppConditionalType)dc_app_lookup_get_value(app_data->lookup, node->conditional.type)->value_integer;
    bool                 use_val2 = node->conditional.value2 != DC_APP_VAL_INDEX_UNDEFINED;

    // evaluate
    bool result = false;
    if (use_val2) {
        DcValue *val2 = dc_app_lookup_get_value(app_data->lookup, node->conditional.value2);
        if (!val2) {
            DC_LOG_WARN("If", "Value2 is undefined, skipping conditional");
            return;
        }

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
                break;
            default:
                DC_LOG_WARN("If", "Unknown conditional type %d with Val2", type);
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
                DC_LOG_WARN("If", "Unknown conditional type %d with Val1 only", type);
                break;
        }
    }

    // set state flags for child state event nodes to check
    node->conditional.state_flags = result ? NODE_STATE_FLAG_TRUE : NODE_STATE_FLAG_FALSE;

    // draw children (state event nodes will check our state_flags)
    _draw_node_list(app_data, node->conditional.child, parent_position, parent_dimensions, parent_transform);
}

static void _draw_node_function(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    if (!node->function.callback) return;

    if (node->function.fire_call != DC_APP_VAL_INDEX_UNDEFINED) {
        DcValue *val = dc_app_lookup_get_value(app_data->lookup, node->function.fire_call);
        if (!dc_value_is_equal(val, &node->function.last_fire_call_value)) {
            node->function.callback();
            node->function.last_fire_call_value = *val;
        }
    } else {
        node->function.callback();
    }
}

static void _draw_node_image(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_dimension[2] = {
        node->image.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->image.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation       = node->image.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->image.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->image.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->image.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->image.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

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
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->image.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->image.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->image.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->image.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2];
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
                case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
                case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
                default: DC_LOG_WARN("Image", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
                case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
                case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
                default: DC_LOG_WARN("Image", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->image.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

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
                DC_LOG_WARN("Text", "Unknown X alignment: %d", local_aligns[0]);
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
                DC_LOG_WARN("Text", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->image.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->image.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float anchor[2] = {0, 0};
        DcAppAlignType parent_align_x = node->image.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->image.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Image", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->image.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->image.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Image", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->image.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->image.position.y)->value_double : 0};

        // apply negate
        if (node->image.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->image.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->image.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->image.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

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
                    DC_LOG_WARN("Image", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
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
                    DC_LOG_WARN("Image", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
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

    // draw (skip if texture failed to load)
    if (node->image.texture_index != TEXTURE_INDEX_UNDEFINED) {
        plBindGroupHandle bind_group_handle = app_data->sb_textures[node->image.texture_index].bind_group_handle;
        _ext_draw->add_image_quad(_draw_batch_get_2d(app_data), bind_group_handle.uData, point0, point1, point2, point3);
    }

    // mouse events
    if (node->image.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

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

        // set state flags for children to check
        node->image.state_flags = NODE_STATE_FLAG_NONE;
        if (app_data->frame_data.pressed_node == node_index) {
            node->image.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (app_data->frame_data.active_node == node_index) {
            node->image.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (app_data->frame_data.released_node == node_index) {
            node->image.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (app_data->frame_data.hovered_node == node_index) {
            node->image.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 position   = (plVec2){0.0f, 0.0f};
    plVec2 dimensions = (plVec2){dimension[0], dimension[1]};
    _draw_node_list(app_data, node->image.child, &position, &dimensions, &transform);
}

static void _draw_node_line(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_rotation       = node->line.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->line.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->line.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->line.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->line.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

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
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->line.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->line.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->line.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->line.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2];
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
                case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
                case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
                default: DC_LOG_WARN("Line", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
                case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
                case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
                default: DC_LOG_WARN("Line", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->line.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

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

        // apply negate
        if (node->line.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->line.negate_x)->value_boolean) {
            position[0] = -position[0];
        }
        if (node->line.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->line.negate_y)->value_boolean) {
            position[1] = -position[1];
        }

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
    int    num_points = sbcount(node->line.sb_vertices);
    plVec2 raw_points[_NODE_LINE_MAX_POINTS];
    plVec2 points[_NODE_LINE_MAX_POINTS];
    for (int ii = 0; ii < num_points; ii++) {

        // get raw point position
        float point_x = (float)dc_app_lookup_get_value(app_data->lookup, node->line.sb_vertices[ii].position.x)->value_double;
        float point_y = (float)dc_app_lookup_get_value(app_data->lookup, node->line.sb_vertices[ii].position.y)->value_double;

        // apply vertex negate
        if (node->line.sb_vertices[ii].negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->line.sb_vertices[ii].negate_x)->value_boolean) {
            point_x = -point_x;
        }
        if (node->line.sb_vertices[ii].negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->line.sb_vertices[ii].negate_y)->value_boolean) {
            point_y = -point_y;
        }

        // apply vertex parent_align offset
        if (node->line.sb_vertices[ii].parent_align.x != DC_APP_VAL_INDEX_UNDEFINED) {
            int parent_align_x = dc_app_lookup_get_value(app_data->lookup, node->line.sb_vertices[ii].parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_LEFT:
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    point_x += parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    point_x += parent_dimensions->x;
                    break;
                default:
                    break;
            }
        }
        if (node->line.sb_vertices[ii].parent_align.y != DC_APP_VAL_INDEX_UNDEFINED) {
            int parent_align_y = dc_app_lookup_get_value(app_data->lookup, node->line.sb_vertices[ii].parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    point_y += parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    point_y += parent_dimensions->y;
                    break;
                default:
                    break;
            }
        }

        raw_points[ii] = (plVec2){point_x, point_y};

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
    if (node->line.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) {
        float line_thickness = node->line.line_width == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->line.line_width)->value_double;
        float line_color[4]  = {
            node->line.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->line.line_color.r)->value_double,
            node->line.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->line.line_color.g)->value_double,
            node->line.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->line.line_color.b)->value_double,
            node->line.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->line.line_color.a)->value_double,
        };
        uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
        _ext_draw->add_lines(_draw_batch_get_2d(app_data), points, num_points, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
    }
}

static void _draw_node_panel(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // DisplayIndex/ActiveDisplay check - skip panel if index doesn't match window's active display
    DcAppValIndex window_active_display = app_data->sb_nodes[app_data->window].window.active_display;
    DcAppValIndex panel_index = node->panel.index;
    if (window_active_display != DC_APP_VAL_INDEX_UNDEFINED && panel_index != DC_APP_VAL_INDEX_UNDEFINED) {
        int active_display_value = dc_app_lookup_get_value(app_data->lookup, window_active_display)->value_integer;
        int panel_index_value = dc_app_lookup_get_value(app_data->lookup, panel_index)->value_integer;
        if (active_display_value != panel_index_value) {
            return;  // skip this panel - DisplayIndex doesn't match ActiveDisplay
        }
    }

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

    // track connection state for test pattern fallback
    bool is_connected = false;

    // switch over types to get the raw image data in the buffer
    switch (node->pixelstream.type) {
        case DC_APP_PIXELSTREAM_TYPE_MJPEG: {

            is_connected = dc_ps_mjpeg_server_is_connected(node->pixelstream.mjpeg.handle);
            if (!is_connected) {
                break;
            }

            // get latest frame data here
            if (dc_ps_mjpeg_server_has_new_data(node->pixelstream.mjpeg.handle)) {

                // get raw data from server
                size_t jpeg_size;
                dc_ps_mjpeg_get_server_data(node->pixelstream.mjpeg.handle, node->pixelstream.mjpeg.raw_jpeg, node->pixelstream.mjpeg.raw_jpeg_size, &jpeg_size);

                // free prior image data
                _ext_image->free(node->pixelstream.frame);

                // convert to raw image format
                int channels;
                node->pixelstream.frame = _ext_image->load(node->pixelstream.mjpeg.raw_jpeg, (int)jpeg_size, &node->pixelstream.frame_width, &node->pixelstream.frame_height, &channels, 4);

                // check size
                if (node->pixelstream.frame_height * node->pixelstream.frame_width > _NODE_PIXELSTREAM_MAX_WIDTH * _NODE_PIXELSTREAM_MAX_HEIGHT) {
                    DC_LOG_WARN("PixelStream", "Max image dimensions exceeded");
                }
            }
            break;
        }
        case DC_APP_PIXELSTREAM_TYPE_SHMEM: {

            is_connected = dc_ps_shmem_is_connected(node->pixelstream.shmem.handle);
            if (!is_connected) {
                break;
            }

            // get latest frame data
            if (dc_ps_shmem_has_new_data(node->pixelstream.shmem.handle)) {

                // get dimensions
                uint32_t width  = dc_ps_shmem_get_width(node->pixelstream.shmem.handle);
                uint32_t height = dc_ps_shmem_get_height(node->pixelstream.shmem.handle);

                // check size
                if (width * height > _NODE_PIXELSTREAM_MAX_WIDTH * _NODE_PIXELSTREAM_MAX_HEIGHT) {
                    DC_LOG_WARN("PixelStream", "Shmem max image dimensions exceeded");
                    break;
                }

                // allocate frame buffer if needed
                size_t frame_size = (size_t)width * height * 4;
                node->pixelstream.frame = realloc(node->pixelstream.frame, frame_size);

                // get data directly (already RGBA)
                size_t out_size;
                dc_ps_shmem_get_data(node->pixelstream.shmem.handle, node->pixelstream.frame, frame_size, &out_size);

                node->pixelstream.frame_width  = (int)width;
                node->pixelstream.frame_height = (int)height;
            }
            break;
        }
        default:
            DC_LOG_ERROR("PixelStream", "Unknown type: %d", node->pixelstream.type);
            break;
    }

    // determine whether to use test pattern
    bool use_test_pattern = !is_connected && node->pixelstream.test_pattern_texture_index != TEXTURE_INDEX_UNDEFINED;

    // don't draw anything if no data and no test pattern
    if (node->pixelstream.frame == NULL && !use_test_pattern) {
        return;
    }

    // copy to GPU image (only if not using test pattern)
    if (!use_test_pattern) {
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
    bool use_pivot_parent_align = (node->pixelstream.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->pixelstream.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

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
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->pixelstream.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->pixelstream.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2];
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
                case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
                case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
                default: DC_LOG_WARN("Pixelstream", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
                case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
                case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
                default: DC_LOG_WARN("Pixelstream", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

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
                DC_LOG_WARN("Text", "Unknown X alignment: %d", local_aligns[0]);
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
                DC_LOG_WARN("Text", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->pixelstream.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->pixelstream.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float anchor[2] = {0, 0};
        DcAppAlignType parent_align_x = node->pixelstream.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->pixelstream.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("PixelStream", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->pixelstream.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->pixelstream.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("PixelStream", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->pixelstream.position.y)->value_double : 0};

        // apply negate
        if (node->pixelstream.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->pixelstream.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->pixelstream.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->pixelstream.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

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
                    DC_LOG_WARN("PixelStream", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
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
                    DC_LOG_WARN("PixelStream", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
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
    plVec2 min_uv = {0.0f, 0.0f};
    plVec2 max_uv;
    plBindGroupHandle bind_group_handle;

    if (use_test_pattern) {
        // test pattern uses full texture
        _Texture test_tex = app_data->sb_textures[node->pixelstream.test_pattern_texture_index];
        max_uv = (plVec2){1.0f, 1.0f};
        bind_group_handle = test_tex.bind_group_handle;
    } else {
        // streaming texture
        plVec3 pl_texture_dimensions = pl_texture->tDesc.tDimensions;
        max_uv = (plVec2){
            ((float)node->pixelstream.frame_width) / pl_texture_dimensions.x,
            ((float)node->pixelstream.frame_height) / pl_texture_dimensions.y};
        bind_group_handle = app_data->sb_textures[node->pixelstream.texture_index].bind_group_handle;
    }

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
    _ext_draw->add_image_quad_ex(_draw_batch_get_2d(app_data), bind_group_handle.uData, point0, point1, point2, point3, uv0, uv1, uv2, uv3, 0xFFFFFFFF);

    // mouse events
    if (node->pixelstream.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

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

        // set state flags for children to check
        node->pixelstream.state_flags = NODE_STATE_FLAG_NONE;
        if (app_data->frame_data.pressed_node == node_index) {
            node->pixelstream.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (app_data->frame_data.active_node == node_index) {
            node->pixelstream.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (app_data->frame_data.released_node == node_index) {
            node->pixelstream.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (app_data->frame_data.hovered_node == node_index) {
            node->pixelstream.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 position   = (plVec2){0.0f, 0.0f};
    plVec2 dimensions = (plVec2){dimension[0], dimension[1]};
    _draw_node_list(app_data, node->pixelstream.child, &position, &dimensions, &transform);
}

static void _draw_node_polygon(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_rotation       = node->polygon.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->polygon.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->polygon.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->polygon.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->polygon.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

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
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->polygon.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->polygon.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->polygon.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->polygon.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2];
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
                case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
                case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
                default: DC_LOG_WARN("Polygon", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
                case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
                case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
                default: DC_LOG_WARN("Polygon", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->polygon.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform position
    {
        bool use_position[2] = {
            node->polygon.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->polygon.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float anchor[2] = {0, 0};
        DcAppAlignType parent_align_x = node->polygon.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->polygon.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("polygon", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->polygon.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->polygon.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("polygon", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.position.y)->value_double : 0};

        // apply negate
        if (node->polygon.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->polygon.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->polygon.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->polygon.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // get points, min/max
    plVec2 min_pos    = (plVec2){FLT_MAX, FLT_MAX};
    plVec2 max_pos    = (plVec2){FLT_MIN, FLT_MIN};
    int    num_points = sbcount(node->polygon.sb_vertices);
    plVec2 raw_points[_NODE_POLYGON_MAX_POINTS];
    plVec2 points[_NODE_POLYGON_MAX_POINTS];
    for (int ii = 0; ii < num_points; ii++) {

        // get raw point position
        float point_x = (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.sb_vertices[ii].position.x)->value_double;
        float point_y = (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.sb_vertices[ii].position.y)->value_double;

        // apply vertex negate
        if (node->polygon.sb_vertices[ii].negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->polygon.sb_vertices[ii].negate_x)->value_boolean) {
            point_x = -point_x;
        }
        if (node->polygon.sb_vertices[ii].negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->polygon.sb_vertices[ii].negate_y)->value_boolean) {
            point_y = -point_y;
        }

        // apply vertex parent_align offset
        if (node->polygon.sb_vertices[ii].parent_align.x != DC_APP_VAL_INDEX_UNDEFINED) {
            int parent_align_x = dc_app_lookup_get_value(app_data->lookup, node->polygon.sb_vertices[ii].parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_LEFT:
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    point_x += parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    point_x += parent_dimensions->x;
                    break;
                default:
                    break;
            }
        }
        if (node->polygon.sb_vertices[ii].parent_align.y != DC_APP_VAL_INDEX_UNDEFINED) {
            int parent_align_y = dc_app_lookup_get_value(app_data->lookup, node->polygon.sb_vertices[ii].parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    point_y += parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    point_y += parent_dimensions->y;
                    break;
                default:
                    break;
            }
        }

        raw_points[ii] = (plVec2){point_x, point_y};

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
    if (node->polygon.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) {

        float fill_color[4] = {
            node->polygon.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.fill_color.r)->value_double,
            node->polygon.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.fill_color.g)->value_double,
            node->polygon.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.fill_color.b)->value_double,
            node->polygon.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.fill_color.a)->value_double,
        };
        uint32_t pl_fill_color = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
        _ext_draw->add_convex_polygon_filled(_draw_batch_get_2d(app_data), points, num_points, (plDrawSolidOptions){.uColor = pl_fill_color});
    }

    // draw outline
    if (node->polygon.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) {
        float line_thickness = node->polygon.line_width == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.line_width)->value_double;
        float line_color[4]  = {
            node->polygon.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.line_color.r)->value_double,
            node->polygon.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.line_color.g)->value_double,
            node->polygon.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.line_color.b)->value_double,
            node->polygon.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->polygon.line_color.a)->value_double,
        };
        uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
        _ext_draw->add_polygon(_draw_batch_get_2d(app_data), points, num_points, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
    }

    // mouse events
    if (node->polygon.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

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

        // set state flags for children to check
        node->polygon.state_flags = NODE_STATE_FLAG_NONE;
        if (app_data->frame_data.pressed_node == node_index) {
            node->polygon.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (app_data->frame_data.active_node == node_index) {
            node->polygon.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (app_data->frame_data.released_node == node_index) {
            node->polygon.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (app_data->frame_data.hovered_node == node_index) {
            node->polygon.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 position   = (plVec2){min_pos.x, min_pos.y};
    plVec2 dimensions = (plVec2){max_pos.x - min_pos.x, max_pos.y - min_pos.y};
    _draw_node_list(app_data, node->polygon.child, &position, &dimensions, &transform);
}

static void _draw_node_rectangle(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_dimension[2] = {
        node->rectangle.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->rectangle.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation       = node->rectangle.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->rectangle.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->rectangle.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->rectangle.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->rectangle.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

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
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->rectangle.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->rectangle.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->rectangle.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->rectangle.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2];
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
                case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
                case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
                default: DC_LOG_WARN("Rectangle", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
                case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
                case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
                default: DC_LOG_WARN("Rectangle", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

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
                DC_LOG_WARN("Text", "Unknown X alignment: %d", local_aligns[0]);
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
                DC_LOG_WARN("Text", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        // compute matrix
        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);

        // apply transform
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->rectangle.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->rectangle.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float anchor[2] = {0, 0};
        DcAppAlignType parent_align_x = node->rectangle.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->rectangle.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Rectangle", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->rectangle.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->rectangle.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Rectangle", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.position.y)->value_double : 0};

        // apply negate
        if (node->rectangle.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->rectangle.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->rectangle.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->rectangle.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

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
                    DC_LOG_WARN("Rectangle", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
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
                    DC_LOG_WARN("Rectangle", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
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
    if (node->rectangle.config_flags & NODE_CONFIG_FLAG_FILL_ENABLED) {

        float fill_color[4] = {
            node->rectangle.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.fill_color.r)->value_double,
            node->rectangle.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.fill_color.g)->value_double,
            node->rectangle.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.fill_color.b)->value_double,
            node->rectangle.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.fill_color.a)->value_double,
        };
        uint32_t pl_fill_color = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
        _ext_draw->add_convex_polygon_filled(_draw_batch_get_2d(app_data), points, 4, (plDrawSolidOptions){.uColor = pl_fill_color});
    }

    // draw outline
    if (node->rectangle.config_flags & NODE_CONFIG_FLAG_LINE_ENABLED) {
        float line_thickness = node->rectangle.line_width == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.line_width)->value_double;
        float line_color[4]  = {
            node->rectangle.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.line_color.r)->value_double,
            node->rectangle.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.line_color.g)->value_double,
            node->rectangle.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.line_color.b)->value_double,
            node->rectangle.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->rectangle.line_color.a)->value_double,
        };
        uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
        _ext_draw->add_polygon(_draw_batch_get_2d(app_data), points, 4, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
    }

    // mouse events
    if (node->rectangle.config_flags & NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS) {

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

        // set state flags for children to check
        node->rectangle.state_flags = NODE_STATE_FLAG_NONE;
        if (app_data->frame_data.pressed_node == node_index) {
            node->rectangle.state_flags |= NODE_STATE_FLAG_PRESSED;
        }
        if (app_data->frame_data.active_node == node_index) {
            node->rectangle.state_flags |= NODE_STATE_FLAG_ACTIVE;
        }
        if (app_data->frame_data.released_node == node_index) {
            node->rectangle.state_flags |= NODE_STATE_FLAG_RELEASED;
        }
        if (app_data->frame_data.hovered_node == node_index) {
            node->rectangle.state_flags |= NODE_STATE_FLAG_HOVERED;
        }
    }

    // draw children
    plVec2 position   = (plVec2){0.0f, 0.0f};
    plVec2 dimensions = (plVec2){dimension[0], dimension[1]};
    _draw_node_list(app_data, node->rectangle.child, &position, &dimensions, &transform);
}

static void _draw_node_mouse_motion(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    // Only update if mouse is down (being dragged)
    if (!app_data->frame_data.is_mouse_down) {
        return;
    }

    // Get mouse position in window coordinates
    plVec2 mouse_pos = app_data->frame_data.mouse_position;

    // Transform mouse position from screen space to parent's virtual coordinate space
    plMat4 inv_transform = pl_mat4t_invert(parent_transform);
    plVec4 mouse_screen  = {mouse_pos.x, mouse_pos.y, 0.0f, 1.0f};
    plVec4 mouse_local   = pl_mul_mat4_vec4(&inv_transform, mouse_screen);

    // Set VariableX if defined
    if (node->mouse_motion.var_x != DC_APP_VAR_INDEX_UNDEFINED) {
        DcValue *var_x = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, node->mouse_motion.var_x)->value_index);
        if (var_x->type == DC_VALUE_TYPE_DOUBLE) {
            var_x->value_double = mouse_local.x;
        } else if (var_x->type == DC_VALUE_TYPE_INTEGER) {
            var_x->value_integer = (int)mouse_local.x;
        }
    }

    // Set VariableY if defined
    if (node->mouse_motion.var_y != DC_APP_VAR_INDEX_UNDEFINED) {
        DcValue *var_y = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, node->mouse_motion.var_y)->value_index);
        if (var_y->type == DC_VALUE_TYPE_DOUBLE) {
            var_y->value_double = mouse_local.y;
        } else if (var_y->type == DC_VALUE_TYPE_INTEGER) {
            var_y->value_integer = (int)mouse_local.y;
        }
    }
}

static bool _apply_set_operation(_AppData *app_data, DcAppVarIndex var_index, DcValue *var_value, DcValue *op_value, DcAppSetType operation) {
    switch (operation) {
        case DC_APP_SET_TYPE_UNDEFINED:
        case DC_APP_SET_TYPE_EQUAL:
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    strncpy(var_value->value_string, op_value->value_string, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    var_value->value_string[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
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
                    if (op_value->value_integer == 0) {
                        DC_LOG_ERROR("Set", "Divide by zero (int)");
                        return false;
                    }
                    var_value->value_integer /= op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (op_value->value_double == 0.0) {
                        DC_LOG_ERROR("Set", "Divide by zero (double)");
                        return false;
                    }
                    var_value->value_double /= op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    if (op_value->value_boolean == 0) {
                        DC_LOG_ERROR("Set", "Divide by zero (bool)");
                        return false;
                    }
                    var_value->value_boolean /= op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_MIN:
            // min(var, operand) - caps value at operand (upper bound)
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer > op_value->value_integer)
                        var_value->value_integer = op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (var_value->value_double > op_value->value_double)
                        var_value->value_double = op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_MAX:
            // max(var, operand) - floors value at operand (lower bound)
            switch (var_value->type) {
                case DC_VALUE_TYPE_STRING:
                    break;
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer < op_value->value_integer)
                        var_value->value_integer = op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (var_value->value_double < op_value->value_double)
                        var_value->value_double = op_value->value_double;
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_PUSH:
            dc_app_lookup_var_push(app_data->lookup, var_index);
            return false; // don't refresh - we're just saving state

        case DC_APP_SET_TYPE_POP:
            dc_app_lookup_var_pop(app_data->lookup, var_index);
            break;

        case DC_APP_SET_TYPE_NEGATE:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = -var_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = -var_value->value_double;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_RECIPROCAL:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer == 0) {
                        DC_LOG_ERROR("Set", "Reciprocal of zero (int)");
                        break;
                    }
                    var_value->value_integer = 1 / var_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (var_value->value_double == 0.0) {
                        DC_LOG_ERROR("Set", "Reciprocal of zero (double)");
                        break;
                    }
                    var_value->value_double = 1.0 / var_value->value_double;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_ABSOLUTE:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer < 0)
                        var_value->value_integer = -var_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = fabs(var_value->value_double);
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_SQUARE:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = var_value->value_integer * var_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = var_value->value_double * var_value->value_double;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_SQRT:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer < 0) {
                        DC_LOG_ERROR("Set", "Sqrt of negative (int)");
                        break;
                    }
                    var_value->value_integer = (int)floor(sqrt((double)var_value->value_integer));
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (var_value->value_double < 0.0) {
                        DC_LOG_ERROR("Set", "Sqrt of negative (double)");
                        break;
                    }
                    var_value->value_double = sqrt(var_value->value_double);
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_MODULO:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    if (op_value->value_integer == 0) {
                        DC_LOG_ERROR("Set", "Modulo by zero (int)");
                        break;
                    }
                    var_value->value_integer %= op_value->value_integer;
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (op_value->value_double == 0.0) {
                        DC_LOG_ERROR("Set", "Modulo by zero (double)");
                        break;
                    }
                    var_value->value_double = fmod(var_value->value_double, op_value->value_double);
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    if (op_value->value_boolean == 0) {
                        DC_LOG_ERROR("Set", "Modulo by zero (bool)");
                        break;
                    }
                    var_value->value_boolean %= op_value->value_boolean;
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_POWER:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = (int)pow((double)var_value->value_integer, (double)op_value->value_integer);
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = pow(var_value->value_double, op_value->value_double);
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean = (int)pow((double)var_value->value_boolean, (double)op_value->value_boolean);
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_LOG:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    if (var_value->value_integer <= 0) {
                        DC_LOG_ERROR("Set", "Log domain error (int)");
                        break;
                    }
                    var_value->value_integer = (int)log((double)var_value->value_integer); // natural log
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    if (var_value->value_double <= 0.0) {
                        DC_LOG_ERROR("Set", "Log domain error (double)");
                        break;
                    }
                    var_value->value_double = log(var_value->value_double); // natural log
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_EXP:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = (int)exp((double)var_value->value_integer);
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = exp(var_value->value_double);
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_ROUND:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    // already an integer; no-op
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = round(var_value->value_double);
                    break;
                default:
                    break;
            }
            break;

        case DC_APP_SET_TYPE_SIGN:
            switch (var_value->type) {
                case DC_VALUE_TYPE_INTEGER:
                    var_value->value_integer = (var_value->value_integer > 0) - (var_value->value_integer < 0);
                    break;
                case DC_VALUE_TYPE_DOUBLE:
                    var_value->value_double = (var_value->value_double > 0.0) - (var_value->value_double < 0.0);
                    break;
                case DC_VALUE_TYPE_BOOLEAN:
                    var_value->value_boolean = var_value->value_boolean ? 1 : 0;
                    break;
                default:
                    break;
            }
            break;

        default:
            DC_LOG_ERROR("Set", "Invalid operator: %d", operation);
            return false;
    }

    return true; // refresh needed
}

static void _draw_node_set(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // skip if variable or operand is undefined
    if (node->set.var_index == DC_APP_VAR_INDEX_UNDEFINED || node->set.operand == DC_APP_VAL_INDEX_UNDEFINED) {
        return;
    }

    DcValue *op_value = dc_app_lookup_get_value(app_data->lookup, node->set.operand);

    // get operation
    DcAppSetType operation = node->set.operation == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_SET_TYPE_UNDEFINED : (DcAppSetType)(dc_app_lookup_get_value(app_data->lookup, node->set.operation)->value_integer);

    // if deferred, snapshot the value and defer execution
    if (node->set.deferred != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->set.deferred)->value_boolean) {
        _DeferredSetOp qop;
        qop.var_index = node->set.var_index;
        qop.operation = operation;
        qop.value     = *op_value; // struct copy snapshot
        sbpush(app_data->sb_deferred_sets, qop);
        return;
    }

    // immediate execution
    DcValue *var_value = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, node->set.var_index)->value_index);
    if (_apply_set_operation(app_data, node->set.var_index, var_value, op_value, operation)) {
        // re-fetch var_value after POP (value_index may have changed)
        if (operation == DC_APP_SET_TYPE_POP) {
            var_value = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, node->set.var_index)->value_index);
        }
        dc_value_refresh(var_value);
    }
}

static void _flush_deferred_sets(_AppData *app_data) {
    int count = sbcount(app_data->sb_deferred_sets);
    for (int i = 0; i < count; i++) {
        _DeferredSetOp *qop = &app_data->sb_deferred_sets[i];
        DcValue *var_value = dc_app_lookup_get_value(
            app_data->lookup,
            dc_app_lookup_get_var(app_data->lookup, qop->var_index)->value_index);
        if (_apply_set_operation(app_data, qop->var_index, var_value, &qop->value, qop->operation)) {
            // re-fetch var_value after POP (value_index may have changed)
            if (qop->operation == DC_APP_SET_TYPE_POP) {
                var_value = dc_app_lookup_get_value(
                    app_data->lookup,
                    dc_app_lookup_get_var(app_data->lookup, qop->var_index)->value_index);
            }
            dc_value_refresh(var_value);
        }
    }
    sbclear(app_data->sb_deferred_sets);
}

static void _draw_node_sphere(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // boolean checks
    bool use_radius         = node->sphere.radius != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_rotation       = node->sphere.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->sphere.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->sphere.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->sphere.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->sphere.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get radius
    float radius, diameter;
    if (use_radius) {
        radius   = (float)dc_app_lookup_get_value(app_data->lookup, node->sphere.radius)->value_double;
        diameter = 2 * radius;
    } else {
        diameter = fminf(parent_dimensions->x, parent_dimensions->y);
        radius   = diameter / 2;
    }

    // 2D transform (for positioning in orthographic view)
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {
            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->sphere.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->sphere.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->sphere.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->sphere.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->sphere.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->sphere.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->sphere.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2];
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
                case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
                case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
                default: DC_LOG_WARN("Sphere", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
                case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
                case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
                default: DC_LOG_WARN("Sphere", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->sphere.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    // note: sphere is built centered at origin, so alignment works differently than rect-based elements
    {
        DcAppAlignType local_aligns[2] = {
            node->sphere.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->sphere.local_align.x)->value_integer,
            node->sphere.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->sphere.local_align.y)->value_integer};

        float trans_align_offsets[2];
        switch (local_aligns[0]) {
            case DC_APP_ALIGN_TYPE_LEFT:
                trans_align_offsets[0] = radius;  // move right so left edge aligns
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_CENTER:
                trans_align_offsets[0] = 0;  // sphere is already centered
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                trans_align_offsets[0] = -radius;  // move left so right edge aligns
                break;
            default:
                DC_LOG_WARN("Sphere", "Unknown X alignment: %d", local_aligns[0]);
                break;
        }
        switch (local_aligns[1]) {
            case DC_APP_ALIGN_TYPE_BOTTOM:
                trans_align_offsets[1] = radius;  // move down so bottom edge aligns
                break;
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_MIDDLE:
                trans_align_offsets[1] = 0;  // sphere is already centered
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                trans_align_offsets[1] = -radius;  // move up so top edge aligns
                break;
            default:
                DC_LOG_WARN("Sphere", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->sphere.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->sphere.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float anchor[2] = {0, 0};
        DcAppAlignType parent_align_x = node->sphere.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->sphere.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("Sphere", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->sphere.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->sphere.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("Sphere", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->sphere.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->sphere.position.y)->value_double : 0};

        // apply negate
        if (node->sphere.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->sphere.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->sphere.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->sphere.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation (around local pivot)
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {
            DcAppAlignType local_pivot_aligns[2] = {
                node->sphere.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->sphere.pivot_local_align.x)->value_integer,
                node->sphere.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->sphere.pivot_local_align.y)->value_integer};

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
                    DC_LOG_WARN("Sphere", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
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
                    DC_LOG_WARN("Sphere", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->sphere.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // build sphere transform: first internal rotation, then 2D positioning/scaling
    // start with internal rotation (roll, pitch, yaw) - applied first (rightmost in multiplication)
    plMat4 sphere_transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    {
        float roll  = node->sphere.rpy.roll  == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->sphere.rpy.roll)->value_double);
        float pitch = node->sphere.rpy.pitch == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->sphere.rpy.pitch)->value_double);
        float yaw   = node->sphere.rpy.yaw   == DC_APP_VAL_INDEX_UNDEFINED ? 0.0f : pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->sphere.rpy.yaw)->value_double);

        // offset yaw by -90 degrees to match legacy ADI ball orientation
        yaw -= (float)M_PI_2;

        // apply rotations in order: yaw (Y), pitch (X), roll (Z) for globe-like rotation
        if (yaw != 0.0f) {
            plMat4 yaw_xform = pl_mat4_rotate_vec3(yaw, (plVec3){0.0f, 1.0f, 0.0f});
            sphere_transform = pl_mul_mat4t(&sphere_transform, &yaw_xform);
        }
        if (pitch != 0.0f) {
            plMat4 pitch_xform = pl_mat4_rotate_vec3(pitch, (plVec3){1.0f, 0.0f, 0.0f});
            sphere_transform = pl_mul_mat4t(&sphere_transform, &pitch_xform);
        }
        if (roll != 0.0f) {
            plMat4 roll_xform = pl_mat4_rotate_vec3(roll, (plVec3){0.0f, 0.0f, 1.0f});
            sphere_transform = pl_mul_mat4t(&sphere_transform, &roll_xform);
        }
    }

    // then apply the 2D transform (scale + position) - applied after rotation
    sphere_transform = pl_mul_mat4t(&transform, &sphere_transform);

    // get fill color
    float fill_color[4] = {
        node->sphere.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->sphere.fill_color.r)->value_double,
        node->sphere.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->sphere.fill_color.g)->value_double,
        node->sphere.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->sphere.fill_color.b)->value_double,
        node->sphere.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(app_data->lookup, node->sphere.fill_color.a)->value_double,
    };
    uint32_t pl_fill_color = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);

    // create sphere geometry definition - sphere is built at origin, transform handles positioning
    plSphere sphere_def = {
        .tCenter = (plVec3){0.0f, 0.0f, 0.0f},
        .fRadius = radius
    };

    // get a 3D draw list from the batch system
    plDrawList3D *draw_list_3d = _draw_batch_get_3d(app_data);

    // draw textured or solid sphere
    if (node->sphere.texture_index != TEXTURE_INDEX_UNDEFINED) {
        // textured sphere
        _Texture *texture = &app_data->sb_textures[node->sphere.texture_index];
        plTextureID texture_id = texture->bind_group_handle.uData;
        _ext_draw->add_3d_sphere_textured(draw_list_3d, texture_id, sphere_def, &sphere_transform, 32, 32, pl_fill_color);
    } else {
        // solid sphere
        _ext_draw->add_3d_sphere_filled(draw_list_3d, sphere_def, 32, 32, (plDrawSolidOptions){.uColor = pl_fill_color});
    }
}

static void _draw_node_stencil(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    int num_children = sbcount(node->stencil.sb_children);
    int depth = ++app_data->stencil_depth;

    for (int i = 0; i < num_children; i++) {
        _StencilChild *stencil_child = &node->stencil.sb_children[i];

        switch (stencil_child->type) {
            case STENCIL_CHILD_TYPE_ADD:
                app_data->active_2d_shader_override          = &app_data->stencil_create_2d_shader;
                app_data->active_sdf_shader_override         = &app_data->stencil_create_sdf_shader;
                app_data->active_3d_solid_shader_override    = &app_data->stencil_create_3d_solid_shader;
                app_data->active_3d_textured_shader_override = &app_data->stencil_create_3d_textured_shader;
                break;
            case STENCIL_CHILD_TYPE_REMOVE:
                app_data->active_2d_shader_override          = &app_data->stencil_remove_2d_shader;
                app_data->active_sdf_shader_override         = &app_data->stencil_remove_sdf_shader;
                app_data->active_3d_solid_shader_override    = &app_data->stencil_remove_3d_solid_shader;
                app_data->active_3d_textured_shader_override = &app_data->stencil_remove_3d_textured_shader;
                break;
            case STENCIL_CHILD_TYPE_DRAW:
                app_data->active_2d_shader_override          = &app_data->stencil_draw_2d_shader[depth - 1];
                app_data->active_sdf_shader_override         = &app_data->stencil_draw_sdf_shader[depth - 1];
                app_data->active_3d_solid_shader_override    = &app_data->stencil_draw_3d_solid_shader[depth - 1];
                app_data->active_3d_textured_shader_override = &app_data->stencil_draw_3d_textured_shader[depth - 1];
                break;
            default:
                continue;
        }

        app_data->stencil_2d_dirty = true;
        app_data->stencil_3d_dirty = true;
        _draw_node_list(app_data, stencil_child->child, parent_position, parent_dimensions, parent_transform);
    }

    // cleanup: replay ADD geometry with decrement shader to restore stencil buffer
    for (int i = 0; i < num_children; i++) {
        _StencilChild *stencil_child = &node->stencil.sb_children[i];
        if (stencil_child->type == STENCIL_CHILD_TYPE_ADD) {
            app_data->active_2d_shader_override          = &app_data->stencil_cleanup_2d_shader;
            app_data->active_sdf_shader_override         = &app_data->stencil_cleanup_sdf_shader;
            app_data->active_3d_solid_shader_override    = &app_data->stencil_cleanup_3d_solid_shader;
            app_data->active_3d_textured_shader_override = &app_data->stencil_cleanup_3d_textured_shader;
            app_data->stencil_2d_dirty = true;
            app_data->stencil_3d_dirty = true;
            _draw_node_list(app_data, stencil_child->child, parent_position, parent_dimensions, parent_transform);
        }
    }

    app_data->stencil_depth--;

    // restore parent draw shader or reset to normal rendering
    if (app_data->stencil_depth > 0) {
        app_data->active_2d_shader_override          = &app_data->stencil_draw_2d_shader[app_data->stencil_depth - 1];
        app_data->active_sdf_shader_override         = &app_data->stencil_draw_sdf_shader[app_data->stencil_depth - 1];
        app_data->active_3d_solid_shader_override    = &app_data->stencil_draw_3d_solid_shader[app_data->stencil_depth - 1];
        app_data->active_3d_textured_shader_override = &app_data->stencil_draw_3d_textured_shader[app_data->stencil_depth - 1];
    } else {
        app_data->active_2d_shader_override          = NULL;
        app_data->active_sdf_shader_override         = NULL;
        app_data->active_3d_solid_shader_override    = NULL;
        app_data->active_3d_textured_shader_override = NULL;
    }
    app_data->stencil_2d_dirty = true;
    app_data->stencil_3d_dirty = true;
}

static void _draw_node_planet_view(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    (void)node_index;

    // look up planet def
    _PlanetDef *def = &app_data->sb_planet_defs[node->planet_view.planet_def_index];
    if (def->index == PLANET_INDEX_UNDEFINED) return;

    plPlanet *planet = app_data->sb_planets[def->index];
    if (!planet) return;

    // boolean checks
    bool use_dimension[2] = {
        node->planet_view.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->planet_view.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
    bool use_rotation       = node->planet_view.rotation != DC_APP_VAL_INDEX_UNDEFINED;
    bool use_pivot_position = (node->planet_view.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->planet_view.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_pivot_parent_align = (node->planet_view.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->planet_view.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

    // get dimensions
    float dimension[2] = {
        use_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.dimension.x)->value_double : parent_dimensions->x,
        use_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.dimension.y)->value_double : parent_dimensions->y};

    // transform
    plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // xform rotation (around a point)
    {
        if (use_rotation && use_pivot_position) {

            float pivot_position[2] = {
                (float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.pivot_position.x)->value_double,
                (float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.pivot_position.y)->value_double};
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        } else if (use_rotation && use_pivot_parent_align) {

            DcAppAlignType parent_pivot_aligns[2] = {
                node->planet_view.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->planet_view.pivot_parent_align.x)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED,
                node->planet_view.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                    ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->planet_view.pivot_parent_align.y)->value_integer
                    : DC_APP_ALIGN_TYPE_UNDEFINED};

            float pivot_position[2];
            switch (parent_pivot_aligns[0]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
                case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
                case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
                default: DC_LOG_WARN("PlanetView", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
            }
            switch (parent_pivot_aligns[1]) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
                case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
                case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
                default: DC_LOG_WARN("PlanetView", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // xform local alignment
    {
        DcAppAlignType local_aligns[2] = {
            node->planet_view.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->planet_view.local_align.x)->value_integer,
            node->planet_view.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->planet_view.local_align.y)->value_integer};

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
                DC_LOG_WARN("PlanetView", "Unknown X alignment: %d", local_aligns[0]);
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
                DC_LOG_WARN("PlanetView", "Unknown Y alignment: %d", local_aligns[1]);
                break;
        }

        plMat4 trans_local_align_xform = pl_mat4_translate_xyz(trans_align_offsets[0], trans_align_offsets[1], 0.0f);
        transform = pl_mul_mat4t(&transform, &trans_local_align_xform);
    }

    // xform position
    {
        bool use_position[2] = {
            node->planet_view.position.x != DC_APP_VAL_INDEX_UNDEFINED,
            node->planet_view.position.y != DC_APP_VAL_INDEX_UNDEFINED};

        float anchor[2] = {0, 0};
        DcAppAlignType parent_align_x = node->planet_view.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->planet_view.parent_align.x)->value_integer;
        switch (parent_align_x) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_LEFT:
                anchor[0] = 0;
                break;
            case DC_APP_ALIGN_TYPE_CENTER:
                anchor[0] = parent_dimensions->x / 2;
                break;
            case DC_APP_ALIGN_TYPE_RIGHT:
                anchor[0] = parent_dimensions->x;
                break;
            default:
                DC_LOG_WARN("PlanetView", "Invalid parent_align_x: %d", parent_align_x);
                break;
        }
        DcAppAlignType parent_align_y = node->planet_view.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->planet_view.parent_align.y)->value_integer;
        switch (parent_align_y) {
            case DC_APP_ALIGN_TYPE_UNDEFINED:
            case DC_APP_ALIGN_TYPE_BOTTOM:
                anchor[1] = 0;
                break;
            case DC_APP_ALIGN_TYPE_MIDDLE:
                anchor[1] = parent_dimensions->y / 2;
                break;
            case DC_APP_ALIGN_TYPE_TOP:
                anchor[1] = parent_dimensions->y;
                break;
            default:
                DC_LOG_WARN("PlanetView", "Invalid parent_align_y: %d", parent_align_y);
                break;
        }

        float offset[2] = {
            use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.position.x)->value_double : 0,
            use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.position.y)->value_double : 0};

        if (node->planet_view.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->planet_view.negate_x)->value_boolean) {
            offset[0] = -offset[0];
        }
        if (node->planet_view.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->planet_view.negate_y)->value_boolean) {
            offset[1] = -offset[1];
        }

        float position[2] = {
            parent_position->x + anchor[0] + offset[0],
            parent_position->y + anchor[1] + offset[1]};

        plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
        transform = pl_mul_mat4t(&transform, &trans_position_xform);
    }

    // xform local rotation
    {
        if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

            DcAppAlignType local_pivot_aligns[2] = {
                node->planet_view.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->planet_view.pivot_local_align.x)->value_integer,
                node->planet_view.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->planet_view.pivot_local_align.y)->value_integer};

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
                    DC_LOG_WARN("PlanetView", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
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
                    DC_LOG_WARN("PlanetView", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
                    break;
            }
            float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.rotation)->value_double);

            plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
            plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
            plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

            transform = pl_mul_mat4t(&transform, &trans_from_origin_xform);
            transform = pl_mul_mat4t(&transform, &rotate_xform);
            transform = pl_mul_mat4t(&transform, &trans_to_origin_xform);
        }
    }

    // PL specific fixes
    {
        plMat4 trans_pl_origin_xform = pl_mat4_translate_xyz(0, dimension[1], 0.0f);
        plMat4 scale_invert_y_xform = pl_mat4_scale_xyz(1.0f, -1.0f, 1.0f);
        transform = pl_mul_mat4t(&transform, &trans_pl_origin_xform);
        transform = pl_mul_mat4t(&transform, &scale_invert_y_xform);
    }

    // parent transform
    transform = pl_mul_mat4t(parent_transform, &transform);

    // compute quad points
    plVec4 point0_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){0.0f, 0.0f, 0.0f, 1.0f});
    plVec4 point1_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){0.0f, dimension[1], 0.0f, 1.0f});
    plVec4 point2_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){dimension[0], dimension[1], 0.0f, 1.0f});
    plVec4 point3_vec4 = pl_mul_mat4_vec4(&transform, (plVec4){dimension[0], 0.0f, 0.0f, 1.0f});
    plVec2 point0      = (plVec2){point0_vec4.x, point0_vec4.y};
    plVec2 point1      = (plVec2){point1_vec4.x, point1_vec4.y};
    plVec2 point2      = (plVec2){point2_vec4.x, point2_vec4.y};
    plVec2 point3      = (plVec2){point3_vec4.x, point3_vec4.y};

    // camera setup
    bool use_lle = (node->planet_view.lle.lat != DC_APP_VAL_INDEX_UNDEFINED &&
                    node->planet_view.lle.lon != DC_APP_VAL_INDEX_UNDEFINED &&
                    node->planet_view.lle.ele != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_xyz = (node->planet_view.xyz.x != DC_APP_VAL_INDEX_UNDEFINED &&
                    node->planet_view.xyz.y != DC_APP_VAL_INDEX_UNDEFINED &&
                    node->planet_view.xyz.z != DC_APP_VAL_INDEX_UNDEFINED);
    bool use_ortho = (node->planet_view.orthographic != DC_APP_VAL_INDEX_UNDEFINED &&
                      dc_app_lookup_get_value(app_data->lookup, node->planet_view.orthographic)->value_boolean);

    plCamera camera = {0};
    camera.tType        = PL_CAMERA_TYPE_PERSPECTIVE;
    camera.fFieldOfView = 60.0f * (float)(M_PI / 180.0);
    camera.fAspectRatio = (dimension[1] > 0.0f) ? dimension[0] / dimension[1] : 1.0f;
    camera.fNearZ       = 1.0f;
    camera.fFarZ        = 100000000.0f;
    camera.fWidth       = dimension[0];
    camera.fHeight      = dimension[1];

    if (use_xyz) {
        double cam_x = dc_app_lookup_get_value(app_data->lookup, node->planet_view.xyz.x)->value_double;
        double cam_y = dc_app_lookup_get_value(app_data->lookup, node->planet_view.xyz.y)->value_double;
        double cam_z = dc_app_lookup_get_value(app_data->lookup, node->planet_view.xyz.z)->value_double;
        _ext_camera->set_pos(&camera, cam_x, cam_y, cam_z);

        if (node->planet_view.rpy.pitch != DC_APP_VAL_INDEX_UNDEFINED &&
            node->planet_view.rpy.yaw   != DC_APP_VAL_INDEX_UNDEFINED) {
            float pitch = (float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.rpy.pitch)->value_double;
            float yaw   = (float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.rpy.yaw)->value_double;
            _ext_camera->set_pitch_yaw(&camera, pl_radiansf(pitch), pl_radiansf(yaw));
        }
        if (node->planet_view.rpy.roll != DC_APP_VAL_INDEX_UNDEFINED) {
            camera.fRoll = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.rpy.roll)->value_double);
        }
    } else if (use_lle) {
        double lat_deg = dc_app_lookup_get_value(app_data->lookup, node->planet_view.lle.lat)->value_double;
        double lon_deg = dc_app_lookup_get_value(app_data->lookup, node->planet_view.lle.lon)->value_double;
        double ele     = dc_app_lookup_get_value(app_data->lookup, node->planet_view.lle.ele)->value_double;

        double lat_rad = lat_deg * M_PI / 180.0;
        double lon_rad = lon_deg * M_PI / 180.0;
        double r       = def->radius + ele;

        _ext_camera->set_pos(&camera,
            r * cos(lat_rad) * sin(lon_rad),
            r * sin(lat_rad),
            r * cos(lat_rad) * cos(lon_rad));

        _ext_camera->look_at(&camera, camera.tPosDouble, (plDVec3){0, 0, 0});

        if (node->planet_view.heading != DC_APP_VAL_INDEX_UNDEFINED) {
            float heading_deg = (float)dc_app_lookup_get_value(app_data->lookup, node->planet_view.heading)->value_double;
            camera.fRoll = pl_radiansf(heading_deg);
        }
    }

    _ext_camera->update(&camera);

    if (use_ortho) {
        double cam_dist = sqrt(camera.tPosDouble.x * camera.tPosDouble.x +
                               camera.tPosDouble.y * camera.tPosDouble.y +
                               camera.tPosDouble.z * camera.tPosDouble.z);
        double surface_dist = cam_dist - def->radius;
        if (surface_dist < 1.0) surface_dist = 1.0;

        float half_h = (float)surface_dist * tanf(camera.fFieldOfView / 2.0f);
        float half_w = half_h * camera.fAspectRatio;

        camera.tProjMat = (plMat4){0};
        camera.tProjMat.col[0].x = 1.0f / half_w;
        camera.tProjMat.col[1].y = 1.0f / half_h;
        camera.tProjMat.col[2].z = 1.0f / (camera.fFarZ - camera.fNearZ);
        camera.tProjMat.col[3].w = 1.0f;
    }

    // render view
    plPlanetView *view = app_data->sb_planet_views[node->planet_view.planet_view_index];
    plCommandBuffer *cmd_buf = _ext_starter->get_command_buffer();
    _ext_planet->render_view(view, &camera, cmd_buf);
    _ext_starter->submit_command_buffer(cmd_buf);

    plBindGroupHandle bind_group = _ext_planet->get_view_texture(view);
    _ext_draw->add_image_quad(_draw_batch_get_2d(app_data), bind_group.uData, point0, point1, point2, point3);
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
        static char val_str[256]; // assume text won't be that long..
        if (node->text.sb_vals[ii] == DC_APP_VAL_INDEX_UNDEFINED) {
            val_str[0] = '\0'; // empty string for undefined variable
        } else {
            DcValue *val = dc_app_lookup_get_value(app_data->lookup, node->text.sb_vals[ii]);
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
                    DC_LOG_WARN("Text", "Unknown value type: %d", format_type);
            }
        }
        sbpushn(sb_text, val_str, (int)strlen(val_str));
    }

    // ending filler
    char *filler = &(node->text.sb_fillers[node->text.sb_filler_indices[sbcount(node->text.sb_vals)]]);
    sbpushn(sb_text, filler, (int)strlen(filler));
    sbpush(sb_text, '\0');

    // log
    if (node->text.log != DC_APP_VAL_INDEX_UNDEFINED) {
        const char *label = dc_app_lookup_get_value(app_data->lookup, node->text.log)->value_string;
        printf("[%s] %s\n", label, sb_text);
    }

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
    bool use_pivot_parent_align = (node->text.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED || node->text.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED);

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
            } else if (use_rotation && use_pivot_parent_align) {

                DcAppAlignType parent_pivot_aligns[2] = {
                    node->text.pivot_parent_align.x != DC_APP_VAL_INDEX_UNDEFINED
                        ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->text.pivot_parent_align.x)->value_integer
                        : DC_APP_ALIGN_TYPE_UNDEFINED,
                    node->text.pivot_parent_align.y != DC_APP_VAL_INDEX_UNDEFINED
                        ? (DcAppAlignType)dc_app_lookup_get_value(app_data->lookup, node->text.pivot_parent_align.y)->value_integer
                        : DC_APP_ALIGN_TYPE_UNDEFINED};

                float pivot_position[2];
                switch (parent_pivot_aligns[0]) {
                    case DC_APP_ALIGN_TYPE_UNDEFINED:
                    case DC_APP_ALIGN_TYPE_LEFT:     pivot_position[0] = 0; break;
                    case DC_APP_ALIGN_TYPE_CENTER:   pivot_position[0] = parent_dimensions->x / 2; break;
                    case DC_APP_ALIGN_TYPE_RIGHT:    pivot_position[0] = parent_dimensions->x; break;
                    default: DC_LOG_WARN("Text", "Unknown pivot X alignment: %d", parent_pivot_aligns[0]); break;
                }
                switch (parent_pivot_aligns[1]) {
                    case DC_APP_ALIGN_TYPE_UNDEFINED:
                    case DC_APP_ALIGN_TYPE_BOTTOM:   pivot_position[1] = 0; break;
                    case DC_APP_ALIGN_TYPE_MIDDLE:   pivot_position[1] = parent_dimensions->y / 2; break;
                    case DC_APP_ALIGN_TYPE_TOP:      pivot_position[1] = parent_dimensions->y; break;
                    default: DC_LOG_WARN("Text", "Unknown pivot Y alignment: %d", parent_pivot_aligns[1]); break;
                }
                float rotation = pl_radiansf((float)dc_app_lookup_get_value(app_data->lookup, node->text.rotation)->value_double);

                plMat4 trans_from_origin_xform = pl_mat4_translate_xyz(pivot_position[0], pivot_position[1], 0.0f);
                plMat4 rotate_xform            = pl_mat4_rotate_vec3(rotation, (plVec3){0.0f, 0.0f, 1.0f});
                plMat4 trans_to_origin_xform   = pl_mat4_translate_xyz(-1 * pivot_position[0], -1 * pivot_position[1], 0.0f);

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
                    DC_LOG_WARN("Text", "Unknown X alignment: %d", local_aligns[0]);
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
                    DC_LOG_WARN("Text", "Unknown Y alignment: %d", local_aligns[1]);
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
            bool use_position[2] = {
                node->text.position.x != DC_APP_VAL_INDEX_UNDEFINED,
                node->text.position.y != DC_APP_VAL_INDEX_UNDEFINED};

            float anchor[2] = {0, 0};
            DcAppAlignType parent_align_x = node->text.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->text.parent_align.x)->value_integer;
            switch (parent_align_x) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_LEFT:
                    anchor[0] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_CENTER:
                    anchor[0] = parent_dimensions->x / 2;
                    break;
                case DC_APP_ALIGN_TYPE_RIGHT:
                    anchor[0] = parent_dimensions->x;
                    break;
                default:
                    DC_LOG_WARN("Text", "Invalid parent_align_x: %d", parent_align_x);
                    break;
            }
            DcAppAlignType parent_align_y = node->text.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(app_data->lookup, node->text.parent_align.y)->value_integer;
            switch (parent_align_y) {
                case DC_APP_ALIGN_TYPE_UNDEFINED:
                case DC_APP_ALIGN_TYPE_BOTTOM:
                    anchor[1] = 0;
                    break;
                case DC_APP_ALIGN_TYPE_MIDDLE:
                    anchor[1] = parent_dimensions->y / 2;
                    break;
                case DC_APP_ALIGN_TYPE_TOP:
                    anchor[1] = parent_dimensions->y;
                    break;
                default:
                    DC_LOG_WARN("Text", "Invalid parent_align_y: %d", parent_align_y);
                    break;
            }

            float offset[2] = {
                use_position[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->text.position.x)->value_double : 0,
                use_position[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->text.position.y)->value_double : 0};

            // apply negate
            if (node->text.negate_x != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->text.negate_x)->value_boolean) {
                offset[0] = -offset[0];
            }
            if (node->text.negate_y != DC_APP_VAL_INDEX_UNDEFINED && dc_app_lookup_get_value(app_data->lookup, node->text.negate_y)->value_boolean) {
                offset[1] = -offset[1];
            }

            float position[2] = {
                parent_position->x + anchor[0] + offset[0],
                parent_position->y + anchor[1] + offset[1]};

            plMat4 trans_position_xform = pl_mat4_translate_xyz(position[0], position[1], 0.0f);
            transform = pl_mul_mat4t(&transform, &trans_position_xform);
        }

        // xform local rotation
        {
            if (use_rotation && !use_pivot_position && !use_pivot_parent_align) {

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
                        DC_LOG_WARN("Text", "Unknown pivot X alignment: %d", local_pivot_aligns[0]);
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
                        DC_LOG_WARN("Text", "Unknown pivot Y alignment: %d", local_pivot_aligns[1]);
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

        // shadow pass
        if (node->text.shadow_offset != DC_APP_VAL_INDEX_UNDEFINED) {
            plDrawTextOptions shadow_opts = text_options;
            shadow_opts.uColor = PL_COLOR_32_RGBA(0, 0, 0, 1);

            float offset = (float)dc_app_lookup_get_value(app_data->lookup, node->text.shadow_offset)->value_double;
            shadow_opts.tTransform.x13 += offset;
            shadow_opts.tTransform.x23 += offset;

            _ext_draw->add_text(_draw_batch_get_2d(app_data), (plVec2){0, 0}, &sb_text[subtext_indices[ii]], shadow_opts);
        }

        // draw
        _ext_draw->add_text(_draw_batch_get_2d(app_data), (plVec2){0, 0}, &sb_text[subtext_indices[ii]], text_options);
    }
}

static void _draw_node_window(_AppData *app_data, _NodeIndex node_index, _Node *node, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {

    // TODO move this code to only the resize() function

    // current dimensions
    uint32_t dimensionX, dimensionY;
    _ext_windows->get_size(app_data->pl_window, &dimensionX, &dimensionY);

    // boolean checks
    bool use_virtual_dimension[2] = {
        node->window.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
        node->window.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};

    // all transform parameters
    float dimension[2]         = {(float)dimensionX, (float)dimensionY};
    float virtual_dimension[2] = {
        use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(app_data->lookup, node->window.virtual_dimension.x)->value_double : node->window.init_dimension.x,
        use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(app_data->lookup, node->window.virtual_dimension.y)->value_double : node->window.init_dimension.y};

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

//-----------------------------------------------------------------------------
// [SECTION] draw batch utils
//-----------------------------------------------------------------------------

static void _draw_batch_reset(_AppData *app_data) {
    // clear the batches array (doesn't free memory, just resets count)
    sbclear(app_data->sb_draw_batches);

    // reset pool indices
    app_data->draw_list_2d_index = 0;
    app_data->draw_list_3d_index = 0;
}

static plDrawLayer2D *_draw_batch_get_2d(_AppData *app_data) {
    // if last batch is already 2D, return the same layer
    int count = sbcount(app_data->sb_draw_batches);
    if (count > 0 && app_data->sb_draw_batches[count - 1].type == DRAW_BATCH_TYPE_2D) {
        plDrawLayer2D *layer = app_data->sb_draw_batches[count - 1].draw_list_2d.layer;

        // inject stencil override on phase change
        if (app_data->stencil_2d_dirty) {
            _ext_draw_backend->set_shader(layer, app_data->active_2d_shader_override, app_data->active_sdf_shader_override);
            app_data->stencil_2d_dirty = false;
        }
        return layer;
    }

    // grow pool if needed - request from extension
    int pool_size = sbcount(app_data->sb_draw_list_2d_pool);
    if (app_data->draw_list_2d_index >= pool_size) {
        plDrawList2D  *new_draw_list = _ext_draw->request_2d_drawlist();
        plDrawLayer2D *new_layer     = _ext_draw->request_2d_layer(new_draw_list);
        _DrawList2D    new_entry     = {.draw_list = new_draw_list, .layer = new_layer};
        sbpush(app_data->sb_draw_list_2d_pool, new_entry);
    }

    // get draw list + layer from pool
    _DrawList2D *draw_list_2d = &app_data->sb_draw_list_2d_pool[app_data->draw_list_2d_index];
    app_data->draw_list_2d_index++;

    // add batch entry
    _DrawBatch batch = {
        .type = DRAW_BATCH_TYPE_2D,
        .draw_list_2d = *draw_list_2d
    };
    sbpush(app_data->sb_draw_batches, batch);

    // inject stencil override if active (new batch after batch break)
    if (app_data->stencil_2d_dirty || app_data->active_2d_shader_override || app_data->active_sdf_shader_override) {
        _ext_draw_backend->set_shader(draw_list_2d->layer, app_data->active_2d_shader_override, app_data->active_sdf_shader_override);
        app_data->stencil_2d_dirty = false;
    }

    return draw_list_2d->layer;
}

static plDrawList3D *_draw_batch_get_3d(_AppData *app_data) {
    // if last batch is already 3D, return the same draw list
    int count = sbcount(app_data->sb_draw_batches);
    if (count > 0 && app_data->sb_draw_batches[count - 1].type == DRAW_BATCH_TYPE_3D) {
        plDrawList3D *draw_list = app_data->sb_draw_batches[count - 1].draw_list_3d;

        // inject stencil override on phase change
        if (app_data->stencil_3d_dirty) {
            _ext_draw_backend->set_3d_shader(draw_list,
                app_data->active_3d_solid_shader_override,
                app_data->active_3d_textured_shader_override);
            app_data->stencil_3d_dirty = false;
        }
        return draw_list;
    }

    // grow pool if needed - request from extension
    int pool_size = sbcount(app_data->sb_draw_list_3d_pool);
    if (app_data->draw_list_3d_index >= pool_size) {
        plDrawList3D *new_list = _ext_draw->request_3d_drawlist();
        sbpush(app_data->sb_draw_list_3d_pool, new_list);
    }

    // get draw list from pool
    plDrawList3D *draw_list = app_data->sb_draw_list_3d_pool[app_data->draw_list_3d_index];
    app_data->draw_list_3d_index++;

    // add batch entry
    _DrawBatch batch = {
        .type = DRAW_BATCH_TYPE_3D,
        .draw_list_3d = draw_list
    };
    sbpush(app_data->sb_draw_batches, batch);

    // inject stencil override if active (new batch after batch break)
    if (app_data->stencil_3d_dirty || app_data->active_3d_solid_shader_override || app_data->active_3d_textured_shader_override) {
        _ext_draw_backend->set_3d_shader(draw_list,
            app_data->active_3d_solid_shader_override,
            app_data->active_3d_textured_shader_override);
        app_data->stencil_3d_dirty = false;
    }

    return draw_list;
}
