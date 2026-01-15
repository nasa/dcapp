#include "dcapp.h"

#include "../src/utils/file.h"
#include "../src/utils/string.h"
#include "libxml/xmlstring.h"

#include <ctype.h>

// Forward declarations
static DcAppVarIndex _register_anonymous_variable(_AppData *app_data, DcValueType type, const char *initial_value_str);
static _NodeIndex    _process_xml_node(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_nonelem(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_arc(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_blink(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_button(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_button_disabled(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_button_enabled(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_button_indicator_off(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_button_indicator_on(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_button_pressed(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_button_released(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_button_transition(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_circle(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_constant(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_container(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_dcapp(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_default(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_false(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_function(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_if(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_image(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_line(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_logic(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_mouse_active(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_mouse_hovered(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_mouse_inactive(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_mouse_motion(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_mouse_pressed(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_mouse_released(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_panel(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_pixelstream(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_polygon(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_rectangle(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_set(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_sphere(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_stencil(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_stencil_add(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_stencil_draw(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_stencil_remove(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_style(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_terrain(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_terrain_dem(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_text(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_trick_from(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_trick_io(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_trick_to(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_trick_variable(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_true(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_variable(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_vertex(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_window(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);

static DcAppVarIndex _register_anonymous_variable(_AppData *app_data, DcValueType type, const char *initial_value_str) {
    static unsigned int anon_var_counter = 0;

    // create anon name
    char anon_name[32];
    snprintf(anon_name, sizeof(anon_name), "__anon_%u__", anon_var_counter++);

    // register variable
    DcAppLookupVar var = {0};
    var.value_index    = dc_app_create_and_register_typed_value_from_string(app_data->lookup, type, initial_value_str);
    return dc_app_lookup_register_var(app_data->lookup, anon_name, &var);
}

static _NodeIndex _process_xml_node_children(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex node_index, DcAppElemType elem_type, const char *directory) {
    xmlNodePtr xml_child_node = xml_node->children;

    _NodeIndex first_child_index         = NODE_INDEX_UNDEFINED;
    _NodeIndex previous_child_node_index = NODE_INDEX_UNDEFINED;
    while (xml_child_node) {

        _NodeIndex child_node_index = _process_xml_node(app_data, xml_child_node, node_index, elem_type, directory);

        if (child_node_index != NODE_INDEX_UNDEFINED) {

            // get node addresses here since the address could change per node process
            _Node *node                = _get_node(app_data, node_index);
            _Node *child_node          = _get_node(app_data, child_node_index);
            _Node *previous_child_node = _get_node(app_data, previous_child_node_index);

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
            _Node     *last_child_node       = _get_node(app_data, last_child_node_index);
            while (last_child_node->next != NODE_INDEX_UNDEFINED) {
                last_child_node_index = last_child_node->next;
                last_child_node       = _get_node(app_data, last_child_node_index);
            }
            previous_child_node_index = last_child_node_index;
        }

        // increment pointer
        xml_child_node = xml_child_node->next;
    }

    return first_child_index;
}
static _NodeIndex _process_xml_node(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    char directory_buffer[DC_UTILS_FILEPATH_BUFFER_SIZE];
    xmlChar *dir_attr = xmlGetProp(xml_node, BAD_CAST "_Directory");
    if (dir_attr) {
        strncpy(directory_buffer, (const char *)dir_attr, sizeof(directory_buffer) - 1);
        directory_buffer[sizeof(directory_buffer) - 1] = '\0';
        directory = directory_buffer;
        xmlFree(dir_attr);
    }

    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);
    switch (elem_type) {
        case DC_APP_ELEM_TYPE_NONELEM:
            return _process_xml_node_nonelem(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_ARC:
            return _process_xml_node_arc(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BLINK:
            return _process_xml_node_blink(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON:
            return _process_xml_node_button(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON_DISABLED:
            return _process_xml_node_button_disabled(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON_ENABLED:
            return _process_xml_node_button_enabled(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_OFF:
            return _process_xml_node_button_indicator_off(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_ON:
            return _process_xml_node_button_indicator_on(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON_TRANSITION:
            return _process_xml_node_button_transition(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_CIRCLE:
            return _process_xml_node_circle(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_CONSTANT:
            return _process_xml_node_constant(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_CONTAINER:
            return _process_xml_node_container(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_DCAPP:
            return _process_xml_node_dcapp(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_DEFAULT:
            return _process_xml_node_default(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_FALSE:
            return _process_xml_node_false(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_FUNCTION:
            return _process_xml_node_function(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_IF:
            return _process_xml_node_if(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_IMAGE:
            return _process_xml_node_image(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_LINE:
            return _process_xml_node_line(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_LOGIC:
            return _process_xml_node_logic(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_ACTIVE:
            return _process_xml_node_mouse_active(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_HOVERED:
            return _process_xml_node_mouse_hovered(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_INACTIVE:
            return _process_xml_node_mouse_inactive(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_MOTION:
            return _process_xml_node_mouse_motion(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_PRESSED:
            return _process_xml_node_mouse_pressed(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_RELEASED:
            return _process_xml_node_mouse_released(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PANEL:
            return _process_xml_node_panel(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PIXELSTREAM:
            return _process_xml_node_pixelstream(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_POLYGON:
            return _process_xml_node_polygon(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_RECTANGLE:
            return _process_xml_node_rectangle(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_SET:
            return _process_xml_node_set(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_SPHERE:
            return _process_xml_node_sphere(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_STENCIL:
            return _process_xml_node_stencil(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_STENCIL_ADD:
            return _process_xml_node_stencil_add(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_STENCIL_DRAW:
            return _process_xml_node_stencil_draw(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_STENCIL_REMOVE:
            return _process_xml_node_stencil_remove(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_STYLE:
            return _process_xml_node_style(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TERRAIN:
            return _process_xml_node_terrain(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TERRAIN_DEM:
            return _process_xml_node_terrain_dem(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TEXT:
            return _process_xml_node_text(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TRICK_FROM:
            return _process_xml_node_trick_from(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TRICK_IO:
            return _process_xml_node_trick_io(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TRICK_TO:
            return _process_xml_node_trick_to(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TRICK_VARIABLE:
            return _process_xml_node_trick_variable(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TRUE:
            return _process_xml_node_true(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_VARIABLE:
            return _process_xml_node_variable(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_VERTEX:
            return _process_xml_node_vertex(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_WINDOW:
            return _process_xml_node_window(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        default:
            return NODE_INDEX_UNDEFINED;
    }
}

static _NodeIndex _process_xml_node_nonelem(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    // ignore non-element nodes
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_arc(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_ARC;
    dc_node.parent = parent_node_index;
    dc_node.next   = NODE_INDEX_UNDEFINED;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.arc.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.arc.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.arc.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.arc.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.arc.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    } else {
        dc_node.arc.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.arc.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    } else {
        dc_node.arc.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (raw_x_align) {
        dc_node.arc.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    } else {
        dc_node.arc.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (raw_y_align) {
        dc_node.arc.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    } else {
        dc_node.arc.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation (where the center of the arc points, 0 = top)
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.arc.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
    } else {
        dc_node.arc.rotation = DC_APP_VAL_INDEX_UNDEFINED;
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
        dc_node.arc.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.arc.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

        dc_node.arc.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.arc.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
        if (raw_pivot_align_x) {
            dc_node.arc.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
            xmlFree(raw_pivot_align_x);
        } else {
            dc_node.arc.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        }

        xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
        if (raw_pivot_align_y) {
            dc_node.arc.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
            xmlFree(raw_pivot_align_y);
        } else {
            dc_node.arc.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
        }

        dc_node.arc.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.arc.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
    } else {
        fprintf(stderr, "DCAPP _process_xml_node(): Arc: invalid PivotParameters; must use both PivotPosition params, or none.\n");
    }

    // radius
    xmlChar *raw_radius = xmlGetProp(xml_node, BAD_CAST "Radius");
    if (raw_radius) {
        dc_node.arc.radius = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius);
        xmlFree(raw_radius);
    } else {
        dc_node.arc.radius = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // angle (span of the arc in degrees)
    xmlChar *raw_angle = xmlGetProp(xml_node, BAD_CAST "Angle");
    if (raw_angle) {
        dc_node.arc.angle = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_angle);
        xmlFree(raw_angle);
    } else {
        // default to 90 degrees if not specified
        dc_node.arc.angle = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, "90");
    }

    // segments
    xmlChar *raw_segments = xmlGetProp(xml_node, BAD_CAST "Segments");
    if (raw_segments) {
        dc_node.arc.num_segments = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_segments);
        xmlFree(raw_segments);
    } else {
        dc_node.arc.num_segments = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.arc.line_width = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    } else {
        dc_node.arc.line_width = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // colors
    dc_node.arc.fill_enabled = _load_color_from_string(app_data, xml_node, "FillColor", &(dc_node.arc.fill_color));
    dc_node.arc.line_enabled = _load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.arc.line_color));

    // pie mode (draw lines from endpoints to center)
    xmlChar *raw_pie = xmlGetProp(xml_node, BAD_CAST "Pie");
    if (raw_pie) {
        dc_node.arc.pie = (strcmp((const char *)raw_pie, "true") == 0 || strcmp((const char *)raw_pie, "1") == 0);
        xmlFree(raw_pie);
    } else {
        dc_node.arc.pie = false;
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    return node_index;
}

static _NodeIndex _process_xml_node_circle(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
        dc_node.circle.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.circle.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.circle.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.circle.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.circle.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    } else {
        dc_node.circle.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.circle.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    } else {
        dc_node.circle.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.circle.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    } else {
        dc_node.circle.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.circle.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    } else {
        dc_node.circle.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.circle.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

        dc_node.circle.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.circle.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

        dc_node.circle.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.circle.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

        xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
        if (raw_pivot_align_x) {
            dc_node.circle.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
            xmlFree(raw_pivot_align_x);
        } else {
            dc_node.circle.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        }

        xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
        if (raw_pivot_align_y) {
            dc_node.circle.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
            xmlFree(raw_pivot_align_y);
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
        dc_node.circle.radius = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius);
        xmlFree(raw_radius);
    } else {
        dc_node.circle.radius = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // segments
    xmlChar *raw_segments = xmlGetProp(xml_node, BAD_CAST "Segments");
    if (raw_segments) {
        dc_node.circle.num_segments = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_segments);
        xmlFree(raw_segments);
    } else {
        dc_node.circle.num_segments = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.circle.line_width = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    } else {
        dc_node.circle.line_width = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // colors
    dc_node.circle.fill_enabled = _load_color_from_string(app_data, xml_node, "FillColor", &(dc_node.circle.fill_color));
    dc_node.circle.line_enabled = _load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.circle.line_color));

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children
    _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // enable/disable mouse events
    _MouseEventChildren *mouse_events = &(_get_node(app_data, node_index)->circle.mouse_events);
    mouse_events->enabled =
        (mouse_events->pressed != NODE_INDEX_UNDEFINED) ||
        (mouse_events->released != NODE_INDEX_UNDEFINED) ||
        (mouse_events->active != NODE_INDEX_UNDEFINED) ||
        (mouse_events->inactive != NODE_INDEX_UNDEFINED) ||
        (mouse_events->hovered != NODE_INDEX_UNDEFINED);

    // return
    return node_index;
}

// Counter for generating unique anonymous variable names
static unsigned int _anon_var_counter = 0;

// Helper: Create an anonymous variable and return its index
static DcAppVarIndex _create_anonymous_variable(_AppData *app_data, DcValueType type, const char *initial_value_str) {
    char name[DC_VALUE_STRING_BUFFER_SIZE];
    snprintf(name, sizeof(name), "__anon_%u", _anon_var_counter++);

    DcValue initial_value = dc_value_create_value_string(initial_value_str);
    initial_value.type    = type;

    DcAppLookupVar var = {};
    var.value_index    = dc_app_lookup_register_value(app_data->lookup, &initial_value);
    return dc_app_lookup_register_var(app_data->lookup, name, &var);
}

static _NodeIndex _process_xml_node_blink(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node;
    dc_node.type   = NODE_TYPE_BLINK;
    dc_node.parent = parent_node_index;
    dc_node.next   = NODE_INDEX_UNDEFINED;

    // frequency (blinks per second)
    xmlChar *raw_frequency = xmlGetProp(xml_node, BAD_CAST "Frequency");
    if (raw_frequency) {
        dc_node.blink.frequency = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_frequency);
        xmlFree(raw_frequency);
    } else {
        DcValue temp_value      = dc_value_create_value_double(1.0);
        dc_node.blink.frequency = dc_app_lookup_register_value(app_data->lookup, &temp_value);
    }

    // duty cycle (fraction on, 0.0 to 1.0)
    xmlChar *raw_duty_cycle = xmlGetProp(xml_node, BAD_CAST "DutyCycle");
    if (raw_duty_cycle) {
        dc_node.blink.duty_cycle = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_duty_cycle);
        xmlFree(raw_duty_cycle);
    } else {
        DcValue temp_value       = dc_value_create_value_double(0.5);
        dc_node.blink.duty_cycle = dc_app_lookup_register_value(app_data->lookup, &temp_value);
    }

    // duration (seconds, <= 0 = indefinite toggle)
    xmlChar *raw_duration = xmlGetProp(xml_node, BAD_CAST "Duration");
    if (raw_duration) {
        dc_node.blink.duration = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_duration);
        xmlFree(raw_duration);
    } else {
        DcValue temp_value     = dc_value_create_value_double(-1.0);
        dc_node.blink.duration = dc_app_lookup_register_value(app_data->lookup, &temp_value);
    }

    // variable that triggers blink
    xmlChar *raw_var = xmlGetProp(xml_node, BAD_CAST "Variable");
    if (raw_var) {
        dc_node.blink.var = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_var);
        xmlFree(raw_var);
    } else {
        dc_node.blink.var = DC_APP_VAR_INDEX_UNDEFINED;
    }

    // initialize child
    dc_node.blink.child = NODE_INDEX_UNDEFINED;

    // initialize runtime state
    dc_node.blink.remaining_duration  = 0.0;
    dc_node.blink.last_frame_time     = 0.0;
    dc_node.blink.last_trigger_value  = 0;

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // update child index
    _Node *node       = _get_node(app_data, node_index);
    node->blink.child = first_child_index;

    // return
    return node_index;
}
static _NodeIndex _process_xml_node_button(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_BUTTON;
    dc_node.parent = parent_node_index;
    dc_node.next   = NODE_INDEX_UNDEFINED;

    // initialize child indices
    dc_node.button.child_enabled       = NODE_INDEX_UNDEFINED;
    dc_node.button.child_disabled      = NODE_INDEX_UNDEFINED;
    dc_node.button.child_indicator_on  = NODE_INDEX_UNDEFINED;
    dc_node.button.child_indicator_off = NODE_INDEX_UNDEFINED;
    dc_node.button.child_transition    = NODE_INDEX_UNDEFINED;
    dc_node.button.child_pressed       = NODE_INDEX_UNDEFINED;
    dc_node.button.child_released      = NODE_INDEX_UNDEFINED;
    dc_node.button.child               = NODE_INDEX_UNDEFINED;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.button.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.button.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.button.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.button.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.button.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    } else {
        dc_node.button.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.button.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    } else {
        dc_node.button.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // virtual x dimension
    xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
    if (!raw_x_virtual_dimension) {
        raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
    }
    if (raw_x_virtual_dimension) {
        dc_node.button.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    } else {
        dc_node.button.virtual_dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // virtual y dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.button.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    } else {
        dc_node.button.virtual_dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.button.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    } else {
        dc_node.button.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.button.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    } else {
        dc_node.button.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.button.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    } else {
        dc_node.button.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.button.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    } else {
        dc_node.button.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.button.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
    } else {
        dc_node.button.rotation = DC_APP_VAL_INDEX_UNDEFINED;
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

        dc_node.button.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.button.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

        dc_node.button.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.button.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

        xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
        if (raw_pivot_align_x) {
            dc_node.button.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
            xmlFree(raw_pivot_align_x);
        } else {
            dc_node.button.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        }

        xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
        if (raw_pivot_align_y) {
            dc_node.button.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
            xmlFree(raw_pivot_align_y);
        } else {
            dc_node.button.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
        }

        dc_node.button.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.button.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
    } else {
        fprintf(stderr, "DCAPP _process_xml_node_button(): invalid PivotParameters; must use both PivotPosition params, or none.\n");
    }

    // button type
    xmlChar *raw_type   = xmlGetProp(xml_node, BAD_CAST "Type");
    dc_node.button.type = DC_APP_BUTTON_TYPE_STANDARD;
    if (raw_type) {
        dc_node.button.type = dc_utils_string_to_integer((const char *)raw_type);
        xmlFree(raw_type);
    }

    // process value inheritance
    {
        xmlChar *raw_default_on   = xmlGetProp(xml_node, BAD_CAST "On");
        xmlChar *raw_default_off  = xmlGetProp(xml_node, BAD_CAST "Off");
        xmlChar *raw_enabled_on   = xmlGetProp(xml_node, BAD_CAST "EnabledOn");
        xmlChar *raw_target_on    = xmlGetProp(xml_node, BAD_CAST "TargetOn");
        xmlChar *raw_target_off   = xmlGetProp(xml_node, BAD_CAST "TargetOff");
        xmlChar *raw_indicator_on = xmlGetProp(xml_node, BAD_CAST "IndicatorOn");

        // process defaults
        DcAppValIndex default_on_val = DC_APP_VAL_INDEX_UNDEFINED;
        if (raw_default_on) {
            default_on_val = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_STRING, (const char *)raw_default_on);
        }
        DcAppValIndex default_off_val = DC_APP_VAL_INDEX_UNDEFINED;
        if (raw_default_off) {
            default_off_val = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_STRING, (const char *)raw_default_off);
        }

        // process target
        if (raw_target_on) {
            dc_node.button.val_target_on = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_STRING, (const char *)raw_target_on);
        } else {
            if (default_on_val != DC_APP_VAL_INDEX_UNDEFINED) {
                dc_node.button.val_target_on = default_on_val;
            } else {
                DcValue temp_value           = dc_value_create_value_integer(1);
                dc_node.button.val_target_on = dc_app_lookup_register_value(app_data->lookup, &temp_value);
            }
        }
        if (raw_target_off) {
            dc_node.button.val_target_off = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_STRING, (const char *)raw_target_off);
        } else {
            if (default_off_val != DC_APP_VAL_INDEX_UNDEFINED) {
                dc_node.button.val_target_off = default_off_val;
            } else {
                DcValue temp_value            = dc_value_create_value_integer(0);
                dc_node.button.val_target_off = dc_app_lookup_register_value(app_data->lookup, &temp_value);
            }
        }

        // process indicator
        if (raw_indicator_on) {
            dc_node.button.val_indicator_on = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_STRING, (const char *)raw_indicator_on);
        } else {
            dc_node.button.val_indicator_on = dc_node.button.val_target_on;
        }

        // process enabled
        if (raw_enabled_on) {
            dc_node.button.val_enabled_on = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_STRING, (const char *)raw_enabled_on);
        } else {
            DcValue temp_value            = dc_value_create_value_integer(1);
            dc_node.button.val_enabled_on = dc_app_lookup_register_value(app_data->lookup, &temp_value);
        }

        // cleanup
        xmlFree(raw_default_on);
        xmlFree(raw_default_off);
        xmlFree(raw_enabled_on);
        xmlFree(raw_target_on);
        xmlFree(raw_target_off);
        xmlFree(raw_indicator_on);
    }

    // process variable inheritance
    {
        xmlChar *raw_default_variable   = xmlGetProp(xml_node, BAD_CAST "Variable");
        xmlChar *raw_enabled_variable   = xmlGetProp(xml_node, BAD_CAST "EnabledVariable");
        xmlChar *raw_target_variable    = xmlGetProp(xml_node, BAD_CAST "TargetVariable");
        xmlChar *raw_indicator_variable = xmlGetProp(xml_node, BAD_CAST "IndicatorVariable");

        // default variable
        DcAppVarIndex default_variable_index = DC_APP_VAR_INDEX_UNDEFINED;
        if (raw_default_variable) {
            default_variable_index = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_default_variable);
        }

        // target variable
        if (raw_target_variable) {
            dc_node.button.var_target = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_target_variable);
        } else {
            if (default_variable_index != DC_APP_VAR_INDEX_UNDEFINED) {
                dc_node.button.var_target = default_variable_index;
            } else {
                const char *initial_value_str = dc_app_lookup_get_value(app_data->lookup, dc_node.button.val_target_off)->value_string;
                dc_node.button.var_target     = _register_anonymous_variable(app_data, DC_VALUE_TYPE_STRING, initial_value_str);
            }
        }

        // indicator variable
        if (raw_indicator_variable) {
            dc_node.button.var_indicator = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_indicator_variable);
        } else {
            dc_node.button.var_indicator = dc_node.button.var_target;
        }

        // enabled
        if (raw_enabled_variable) {
            dc_node.button.var_enabled = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_enabled_variable);
        } else {
            dc_node.button.var_enabled    = DC_APP_VAR_INDEX_UNDEFINED;
            const char *initial_value_str = dc_app_lookup_get_value(app_data->lookup, dc_node.button.val_enabled_on)->value_string;
            dc_node.button.var_enabled    = _register_anonymous_variable(app_data, DC_VALUE_TYPE_STRING, initial_value_str);
        }

        // cleanup
        xmlFree(raw_default_variable);
        xmlFree(raw_enabled_variable);
        xmlFree(raw_target_variable);
        xmlFree(raw_indicator_variable);
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // handle non event elements
    if (first_child_index != NODE_INDEX_UNDEFINED) {
        _Node *node        = _get_node(app_data, node_index);
        node->button.child = first_child_index;
    }

    // return
    return node_index;

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_button_disabled(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            _NodeIndex first_child_index       = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            _Node     *parent_node             = _get_node(app_data, parent_node_index);
            parent_node->button.child_disabled = first_child_index;
            break;
        }
        default:
            fprintf(stderr, "DCApp _process_xml_node_button_disabled(): Invalid parent of type %s for <Disabled>\n", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_button_enabled(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            _NodeIndex first_child_index      = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            _Node     *parent_node            = _get_node(app_data, parent_node_index);
            parent_node->button.child_enabled = first_child_index;
            break;
        }
        default:
            fprintf(stderr, "DCApp _process_xml_node_button_enabled(): Invalid parent of type %s for <Enabled>\n", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_button_indicator_off(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            _NodeIndex first_child_index            = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            _Node     *parent_node                  = _get_node(app_data, parent_node_index);
            parent_node->button.child_indicator_off = first_child_index;
            break;
        }
        default:
            fprintf(stderr, "DCApp _process_xml_node_button_indicator_off(): Invalid parent of type %s for <Off>\n", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_button_indicator_on(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            _NodeIndex first_child_index           = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            _Node     *parent_node                 = _get_node(app_data, parent_node_index);
            parent_node->button.child_indicator_on = first_child_index;
            break;
        }
        default:
            fprintf(stderr, "DCApp _process_xml_node_button_indicator_on(): Invalid parent of type %s for <On>\n", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_button_transition(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            _NodeIndex first_child_index         = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            _Node     *parent_node               = _get_node(app_data, parent_node_index);
            parent_node->button.child_transition = first_child_index;
            break;
        }
        default:
            fprintf(stderr, "DCApp _process_xml_node_button_transition(): Invalid parent of type %s for <Transition>\n", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_constant(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    // ignore at this point
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_container(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
        dc_node.container.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.container.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.container.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.container.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.container.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    } else {
        dc_node.container.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.container.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    } else {
        dc_node.container.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // virtual x dimension
    xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
    if (!raw_x_virtual_dimension) {
        raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
    }
    if (raw_x_virtual_dimension) {
        dc_node.container.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    } else {
        dc_node.container.virtual_dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // virtual y virtual_dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.container.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    } else {
        dc_node.container.virtual_dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.container.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    } else {
        dc_node.container.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.container.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    } else {
        dc_node.container.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.container.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    } else {
        dc_node.container.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.container.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    } else {
        dc_node.container.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.container.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

        dc_node.container.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.container.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

        dc_node.container.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.container.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

        xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
        if (raw_pivot_align_x) {
            dc_node.container.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
            xmlFree(raw_pivot_align_x);
        } else {
            dc_node.container.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        }

        xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
        if (raw_pivot_align_y) {
            dc_node.container.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
            xmlFree(raw_pivot_align_y);
        } else {
            dc_node.container.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
        }

        dc_node.container.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.container.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
    } else {
        fprintf(stderr, "DCAPP _process_xml_node(): Container: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // update child index
    _Node *node           = _get_node(app_data, node_index);
    node->container.child = first_child_index;

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_dcapp(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _process_xml_node_children(app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_default(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    // ignore at this point
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_false(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_IF: {

            // process children
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);

            // update child false node
            _Node *parent_node                   = _get_node(app_data, parent_node_index);
            parent_node->conditional.child_false = first_child_index;
            break;
        }
        default:
            fprintf(stderr, "DCAPP _process_xml_node(): Invalid elem parent of type %s for <False>\n", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_function(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_FUNCTION;
    dc_node.parent = parent_node_index;
    dc_node.next   = NODE_INDEX_UNDEFINED;

    // get function name
    xmlChar *raw_name = xmlGetProp(xml_node, BAD_CAST "Name");
    if (raw_name) {
        if (app_data->logic_lib) {
            dc_node.function.callback = (void (*)(void))_ext_library->load_function(app_data->logic_lib, (const char *)raw_name);
            if (!dc_node.function.callback) {
                fprintf(stderr, "DCAPP _process_xml_node_function(): Failed to load function '%s' from logic library\n", (const char *)raw_name);
            }
        } else {
            fprintf(stderr, "DCAPP _process_xml_node_function(): No logic library loaded, cannot load function '%s'\n", (const char *)raw_name);
        }
        xmlFree(raw_name);
    } else {
        fprintf(stderr, "DCAPP _process_xml_node_function(): Missing 'Name' attribute\n");
    }

    // register node
    return _register_node(app_data, &dc_node);
}

static _NodeIndex _process_xml_node_if(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node                   = {};
    dc_node.type                    = NODE_TYPE_CONDITIONAL;
    dc_node.parent                  = parent_node_index;
    dc_node.next                    = NODE_INDEX_UNDEFINED;
    dc_node.conditional.child_true  = NODE_INDEX_UNDEFINED;
    dc_node.conditional.child_false = NODE_INDEX_UNDEFINED;

    // conditional type
    xmlChar *raw_type = xmlGetProp(xml_node, BAD_CAST "Operation");
    if (raw_type) {
        dc_node.conditional.type = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_type);
        xmlFree(raw_type);
    } else {
        dc_node.conditional.type = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // value1
    xmlChar *raw_value1 = xmlGetProp(xml_node, BAD_CAST "Value");
    if (!raw_value1) {
        raw_value1 = xmlGetProp(xml_node, BAD_CAST "Value1");
    }
    if (raw_value1) {
        dc_node.conditional.value1 = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_STRING, (const char *)raw_value1);
        xmlFree(raw_value1);
    } else {
        fprintf(stderr, "DCAPP _process_xml_node: Conditional: no value specified\n");
    }

    // value2
    xmlChar *raw_value2 = xmlGetProp(xml_node, BAD_CAST "Value2");
    if (raw_value2) {
        dc_node.conditional.value2 = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_STRING, (const char *)raw_value2);
        xmlFree(raw_value2);
    } else {
        dc_node.conditional.value2 = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children (assigning to true/false handled in separate cases, e.g. DC_APP_ELEM_TYPE_TRUE)
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // handle implicit <True> elements
    if (first_child_index != NODE_INDEX_UNDEFINED) {

        // ignore if True element already exists
        _Node *node = _get_node(app_data, node_index);
        if (node->conditional.child_true == NODE_INDEX_UNDEFINED) {
            node->conditional.child_true = first_child_index;
        } else {
            printf("DCApp _process_xml_node: Conditional: <If> element has <True> explicit and implicit elements. Ignoring the implicit definitions\n");
        }
    }

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_image(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
    for (int ii = 0; ii < sbcount(app_data->sb_textures); ii++) {
        const char *comp_name = &(app_data->sb_texture_names[app_data->sb_texture_name_offsets[ii]]);
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
        _Texture texture = _create_texture(app_data, image_width, image_height, canon_filepath);

        // set the initial pl_texture usage (this is a no-op in metal but does layout transition for vulkan)
        plBlitEncoder *encoder = _ext_starter->get_blit_encoder();
        _ext_gfx->set_texture_usage(encoder, texture.texture_handle, PL_TEXTURE_USAGE_SAMPLED, 0);

        // copy memory to mapped staging buffer
        plBuffer *staging_buffer = _ext_gfx->get_buffer(device, app_data->pl_staging_buffer_handle);
        memcpy(staging_buffer->tMemoryAllocation.pHostMapped, image_data, image_width * image_height * 4);

        // copy staging buffer to image
        plBufferImageCopy buffer_image_copy;
        memset(&buffer_image_copy, 0, sizeof(plBufferImageCopy));
        buffer_image_copy.uImageWidth    = (uint32_t)image_width;
        buffer_image_copy.uImageHeight   = (uint32_t)image_height;
        buffer_image_copy.uImageDepth    = 1;
        buffer_image_copy.uLayerCount    = 1;
        buffer_image_copy.szBufferOffset = 0;
        _ext_gfx->copy_buffer_to_texture(encoder, app_data->pl_staging_buffer_handle, texture.texture_handle, 1, &buffer_image_copy);

        // return encoder
        _ext_starter->return_blit_encoder(encoder);

        // free image data
        _ext_image->free(image_data);

        // add texture to internal arrays
        sbpush(app_data->sb_texture_name_offsets, sbcount(app_data->sb_texture_names));
        sbpushn(app_data->sb_texture_names, canon_filepath, (int)strlen(canon_filepath) + 1);
        sbpush(app_data->sb_textures, texture);
        texture_index = sbcount(app_data->sb_textures) - 1;
    }

    // update structure
    dc_node.image.texture_index = texture_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.image.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.image.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.image.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.image.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.image.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    } else {
        dc_node.image.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.image.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    } else {
        dc_node.image.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.image.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    } else {
        dc_node.image.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.image.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    } else {
        dc_node.image.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.image.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    } else {
        dc_node.image.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.image.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    } else {
        dc_node.image.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.image.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

        dc_node.image.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.image.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

        dc_node.image.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.image.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

        xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
        if (raw_pivot_align_x) {
            dc_node.image.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
            xmlFree(raw_pivot_align_x);
        } else {
            dc_node.image.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        }

        xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
        if (raw_pivot_align_y) {
            dc_node.image.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
            xmlFree(raw_pivot_align_y);
        } else {
            dc_node.image.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
        }

        dc_node.image.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.image.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
    } else {
        fprintf(stderr, "DCAPP _process_xml_node(): Image: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children
    _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // enable/disable mouse events
    _MouseEventChildren *mouse_events = &(_get_node(app_data, node_index)->image.mouse_events);
    mouse_events->enabled =
        (mouse_events->pressed != NODE_INDEX_UNDEFINED) ||
        (mouse_events->released != NODE_INDEX_UNDEFINED) ||
        (mouse_events->active != NODE_INDEX_UNDEFINED) ||
        (mouse_events->inactive != NODE_INDEX_UNDEFINED) ||
        (mouse_events->hovered != NODE_INDEX_UNDEFINED);

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_line(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
        dc_node.line.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.line.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.line.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.line.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.line.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

        dc_node.line.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.line.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (raw_pivot_position_x || raw_pivot_position_y) {
        fprintf(stderr, "DCAPP _process_xml_node(): line: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.line.line_width = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    } else {
        dc_node.line.line_width = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // colors
    dc_node.line.line_enabled = _load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.line.line_color));

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children
    _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_logic(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    if (app_data->logic_lib) {
        fprintf(stderr, "DCApp _process_xml_node_logic(): duplicate <Logic> definitions\n");
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

        // try loading with different platform extensions
        // strip existing extension if present (.so, .dylib, .dll)
        char base_filepath[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(base_filepath, abs_filepath, DC_VALUE_STRING_BUFFER_SIZE - 1);
        base_filepath[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';

        char *ext = strrchr(base_filepath, '.');
        if (ext && (strcmp(ext, ".so") == 0 || strcmp(ext, ".dylib") == 0 || strcmp(ext, ".dll") == 0)) {
            *ext = '\0';  // strip the extension
        }

        // try each platform extension
        const char *extensions[] = { ".so", ".dylib", ".dll" };
        char try_filepath[DC_VALUE_STRING_BUFFER_SIZE];
        bool loaded = false;

        for (int i = 0; i < 3 && !loaded; i++) {
            snprintf(try_filepath, sizeof(try_filepath), "%s%s", base_filepath, extensions[i]);
            const plLibraryDesc logic_so_desc = {
                .tFlags = PL_LIBRARY_FLAGS_NONE,
                .pcName = try_filepath,
            };
            if (_ext_library->load(logic_so_desc, &app_data->logic_lib) == PL_LIBRARY_RESULT_SUCCESS) {
                loaded = true;
            }
        }

        if (!loaded) {
            fprintf(stderr, "DCApp _process_xml_node_logic(): Failed to load logic library (%s.so/.dylib/.dll)\n", base_filepath);
        }

        // load functions
        app_data->logic_pre_init = (void (*)(_GetVariableValueAddr))_ext_library->load_function(app_data->logic_lib, "display_pre_init");
        app_data->logic_init     = (void (*)(void))_ext_library->load_function(app_data->logic_lib, "display_init");
        app_data->logic_draw     = (void (*)(void))_ext_library->load_function(app_data->logic_lib, "display_draw");
        app_data->logic_close    = (void (*)(void))_ext_library->load_function(app_data->logic_lib, "display_close");
    } else {
        fprintf(stderr, "DCAPP _process_xml_node_logic(): <Logic> has no File element\n");
    }

    // return
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_mouse_active(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_CIRCLE: {
            parent_node->circle.mouse_events.active = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_IMAGE: {
            parent_node->image.mouse_events.active = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_POLYGON: {
            parent_node->polygon.mouse_events.active = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_RECTANGLE: {
            parent_node->rectangle.mouse_events.active = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        default:
            fprintf(stderr, "DCApp _process_xml_node() mouse_active: unknown parent of type %s\n",
                    _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_mouse_hovered(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_CIRCLE: {
            parent_node->circle.mouse_events.hovered = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_IMAGE: {
            parent_node->image.mouse_events.hovered = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_POLYGON: {
            parent_node->polygon.mouse_events.hovered = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_RECTANGLE: {
            parent_node->rectangle.mouse_events.hovered = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        default:
            fprintf(stderr, "DCApp _process_xml_node() mouse_hovered: unknown parent of type %s\n",
                    _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_mouse_inactive(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_CIRCLE: {
            parent_node->circle.mouse_events.inactive = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_IMAGE: {
            parent_node->image.mouse_events.inactive = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_POLYGON: {
            parent_node->polygon.mouse_events.inactive = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_RECTANGLE: {
            parent_node->rectangle.mouse_events.inactive = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        default:
            fprintf(stderr, "DCApp _process_xml_node() mouse_inactive: unknown parent of type %s\n",
                    _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_mouse_motion(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_MOUSE_MOTION;
    dc_node.parent = parent_node_index;
    dc_node.next   = NODE_INDEX_UNDEFINED;

    // VariableX
    xmlChar *raw_var_x = xmlGetProp(xml_node, BAD_CAST "VariableX");
    if (raw_var_x) {
        dc_node.mouse_motion.var_x = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_var_x);
        xmlFree(raw_var_x);
    } else {
        dc_node.mouse_motion.var_x = DC_APP_VAR_INDEX_UNDEFINED;
    }

    // VariableY
    xmlChar *raw_var_y = xmlGetProp(xml_node, BAD_CAST "VariableY");
    if (raw_var_y) {
        dc_node.mouse_motion.var_y = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_var_y);
        xmlFree(raw_var_y);
    } else {
        dc_node.mouse_motion.var_y = DC_APP_VAR_INDEX_UNDEFINED;
    }

    return _register_node(app_data, &dc_node);
}

static _NodeIndex _process_xml_node_mouse_pressed(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON: {
            parent_node->button.child_pressed = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_CIRCLE: {
            parent_node->circle.mouse_events.pressed = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_IMAGE: {
            parent_node->image.mouse_events.pressed = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_POLYGON: {
            parent_node->polygon.mouse_events.pressed = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_RECTANGLE: {
            parent_node->rectangle.mouse_events.pressed = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        default:
            fprintf(stderr, "DCApp _process_xml_node() mouse_pressed: unknown parent of type %s\n",
                    _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_mouse_released(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON: {
            parent_node->button.child_released = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_CIRCLE: {
            parent_node->circle.mouse_events.released = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_IMAGE: {
            parent_node->image.mouse_events.released = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_POLYGON: {
            parent_node->polygon.mouse_events.released = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        case NODE_TYPE_RECTANGLE: {
            parent_node->rectangle.mouse_events.released = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            break;
        }
        default:
            fprintf(stderr, "DCApp _process_xml_node() mouse_released: unknown parent of type %s\n",
                    _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_panel(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
        dc_node.panel.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    } else {
        dc_node.panel.virtual_dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // virtual y virtual_dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.panel.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    } else {
        dc_node.panel.virtual_dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // update child index
    _Node *node       = _get_node(app_data, node_index);
    node->panel.child = first_child_index;

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_pixelstream(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
        _Texture texture = _create_texture(app_data, _NODE_PIXELSTREAM_MAX_WIDTH, _NODE_PIXELSTREAM_MAX_HEIGHT, "pixelstream");

        // add texture to internal arrays
        sbpush(app_data->sb_texture_name_offsets, sbcount(app_data->sb_texture_names));
        sbpushn(app_data->sb_texture_names, "--dummy--", (int)strlen("--dummy--") + 1);
        sbpush(app_data->sb_textures, texture);
        _TextureIndex texture_index = sbcount(app_data->sb_textures) - 1;

        // update structure
        dc_node.pixelstream.texture_index = texture_index;
    }

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.pixelstream.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.pixelstream.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.pixelstream.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.pixelstream.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.pixelstream.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    } else {
        dc_node.pixelstream.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.pixelstream.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    } else {
        dc_node.pixelstream.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.pixelstream.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    } else {
        dc_node.pixelstream.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.pixelstream.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    } else {
        dc_node.pixelstream.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.pixelstream.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    } else {
        dc_node.pixelstream.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.pixelstream.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    } else {
        dc_node.pixelstream.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.pixelstream.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

        dc_node.pixelstream.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.pixelstream.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

        dc_node.pixelstream.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.pixelstream.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

        xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
        if (raw_pivot_align_x) {
            dc_node.pixelstream.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
            xmlFree(raw_pivot_align_x);
        } else {
            dc_node.pixelstream.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        }

        xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
        if (raw_pivot_align_y) {
            dc_node.pixelstream.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
            xmlFree(raw_pivot_align_y);
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
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children
    _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // enable/disable mouse events
    _MouseEventChildren *mouse_events = &(_get_node(app_data, node_index)->pixelstream.mouse_events);
    mouse_events->enabled =
        (mouse_events->pressed != NODE_INDEX_UNDEFINED) ||
        (mouse_events->released != NODE_INDEX_UNDEFINED) ||
        (mouse_events->active != NODE_INDEX_UNDEFINED) ||
        (mouse_events->inactive != NODE_INDEX_UNDEFINED) ||
        (mouse_events->hovered != NODE_INDEX_UNDEFINED);

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_polygon(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
        dc_node.polygon.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.polygon.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.polygon.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.polygon.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.polygon.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

        dc_node.polygon.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.polygon.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (raw_pivot_position_x || raw_pivot_position_y) {
        fprintf(stderr, "DCAPP _process_xml_node(): polygon: invalid PivotParameters; must use both PivotPosition params, or none. Using one is not allowed.\n");
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.polygon.line_width = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    } else {
        dc_node.polygon.line_width = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // colors
    dc_node.polygon.fill_enabled = _load_color_from_string(app_data, xml_node, "FillColor", &(dc_node.polygon.fill_color));
    dc_node.polygon.line_enabled = _load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.polygon.line_color));

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children
    _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // enable/disable mouse events
    _MouseEventChildren *mouse_events = &(_get_node(app_data, node_index)->polygon.mouse_events);
    mouse_events->enabled =
        (mouse_events->pressed != NODE_INDEX_UNDEFINED) ||
        (mouse_events->released != NODE_INDEX_UNDEFINED) ||
        (mouse_events->active != NODE_INDEX_UNDEFINED) ||
        (mouse_events->inactive != NODE_INDEX_UNDEFINED) ||
        (mouse_events->hovered != NODE_INDEX_UNDEFINED);

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_rectangle(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
        dc_node.rectangle.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.rectangle.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.rectangle.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.rectangle.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.rectangle.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    } else {
        dc_node.rectangle.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.rectangle.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    } else {
        dc_node.rectangle.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.rectangle.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    } else {
        dc_node.rectangle.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.rectangle.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    } else {
        dc_node.rectangle.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.rectangle.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    } else {
        dc_node.rectangle.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.rectangle.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    } else {
        dc_node.rectangle.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.rectangle.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

        dc_node.rectangle.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.rectangle.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

        dc_node.rectangle.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.rectangle.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

        xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
        if (raw_pivot_align_x) {
            dc_node.rectangle.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
            xmlFree(raw_pivot_align_x);
        } else {
            dc_node.rectangle.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        }

        xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
        if (raw_pivot_align_y) {
            dc_node.rectangle.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
            xmlFree(raw_pivot_align_y);
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
        dc_node.rectangle.line_width = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    } else {
        dc_node.rectangle.line_width = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // colors
    dc_node.rectangle.fill_enabled = _load_color_from_string(app_data, xml_node, "FillColor", &(dc_node.rectangle.fill_color));
    dc_node.rectangle.line_enabled = _load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.rectangle.line_color));

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children
    _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // enable/disable mouse events
    _MouseEventChildren *mouse_events = &(_get_node(app_data, node_index)->rectangle.mouse_events);
    mouse_events->enabled =
        (mouse_events->pressed != NODE_INDEX_UNDEFINED) ||
        (mouse_events->released != NODE_INDEX_UNDEFINED) ||
        (mouse_events->active != NODE_INDEX_UNDEFINED) ||
        (mouse_events->inactive != NODE_INDEX_UNDEFINED) ||
        (mouse_events->hovered != NODE_INDEX_UNDEFINED);

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_set(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_SET;
    dc_node.parent = parent_node_index;
    dc_node.next   = NODE_INDEX_UNDEFINED;

    // variable
    xmlChar *raw_variable = xmlGetProp(xml_node, BAD_CAST "Variable");
    if (raw_variable) {
        dc_node.set.var_index = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_variable);
        xmlFree(raw_variable);
    } else {
        fprintf(stderr, "DCAPP _process_xml_node: Missing attribute 'Variable' in <Set> element\n");
    }

    // operand
    xmlChar *raw_operand = xmlNodeGetContent(xml_node);
    if (raw_operand) {
        char operand[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(operand, (const char *)raw_operand, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_operand);
        dc_utils_trim_whitespace_inplace(operand);
        operand[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        dc_node.set.operand = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, operand);
    } else {
        fprintf(stderr, "DCAPP _process_xml_node: Missing Node content in <Set> element\n");
    }

    // operator
    xmlChar *raw_operator = xmlGetProp(xml_node, BAD_CAST "Operator");
    if (raw_operator) {
        dc_node.set.operation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_operator);
        xmlFree(raw_operator);
    } else {
        dc_node.set.operation = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // register node
    return _register_node(app_data, &dc_node);
}

static _NodeIndex _process_xml_node_sphere(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_SPHERE;
    dc_node.parent = parent_node_index;
    dc_node.next   = NODE_INDEX_UNDEFINED;

    // default texture to undefined
    dc_node.sphere.texture_index = TEXTURE_INDEX_UNDEFINED;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.sphere.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.sphere.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.sphere.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.sphere.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.sphere.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    } else {
        dc_node.sphere.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.sphere.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    } else {
        dc_node.sphere.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.sphere.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    } else {
        dc_node.sphere.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.sphere.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    } else {
        dc_node.sphere.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // pivot local x align
    xmlChar *raw_pivot_local_x_align = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
    if (raw_pivot_local_x_align) {
        dc_node.sphere.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_local_x_align);
        xmlFree(raw_pivot_local_x_align);
    } else {
        dc_node.sphere.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // pivot local y align
    xmlChar *raw_pivot_local_y_align = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
    if (raw_pivot_local_y_align) {
        dc_node.sphere.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_local_y_align);
        xmlFree(raw_pivot_local_y_align);
    } else {
        dc_node.sphere.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // pivot x position
    xmlChar *raw_pivot_x_position = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
    if (raw_pivot_x_position) {
        dc_node.sphere.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_x_position);
        xmlFree(raw_pivot_x_position);
    } else {
        dc_node.sphere.pivot_position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // pivot y position
    xmlChar *raw_pivot_y_position = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
    if (raw_pivot_y_position) {
        dc_node.sphere.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_y_position);
        xmlFree(raw_pivot_y_position);
    } else {
        dc_node.sphere.pivot_position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation (external 2D rotation)
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (raw_rotation) {
        dc_node.sphere.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
    } else {
        dc_node.sphere.rotation = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // radius
    xmlChar *raw_radius = xmlGetProp(xml_node, BAD_CAST "Radius");
    if (raw_radius) {
        dc_node.sphere.radius = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius);
        xmlFree(raw_radius);
    } else {
        dc_node.sphere.radius = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // fill color
    dc_node.sphere.fill_color.r = DC_APP_VAL_INDEX_UNDEFINED;
    dc_node.sphere.fill_color.g = DC_APP_VAL_INDEX_UNDEFINED;
    dc_node.sphere.fill_color.b = DC_APP_VAL_INDEX_UNDEFINED;
    dc_node.sphere.fill_color.a = DC_APP_VAL_INDEX_UNDEFINED;
    _load_color_from_string(app_data, xml_node, "FillColor", &dc_node.sphere.fill_color);

    // internal rotation (roll, pitch, yaw)
    xmlChar *raw_roll = xmlGetProp(xml_node, BAD_CAST "Roll");
    if (raw_roll) {
        dc_node.sphere.rpy.roll = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_roll);
        xmlFree(raw_roll);
    } else {
        dc_node.sphere.rpy.roll = DC_APP_VAL_INDEX_UNDEFINED;
    }

    xmlChar *raw_pitch = xmlGetProp(xml_node, BAD_CAST "Pitch");
    if (raw_pitch) {
        dc_node.sphere.rpy.pitch = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pitch);
        xmlFree(raw_pitch);
    } else {
        dc_node.sphere.rpy.pitch = DC_APP_VAL_INDEX_UNDEFINED;
    }

    xmlChar *raw_yaw = xmlGetProp(xml_node, BAD_CAST "Yaw");
    if (raw_yaw) {
        dc_node.sphere.rpy.yaw = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_yaw);
        xmlFree(raw_yaw);
    } else {
        dc_node.sphere.rpy.yaw = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // optional texture (Image attribute)
    xmlChar *filepath = xmlGetProp(xml_node, BAD_CAST "Image");
    if (filepath) {
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
        for (int ii = 0; ii < sbcount(app_data->sb_textures); ii++) {
            const char *comp_name = &(app_data->sb_texture_names[app_data->sb_texture_name_offsets[ii]]);
            if (strcmp(canon_filepath, comp_name) == 0) {
                texture_index = ii;
            }
        }

        // load new texture if it doesn't exist
        if (texture_index == TEXTURE_INDEX_UNDEFINED) {
            plDevice *device = _ext_starter->get_device();

            size_t         file_data_size;
            unsigned char *file_data = dc_utils_load_binary_file(canon_filepath, &file_data_size);

            int            image_width, image_height, channels;
            unsigned char *image_data = _ext_image->load(file_data, (int)file_data_size, &image_width, &image_height, &channels, 4);
            free(file_data);

            if (!image_data) {
                fprintf(stderr, "DCAPP _process_xml_node_sphere: Failed to load image: %s\n", canon_filepath);
                return _register_node(app_data, &dc_node);
            }

            _Texture texture = _create_texture(app_data, image_width, image_height, canon_filepath);

            plBlitEncoder *encoder = _ext_starter->get_blit_encoder();
            _ext_gfx->set_texture_usage(encoder, texture.texture_handle, PL_TEXTURE_USAGE_SAMPLED, 0);

            plBuffer *staging_buffer = _ext_gfx->get_buffer(device, app_data->pl_staging_buffer_handle);
            memcpy(staging_buffer->tMemoryAllocation.pHostMapped, image_data, image_width * image_height * 4);

            plBufferImageCopy buffer_image_copy;
            memset(&buffer_image_copy, 0, sizeof(plBufferImageCopy));
            buffer_image_copy.uImageWidth    = (uint32_t)image_width;
            buffer_image_copy.uImageHeight   = (uint32_t)image_height;
            buffer_image_copy.uImageDepth    = 1;
            buffer_image_copy.uLayerCount    = 1;
            buffer_image_copy.szBufferOffset = 0;
            _ext_gfx->copy_buffer_to_texture(encoder, app_data->pl_staging_buffer_handle, texture.texture_handle, 1, &buffer_image_copy);

            _ext_starter->return_blit_encoder(encoder);
            _ext_image->free(image_data);

            sbpush(app_data->sb_texture_name_offsets, sbcount(app_data->sb_texture_names));
            sbpushn(app_data->sb_texture_names, canon_filepath, (int)strlen(canon_filepath) + 1);
            sbpush(app_data->sb_textures, texture);
            texture_index = sbcount(app_data->sb_textures) - 1;
        }

        dc_node.sphere.texture_index = texture_index;
    }

    // register node
    return _register_node(app_data, &dc_node);
}

static _NodeIndex _process_xml_node_stencil(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node               = {};
    dc_node.type                = NODE_TYPE_STENCIL;
    dc_node.parent              = parent_node_index;
    dc_node.next                = NODE_INDEX_UNDEFINED;
    dc_node.stencil.sb_children = NULL;

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children (StencilAdd, StencilRemove, StencilDraw)
    _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    return node_index;
}

static _NodeIndex _process_xml_node_stencil_add(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_STENCIL: {
            // process children
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);

            // add entry to stencil's children buffer
            _StencilChild stencil_child = {
                .child = first_child_index,
                .type  = STENCIL_CHILD_TYPE_ADD,
            };
            _Node *parent_node = _get_node(app_data, parent_node_index);
            sbpush(parent_node->stencil.sb_children, stencil_child);
            break;
        }
        default:
            fprintf(stderr, "DCAPP _process_xml_node(): Invalid elem parent of type %s for <StencilAdd>\n", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_stencil_draw(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_STENCIL: {
            // process children
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);

            // add entry to stencil's children buffer
            _StencilChild stencil_child = {
                .child = first_child_index,
                .type  = STENCIL_CHILD_TYPE_DRAW,
            };
            _Node *parent_node = _get_node(app_data, parent_node_index);
            sbpush(parent_node->stencil.sb_children, stencil_child);
            break;
        }
        default:
            fprintf(stderr, "DCAPP _process_xml_node(): Invalid elem parent of type %s for <StencilDraw>\n", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_stencil_remove(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_STENCIL: {
            // process children
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);

            // add entry to stencil's children buffer
            _StencilChild stencil_child = {
                .child = first_child_index,
                .type  = STENCIL_CHILD_TYPE_REMOVE,
            };
            _Node *parent_node = _get_node(app_data, parent_node_index);
            sbpush(parent_node->stencil.sb_children, stencil_child);
            break;
        }
        default:
            fprintf(stderr, "DCAPP _process_xml_node(): Invalid elem parent of type %s for <StencilRemove>\n", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_style(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    // ignore at this point
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_terrain(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
        dc_node.terrain.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.terrain.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.terrain.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.terrain.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.terrain.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    } else {
        dc_node.terrain.dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.terrain.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    } else {
        dc_node.terrain.dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.terrain.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    } else {
        dc_node.terrain.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.terrain.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    } else {
        dc_node.terrain.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.terrain.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    } else {
        dc_node.terrain.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.terrain.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    } else {
        dc_node.terrain.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.terrain.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

        dc_node.terrain.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.terrain.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

        dc_node.terrain.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.terrain.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

        xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
        if (raw_pivot_align_x) {
            dc_node.terrain.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
            xmlFree(raw_pivot_align_x);
        } else {
            dc_node.terrain.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        }

        xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
        if (raw_pivot_align_y) {
            dc_node.terrain.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
            xmlFree(raw_pivot_align_y);
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
        dc_node.terrain.lle.lat = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
        xmlFree(raw_lat);
    } else {
        fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Latitude with Terrain primitive");
    }

    // lon
    xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
    if (raw_lon) {
        dc_node.terrain.lle.lon = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
        xmlFree(raw_lon);
    } else {
        fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Longitude with Terrain primitive");
    }

    // ele
    xmlChar *raw_ele = xmlGetProp(xml_node, BAD_CAST "Elevation");
    if (raw_ele) {
        dc_node.terrain.lle.ele = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_ele);
        xmlFree(raw_ele);
    } else {
        fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Elevation with Terrain primitive");
    }

    // roll
    xmlChar *raw_roll = xmlGetProp(xml_node, BAD_CAST "Roll");
    if (raw_roll) {
        dc_node.terrain.rpy.roll = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_roll);
        xmlFree(raw_roll);
    } else {
        fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Roll with Terrain primitive");
    }

    // pitch
    xmlChar *raw_pitch = xmlGetProp(xml_node, BAD_CAST "Pitch");
    if (raw_pitch) {
        dc_node.terrain.rpy.pitch = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pitch);
        xmlFree(raw_pitch);
    } else {
        fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Pitch with Terrain primitive");
    }

    // yaw
    xmlChar *raw_yaw = xmlGetProp(xml_node, BAD_CAST "Yaw");
    if (raw_yaw) {
        dc_node.terrain.rpy.yaw = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_yaw);
        xmlFree(raw_yaw);
    } else {
        fprintf(stderr, "DCApp _process_xml_node(): Must supply attribute Yaw with Terrain primitive");
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children
    _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_terrain_dem(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node *parent_node = _get_node(app_data, parent_node_index);
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

static _NodeIndex _process_xml_node_text(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
        cleaned_text[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        dc_utils_trim_whitespace_inplace(cleaned_text);

        static char *sb_curr_filler = NULL;
        for (size_t ii = 0; ii < strlen(cleaned_text);) {
            if (cleaned_text[ii] == '\\') {
                // handle escape characters
                if (ii + 1 < strlen(cleaned_text)) {

                    // if a C style escape sequence, add the combined character
                    // (e.g. '\\' + 'n' => '\n')
                    char next_char = cleaned_text[ii + 1];
                    if (next_char == 'n') {
                        sbpush(sb_curr_filler, '\n');
                    } else if (next_char == 'r') {
                        sbpush(sb_curr_filler, '\r');
                    } else if (next_char == 't') {
                        sbpush(sb_curr_filler, '\t');
                    } else if (next_char == '\\') {
                        sbpush(sb_curr_filler, '\\');
                    } else if (next_char == '"') {
                        sbpush(sb_curr_filler, '"');
                    } else if (next_char == '\'') {
                        sbpush(sb_curr_filler, '\'');
                    } else if (dc_utils_char_in(next_char, "@#$%")) {
                        // push only the latter character
                        sbpush(sb_curr_filler, next_char);
                    } else {
                        // unknown escape; keep the backslash and next char
                        sbpush(sb_curr_filler, '\\');
                        sbpush(sb_curr_filler, next_char);
                    }

                    // increment to character after
                    ii += 2;
                } else {
                    sbpush(sb_curr_filler, cleaned_text[ii++]);
                }
                continue;
            }

            if (cleaned_text[ii] == '@') {
                size_t start = ii;
                ii++;

                char var[DC_VALUE_STRING_BUFFER_SIZE] = {0};

                if (ii < strlen(cleaned_text) && cleaned_text[ii] == '{') {
                    // braced variable: @{varname}
                    ii++; // skip '{'
                    int end = dc_utils_str_find_first(&(cleaned_text[ii]), '}');
                    if (end == -1) {
                        // No closing brace, treat as normal text
                        sbpushn(sb_curr_filler, &(cleaned_text[start]), (int)(ii - start));
                        continue;
                    }
                    strncpy(var, &(cleaned_text[ii]), end);
                    var[end] = '\0';
                    ii += end + 1; // skip past varname and '}'
                } else {
                    // non-braced variable: @varname
                    size_t start_var = ii;
                    while (ii < strlen(cleaned_text) && !isspace(cleaned_text[ii]) && cleaned_text[ii] != '(') {
                        ii++;
                    }
                    size_t len = ii - start_var;
                    strncpy(var, &(cleaned_text[start_var]), len);
                    var[len] = '\0';
                }

                DcAppValIndex   var_index = dc_app_lookup_get_var_index(app_data->lookup, var);
                DcAppLookupVar *var_var   = dc_app_lookup_get_var(app_data->lookup, var_index);
                sbpush(dc_node.text.sb_vals, var_var->value_index);
                // Check for format specifier
                char format_spec[DC_VALUE_STRING_BUFFER_SIZE] = {0};
                if (ii < strlen(cleaned_text) && cleaned_text[ii] == '(') {
                    int close = dc_utils_str_find_first(&(cleaned_text[ii]), ')');
                    if (close > 1) {
                        // copy content between ( and )
                        size_t len = close - 1;
                        strncpy(format_spec, &(cleaned_text[ii + 1]), len);
                        format_spec[len] = '\0';
                        ii += close + 1; // skip past '(' content and ')'

                        // get format + type
                        if (dc_utils_is_format_specifier_int(format_spec)) {
                            sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_INTEGER);
                            sbpush(dc_node.text.sb_format_indices, (uint8_t)sbcount(dc_node.text.sb_formats));
                            sbpushn(dc_node.text.sb_formats, format_spec, (int)strlen(format_spec) + 1);
                        } else if (dc_utils_is_format_specifier_double(format_spec)) {
                            sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_DOUBLE);
                            sbpush(dc_node.text.sb_format_indices, (uint8_t)sbcount(dc_node.text.sb_formats));
                            sbpushn(dc_node.text.sb_formats, format_spec, (int)strlen(format_spec) + 1);
                        } else if (dc_utils_is_format_specifier_string(format_spec)) {
                            sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                            sbpush(dc_node.text.sb_format_indices, (uint8_t)sbcount(dc_node.text.sb_formats));
                            sbpushn(dc_node.text.sb_formats, format_spec, (int)strlen(format_spec) + 1);
                        } else if (dc_utils_is_format_specifier_bool(format_spec)) {
                            sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_BOOLEAN);
                            sbpush(dc_node.text.sb_format_indices, (uint8_t)sbcount(dc_node.text.sb_formats));
                            sbpushn(dc_node.text.sb_formats, format_spec, (int)strlen(format_spec) + 1);
                        } else {
                            fprintf(stderr, "DCApp _process_xml_node(): Unknown format specifier: %s\n", format_spec);
                            sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                            sbpush(dc_node.text.sb_format_indices, (uint8_t)sbcount(dc_node.text.sb_formats));
                            sbpushn(dc_node.text.sb_formats, "%s", 3);
                        }
                    } else {
                        sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                        sbpush(dc_node.text.sb_format_indices, (uint8_t)sbcount(dc_node.text.sb_formats));
                        sbpushn(dc_node.text.sb_formats, "%s", 3);
                    }
                } else {
                    sbpush(dc_node.text.sb_format_types, DC_VALUE_TYPE_STRING);
                    sbpush(dc_node.text.sb_format_indices, (uint8_t)sbcount(dc_node.text.sb_formats));
                    sbpushn(dc_node.text.sb_formats, "%s", 3);
                }

                // add the current filler to list of fillers
                sbpush(dc_node.text.sb_filler_indices, (uint8_t)sbcount(dc_node.text.sb_fillers));
                sbpushn(dc_node.text.sb_fillers, sb_curr_filler, sbcount(sb_curr_filler));
                sbpush(dc_node.text.sb_fillers, '\0');
                sbclear(sb_curr_filler);

                continue;
            }

            // Default: append character to result
            sbpush(sb_curr_filler, cleaned_text[ii++]);
        }

        // append the remaining filler
        sbpush(dc_node.text.sb_filler_indices, (uint8_t)sbcount(dc_node.text.sb_fillers));
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
        dc_node.text.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.text.position.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.text.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.text.position.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.text.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    } else {
        dc_node.text.local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.text.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    } else {
        dc_node.text.local_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.text.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    } else {
        dc_node.text.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.text.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    } else {
        dc_node.text.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.text.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

        dc_node.text.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.text.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

        dc_node.text.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        dc_node.text.pivot_local_align.y = DC_APP_VAL_INDEX_UNDEFINED;

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {

        xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
        if (raw_pivot_align_x) {
            dc_node.text.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
            xmlFree(raw_pivot_align_x);
        } else {
            dc_node.text.pivot_local_align.x = DC_APP_VAL_INDEX_UNDEFINED;
        }

        xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
        if (raw_pivot_align_y) {
            dc_node.text.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
            xmlFree(raw_pivot_align_y);
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
        dc_node.text.size = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_size);
        xmlFree(raw_size);
    } else {
        dc_node.text.size = DC_APP_VAL_INDEX_UNDEFINED;
    }

    dc_node.text.fill_enabled = _load_color_from_string(app_data, xml_node, "FillColor", &(dc_node.text.fill_color));
    dc_node.text.line_enabled = _load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.text.line_color));

    // register node
    return _register_node(app_data, &dc_node);
}

static _NodeIndex _process_xml_node_trick_from(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_TRICK_IO: {
            _process_xml_node_children(app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
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

static _NodeIndex _process_xml_node_trick_io(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
    int      port     = 0;
    if (raw_port) {
        port = (int)dc_utils_string_to_double((const char *)raw_port);
        xmlFree(raw_port);
    } else {
        fprintf(stderr, "DCApp _process_xml_node: Missing Port for TrickIO\n");
    }

    // data rate
    xmlChar *raw_data_rate = xmlGetProp(xml_node, BAD_CAST "DataRate");
    double   data_rate     = 0.1;
    if (raw_data_rate) {
        data_rate = dc_utils_string_to_double((const char *)raw_data_rate);
        xmlFree(raw_data_rate);
    }

    // create trick instance
    _TrickContext trick_context = {};
    trick_context.trick         = dc_trick_create(host, port, (float)data_rate, 1);
    sbpush(app_data->sb_tricks, trick_context);

    // process children
    _process_xml_node_children(app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);

    // return
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_trick_to(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_TRICK_IO: {
            _process_xml_node_children(app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
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

static _NodeIndex _process_xml_node_trick_variable(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

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
        dc_utils_trim_whitespace_inplace(dcapp_var);
        dcapp_var[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
    } else {
        fprintf(stderr, "DCApp _process_xml_node: Missing dcapp Var path for TrickVariable\n");
    }

    // units
    xmlChar *raw_units = xmlGetProp(xml_node, BAD_CAST "Units");
    char     cleaned_units[DC_VALUE_STRING_BUFFER_SIZE];
    char    *units = NULL;
    if (raw_units) {
        strncpy(cleaned_units, (const char *)raw_units, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_units);
        units = cleaned_units;
    }

    // get current trick context
    _TrickContext *trick_context = &(app_data->sb_tricks[sbcount(app_data->sb_tricks) - 1]);

    // handle depending on parent
    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_TRICK_FROM: {

            // create + add rx var
            _TrickRxVarContext var = {};
            var.trick_var_index    = dc_trick_add_rx_var(trick_context->trick, trick_path, units);
            var.dcapp_var_index    = dc_app_lookup_get_var_index(app_data->lookup, dcapp_var);
            sbpush(trick_context->sb_rx_var_contexts, var);
            break;
        }
        case DC_APP_ELEM_TYPE_TRICK_TO: {

            // create + add tx var
            _TrickTxVarContext var = {};
            var.dcapp_var_index    = dc_app_lookup_get_var_index(app_data->lookup, dcapp_var);
            DcValue *dc_var_value  = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, var.dcapp_var_index)->value_index);
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

static _NodeIndex _process_xml_node_true(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_IF: {

            // process children
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);

            // update child true node
            _Node *parent_node                  = _get_node(app_data, parent_node_index);
            parent_node->conditional.child_true = first_child_index;
            break;
        }
        default:
            fprintf(stderr, "DCApp _process_xml_node(): Invalid elem parent of type %s for <True>.\n", dc_app_elem_type_to_string(parent_elem_type));
    }

    // return
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_variable(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    // name
    xmlChar *raw_name = xmlNodeGetContent(xml_node);
    char     name[DC_VALUE_STRING_BUFFER_SIZE];
    if (raw_name) {
        strncpy(name, (const char *)raw_name, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_name);
        dc_utils_trim_whitespace_inplace(name);
        name[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
    } else {
        fprintf(stderr, "DCApp _process_xml_node: Non-existent node content in <Variable> definition\n");
    }

    xmlChar    *raw_type = xmlGetProp(xml_node, BAD_CAST "Type");
    DcValueType type     = DC_VALUE_TYPE_STRING;
    if (raw_type) {
        type = dc_utils_string_to_integer((const char *)raw_type);
        xmlFree(raw_type);
    }

    xmlChar *raw_initial_value = xmlGetProp(xml_node, BAD_CAST "InitialValue");
    DcValue  initial_value;
    if (raw_initial_value) {
        initial_value = dc_value_create_value_string((const char *)raw_initial_value);
        xmlFree(raw_initial_value);
    } else {
        initial_value = dc_value_create_value_string("");
    }
    initial_value.type = type;

    // register var
    DcAppLookupVar var      = {};
    var.value_index         = dc_app_lookup_register_value(app_data->lookup, &initial_value);
    DcAppVarIndex var_index = dc_app_lookup_register_var(app_data->lookup, name, &var);

    // return
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_vertex(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_LINE:
        case NODE_TYPE_POLYGON: {

            // position
            _ValIndex2 position = {DC_APP_VAL_INDEX_UNDEFINED, DC_APP_VAL_INDEX_UNDEFINED};

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
                xmlFree(raw_x_position);
            } else {
                fprintf(stderr, "DCAPP _process_xml_node: Invalid Vertex: No X attribute\n");
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
                xmlFree(raw_y_position);
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

static _NodeIndex _process_xml_node_window(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node;
    dc_node.type   = NODE_TYPE_WINDOW;
    dc_node.parent = parent_node_index;
    dc_node.next   = NODE_INDEX_UNDEFINED;

    // title
    xmlChar *raw_title = xmlGetProp(xml_node, BAD_CAST "Title");
    if (raw_title) {
        dc_node.window.title = strdup((const char *)raw_title);
        xmlFree(raw_title);
    } else {
        dc_node.window.title = strdup("dcapp");
    }

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.window.init_position.x = (float)dc_utils_string_to_double((const char *)raw_x_position);
        xmlFree(raw_x_position);
    } else {
        dc_node.window.init_position.x = 0.0f;
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.window.init_position.y = (float)dc_utils_string_to_double((const char *)raw_y_position);
        xmlFree(raw_y_position);
    } else {
        dc_node.window.init_position.y = 0.0f;
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.window.init_dimension.x = (float)dc_utils_string_to_double((const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    } else {
        fprintf(stderr, "DCApp _process_xml_node: missing x dimension for Window\n");
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.window.init_dimension.y = (float)dc_utils_string_to_double((const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    } else {
        fprintf(stderr, "DCApp _process_xml_node: missing y dimension for Window\n");
    }

    // virtual x dimension
    xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
    if (!raw_x_virtual_dimension) {
        raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
    }
    if (raw_x_virtual_dimension) {
        dc_node.window.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    } else {
        dc_node.window.virtual_dimension.x = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // virtual y virtual_dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.window.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    } else {
        dc_node.window.virtual_dimension.y = DC_APP_VAL_INDEX_UNDEFINED;
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // init PL graphics backend
    // TODO really don't like this approach
    _init_app_data(app_data, &dc_node);

    // process children
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // update child index
    _Node *node        = _get_node(app_data, node_index);
    node->window.child = first_child_index;

    // set global window
    app_data->window = node_index;

    // return
    return node_index;
}

#include "_dcapp_utils.c"
