#include "dcapp.h"

#include "../src/app/enums.h"

static void _draw_node_list(_PlAppData *pl_app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *node_transform) {
    _NodeIndex current_node_index = node_index;
    while (current_node_index != NODE_INDEX_UNDEFINED) {
        _draw_node(pl_app_data, current_node_index, parent_position, parent_dimensions, node_transform);
        current_node_index = _get_node(current_node_index)->next;
    }
}

static void _draw_node(_PlAppData *pl_app_data, _NodeIndex node_index, plVec2 *parent_position, plVec2 *parent_dimensions, plMat4 *parent_transform) {
    if (node_index == NODE_INDEX_UNDEFINED) {
        fprintf(stderr, "DCAPP _draw_node(): attempting to draw undefined node index\n");
    }

    _Node *node = _get_node(node_index);
    switch (node->type) {

        case NODE_TYPE_CIRCLE: {

            // boolean checks
            bool use_radius         = node->circle.radius != DC_APP_VAL_INDEX_UNDEFINED;
            bool use_rotation       = node->circle.rotation != DC_APP_VAL_INDEX_UNDEFINED;
            bool use_pivot_position = (node->circle.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->circle.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

            // get dimensions
            float radius, diameter;
            if (use_radius) {
                radius   = (float)dc_app_lookup_get_value(_dc_data.lookup, node->circle.radius)->value_double;
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
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->circle.pivot_position.x)->value_double,
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->circle.pivot_position.y)->value_double};
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->circle.rotation)->value_double);

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
                    node->circle.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->circle.local_align.x)->value_integer,
                    node->circle.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->circle.local_align.y)->value_integer};

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
                    position[0] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->circle.position.x)->value_double;
                } else {
                    DcAppAlignType parent_align_x = node->circle.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->circle.parent_align.x)->value_integer;
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
                    position[1] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->circle.position.y)->value_double;
                } else {
                    DcAppAlignType parent_align_y = node->circle.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->circle.parent_align.y)->value_integer;
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
                        node->circle.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->circle.pivot_local_align.x)->value_integer,
                        node->circle.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->circle.pivot_local_align.y)->value_integer};

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
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->circle.rotation)->value_double);

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
            float  num_points = node->circle.num_segments == DC_APP_VAL_INDEX_UNDEFINED ? 40 : dc_app_lookup_get_value(_dc_data.lookup, node->circle.num_segments)->value_integer;
            plVec2 points[_NODE_CIRCLE_MAX_SEGMENTS];
            for (int ii = 0; ii < num_points; ii++) {
                float  angle  = ii * (2 * M_PI / num_points);
                plVec4 point4 = (plVec4){
                    radius * (1.0f + cos(angle)),
                    radius * (1.0f + sin(angle)),
                    0, 1};
                point4     = pl_mul_mat4_vec4(&transform, point4);
                points[ii] = (plVec2){point4.x, point4.y};
            }

            // draw fill
            if (node->circle.fill_enabled) {

                float fill_color[4] = {
                    node->circle.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->circle.fill_color.r)->value_double,
                    node->circle.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->circle.fill_color.g)->value_double,
                    node->circle.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->circle.fill_color.b)->value_double,
                    node->circle.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1 : dc_app_lookup_get_value(_dc_data.lookup, node->circle.fill_color.a)->value_double,
                };
                uint32_t pl_fill_color = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
                _ext_draw->add_convex_polygon_filled(pl_app_data->layer, points, num_points, (plDrawSolidOptions){.uColor = pl_fill_color});
            }

            // draw outline
            if (node->circle.line_enabled) {
                float line_thickness = (float)dc_app_lookup_get_value(_dc_data.lookup, node->circle.line_width)->value_double;
                float line_color[4]  = {
                    node->circle.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->circle.line_color.r)->value_double,
                    node->circle.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->circle.line_color.g)->value_double,
                    node->circle.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->circle.line_color.b)->value_double,
                    node->circle.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1 : dc_app_lookup_get_value(_dc_data.lookup, node->circle.line_color.a)->value_double,
                };
                uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
                _ext_draw->add_polygon(pl_app_data->layer, points, num_points, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
            }

            // mouse events
            if (node->circle.mouse_events.enabled) {

                // process mouse position
                plVec4 mouse_position = (plVec4){
                    _frame_data.mouse_position.x,
                    _frame_data.mouse_position.y,
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
                    _frame_data.next_hovered_node = node_index;

                    if (_frame_data.is_mouse_pressed) {
                        _frame_data.next_pressed_node = node_index;
                    }
                }

                // draw mouse events
                plVec2 position   = (plVec2){0.0f, 0.0f};
                plVec2 dimensions = (plVec2){diameter, diameter};
                if (_frame_data.pressed_node == node_index) {
                    _draw_node_list(pl_app_data, node->circle.mouse_events.pressed, &position, &dimensions, &transform);
                } else if (_frame_data.active_node == node_index) {
                    _draw_node_list(pl_app_data, node->circle.mouse_events.active, &position, &dimensions, &transform);
                } else if (_frame_data.released_node == node_index) {
                    _draw_node_list(pl_app_data, node->circle.mouse_events.released, &position, &dimensions, &transform);
                } else if (_frame_data.hovered_node == node_index) {
                    _draw_node_list(pl_app_data, node->circle.mouse_events.hovered, &position, &dimensions, &transform);
                } else {
                    _draw_node_list(pl_app_data, node->circle.mouse_events.inactive, &position, &dimensions, &transform);
                }
            }
            break;
        }

        case NODE_TYPE_CONTAINER: {

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
                use_dimension[0] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->container.dimension.x)->value_double : parent_dimensions->x,
                use_dimension[1] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->container.dimension.y)->value_double : parent_dimensions->y};

            // get virtual dimensions
            float virtual_dimension[2] = {
                use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->container.virtual_dimension.x)->value_double : dimension[0],
                use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->container.virtual_dimension.y)->value_double : dimension[1]};

            // transform
            plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

            // xform rotation (around a point)
            {
                if (use_rotation && use_pivot_position) {

                    // get pivot XY, rotation
                    float pivot_position[2] = {
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->container.pivot_position.x)->value_double,
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->container.pivot_position.y)->value_double};
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->container.rotation)->value_double);

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
                    node->container.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->container.local_align.x)->value_integer,
                    node->container.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->container.local_align.y)->value_integer};

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
                    position[0] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->container.position.x)->value_double;
                } else {
                    DcAppAlignType parent_align_x = node->container.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->container.parent_align.x)->value_integer;
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
                    position[1] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->container.position.y)->value_double;
                } else {
                    DcAppAlignType parent_align_y = node->container.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->container.parent_align.y)->value_integer;
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
                        node->container.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->container.pivot_local_align.x)->value_integer,
                        node->container.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->container.pivot_local_align.y)->value_integer};

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
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->container.rotation)->value_double);

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
            _draw_node_list(pl_app_data, node->container.child, &position_vec2, &virtual_dimensions_vec2, &transform);
            break;
        }

        case NODE_TYPE_CONDITIONAL: {
            DcValue             *val1     = dc_app_lookup_get_value(_dc_data.lookup, node->conditional.value1);
            DcAppConditionalType type     = (DcAppConditionalType)dc_app_lookup_get_value(_dc_data.lookup, node->conditional.type)->value_integer;
            bool                 use_val2 = node->conditional.value2 != DC_APP_VAL_INDEX_UNDEFINED;

            // evaluate
            bool result;
            if (use_val2) {
                DcValue *val2 = dc_app_lookup_get_value(_dc_data.lookup, node->conditional.value2);

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
                _draw_node_list(pl_app_data, node->conditional.child_true, parent_position, parent_dimensions, parent_transform);
            } else {
                _draw_node_list(pl_app_data, node->conditional.child_false, parent_position, parent_dimensions, parent_transform);
            }
            break;
        }

        case NODE_TYPE_IMAGE: {

            // boolean checks
            bool use_dimension[2] = {
                node->image.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
                node->image.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
            bool use_rotation       = node->image.rotation != DC_APP_VAL_INDEX_UNDEFINED;
            bool use_pivot_position = (node->image.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->image.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

            // get dimensions
            float dimension[2] = {
                use_dimension[0] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->image.dimension.x)->value_double : parent_dimensions->x,
                use_dimension[1] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->image.dimension.y)->value_double : parent_dimensions->y};

            // transform
            plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

            // xform rotation (around a point)
            {
                if (use_rotation && use_pivot_position) {

                    // get pivot XY, rotation
                    float pivot_position[2] = {
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->image.pivot_position.x)->value_double,
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->image.pivot_position.y)->value_double};
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->image.rotation)->value_double);

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
                    node->image.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->image.local_align.x)->value_integer,
                    node->image.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->image.local_align.y)->value_integer};

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
                    position[0] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->image.position.x)->value_double;
                } else {
                    DcAppAlignType parent_align_x = node->image.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->image.parent_align.x)->value_integer;
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
                    position[1] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->image.position.y)->value_double;
                } else {
                    DcAppAlignType parent_align_y = node->image.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->image.parent_align.y)->value_integer;
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
                        node->image.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->image.pivot_local_align.x)->value_integer,
                        node->image.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->image.pivot_local_align.y)->value_integer};

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
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->image.rotation)->value_double);

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

            // draw
            // use transform here::
            _ext_draw->add_image(pl_app_data->layer, node->image.texture.bind_group_handle.uData, (plVec2){100.0f, 100.0f}, (plVec2){700.0f, 700.0f});

            // mouse events
            if (node->image.mouse_events.enabled) {

                // process mouse position
                plVec4 mouse_position = (plVec4){
                    _frame_data.mouse_position.x,
                    _frame_data.mouse_position.y,
                    0, 1};
                plMat4 transform_inverse = pl_mat4t_invert(&transform);
                mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

                // check whether mouse is over/in
                bool inside = mouse_position.x > 0 && mouse_position.x < dimension[0] && mouse_position.y > 0 && mouse_position.y < dimension[1];

                // update global states
                if (inside) {
                    _frame_data.next_hovered_node = node_index;

                    if (_frame_data.is_mouse_pressed) {
                        _frame_data.next_pressed_node = node_index;
                    }
                }

                // draw mouse events
                plVec2 position   = (plVec2){0.0f, 0.0f};
                plVec2 dimensions = (plVec2){dimension[0], dimension[1]};
                if (_frame_data.pressed_node == node_index) {
                    _draw_node_list(pl_app_data, node->image.mouse_events.pressed, &position, &dimensions, &transform);
                } else if (_frame_data.active_node == node_index) {
                    _draw_node_list(pl_app_data, node->image.mouse_events.active, &position, &dimensions, &transform);
                } else if (_frame_data.released_node == node_index) {
                    _draw_node_list(pl_app_data, node->image.mouse_events.released, &position, &dimensions, &transform);
                } else if (_frame_data.hovered_node == node_index) {
                    _draw_node_list(pl_app_data, node->image.mouse_events.hovered, &position, &dimensions, &transform);
                } else {
                    _draw_node_list(pl_app_data, node->image.mouse_events.inactive, &position, &dimensions, &transform);
                }
            }
            break;
        }

        case NODE_TYPE_LINE: {

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
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->line.pivot_position.x)->value_double,
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->line.pivot_position.y)->value_double};
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->line.rotation)->value_double);

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
                    use_position[0] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->line.position.x)->value_double : 0.0f,
                    use_position[1] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->line.position.y)->value_double : 0.0f,
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
                    (float)dc_app_lookup_get_value(_dc_data.lookup, node->line.sb_points[ii].x)->value_double,
                    (float)dc_app_lookup_get_value(_dc_data.lookup, node->line.sb_points[ii].y)->value_double};

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
                float line_thickness = (float)dc_app_lookup_get_value(_dc_data.lookup, node->line.line_width)->value_double;
                float line_color[4]  = {
                    node->line.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->line.line_color.r)->value_double,
                    node->line.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->line.line_color.g)->value_double,
                    node->line.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->line.line_color.b)->value_double,
                    node->line.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1 : dc_app_lookup_get_value(_dc_data.lookup, node->line.line_color.a)->value_double,
                };
                uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
                _ext_draw->add_lines(pl_app_data->layer, points, num_points, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
            }

            break;
        }

        case NODE_TYPE_PANEL: {

            // boolean checks
            bool use_virtual_dimension[2] = {
                node->panel.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
                node->panel.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};

            // get virtual dimensions
            float virtual_dimension[2] = {
                use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->panel.virtual_dimension.x)->value_double : parent_dimensions->x,
                use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->panel.virtual_dimension.y)->value_double : parent_dimensions->y};

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
            _draw_node_list(pl_app_data, node->panel.child, &position_vec2, &virtual_dimensions_vec2, &transform);
            break;
        }

        case NODE_TYPE_POLYGON: {

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
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->polygon.pivot_position.x)->value_double,
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->polygon.pivot_position.y)->value_double};
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->polygon.rotation)->value_double);

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
                float position[2];
                if (use_position[0]) {
                    position[0] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->polygon.position.x)->value_double;
                }
                if (use_position[1]) {
                    position[1] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->polygon.position.y)->value_double;
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
            int    num_points = sbcount(node->polygon.sb_points);
            plVec2 raw_points[_NODE_POLYGON_MAX_POINTS];
            plVec2 points[_NODE_POLYGON_MAX_POINTS];
            for (int ii = 0; ii < num_points; ii++) {

                // get raw point
                raw_points[ii] = (plVec2){
                    (float)dc_app_lookup_get_value(_dc_data.lookup, node->polygon.sb_points[ii].x)->value_double,
                    (float)dc_app_lookup_get_value(_dc_data.lookup, node->polygon.sb_points[ii].y)->value_double};

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
                    node->polygon.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->polygon.fill_color.r)->value_double,
                    node->polygon.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->polygon.fill_color.g)->value_double,
                    node->polygon.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->polygon.fill_color.b)->value_double,
                    node->polygon.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1 : dc_app_lookup_get_value(_dc_data.lookup, node->polygon.fill_color.a)->value_double,
                };
                uint32_t pl_fill_color = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
                _ext_draw->add_convex_polygon_filled(pl_app_data->layer, points, num_points, (plDrawSolidOptions){.uColor = pl_fill_color});
            }

            // draw outline
            if (node->polygon.line_enabled) {
                float line_thickness = (float)dc_app_lookup_get_value(_dc_data.lookup, node->polygon.line_width)->value_double;
                float line_color[4]  = {
                    node->polygon.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->polygon.line_color.r)->value_double,
                    node->polygon.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->polygon.line_color.g)->value_double,
                    node->polygon.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->polygon.line_color.b)->value_double,
                    node->polygon.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1 : dc_app_lookup_get_value(_dc_data.lookup, node->polygon.line_color.a)->value_double,
                };
                uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
                _ext_draw->add_polygon(pl_app_data->layer, points, num_points, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
            }

            // mouse events
            if (node->polygon.mouse_events.enabled) {

                // process mouse position
                plVec4 mouse_position = (plVec4){
                    _frame_data.mouse_position.x,
                    _frame_data.mouse_position.y,
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
                    _frame_data.next_hovered_node = node_index;

                    if (_frame_data.is_mouse_pressed) {
                        _frame_data.next_pressed_node = node_index;
                    }
                }

                // draw mouse events
                plVec2 position   = (plVec2){min_pos.x, min_pos.y};
                plVec2 dimensions = (plVec2){max_pos.x - min_pos.x, max_pos.y - min_pos.y};
                if (_frame_data.pressed_node == node_index) {
                    _draw_node_list(pl_app_data, node->polygon.mouse_events.pressed, &position, &dimensions, &transform);
                } else if (_frame_data.active_node == node_index) {
                    _draw_node_list(pl_app_data, node->polygon.mouse_events.active, &position, &dimensions, &transform);
                } else if (_frame_data.released_node == node_index) {
                    _draw_node_list(pl_app_data, node->polygon.mouse_events.released, &position, &dimensions, &transform);
                } else if (_frame_data.hovered_node == node_index) {
                    _draw_node_list(pl_app_data, node->polygon.mouse_events.hovered, &position, &dimensions, &transform);
                } else {
                    _draw_node_list(pl_app_data, node->polygon.mouse_events.inactive, &position, &dimensions, &transform);
                }
            }
            break;
        }

        case NODE_TYPE_RECTANGLE: {

            // boolean checks
            bool use_dimension[2] = {
                node->rectangle.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
                node->rectangle.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
            bool use_rotation       = node->rectangle.rotation != DC_APP_VAL_INDEX_UNDEFINED;
            bool use_pivot_position = (node->rectangle.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->rectangle.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

            // get dimensions
            float dimension[2] = {
                use_dimension[0] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.dimension.x)->value_double : parent_dimensions->x,
                use_dimension[1] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.dimension.y)->value_double : parent_dimensions->y};

            // transform
            plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

            // xform rotation (around a point)
            {
                if (use_rotation && use_pivot_position) {

                    // get pivot XY, rotation
                    float pivot_position[2] = {
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.pivot_position.x)->value_double,
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.pivot_position.y)->value_double};
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.rotation)->value_double);

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
                    node->rectangle.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.local_align.x)->value_integer,
                    node->rectangle.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.local_align.y)->value_integer};

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
                    position[0] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.position.x)->value_double;
                } else {
                    DcAppAlignType parent_align_x = node->rectangle.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.parent_align.x)->value_integer;
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
                    position[1] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.position.y)->value_double;
                } else {
                    DcAppAlignType parent_align_y = node->rectangle.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.parent_align.y)->value_integer;
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
                        node->rectangle.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.pivot_local_align.x)->value_integer,
                        node->rectangle.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.pivot_local_align.y)->value_integer};

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
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.rotation)->value_double);

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
                    node->rectangle.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.fill_color.r)->value_double,
                    node->rectangle.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.fill_color.g)->value_double,
                    node->rectangle.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.fill_color.b)->value_double,
                    node->rectangle.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1 : dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.fill_color.a)->value_double,
                };
                uint32_t pl_fill_color = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
                _ext_draw->add_convex_polygon_filled(pl_app_data->layer, points, 4, (plDrawSolidOptions){.uColor = pl_fill_color});
            }

            // draw outline
            if (node->rectangle.line_enabled) {
                float line_thickness = (float)dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.line_width)->value_double;
                float line_color[4]  = {
                    node->rectangle.line_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.line_color.r)->value_double,
                    node->rectangle.line_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.line_color.g)->value_double,
                    node->rectangle.line_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.line_color.b)->value_double,
                    node->rectangle.line_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1 : dc_app_lookup_get_value(_dc_data.lookup, node->rectangle.line_color.a)->value_double,
                };
                uint32_t pl_line_color = PL_COLOR_32_RGBA(line_color[0], line_color[1], line_color[2], line_color[3]);
                _ext_draw->add_polygon(pl_app_data->layer, points, 4, (plDrawLineOptions){.uColor = pl_line_color, .fThickness = line_thickness});
            }

            // mouse events
            if (node->rectangle.mouse_events.enabled) {

                // process mouse position
                plVec4 mouse_position = (plVec4){
                    _frame_data.mouse_position.x,
                    _frame_data.mouse_position.y,
                    0, 1};
                plMat4 transform_inverse = pl_mat4t_invert(&transform);
                mouse_position           = pl_mul_mat4_vec4(&transform_inverse, mouse_position);

                // check whether mouse is over/in
                bool inside = mouse_position.x > 0 && mouse_position.x < dimension[0] && mouse_position.y > 0 && mouse_position.y < dimension[1];

                // update global states
                if (inside) {
                    _frame_data.next_hovered_node = node_index;

                    if (_frame_data.is_mouse_pressed) {
                        _frame_data.next_pressed_node = node_index;
                    }
                }

                // draw mouse events
                plVec2 position   = (plVec2){0.0f, 0.0f};
                plVec2 dimensions = (plVec2){dimension[0], dimension[1]};
                if (_frame_data.pressed_node == node_index) {
                    _draw_node_list(pl_app_data, node->rectangle.mouse_events.pressed, &position, &dimensions, &transform);
                } else if (_frame_data.active_node == node_index) {
                    _draw_node_list(pl_app_data, node->rectangle.mouse_events.active, &position, &dimensions, &transform);
                } else if (_frame_data.released_node == node_index) {
                    _draw_node_list(pl_app_data, node->rectangle.mouse_events.released, &position, &dimensions, &transform);
                } else if (_frame_data.hovered_node == node_index) {
                    _draw_node_list(pl_app_data, node->rectangle.mouse_events.hovered, &position, &dimensions, &transform);
                } else {
                    _draw_node_list(pl_app_data, node->rectangle.mouse_events.inactive, &position, &dimensions, &transform);
                }
            }
            break;
        }

        case NODE_TYPE_SET: {

            DcValue *var_value = dc_app_lookup_get_value(_dc_data.lookup, dc_app_lookup_get_var(_dc_data.lookup, node->set.var_index)->value_index);
            DcValue *op_value  = dc_app_lookup_get_value(_dc_data.lookup, node->set.operand);

            // get operation
            DcAppSetType operation = node->set.operation == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_SET_TYPE_UNDEFINED : (DcAppSetType)(dc_app_lookup_get_value(_dc_data.lookup, node->set.operation)->value_integer);

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
                            int num_chars = DC_VALUE_STRING_BUFFER_SIZE - strlen(var_value->value_string) - 1;
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
            dc_app_lookup_refresh_var_from_value(_dc_data.lookup, node->set.var_index);
            break;
        }

        case NODE_TYPE_TERRAIN: {

            // boolean checks
            bool use_dimension[2] = {
                node->terrain.dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
                node->terrain.dimension.y != DC_APP_VAL_INDEX_UNDEFINED};
            bool use_rotation       = node->terrain.rotation != DC_APP_VAL_INDEX_UNDEFINED;
            bool use_pivot_position = (node->terrain.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->terrain.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

            // get dimensions
            float dimension[2] = {
                use_dimension[0] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.dimension.x)->value_double : parent_dimensions->x,
                use_dimension[1] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.dimension.y)->value_double : parent_dimensions->y};

            // transform
            plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

            // xform rotation (around a point)
            {
                if (use_rotation && use_pivot_position) {

                    // get pivot XY, rotation
                    float pivot_position[2] = {
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.pivot_position.x)->value_double,
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.pivot_position.y)->value_double};
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.rotation)->value_double);

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
                    node->terrain.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.local_align.x)->value_integer,
                    node->terrain.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.local_align.y)->value_integer};

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
                    position[0] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.position.x)->value_double;
                } else {
                    DcAppAlignType parent_align_x = node->terrain.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->terrain.parent_align.x)->value_integer;
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
                    position[1] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.position.y)->value_double;
                } else {
                    DcAppAlignType parent_align_y = node->terrain.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->terrain.parent_align.y)->value_integer;
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
                        node->terrain.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.pivot_local_align.x)->value_integer,
                        node->terrain.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.pivot_local_align.y)->value_integer};

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
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->terrain.rotation)->value_double);

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
            // ext_draw->add_text(app_data->layer, (plVec2){0, 0}, sb_text, text_options);
            break;
        }

        case NODE_TYPE_TEXT: {

            // expand text
            static char *sb_text = NULL;
            sbclear(sb_text);
            for (int ii = 0; ii < sbcount(node->text.sb_vals); ii++) {

                // filler
                char *filler = &(node->text.sb_fillers[node->text.sb_filler_indices[ii]]);
                sbpushn(sb_text, filler, strlen(filler));

                // value
                DcValueType format_type = node->text.sb_format_types[ii];
                char       *format      = &(node->text.sb_formats[node->text.sb_format_indices[ii]]);
                DcValue    *val         = dc_app_lookup_get_value(_dc_data.lookup, node->text.sb_vals[ii]);
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
                sbpushn(sb_text, val_str, strlen(val_str));
            }

            // ending filler
            char *filler = &(node->text.sb_fillers[node->text.sb_filler_indices[sbcount(node->text.sb_vals)]]);
            sbpushn(sb_text, filler, strlen(filler));
            sbpush(sb_text, '\0');

            // get text dimensions
            plDrawTextOptions text_options = {0};
            text_options.ptFont            = pl_app_data->cousine_sdf_font;
            float fill_color[4]            = {
                node->text.fill_color.r == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->text.fill_color.r)->value_double,
                node->text.fill_color.g == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->text.fill_color.g)->value_double,
                node->text.fill_color.b == DC_APP_VAL_INDEX_UNDEFINED ? 0 : dc_app_lookup_get_value(_dc_data.lookup, node->text.fill_color.b)->value_double,
                node->text.fill_color.a == DC_APP_VAL_INDEX_UNDEFINED ? 1 : dc_app_lookup_get_value(_dc_data.lookup, node->text.fill_color.a)->value_double,
            };
            text_options.uColor = PL_COLOR_32_RGBA(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
            text_options.fSize  = node->text.size == DC_APP_VAL_INDEX_UNDEFINED ? 1.0f : (float)dc_app_lookup_get_value(_dc_data.lookup, node->text.size)->value_double;
            plVec2 pl_size      = _ext_draw->calculate_text_size(sb_text, text_options);

            // boolean checks
            bool use_rotation       = node->text.rotation != DC_APP_VAL_INDEX_UNDEFINED;
            bool use_pivot_position = (node->text.pivot_position.x != DC_APP_VAL_INDEX_UNDEFINED && node->text.pivot_position.y != DC_APP_VAL_INDEX_UNDEFINED);

            // get dimensions
            float dimension[2] = {pl_size.x, text_options.fSize};

            // transform
            plMat4 transform = (plMat4){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

            // xform rotation (around a point)
            {
                if (use_rotation && use_pivot_position) {

                    // get pivot XY, rotation
                    float pivot_position[2] = {
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->text.pivot_position.x)->value_double,
                        (float)dc_app_lookup_get_value(_dc_data.lookup, node->text.pivot_position.y)->value_double};
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->text.rotation)->value_double);

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
                    node->text.local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->text.local_align.x)->value_integer,
                    node->text.local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->text.local_align.y)->value_integer};

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
                    node->text.position.x != DC_APP_VAL_INDEX_UNDEFINED,
                    node->text.position.y != DC_APP_VAL_INDEX_UNDEFINED};

                // get position
                float position[2];
                if (use_position[0]) {
                    position[0] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->text.position.x)->value_double;
                } else {
                    DcAppAlignType parent_align_x = node->text.parent_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->text.parent_align.x)->value_integer;
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
                    position[1] = (float)dc_app_lookup_get_value(_dc_data.lookup, node->text.position.y)->value_double;
                } else {
                    DcAppAlignType parent_align_y = node->text.parent_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : dc_app_lookup_get_value(_dc_data.lookup, node->text.parent_align.y)->value_integer;
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
                        node->text.pivot_local_align.x == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->text.pivot_local_align.x)->value_integer,
                        node->text.pivot_local_align.y == DC_APP_VAL_INDEX_UNDEFINED ? DC_APP_ALIGN_TYPE_UNDEFINED : (DcAppAlignType)dc_app_lookup_get_value(_dc_data.lookup, node->text.pivot_local_align.y)->value_integer};

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
                            fprintf(stderr, "Unknown pivot alignment in <text> draw call: %d\n", local_pivot_aligns[0]);
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
                            fprintf(stderr, "Unknown pivot alignment in <text> draw call: %d\n", local_pivot_aligns[1]);
                            break;
                    }
                    float rotation = pl_radiansf((float)dc_app_lookup_get_value(_dc_data.lookup, node->text.rotation)->value_double);

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
            _ext_draw->add_text(pl_app_data->layer, (plVec2){0, 0}, sb_text, text_options);

            break;
        }

        case NODE_TYPE_WINDOW: {
            // TODO move this code to only the resize() function

            // current dimensions
            uint32_t dimensionX, dimensionY;
            _ext_windows->get_size(pl_app_data->window, &dimensionX, &dimensionY);

            // boolean checks
            bool use_virtual_dimension[2] = {
                node->panel.virtual_dimension.x != DC_APP_VAL_INDEX_UNDEFINED,
                node->panel.virtual_dimension.y != DC_APP_VAL_INDEX_UNDEFINED};

            // all transform parameters
            float dimension[2]         = {(float)dimensionX, (float)dimensionY};
            float virtual_dimension[2] = {
                use_virtual_dimension[0] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->window.virtual_dimension.x)->value_double : dimension[0],
                use_virtual_dimension[1] ? (float)dc_app_lookup_get_value(_dc_data.lookup, node->window.virtual_dimension.y)->value_double : dimension[1]};

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
            _draw_node_list(pl_app_data, node->window.child, &position_vec2, &virtual_dimensions_vec2, &transform);
            break;
        }

        default:
            break;
    }
}

#include "_dcapp_utils.c"
