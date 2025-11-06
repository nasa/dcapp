#include "dcapp.h"

#include "../src/utils/file.h"
#include "../src/utils/string.h"
#include "libxml/globals.h"
#include "libxml/xmlstring.h"

#include <ctype.h>

// returns the first child (if any)
static _NodeIndex _process_xml_node_children(_PlAppData *pl_app_data, xmlNodePtr xml_node, _NodeIndex node_index, DcAppElemType elem_type, const char *directory) {
    xmlNodePtr xml_child_node = xml_node->children;

    _NodeIndex first_child_index         = NODE_INDEX_UNDEFINED;
    _NodeIndex previous_child_node_index = NODE_INDEX_UNDEFINED;
    while (xml_child_node) {

        _NodeIndex child_node_index = _process_xml_node(pl_app_data, xml_child_node, node_index, elem_type, directory);

        if (child_node_index != NODE_INDEX_UNDEFINED) {

            // get node addresses here since the address could change per node process
            _Node *node                = _get_node(node_index);
            _Node *child_node          = _get_node(child_node_index);
            _Node *previous_child_node = _get_node(previous_child_node_index);

            // if the current node and child exists
            if (node && child_node) {

                // set nodes's first child if this is the first child
                if (previous_child_node_index == NODE_INDEX_UNDEFINED) {
                    first_child_index = child_node_index;
                }
            }

            // if there is a previous node
            if (previous_child_node) {

                // set the next node of the previous node
                previous_child_node->next = child_node_index;
            }

            // set previous child node, accounting for cases where the
            // child node is actually a node list
            _NodeIndex last_child_node_index = child_node_index;
            _Node     *last_child_node       = _get_node(last_child_node_index);
            while (last_child_node->next != NODE_INDEX_UNDEFINED) {
                last_child_node_index = last_child_node->next;
                last_child_node       = _get_node(last_child_node_index);
            }
            previous_child_node_index = last_child_node_index;
        }

        // increment pointer
        xml_child_node = xml_child_node->next;
    }

    return first_child_index;
}

static _NodeIndex _process_xml_node(_PlAppData *pl_app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {

    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);
    switch (elem_type) {

        case DC_APP_ELEM_TYPE_NONELEM: {
            // ignore non-element nodes
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_CIRCLE: {
            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_CIRCLE;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            dc_node.circle.mouse_events.active   = NODE_INDEX_UNDEFINED;
            dc_node.circle.mouse_events.hovered  = NODE_INDEX_UNDEFINED;
            dc_node.circle.mouse_events.inactive = NODE_INDEX_UNDEFINED;
            dc_node.circle.mouse_events.pressed  = NODE_INDEX_UNDEFINED;
            dc_node.circle.mouse_events.released = NODE_INDEX_UNDEFINED;

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.circle.position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                dc_node.circle.position.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.circle.position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                dc_node.circle.position.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent x align
            xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
            if (raw_parent_x_align) {
                char cleaned_parent_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_x_align, (const char *)raw_parent_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_x_align);
                dc_node.circle.parent_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_x_align);
            } else {
                dc_node.circle.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent y align
            xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
            if (raw_parent_y_align) {
                char cleaned_parent_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_y_align, (const char *)raw_parent_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_y_align);
                dc_node.circle.parent_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_y_align);
            } else {
                dc_node.circle.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local x align
            xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
            if (!raw_x_align) {
                raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
            }
            if (raw_x_align) {
                char cleaned_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_align, (const char *)raw_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_align);
                dc_node.circle.local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_align);
            } else {
                dc_node.circle.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local y align
            xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
            if (!raw_y_align) {
                raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
            }
            if (raw_y_align) {
                char cleaned_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_align, (const char *)raw_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_align);
                dc_node.circle.local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_align);
            } else {
                dc_node.circle.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.circle.rotation = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                dc_node.circle.rotation = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.circle.pivot_position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.circle.pivot_position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

                dc_node.circle.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.circle.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

            } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

                xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
                if (raw_pivot_align_x) {
                    char cleaned_pivot_align_x[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_x, (const char *)raw_pivot_align_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_x);
                    dc_node.circle.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_x);
                } else {
                    dc_node.circle.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                }

                xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
                if (raw_pivot_align_y) {
                    char cleaned_pivot_align_y[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_y, (const char *)raw_pivot_align_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_y);
                    dc_node.circle.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_y);
                } else {
                    dc_node.circle.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
                }

                dc_node.circle.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.circle.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
            } else {
                fprintf(stderr, "DCAPP _process_xml_node(): Circle: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // radius
            xmlChar *raw_radius = xmlGetProp(xml_node, BAD_CAST "Radius");
            if (raw_radius) {
                char cleaned_radius[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_radius, (const char *)raw_radius, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_radius);
                dc_node.circle.radius = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_radius);
            } else {
                dc_node.circle.radius = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // segments
            xmlChar *raw_segments = xmlGetProp(xml_node, BAD_CAST "Segments");
            if (raw_segments) {
                char cleaned_segments[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_segments, (const char *)raw_segments, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_segments);
                dc_node.circle.num_segments = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_segments);
            } else {
                dc_node.circle.num_segments = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // line width
            xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
            if (raw_line_width) {
                char cleaned_line_width[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_line_width, (const char *)raw_line_width, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_line_width);
                dc_node.circle.line_width = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_line_width);
            } else {
                dc_node.circle.line_width = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // colors
            dc_node.circle.fill_enabled = _load_color_from_string(xml_node, "FillColor", &(dc_node.circle.fill_color));
            dc_node.circle.line_enabled = _load_color_from_string(xml_node, "LineColor", &(dc_node.circle.line_color));

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _process_xml_node_children(pl_app_data, xml_node, node_index, elem_type, directory);

            // enable/disable mouse events
            _MouseEventChildren *mouse_events = &(_get_node(node_index)->circle.mouse_events);
            mouse_events->enabled =
                (mouse_events->pressed != NODE_INDEX_UNDEFINED) ||
                (mouse_events->released != NODE_INDEX_UNDEFINED) ||
                (mouse_events->active != NODE_INDEX_UNDEFINED) ||
                (mouse_events->inactive != NODE_INDEX_UNDEFINED) ||
                (mouse_events->hovered != NODE_INDEX_UNDEFINED);

            // return
            return node_index;
        }

        case DC_APP_ELEM_TYPE_CONSTANT: {
            // ignore at this point
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_CONTAINER: {
            _Node dc_node;
            dc_node.type   = NODE_TYPE_CONTAINER;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.container.position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                dc_node.container.position.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.container.position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                dc_node.container.position.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // x dimension
            xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
            if (!raw_x_dimension) {
                raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
            }
            if (raw_x_dimension) {
                char cleaned_x_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_dimension, (const char *)raw_x_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_dimension);
                dc_node.container.dimension.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_dimension);
            } else {
                dc_node.container.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y dimension
            xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
            if (!raw_y_dimension) {
                raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
            }
            if (raw_y_dimension) {
                char cleaned_y_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_dimension, (const char *)raw_y_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_dimension);
                dc_node.container.dimension.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_dimension);
            } else {
                dc_node.container.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // virtual x dimension
            xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
            if (!raw_x_virtual_dimension) {
                raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
            }
            if (raw_x_virtual_dimension) {
                char cleaned_x_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_virtual_dimension, (const char *)raw_x_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_virtual_dimension);
                dc_node.container.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_virtual_dimension);
            } else {
                dc_node.container.virtual_dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // virtual y virtual_dimension
            xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
            if (!raw_y_virtual_dimension) {
                raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
            }
            if (raw_y_virtual_dimension) {
                char cleaned_y_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_virtual_dimension, (const char *)raw_y_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_virtual_dimension);
                dc_node.container.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_virtual_dimension);
            } else {
                dc_node.container.virtual_dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local x align
            xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
            if (!raw_x_align) {
                raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
            }
            if (raw_x_align) {
                char cleaned_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_align, (const char *)raw_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_align);
                dc_node.container.local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_align);
            } else {
                dc_node.container.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local y align
            xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
            if (!raw_y_align) {
                raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
            }
            if (raw_y_align) {
                char cleaned_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_align, (const char *)raw_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_align);
                dc_node.container.local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_align);
            } else {
                dc_node.container.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent x align
            xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
            if (raw_parent_x_align) {
                char cleaned_parent_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_x_align, (const char *)raw_parent_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_x_align);
                dc_node.container.parent_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_x_align);
            } else {
                dc_node.container.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent y align
            xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
            if (raw_parent_y_align) {
                char cleaned_parent_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_y_align, (const char *)raw_parent_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_y_align);
                dc_node.container.parent_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_y_align);
            } else {
                dc_node.container.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.container.rotation = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                dc_node.container.rotation = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.container.pivot_position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.container.pivot_position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

                dc_node.container.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.container.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

            } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

                xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
                if (raw_pivot_align_x) {
                    char cleaned_pivot_align_x[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_x, (const char *)raw_pivot_align_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_x);
                    dc_node.container.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_x);
                } else {
                    dc_node.container.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                }

                xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
                if (raw_pivot_align_y) {
                    char cleaned_pivot_align_y[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_y, (const char *)raw_pivot_align_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_y);
                    dc_node.container.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_y);
                } else {
                    dc_node.container.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
                }

                dc_node.container.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.container.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
            } else {
                fprintf(stderr, "DCAPP _process_xml_node(): Container: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _NodeIndex first_child_index = _process_xml_node_children(pl_app_data, xml_node, node_index, elem_type, directory);

            // update child index
            _Node *node           = _get_node(node_index);
            node->container.child = first_child_index;

            // return
            return node_index;
        }

        // really just the root element, left in for legacy reasons
        case DC_APP_ELEM_TYPE_DCAPP: {
            _process_xml_node_children(pl_app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_DEFAULT: {
            // ignore at this point
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_FALSE: {
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_IF: {

                    // process children
                    _NodeIndex first_child_index = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, elem_type, directory);

                    // update child false node
                    _Node *parent_node                   = _get_node(parent_node_index);
                    parent_node->conditional.child_false = first_child_index;
                    break;
                }
                default:
                    fprintf(stderr, "DCAPP _process_xml_node(): Invalid elem parent of type %s for <False>\n", dc_app_elem_type_to_string(parent_elem_type));
            }
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_IF: {

            _Node dc_node                   = {};
            dc_node.type                    = NODE_TYPE_CONDITIONAL;
            dc_node.parent                  = parent_node_index;
            dc_node.next                    = NODE_INDEX_UNDEFINED;
            dc_node.conditional.child_true  = NODE_INDEX_UNDEFINED;
            dc_node.conditional.child_false = NODE_INDEX_UNDEFINED;

            // conditional type
            xmlChar *raw_type = xmlGetProp(xml_node, BAD_CAST "Operation");
            if (raw_type) {
                char cleaned_type[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_type, (const char *)raw_type, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_type);
                dc_node.conditional.type = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_type);
            } else {
                dc_node.conditional.type = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // value1
            xmlChar *raw_value1 = xmlGetProp(xml_node, BAD_CAST "Value");
            if (!raw_value1) {
                raw_value1 = xmlGetProp(xml_node, BAD_CAST "Value1");
            }
            if (raw_value1) {
                char cleaned_value1[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_value1, (const char *)raw_value1, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_value1);
                dc_node.conditional.value1 = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_STRING, cleaned_value1);
            } else {
                fprintf(stderr, "DCAPP _process_xml_node: Conditional: no value specified\n");
            }

            // value2
            xmlChar *raw_value2 = xmlGetProp(xml_node, BAD_CAST "Value2");
            if (raw_value2) {
                char cleaned_value2[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_value2, (const char *)raw_value2, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_value2);
                dc_node.conditional.value2 = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_STRING, cleaned_value2);
            } else {
                dc_node.conditional.value2 = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children (assigning to true/false handled in separate cases, e.g. DC_APP_ELEM_TYPE_TRUE)
            _NodeIndex first_child_index = _process_xml_node_children(pl_app_data, xml_node, node_index, elem_type, directory);

            // handle implicit <True> elements
            if (first_child_index != NODE_INDEX_UNDEFINED) {

                // ignore if True element already exists
                _Node *node = _get_node(node_index);
                if (node->conditional.child_true == NODE_INDEX_UNDEFINED) {
                    node->conditional.child_true = first_child_index;
                } else {
                    printf("DCApp _process_xml_node: Conditional: <If> element has <True> explicit and implicit elements. Ignoring the implicit definitions\n");
                }
            }

            // return
            return node_index;
        }

        case DC_APP_ELEM_TYPE_IMAGE: {

            _Node dc_node;
            memset(&dc_node, 0, sizeof(dc_node));
            dc_node.type   = NODE_TYPE_IMAGE;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            dc_node.image.mouse_events.active   = NODE_INDEX_UNDEFINED;
            dc_node.image.mouse_events.hovered  = NODE_INDEX_UNDEFINED;
            dc_node.image.mouse_events.inactive = NODE_INDEX_UNDEFINED;
            dc_node.image.mouse_events.pressed  = NODE_INDEX_UNDEFINED;
            dc_node.image.mouse_events.released = NODE_INDEX_UNDEFINED;

            // get filepath
            xmlChar *filepath = xmlGetProp(xml_node, BAD_CAST "File");
            if (!filepath) {
                fprintf(stderr, "DCAPP _process_xml_node() image: 'File' field missing for Image node\n");
            }
            char cleaned_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
            strncpy(cleaned_filepath, (const char *)filepath, DC_VALUE_STRING_BUFFER_SIZE - 1);
            xmlFree(filepath);

            // get canonical path
            char canon_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
            if (dc_utils_is_relative_path(cleaned_filepath)) {
                char abs_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
                dc_utils_join_paths(directory, cleaned_filepath, abs_filepath, sizeof(abs_filepath));
                dc_utils_canonicalize_path(abs_filepath, canon_filepath, sizeof(canon_filepath));
            } else {
                dc_utils_canonicalize_path(cleaned_filepath, canon_filepath, sizeof(canon_filepath));
            }

            // check for existing texture
            _TextureIndex texture_index = TEXTURE_INDEX_UNDEFINED;
            for (int ii = 0; ii < sbcount(_sb_textures); ii++) {
                const char *comp_name = &(_sb_texture_names[_sb_texture_name_offsets[ii]]);
                if (strcmp(canon_filepath, comp_name) == 0) {
                    texture_index = ii;
                }
            }

            // load new texture if it doesn't exist
            if (texture_index == TEXTURE_INDEX_UNDEFINED) {

                // get device
                plDevice *device = _ext_starter->get_device();

                // load raw data
                size_t         file_data_size;
                unsigned char *file_data = dc_utils_load_binary_file(canon_filepath, &file_data_size);

                // load as image data
                int            image_width, image_height, channels;
                unsigned char *image_data = _ext_image->load(file_data, (int)file_data_size, &image_width, &image_height, &channels, 4);
                free(file_data);

                // create texture (allocate image, buffer, bind)
                _Texture texture = _create_texture(pl_app_data, image_width, image_height, canon_filepath);

                // set the initial pl_texture usage (this is a no-op in metal but does layout transition for vulkan)
                plBlitEncoder *encoder = _ext_starter->get_blit_encoder();
                _ext_gfx->set_texture_usage(encoder, texture.texture_handle, PL_TEXTURE_USAGE_SAMPLED, 0);

                // copy memory to mapped staging buffer
                plBuffer *staging_buffer = _ext_gfx->get_buffer(device, pl_app_data->staging_buffer_handle);
                memcpy(staging_buffer->tMemoryAllocation.pHostMapped, image_data, image_width * image_height * 4);

                // copy staging buffer to image
                plBufferImageCopy buffer_image_copy;
                memset(&buffer_image_copy, 0, sizeof(plBufferImageCopy));
                buffer_image_copy.uImageWidth    = (uint32_t)image_width;
                buffer_image_copy.uImageHeight   = (uint32_t)image_height;
                buffer_image_copy.uImageDepth    = 1;
                buffer_image_copy.uLayerCount    = 1;
                buffer_image_copy.szBufferOffset = 0;
                _ext_gfx->copy_buffer_to_texture(encoder, pl_app_data->staging_buffer_handle, texture.texture_handle, 1, &buffer_image_copy);

                // return encoder
                _ext_starter->return_blit_encoder(encoder);

                // free image data
                _ext_image->free(image_data);

                // add texture to internal arrays
                sbpush(_sb_texture_name_offsets, sbcount(_sb_texture_names));
                sbpushn(_sb_texture_names, canon_filepath, strlen(canon_filepath) + 1);
                sbpush(_sb_textures, texture);
                texture_index = sbcount(_sb_textures) - 1;
            }

            // update structure
            dc_node.image.texture_index = texture_index;

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.image.position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                dc_node.image.position.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.image.position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                dc_node.image.position.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // x dimension
            xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
            if (!raw_x_dimension) {
                raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
            }
            if (raw_x_dimension) {
                char cleaned_x_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_dimension, (const char *)raw_x_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_dimension);
                dc_node.image.dimension.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_dimension);
            } else {
                dc_node.image.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y dimension
            xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
            if (!raw_y_dimension) {
                raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
            }
            if (raw_y_dimension) {
                char cleaned_y_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_dimension, (const char *)raw_y_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_dimension);
                dc_node.image.dimension.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_dimension);
            } else {
                dc_node.image.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent x align
            xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
            if (raw_parent_x_align) {
                char cleaned_parent_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_x_align, (const char *)raw_parent_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_x_align);
                dc_node.image.parent_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_x_align);
            } else {
                dc_node.image.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent y align
            xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
            if (raw_parent_y_align) {
                char cleaned_parent_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_y_align, (const char *)raw_parent_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_y_align);
                dc_node.image.parent_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_y_align);
            } else {
                dc_node.image.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local x align
            xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
            if (!raw_x_align) {
                raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
            }
            if (raw_x_align) {
                char cleaned_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_align, (const char *)raw_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_align);
                dc_node.image.local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_align);
            } else {
                dc_node.image.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local y align
            xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
            if (!raw_y_align) {
                raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
            }
            if (raw_y_align) {
                char cleaned_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_align, (const char *)raw_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_align);
                dc_node.image.local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_align);
            } else {
                dc_node.image.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.image.rotation = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                dc_node.image.rotation = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.image.pivot_position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.image.pivot_position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

                dc_node.image.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.image.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

            } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

                xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
                if (raw_pivot_align_x) {
                    char cleaned_pivot_align_x[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_x, (const char *)raw_pivot_align_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_x);
                    dc_node.image.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_x);
                } else {
                    dc_node.image.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                }

                xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
                if (raw_pivot_align_y) {
                    char cleaned_pivot_align_y[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_y, (const char *)raw_pivot_align_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_y);
                    dc_node.image.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_y);
                } else {
                    dc_node.image.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
                }

                dc_node.image.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.image.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
            } else {
                fprintf(stderr, "DCAPP _process_xml_node(): Image: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _process_xml_node_children(pl_app_data, xml_node, node_index, elem_type, directory);

            // enable/disable mouse events
            _MouseEventChildren *mouse_events = &(_get_node(node_index)->image.mouse_events);
            mouse_events->enabled =
                (mouse_events->pressed != NODE_INDEX_UNDEFINED) ||
                (mouse_events->released != NODE_INDEX_UNDEFINED) ||
                (mouse_events->active != NODE_INDEX_UNDEFINED) ||
                (mouse_events->inactive != NODE_INDEX_UNDEFINED) ||
                (mouse_events->hovered != NODE_INDEX_UNDEFINED);

            // return
            return node_index;
        }

        // at this point, just used to set the directory path
        case DC_APP_ELEM_TYPE_INCLUDE: {

            xmlChar *file = xmlGetProp(xml_node, BAD_CAST "File");
            char     directory[DC_UTILS_FILEPATH_BUFFER_SIZE];
            dc_utils_get_directory((const char *)file, directory, sizeof(directory));
            return _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
        }

        case DC_APP_ELEM_TYPE_LINE: {
            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_LINE;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.line.position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                dc_node.line.position.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.line.position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                dc_node.line.position.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.line.rotation = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                dc_node.line.rotation = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.line.pivot_position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.line.pivot_position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

            } else if (raw_pivot_position_x || raw_pivot_position_y) {
                fprintf(stderr, "DCAPP _process_xml_node(): line: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // line width
            xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
            if (raw_line_width) {
                char cleaned_line_width[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_line_width, (const char *)raw_line_width, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_line_width);
                dc_node.line.line_width = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_line_width);
            } else {
                dc_node.line.line_width = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // colors
            dc_node.line.line_enabled = _load_color_from_string(xml_node, "LineColor", &(dc_node.line.line_color));

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _process_xml_node_children(pl_app_data, xml_node, node_index, elem_type, directory);

            // return
            return node_index;
        }

        case DC_APP_ELEM_TYPE_LOGIC: {

            if (_dc_data.logic_lib) {
                fprintf(stderr, "DCApp _process_xml_node(): duplicate <Logic> definitions\n");
            }

            if (dc_app_lookup_get_var_count(_dc_data.lookup)) {
                printf("DCApp _process_xml_node(): Warning: Declaring <Logic> after <Variables> have been defined. Ensure this behavior is intended.\n");
            }

            xmlChar *raw_filepath = xmlGetProp(xml_node, BAD_CAST "File");
            if (raw_filepath) {

                // clean filepath
                char cleaned_filepath[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_filepath, (const char *)raw_filepath, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_filepath);

                // convert to absolute path
                char abs_filepath[DC_VALUE_STRING_BUFFER_SIZE];
                if (dc_utils_is_relative_path(cleaned_filepath)) {
                    dc_utils_join_paths(directory, cleaned_filepath, abs_filepath, sizeof(abs_filepath));
                } else {
                    strcpy(abs_filepath, cleaned_filepath);
                }

                // open .so file
                const plLibraryDesc logic_so_desc = {
                    .tFlags = PL_LIBRARY_FLAGS_NONE,
                    .pcName = abs_filepath,
                };
                if (_ext_library->load(logic_so_desc, &_dc_data.logic_lib) != PL_LIBRARY_RESULT_SUCCESS) {
                    fprintf(stderr, "DCApp _process_xml_node(): Failed to load logic .so file (%s)\n", abs_filepath);
                }

                // load functions
                _dc_data.logic_pre_init = (void (*)(void))_ext_library->load_function(_dc_data.logic_lib, "DisplayPreInit");
                _dc_data.logic_init     = (void (*)(void))_ext_library->load_function(_dc_data.logic_lib, "DisplayInit");
                _dc_data.logic_draw     = (void (*)(void))_ext_library->load_function(_dc_data.logic_lib, "DisplayDraw");
                _dc_data.logic_close    = (void (*)(void))_ext_library->load_function(_dc_data.logic_lib, "DisplayClose");
            } else {
                fprintf(stderr, "DCAPP _process_xml_node(): <Logic> has no File element\n");
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_MOUSE_ACTIVE: {
            _Node *parent_node = _get_node(parent_node_index);
            switch (parent_node->type) {
                case NODE_TYPE_CIRCLE: {
                    parent_node->circle.mouse_events.active = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_IMAGE: {
                    parent_node->image.mouse_events.active = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_POLYGON: {
                    parent_node->polygon.mouse_events.active = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_RECTANGLE: {
                    parent_node->rectangle.mouse_events.active = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                default:
                    fprintf(stderr, "DCApp _process_xml_node() mouse_active: unknown parent of type %s\n",
                            _node_type_to_string(parent_node->type));
                    break;
            }
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_MOUSE_HOVERED: {
            _Node *parent_node = _get_node(parent_node_index);
            switch (parent_node->type) {
                case NODE_TYPE_CIRCLE: {
                    parent_node->circle.mouse_events.hovered = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_IMAGE: {
                    parent_node->image.mouse_events.hovered = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_POLYGON: {
                    parent_node->polygon.mouse_events.hovered = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_RECTANGLE: {
                    parent_node->rectangle.mouse_events.hovered = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                default:
                    fprintf(stderr, "DCApp _process_xml_node() mouse_hovered: unknown parent of type %s\n",
                            _node_type_to_string(parent_node->type));
                    break;
            }
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_MOUSE_INACTIVE: {
            _Node *parent_node = _get_node(parent_node_index);
            switch (parent_node->type) {
                case NODE_TYPE_CIRCLE: {
                    parent_node->circle.mouse_events.inactive = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_IMAGE: {
                    parent_node->image.mouse_events.inactive = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_POLYGON: {
                    parent_node->polygon.mouse_events.inactive = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_RECTANGLE: {
                    parent_node->rectangle.mouse_events.inactive = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                default:
                    fprintf(stderr, "DCApp _process_xml_node() mouse_inactive: unknown parent of type %s\n",
                            _node_type_to_string(parent_node->type));
                    break;
            }
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_MOUSE_PRESSED: {
            _Node *parent_node = _get_node(parent_node_index);
            switch (parent_node->type) {
                case NODE_TYPE_CIRCLE: {
                    parent_node->circle.mouse_events.pressed = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_IMAGE: {
                    parent_node->image.mouse_events.pressed = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_POLYGON: {
                    parent_node->polygon.mouse_events.pressed = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_RECTANGLE: {
                    parent_node->rectangle.mouse_events.pressed = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                default:
                    fprintf(stderr, "DCApp _process_xml_node() mouse_pressed: unknown parent of type %s\n",
                            _node_type_to_string(parent_node->type));
                    break;
            }
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_MOUSE_RELEASED: {
            _Node *parent_node = _get_node(parent_node_index);
            switch (parent_node->type) {
                case NODE_TYPE_CIRCLE: {
                    parent_node->circle.mouse_events.released = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_IMAGE: {
                    parent_node->image.mouse_events.released = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_POLYGON: {
                    parent_node->polygon.mouse_events.released = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                case NODE_TYPE_RECTANGLE: {
                    parent_node->rectangle.mouse_events.released = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, parent_elem_type, directory);
                    break;
                }
                default:
                    fprintf(stderr, "DCApp _process_xml_node() mouse_released: unknown parent of type %s\n",
                            _node_type_to_string(parent_node->type));
                    break;
            }
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_PANEL: {
            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_PANEL;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // virtual x dimension
            xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
            if (!raw_x_virtual_dimension) {
                raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
            }
            if (raw_x_virtual_dimension) {
                char cleaned_x_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_virtual_dimension, (const char *)raw_x_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_virtual_dimension);
                dc_node.panel.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_virtual_dimension);
            } else {
                dc_node.panel.virtual_dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // virtual y virtual_dimension
            xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
            if (!raw_y_virtual_dimension) {
                raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
            }
            if (raw_y_virtual_dimension) {
                char cleaned_y_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_virtual_dimension, (const char *)raw_y_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_virtual_dimension);
                dc_node.panel.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_virtual_dimension);
            } else {
                dc_node.panel.virtual_dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _NodeIndex first_child_index = _process_xml_node_children(pl_app_data, xml_node, node_index, elem_type, directory);

            // update child index
            _Node *node       = _get_node(node_index);
            node->panel.child = first_child_index;

            // return
            return node_index;
        }

        case DC_APP_ELEM_TYPE_PIXELSTREAM: {

            _Node dc_node;
            memset(&dc_node, 0, sizeof(dc_node));
            dc_node.type   = NODE_TYPE_PIXELSTREAM;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            dc_node.pixelstream.frame = NULL;

            dc_node.pixelstream.mouse_events.active   = NODE_INDEX_UNDEFINED;
            dc_node.pixelstream.mouse_events.hovered  = NODE_INDEX_UNDEFINED;
            dc_node.pixelstream.mouse_events.inactive = NODE_INDEX_UNDEFINED;
            dc_node.pixelstream.mouse_events.pressed  = NODE_INDEX_UNDEFINED;
            dc_node.pixelstream.mouse_events.released = NODE_INDEX_UNDEFINED;

            // create + bind texture
            {
                // create device image, memory, bind
                _Texture texture = _create_texture(pl_app_data, _NODE_PIXELSTREAM_MAX_WIDTH, _NODE_PIXELSTREAM_MAX_HEIGHT, "pixelstream");

                // add texture to internal arrays
                sbpush(_sb_texture_name_offsets, sbcount(_sb_texture_names));
                sbpushn(_sb_texture_names, "--dummy--", strlen("--dummy--") + 1);
                sbpush(_sb_textures, texture);
                _TextureIndex texture_index = sbcount(_sb_textures) - 1;

                // update structure
                dc_node.pixelstream.texture_index = texture_index;
            }

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.pixelstream.position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                dc_node.pixelstream.position.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.pixelstream.position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                dc_node.pixelstream.position.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // x dimension
            xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
            if (!raw_x_dimension) {
                raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
            }
            if (raw_x_dimension) {
                char cleaned_x_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_dimension, (const char *)raw_x_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_dimension);
                dc_node.pixelstream.dimension.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_dimension);
            } else {
                dc_node.pixelstream.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y dimension
            xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
            if (!raw_y_dimension) {
                raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
            }
            if (raw_y_dimension) {
                char cleaned_y_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_dimension, (const char *)raw_y_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_dimension);
                dc_node.pixelstream.dimension.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_dimension);
            } else {
                dc_node.pixelstream.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent x align
            xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
            if (raw_parent_x_align) {
                char cleaned_parent_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_x_align, (const char *)raw_parent_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_x_align);
                dc_node.pixelstream.parent_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_x_align);
            } else {
                dc_node.pixelstream.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent y align
            xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
            if (raw_parent_y_align) {
                char cleaned_parent_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_y_align, (const char *)raw_parent_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_y_align);
                dc_node.pixelstream.parent_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_y_align);
            } else {
                dc_node.pixelstream.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local x align
            xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
            if (!raw_x_align) {
                raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
            }
            if (raw_x_align) {
                char cleaned_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_align, (const char *)raw_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_align);
                dc_node.pixelstream.local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_align);
            } else {
                dc_node.pixelstream.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local y align
            xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
            if (!raw_y_align) {
                raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
            }
            if (raw_y_align) {
                char cleaned_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_align, (const char *)raw_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_align);
                dc_node.pixelstream.local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_align);
            } else {
                dc_node.pixelstream.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.pixelstream.rotation = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                dc_node.pixelstream.rotation = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.pixelstream.pivot_position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.pixelstream.pivot_position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

                dc_node.pixelstream.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.pixelstream.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

            } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

                xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
                if (raw_pivot_align_x) {
                    char cleaned_pivot_align_x[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_x, (const char *)raw_pivot_align_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_x);
                    dc_node.pixelstream.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_x);
                } else {
                    dc_node.pixelstream.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                }

                xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
                if (raw_pivot_align_y) {
                    char cleaned_pivot_align_y[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_y, (const char *)raw_pivot_align_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_y);
                    dc_node.pixelstream.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_y);
                } else {
                    dc_node.pixelstream.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
                }

                dc_node.pixelstream.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.pixelstream.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
            } else {
                fprintf(stderr, "DCAPP _process_xml_node(): Pixelstream: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // set type
            xmlChar *raw_type = xmlGetProp(xml_node, BAD_CAST "Type");
            if (!raw_type) {
                raw_type = xmlGetProp(xml_node, BAD_CAST "Protocol");
            }
            if (raw_type) {
                dc_node.pixelstream.type = dc_utils_string_to_integer((const char *)raw_type);
                xmlFree(raw_type);
            } else {
                fprintf(stderr, "DCApp _process_xml_node() pixelstream: Missing 'Type' field\n");
            }

            // iterate over type
            switch (dc_node.pixelstream.type) {

                case DC_APP_PIXELSTREAM_TYPE_DYNAMIC_FILE: {
                    // TODO
                    break;
                }

                case DC_APP_PIXELSTREAM_TYPE_MJPEG: {

                    // parse URL
                    xmlChar *raw_url = xmlGetProp(xml_node, BAD_CAST "URL");
                    char     cleaned_url[256];
                    if (raw_url) {
                        strncpy(cleaned_url, (const char *)raw_url, 256);
                        xmlFree(raw_url);
                    } else {
                        fprintf(stderr, "DCApp _process_xml_node() pixelstream: Missing 'URL' field\n");
                    }

                    // parse timeout
                    int      timeout     = 5;
                    xmlChar *raw_timeout = xmlGetProp(xml_node, BAD_CAST "Timeout");
                    if (raw_timeout) {
                        timeout = dc_utils_string_to_integer((const char *)raw_timeout);
                        xmlFree(raw_timeout);
                    }

                    // set mjpeg field
                    dc_node.pixelstream.mjpeg.handle        = dc_ps_mjpeg_add_server(cleaned_url, timeout);
                    dc_node.pixelstream.mjpeg.raw_jpeg_size = _NODE_PIXELSTREAM_MAX_WIDTH * _NODE_PIXELSTREAM_MAX_HEIGHT * 4 * sizeof(float);
                    dc_node.pixelstream.mjpeg.raw_jpeg      = (unsigned char *)malloc(dc_node.pixelstream.mjpeg.raw_jpeg_size);
                    break;
                }

                default:
                    fprintf(stderr, "DCApp _process_xml_node() pixelstream: Unknown pixelstream type\n");
                    break;
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _process_xml_node_children(pl_app_data, xml_node, node_index, elem_type, directory);

            // enable/disable mouse events
            _MouseEventChildren *mouse_events = &(_get_node(node_index)->pixelstream.mouse_events);
            mouse_events->enabled =
                (mouse_events->pressed != NODE_INDEX_UNDEFINED) ||
                (mouse_events->released != NODE_INDEX_UNDEFINED) ||
                (mouse_events->active != NODE_INDEX_UNDEFINED) ||
                (mouse_events->inactive != NODE_INDEX_UNDEFINED) ||
                (mouse_events->hovered != NODE_INDEX_UNDEFINED);

            // return
            return node_index;
        }

        case DC_APP_ELEM_TYPE_POLYGON: {
            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_POLYGON;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            dc_node.polygon.mouse_events.active   = NODE_INDEX_UNDEFINED;
            dc_node.polygon.mouse_events.hovered  = NODE_INDEX_UNDEFINED;
            dc_node.polygon.mouse_events.inactive = NODE_INDEX_UNDEFINED;
            dc_node.polygon.mouse_events.pressed  = NODE_INDEX_UNDEFINED;
            dc_node.polygon.mouse_events.released = NODE_INDEX_UNDEFINED;

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.polygon.position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                dc_node.polygon.position.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.polygon.position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                dc_node.polygon.position.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.polygon.rotation = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                dc_node.polygon.rotation = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.polygon.pivot_position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.polygon.pivot_position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

            } else if (raw_pivot_position_x || raw_pivot_position_y) {
                fprintf(stderr, "DCAPP _process_xml_node(): polygon: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // line width
            xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
            if (raw_line_width) {
                char cleaned_line_width[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_line_width, (const char *)raw_line_width, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_line_width);
                dc_node.polygon.line_width = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_line_width);
            } else {
                dc_node.polygon.line_width = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // colors
            dc_node.polygon.fill_enabled = _load_color_from_string(xml_node, "FillColor", &(dc_node.polygon.fill_color));
            dc_node.polygon.line_enabled = _load_color_from_string(xml_node, "LineColor", &(dc_node.polygon.line_color));

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _process_xml_node_children(pl_app_data, xml_node, node_index, elem_type, directory);

            // enable/disable mouse events
            _MouseEventChildren *mouse_events = &(_get_node(node_index)->polygon.mouse_events);
            mouse_events->enabled =
                (mouse_events->pressed != NODE_INDEX_UNDEFINED) ||
                (mouse_events->released != NODE_INDEX_UNDEFINED) ||
                (mouse_events->active != NODE_INDEX_UNDEFINED) ||
                (mouse_events->inactive != NODE_INDEX_UNDEFINED) ||
                (mouse_events->hovered != NODE_INDEX_UNDEFINED);

            // return
            return node_index;
        }

        case DC_APP_ELEM_TYPE_RECTANGLE: {
            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_RECTANGLE;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            dc_node.rectangle.mouse_events.active   = NODE_INDEX_UNDEFINED;
            dc_node.rectangle.mouse_events.hovered  = NODE_INDEX_UNDEFINED;
            dc_node.rectangle.mouse_events.inactive = NODE_INDEX_UNDEFINED;
            dc_node.rectangle.mouse_events.pressed  = NODE_INDEX_UNDEFINED;
            dc_node.rectangle.mouse_events.released = NODE_INDEX_UNDEFINED;

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.rectangle.position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                dc_node.rectangle.position.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.rectangle.position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                dc_node.rectangle.position.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // x dimension
            xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
            if (!raw_x_dimension) {
                raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
            }
            if (raw_x_dimension) {
                char cleaned_x_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_dimension, (const char *)raw_x_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_dimension);
                dc_node.rectangle.dimension.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_dimension);
            } else {
                dc_node.rectangle.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y dimension
            xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
            if (!raw_y_dimension) {
                raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
            }
            if (raw_y_dimension) {
                char cleaned_y_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_dimension, (const char *)raw_y_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_dimension);
                dc_node.rectangle.dimension.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_dimension);
            } else {
                dc_node.rectangle.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent x align
            xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
            if (raw_parent_x_align) {
                char cleaned_parent_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_x_align, (const char *)raw_parent_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_x_align);
                dc_node.rectangle.parent_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_x_align);
            } else {
                dc_node.rectangle.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent y align
            xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
            if (raw_parent_y_align) {
                char cleaned_parent_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_y_align, (const char *)raw_parent_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_y_align);
                dc_node.rectangle.parent_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_y_align);
            } else {
                dc_node.rectangle.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local x align
            xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
            if (!raw_x_align) {
                raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
            }
            if (raw_x_align) {
                char cleaned_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_align, (const char *)raw_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_align);
                dc_node.rectangle.local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_align);
            } else {
                dc_node.rectangle.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local y align
            xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
            if (!raw_y_align) {
                raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
            }
            if (raw_y_align) {
                char cleaned_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_align, (const char *)raw_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_align);
                dc_node.rectangle.local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_align);
            } else {
                dc_node.rectangle.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.rectangle.rotation = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                dc_node.rectangle.rotation = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.rectangle.pivot_position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.rectangle.pivot_position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

                dc_node.rectangle.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.rectangle.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

            } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

                xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
                if (raw_pivot_align_x) {
                    char cleaned_pivot_align_x[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_x, (const char *)raw_pivot_align_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_x);
                    dc_node.rectangle.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_x);
                } else {
                    dc_node.rectangle.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                }

                xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
                if (raw_pivot_align_y) {
                    char cleaned_pivot_align_y[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_y, (const char *)raw_pivot_align_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_y);
                    dc_node.rectangle.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_y);
                } else {
                    dc_node.rectangle.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
                }

                dc_node.rectangle.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.rectangle.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
            } else {
                fprintf(stderr, "DCAPP _process_xml_node(): Rectangle: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // line width
            xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
            if (raw_line_width) {
                char cleaned_line_width[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_line_width, (const char *)raw_line_width, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_line_width);
                dc_node.rectangle.line_width = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_line_width);
            } else {
                dc_node.rectangle.line_width = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // colors
            dc_node.rectangle.fill_enabled = _load_color_from_string(xml_node, "FillColor", &(dc_node.rectangle.fill_color));
            dc_node.rectangle.line_enabled = _load_color_from_string(xml_node, "LineColor", &(dc_node.rectangle.line_color));

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _process_xml_node_children(pl_app_data, xml_node, node_index, elem_type, directory);

            // enable/disable mouse events
            _MouseEventChildren *mouse_events = &(_get_node(node_index)->rectangle.mouse_events);
            mouse_events->enabled =
                (mouse_events->pressed != NODE_INDEX_UNDEFINED) ||
                (mouse_events->released != NODE_INDEX_UNDEFINED) ||
                (mouse_events->active != NODE_INDEX_UNDEFINED) ||
                (mouse_events->inactive != NODE_INDEX_UNDEFINED) ||
                (mouse_events->hovered != NODE_INDEX_UNDEFINED);

            // return
            return node_index;
        }

        case DC_APP_ELEM_TYPE_SET: {

            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_SET;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // variable
            xmlChar *raw_variable = xmlGetProp(xml_node, BAD_CAST "Variable");
            if (raw_variable) {
                char cleaned_variable[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_variable, (const char *)raw_variable, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_variable);
                dc_node.set.var_index = dc_app_lookup_get_var_index(_dc_data.lookup, cleaned_variable);
            } else {
                fprintf(stderr, "DCAPP _process_xml_node: Missing attribute 'Variable' in <Set> element\n");
            }

            // operand
            xmlChar *raw_operand = xmlNodeGetContent(xml_node);
            if (raw_operand) {
                char cleaned_operand[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_operand, (const char *)raw_operand, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_operand);
                dc_node.set.operand = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_operand);
            } else {
                fprintf(stderr, "DCAPP _process_xml_node: Missing Node content in <Set> element\n");
            }

            // operator
            xmlChar *raw_operator = xmlGetProp(xml_node, BAD_CAST "Operator");
            if (raw_operator) {
                char cleaned_operator[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_operator, (const char *)raw_operator, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_operator);
                dc_node.set.operation = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_operator);
            } else {
                dc_node.set.operation = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // register node
            return _register_node(&dc_node);
        }

        case DC_APP_ELEM_TYPE_STYLE: {
            // ignore at this point
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TERRAIN: {
            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_TERRAIN;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.terrain.position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                dc_node.terrain.position.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.terrain.position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                dc_node.terrain.position.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // x dimension
            xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
            if (!raw_x_dimension) {
                raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
            }
            if (raw_x_dimension) {
                char cleaned_x_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_dimension, (const char *)raw_x_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_dimension);
                dc_node.terrain.dimension.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_dimension);
            } else {
                dc_node.terrain.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y dimension
            xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
            if (!raw_y_dimension) {
                raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
            }
            if (raw_y_dimension) {
                char cleaned_y_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_dimension, (const char *)raw_y_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_dimension);
                dc_node.terrain.dimension.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_dimension);
            } else {
                dc_node.terrain.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local x align
            xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
            if (!raw_x_align) {
                raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
            }
            if (raw_x_align) {
                char cleaned_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_align, (const char *)raw_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_align);
                dc_node.terrain.local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_align);
            } else {
                dc_node.terrain.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local y align
            xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
            if (!raw_y_align) {
                raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
            }
            if (raw_y_align) {
                char cleaned_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_align, (const char *)raw_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_align);
                dc_node.terrain.local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_align);
            } else {
                dc_node.terrain.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent x align
            xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
            if (raw_parent_x_align) {
                char cleaned_parent_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_x_align, (const char *)raw_parent_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_x_align);
                dc_node.terrain.parent_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_x_align);
            } else {
                dc_node.terrain.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent y align
            xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
            if (raw_parent_y_align) {
                char cleaned_parent_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_y_align, (const char *)raw_parent_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_y_align);
                dc_node.terrain.parent_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_y_align);
            } else {
                dc_node.terrain.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.terrain.rotation = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                dc_node.terrain.rotation = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.terrain.pivot_position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.terrain.pivot_position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

                dc_node.terrain.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.terrain.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

            } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

                xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
                if (raw_pivot_align_x) {
                    char cleaned_pivot_align_x[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_x, (const char *)raw_pivot_align_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_x);
                    dc_node.terrain.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_x);
                } else {
                    dc_node.terrain.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                }

                xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
                if (raw_pivot_align_y) {
                    char cleaned_pivot_align_y[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_y, (const char *)raw_pivot_align_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_y);
                    dc_node.terrain.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_y);
                } else {
                    dc_node.terrain.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
                }

                dc_node.terrain.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.terrain.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
            } else {
                fprintf(stderr, "DCAPP _process_xml_node(): terrain: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // lat
            xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "Latitude");
            if (raw_lat) {
                char cleaned_lat[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_lat, (const char *)raw_lat, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_lat);
                dc_node.terrain.lle.lat = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_lat);
            } else {
                fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Latitude with Terrain primitive");
            }

            // lon
            xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
            if (raw_lon) {
                char cleaned_lon[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_lon, (const char *)raw_lon, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_lon);
                dc_node.terrain.lle.lon = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_lon);
            } else {
                fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Longitude with Terrain primitive");
            }

            // ele
            xmlChar *raw_ele = xmlGetProp(xml_node, BAD_CAST "Elevation");
            if (raw_ele) {
                char cleaned_ele[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_ele, (const char *)raw_ele, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_ele);
                dc_node.terrain.lle.ele = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_ele);
            } else {
                fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Elevation with Terrain primitive");
            }

            // roll
            xmlChar *raw_roll = xmlGetProp(xml_node, BAD_CAST "Roll");
            if (raw_roll) {
                char cleaned_roll[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_roll, (const char *)raw_roll, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_roll);
                dc_node.terrain.rpy.roll = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_roll);
            } else {
                fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Roll with Terrain primitive");
            }

            // pitch
            xmlChar *raw_pitch = xmlGetProp(xml_node, BAD_CAST "Pitch");
            if (raw_pitch) {
                char cleaned_pitch[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pitch, (const char *)raw_pitch, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pitch);
                dc_node.terrain.rpy.pitch = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pitch);
            } else {
                fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Pitch with Terrain primitive");
            }

            // yaw
            xmlChar *raw_yaw = xmlGetProp(xml_node, BAD_CAST "Yaw");
            if (raw_yaw) {
                char cleaned_yaw[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_yaw, (const char *)raw_yaw, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_yaw);
                dc_node.terrain.rpy.yaw = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_yaw);
            } else {
                fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Yaw with Terrain primitive");
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // process children
            _process_xml_node_children(pl_app_data, xml_node, node_index, elem_type, directory);

            // return
            return node_index;
        }

        case DC_APP_ELEM_TYPE_TERRAIN_DEM: {

            _Node *parent_node = _get_node(parent_node_index);
            switch (parent_node->type) {
                case NODE_TYPE_TERRAIN: {

                    // file
                    xmlChar *raw_filepath = xmlGetProp(xml_node, BAD_CAST "File");
                    if (raw_filepath) {

                        // clean filepath
                        char cleaned_filepath[DC_VALUE_STRING_BUFFER_SIZE];
                        strncpy(cleaned_filepath, (const char *)raw_filepath, DC_VALUE_STRING_BUFFER_SIZE - 1);
                        xmlFree(raw_filepath);

                        // convert to absolute path
                        char abs_filepath[DC_VALUE_STRING_BUFFER_SIZE];
                        if (dc_utils_is_relative_path(cleaned_filepath)) {
                            dc_utils_join_paths(directory, cleaned_filepath, abs_filepath, sizeof(abs_filepath));
                        } else {
                            strcpy(abs_filepath, cleaned_filepath);
                        }

                        // TODO open file
                        // print for now
                        printf("TerrainDEM file: %s\n", abs_filepath);
                    } else {
                        fprintf(stderr, "DCAPP _process_xml_node(): <Logic> has no File element\n");
                    }

                    break;
                }
                default:
                    fprintf(stderr, "DCApp _process_xml_node(): Invalid parent of type %s for TerrainDEM\n", _node_type_to_string(parent_node->type));
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TEXT: {
            _Node dc_node  = {};
            dc_node.type   = NODE_TYPE_TEXT;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // text
            xmlChar *raw_text = xmlNodeGetContent(xml_node);
            if (raw_text) {
                char cleaned_text[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_text, (const char *)raw_text, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_text);

                static char *sb_curr_filler = NULL;
                for (size_t ii = 0; ii < strlen(cleaned_text);) {
                    if (cleaned_text[ii] == '\\') {
                        // Escape character: skip and add next character to result
                        if (ii + 1 < strlen(cleaned_text)) {
                            sbpush(sb_curr_filler, cleaned_text[ii + 1]);
                            ii += 2;
                        } else {
                            sbpush(sb_curr_filler, cleaned_text[ii++]);
                        }
                        continue;
                    }

                    if (cleaned_text[ii] == '@') {
                        size_t start = ii;
                        ii++;

                        char var[DC_VALUE_STRING_BUFFER_SIZE];
                        bool is_braced = false;

                        if (ii < strlen(cleaned_text) && cleaned_text[ii] == '{') {
                            is_braced = true;
                            ii++;
                            size_t end = dc_utils_str_find_first(&(cleaned_text[ii]), '}');
                            if (end == -1) {
                                // No closing brace, treat as normal text
                                sbpushn(sb_curr_filler, &(cleaned_text[start]), ii - start);
                                continue;
                            }
                            strncpy(var, &(cleaned_text[ii]), end - ii);
                            ii = end + 1;
                        } else {
                            size_t start_var = ii;
                            while (ii < strlen(cleaned_text) && !isspace(cleaned_text[ii]) && cleaned_text[ii] != '(') {
                                ii++;
                            }
                            strncpy(var, &(cleaned_text[start_var]), ii - start_var);
                        }

                        DcAppValIndex   var_index = dc_app_lookup_get_var_index(_dc_data.lookup, var);
                        DcAppLookupVar *var_var   = dc_app_lookup_get_var(_dc_data.lookup, var_index);
                        sbpush(dc_node.text.sb_vals, var_var->value_index);

                        // Check for format specifier
                        char format_spec[DC_VALUE_STRING_BUFFER_SIZE];
                        if (ii < strlen(cleaned_text) && cleaned_text[ii] == '(') {
                            size_t close = dc_utils_str_find_first(&(cleaned_text[ii]), ')');
                            if (close != -1 && (ii == 0 || cleaned_text[ii - 1] != '\\')) {
                                strncpy(format_spec, &(cleaned_text[ii + 1]), close - ii - 1);
                                ii = close + 1;

                                // get format + type
                                if (dc_utils_is_format_specifier_int(format_spec)) {
                                    sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_INTEGER);
                                    sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                    sbpushn(dc_node.text.sb_formats, format_spec, strlen(format_spec) + 1);
                                } else if (dc_utils_is_format_specifier_double(format_spec)) {
                                    sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_DOUBLE);
                                    sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                    sbpushn(dc_node.text.sb_formats, format_spec, strlen(format_spec) + 1);
                                } else if (dc_utils_is_format_specifier_string(format_spec)) {
                                    sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                                    sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                    sbpushn(dc_node.text.sb_formats, format_spec, strlen(format_spec) + 1);
                                } else if (dc_utils_is_format_specifier_bool(format_spec)) {
                                    sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_BOOLEAN);
                                    sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                    sbpushn(dc_node.text.sb_formats, format_spec, strlen(format_spec) + 1);
                                } else {
                                    fprintf(stderr, "DCApp _process_xml_node(): Unknown format specifier in Text element\n");
                                }
                            } else {
                                sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                                sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                                sbpushn(dc_node.text.sb_formats, "%s", 3);
                            }
                        } else {
                            sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                            sbpush(dc_node.text.sb_format_indices, sbcount(dc_node.text.sb_formats));
                            sbpushn(dc_node.text.sb_formats, "%s", 3);
                        }

                        // add the current filler to list of fillers
                        sbpush(dc_node.text.sb_filler_indices, sbcount(dc_node.text.sb_fillers));
                        sbpushn(dc_node.text.sb_fillers, sb_curr_filler, sbcount(sb_curr_filler));
                        sbpush(dc_node.text.sb_fillers, '\0');
                        sbclear(sb_curr_filler);

                        continue;
                    }

                    // Default: append character to result
                    sbpush(sb_curr_filler, cleaned_text[ii++]);
                }

                // append the remaining filler
                sbpush(dc_node.text.sb_filler_indices, sbcount(dc_node.text.sb_fillers));
                sbpushn(dc_node.text.sb_fillers, sb_curr_filler, sbcount(sb_curr_filler));
                sbpush(dc_node.text.sb_fillers, '\0');

                // clear temp buffer
                sbclear(sb_curr_filler);
            } else {
                fprintf(stderr, "DCApp _process_xml_node(): Missing node content for text\n");
            }

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.text.position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                dc_node.text.position.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.text.position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                dc_node.text.position.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local x align
            xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
            if (!raw_x_align) {
                raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
            }
            if (raw_x_align) {
                char cleaned_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_align, (const char *)raw_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_align);
                dc_node.text.local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_align);
            } else {
                dc_node.text.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // local y align
            xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
            if (!raw_y_align) {
                raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
            }
            if (raw_y_align) {
                char cleaned_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_align, (const char *)raw_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_align);
                dc_node.text.local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_align);
            } else {
                dc_node.text.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent x align
            xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
            if (raw_parent_x_align) {
                char cleaned_parent_x_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_x_align, (const char *)raw_parent_x_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_x_align);
                dc_node.text.parent_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_x_align);
            } else {
                dc_node.text.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // parent y align
            xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
            if (raw_parent_y_align) {
                char cleaned_parent_y_align[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_parent_y_align, (const char *)raw_parent_y_align, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_parent_y_align);
                dc_node.text.parent_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_parent_y_align);
            } else {
                dc_node.text.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // rotation
            xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
            if (!raw_rotation) {
                raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
            }
            if (raw_rotation) {
                char cleaned_rotation[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_rotation, (const char *)raw_rotation, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_rotation);
                dc_node.text.rotation = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_rotation);
            } else {
                dc_node.text.rotation = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // pivots
            xmlChar *raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
            if (!raw_pivot_position_x) {
                raw_pivot_position_x = xmlGetProp(xml_node, BAD_CAST "PivotX");
            }
            xmlChar *raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
            if (!raw_pivot_position_y) {
                raw_pivot_position_y = xmlGetProp(xml_node, BAD_CAST "PivotY");
            }
            if (raw_pivot_position_x && raw_pivot_position_y) {

                char cleaned_pivot_position_x[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_x, (const char *)raw_pivot_position_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_x);
                dc_node.text.pivot_position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_x);

                char cleaned_pivot_position_y[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_pivot_position_y, (const char *)raw_pivot_position_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_pivot_position_y);
                dc_node.text.pivot_position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_pivot_position_y);

                dc_node.text.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.text.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

            } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

                xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
                if (raw_pivot_align_x) {
                    char cleaned_pivot_align_x[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_x, (const char *)raw_pivot_align_x, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_x);
                    dc_node.text.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_x);
                } else {
                    dc_node.text.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
                }

                xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
                if (raw_pivot_align_y) {
                    char cleaned_pivot_align_y[DC_VALUE_STRING_BUFFER_SIZE];
                    strncpy(cleaned_pivot_align_y, (const char *)raw_pivot_align_y, DC_VALUE_STRING_BUFFER_SIZE - 1);
                    xmlFree(raw_pivot_align_y);
                    dc_node.text.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_pivot_align_y);
                } else {
                    dc_node.text.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
                }

                dc_node.text.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.text.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
            } else {
                fprintf(stderr, "DCAPP _process_xml_node(): text: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
            }

            // size
            xmlChar *raw_size = xmlGetProp(xml_node, BAD_CAST "Size");
            if (raw_size) {
                char cleaned_size[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_size, (const char *)raw_size, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_size);
                dc_node.text.size = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_size);
            } else {
                dc_node.text.size = DC_APP_VAL_INDEX_UNDEFINED;
            }

            dc_node.text.fill_enabled = _load_color_from_string(xml_node, "FillColor", &(dc_node.text.fill_color));
            dc_node.text.line_enabled = _load_color_from_string(xml_node, "LineColor", &(dc_node.text.line_color));

            // register node
            return _register_node(&dc_node);
        }

        case DC_APP_ELEM_TYPE_TRICK_FROM: {
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_IO: {
                    _process_xml_node_children(pl_app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
                    break;
                }
                default: {
                    fprintf(stderr, "DCApp _process_xml_node(): Invalid elem parent of type %s for FromTrick\n", dc_app_elem_type_to_string(parent_elem_type));
                    break;
                }
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TRICK_IO: {

            // host
            xmlChar *raw_host = xmlGetProp(xml_node, BAD_CAST "Host");
            char     host[DC_VALUE_STRING_BUFFER_SIZE];
            if (raw_host) {
                strncpy(host, (const char *)raw_host, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_host);
            } else {
                fprintf(stderr, "DCApp _process_xml_node: Missing Port for TrickIO\n");
            }

            // port
            xmlChar *raw_port = xmlGetProp(xml_node, BAD_CAST "Port");
            int      port;
            if (raw_port) {
                char cleaned_port[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_port, (const char *)raw_port, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_port);
                port = dc_utils_string_to_double(cleaned_port);
            } else {
                fprintf(stderr, "DCApp _process_xml_node: Missing Port for TrickIO\n");
            }

            // data rate
            xmlChar *raw_data_rate = xmlGetProp(xml_node, BAD_CAST "Rotation");
            double   data_rate     = 0.1;
            if (raw_data_rate) {
                char cleaned_data_rate[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_data_rate, (const char *)raw_data_rate, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_data_rate);
                data_rate = dc_utils_string_to_double(cleaned_data_rate);
            }

            // create trick instance
            _TrickContext trick_context = {};
            trick_context.trick         = dc_trick_create(host, port, data_rate, 1);
            sbpush(_dc_data.sb_tricks, trick_context);

            // process children
            _process_xml_node_children(pl_app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TRICK_TO: {
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_IO: {
                    _process_xml_node_children(pl_app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
                    break;
                }
                default: {
                    fprintf(stderr, "DCApp _process_xml_node(): Invalid elem parent of type %s for ToTrick\n", dc_app_elem_type_to_string(parent_elem_type));
                    break;
                }
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TRICK_VARIABLE: {

            // check for invalid elem type
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_FROM:
                case DC_APP_ELEM_TYPE_TRICK_TO:
                    break;
                default:
                    fprintf(stderr, "DCApp _process_xml_node(): Invalid elem parent of type %s for TrickVariable\n", dc_app_elem_type_to_string(parent_elem_type));
            }

            // var path
            xmlChar *raw_trick_path = xmlGetProp(xml_node, BAD_CAST "Name");
            char     trick_path[DC_VALUE_STRING_BUFFER_SIZE];
            if (raw_trick_path) {
                strncpy(trick_path, (const char *)raw_trick_path, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_trick_path);
            } else {
                fprintf(stderr, "DCApp _process_xml_node: Missing trick Var path for TrickVariable\n");
            }

            // dcapp var
            xmlChar *raw_dcapp_var = xmlNodeGetContent(xml_node);
            char     dcapp_var[DC_VALUE_STRING_BUFFER_SIZE];
            if (raw_dcapp_var) {
                strncpy(dcapp_var, (const char *)raw_dcapp_var, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_dcapp_var);
            } else {
                fprintf(stderr, "DCApp _process_xml_node: Missing dcapp Var path for TrickVariable\n");
            }

            // units
            xmlChar *raw_units = xmlGetProp(xml_node, BAD_CAST "Name");
            char     cleaned_units[DC_VALUE_STRING_BUFFER_SIZE];
            char    *units = NULL;
            if (raw_units) {
                strncpy(cleaned_units, (const char *)raw_units, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_units);
                units = cleaned_units;
            }

            // get current trick context
            _TrickContext *trick_context = &(_dc_data.sb_tricks[sbcount(_dc_data.sb_tricks) - 1]);

            // handle depending on parent
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_TRICK_FROM: {

                    // create + add rx var
                    _TrickRxVarContext var = {};
                    var.trick_var_index    = dc_trick_add_rx_var(trick_context->trick, trick_path, units);
                    var.dcapp_var_index    = dc_app_lookup_get_var_index(_dc_data.lookup, dcapp_var);
                    sbpush(trick_context->sb_rx_var_contexts, var);
                    break;
                }
                case DC_APP_ELEM_TYPE_TRICK_TO: {

                    // create + add tx var
                    _TrickTxVarContext var = {};
                    var.dcapp_var_index    = dc_app_lookup_get_var_index(_dc_data.lookup, dcapp_var);
                    DcValue *dc_var_value  = dc_app_lookup_get_value(_dc_data.lookup, dc_app_lookup_get_var(_dc_data.lookup, var.dcapp_var_index)->value_index);
                    var.trick_var_index    = dc_trick_add_tx_var(trick_context->trick, trick_path, units, dc_var_value->type == DC_VALUE_TYPE_STRING);
                    dc_value_copy(&var.prev_value, dc_var_value);
                    sbpush(trick_context->sb_tx_var_contexts, var);
                    break;
                }
                default:
                    // should never reach here
                    fprintf(stderr, "DCApp _process_xml_node(): Invalid parent for TrickVariable\n");
                    break;
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_TRUE: {
            switch (parent_elem_type) {
                case DC_APP_ELEM_TYPE_IF: {

                    // process children
                    _NodeIndex first_child_index = _process_xml_node_children(pl_app_data, xml_node, parent_node_index, elem_type, directory);

                    // update child true node
                    _Node *parent_node                  = _get_node(parent_node_index);
                    parent_node->conditional.child_true = first_child_index;
                    break;
                }
                default:
                    fprintf(stderr, "DCApp _process_xml_node(): Invalid elem parent of type %s for <True>.\n", dc_app_elem_type_to_string(parent_elem_type));
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_VARIABLE: {

            // name
            xmlChar *raw_name = xmlNodeGetContent(xml_node);
            char     name[DC_VALUE_STRING_BUFFER_SIZE];
            if (raw_name) {
                strncpy(name, (const char *)raw_name, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_name);
            } else {
                fprintf(stderr, "DCApp _process_xml_node: Non-existent node content in <Variable> definition\n");
            }

            xmlChar    *raw_type = xmlGetProp(xml_node, BAD_CAST "Type");
            DcValueType type     = DC_VALUE_TYPE_STRING;
            if (raw_type) {
                char cleaned_type[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_type, (const char *)raw_type, DC_VALUE_STRING_BUFFER_SIZE - 1);
                type = dc_app_value_type_from_string(cleaned_type);
                xmlFree(raw_type);
            }

            xmlChar *raw_initial_value = xmlGetProp(xml_node, BAD_CAST "InitialValue");
            DcValue  initial_value;
            if (raw_initial_value) {
                char cleaned_initial_value[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_initial_value, (const char *)raw_initial_value, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_initial_value);
                initial_value = dc_value_create_value_string(cleaned_initial_value);
            } else {
                initial_value = dc_value_create_value_string("");
            }
            initial_value.type = type;

            // register var, link to extern
            DcAppLookupVar var = {};
            var.value_index    = dc_app_lookup_register_value(_dc_data.lookup, &initial_value);
            if (_dc_data.logic_lib) {
                var.extern_data = _ext_library->load_function(_dc_data.logic_lib, name);
            }
            DcAppVarIndex var_index = dc_app_lookup_register_var(_dc_data.lookup, name, &var);

            // set the extern data to the initial value
            if (_dc_data.logic_lib) {
                dc_app_lookup_refresh_var_from_value(_dc_data.lookup, var_index);
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_VERTEX: {
            _Node *parent_node = _get_node(parent_node_index);
            switch (parent_node->type) {
                case NODE_TYPE_LINE:
                case NODE_TYPE_POLYGON: {

                    // position
                    _ValIndex2 position;

                    // x position
                    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
                    if (!raw_x_position) {
                        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
                    }
                    if (raw_x_position) {
                        char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                        strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                        xmlFree(raw_x_position);
                        position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
                    } else {
                        fprintf(stderr, "DCAPP _process_xml_node: Invalid Vertex: No X attribute\n");
                    }

                    // y position
                    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
                    if (!raw_y_position) {
                        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
                    }
                    if (raw_y_position) {
                        char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                        strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                        xmlFree(raw_y_position);
                        position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
                    } else {
                        fprintf(stderr, "DCAPP _process_xml_node: Invalid Vertex: No Y attribute\n");
                    }

                    switch (parent_node->type) {
                        case NODE_TYPE_LINE:
                            // add to parent
                            sbpush(parent_node->line.sb_points, position);

                            // check point count
                            if (sbcount(parent_node->line.sb_points) > _NODE_LINE_MAX_POINTS) {
                                fprintf(stderr, "_process_xml_node() line: Maximum number of points exceeded\n");
                            }
                            break;
                        case NODE_TYPE_POLYGON:
                            // add to parent
                            sbpush(parent_node->polygon.sb_points, position);

                            // check point count
                            if (sbcount(parent_node->polygon.sb_points) > _NODE_POLYGON_MAX_POINTS) {
                                fprintf(stderr, "_process_xml_node() polygon: Maximum number of points exceeded\n");
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                }
                default:
                    fprintf(stderr, "DCApp _process_xml_node(): Invalid parent of type %s for vertex\n", _node_type_to_string(parent_node->type));
            }

            // return
            return NODE_INDEX_UNDEFINED;
        }

        case DC_APP_ELEM_TYPE_WINDOW: {
            _Node dc_node;
            dc_node.type   = NODE_TYPE_WINDOW;
            dc_node.parent = parent_node_index;
            dc_node.next   = NODE_INDEX_UNDEFINED;

            // title
            xmlChar *raw_title = xmlGetProp(xml_node, BAD_CAST "Title");
            if (raw_title) {
                char cleaned_title[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_title, (const char *)raw_title, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_title);
                dc_node.window.title = strdup(cleaned_title);
            } else {
                dc_node.window.title = strdup("dcapp");
            }

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                char cleaned_x_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_position, (const char *)raw_x_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_position);
                dc_node.window.init_position.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_position);
            } else {
                dc_node.window.init_position.x = 0.0f;
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                char cleaned_y_position[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_position, (const char *)raw_y_position, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_position);
                dc_node.window.init_position.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_position);
            } else {
                dc_node.window.init_position.y = 0.0f;
            }

            // x dimension
            xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
            if (!raw_x_dimension) {
                raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
            }
            if (raw_x_dimension) {
                char cleaned_x_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_dimension, (const char *)raw_x_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_dimension);
                dc_node.window.init_dimension.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_x_dimension);
            } else {
                fprintf(stderr, "DCApp _process_xml_node: missing x dimension for Window\n");
            }

            // y dimension
            xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
            if (!raw_y_dimension) {
                raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
            }
            if (raw_y_dimension) {
                char cleaned_y_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_dimension, (const char *)raw_y_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_dimension);
                dc_node.window.init_dimension.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_INTEGER, cleaned_y_dimension);
            } else {
                fprintf(stderr, "DCApp _process_xml_node: missing y dimension for Window\n");
            }

            // virtual x dimension
            xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
            if (!raw_x_virtual_dimension) {
                raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
            }
            if (raw_x_virtual_dimension) {
                char cleaned_x_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_x_virtual_dimension, (const char *)raw_x_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_x_virtual_dimension);
                dc_node.window.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_x_virtual_dimension);
            } else {
                dc_node.window.virtual_dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // virtual y virtual_dimension
            xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
            if (!raw_y_virtual_dimension) {
                raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
            }
            if (raw_y_virtual_dimension) {
                char cleaned_y_virtual_dimension[DC_VALUE_STRING_BUFFER_SIZE];
                strncpy(cleaned_y_virtual_dimension, (const char *)raw_y_virtual_dimension, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_y_virtual_dimension);
                dc_node.window.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(_dc_data.lookup, DC_VALUE_TYPE_DOUBLE, cleaned_y_virtual_dimension);
            } else {
                dc_node.window.virtual_dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // register node
            _NodeIndex node_index = _register_node(&dc_node);

            // init PL graphics backend
            // TODO really don't like this approach
            _init_pl_app_data(pl_app_data, &dc_node);

            // process children
            _NodeIndex first_child_index = _process_xml_node_children(pl_app_data, xml_node, node_index, elem_type, directory);

            // update child index
            _Node *node        = _get_node(node_index);
            node->window.child = first_child_index;

            // set global window
            _dc_data.window = node_index;

            // return
            return node_index;
        }

        default:
            fprintf(stderr, "DCApp _process_xml_node(): Invalid node type found\n");
            return NODE_INDEX_UNDEFINED;
    }

    fprintf(stderr, "DCApp _process_xml_node(): Unhandled switch statement\n");
    return NODE_INDEX_UNDEFINED;
}

#include "_dcapp_utils.c"
