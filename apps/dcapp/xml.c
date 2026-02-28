#include "dcapp.h"

#include "../../src/utils/file.h"
#include "../../src/utils/log.h"
#include "../../src/utils/string.h"
#include "libxml/tree.h"
#include "libxml/xmlstring.h"

#include <ctype.h>

// Forward declarations
static DcAppVarIndex _register_anonymous_variable(_AppData *app_data, DcValueType type, const char *initial_value_str);
static _NodeIndex    _process_xml_node(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
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
static _NodeIndex    _process_xml_node_constant(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_container(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_dcapp(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_default(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_edge_from(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_edge_io(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_edge_to(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_edge_variable(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_ellipse(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
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
static _NodeIndex    _process_xml_node_nonelem(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_panel(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_pixelstream(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_planet(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_planet_data(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_planet_shader(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_planet_texture(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static void          _planet_shader_abs_to_vfs(const char *abs_path, char *vfs_out, size_t vfs_out_size);
static _NodeIndex    _process_xml_node_planet_view(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_polygon(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_rectangle(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_set(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_sphere(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_stencil(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_stencil_add(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_stencil_draw(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_stencil_remove(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_style(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_text(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_trick_from(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_trick_io(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_trick_to(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_trick_variable(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_true(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_variable(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_vertex(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static _NodeIndex    _process_xml_node_window(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);

// utils (definitions at bottom of file)
static const char *_node_type_to_string(_NodeType type);
static _NodeIndex  _register_node(_AppData *app_data, _Node *node);
static _Texture    _create_texture(_AppData *app_data, uint32_t texture_width, uint32_t texture_height, const char *texture_name);
static void        _init_stencil_pipelines(_AppData* app_data, plDevice* device, plRenderPassHandle render_pass);
static void        _init_app_data(_AppData *app_data, _Node *window_node);

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

        case DC_APP_ELEM_TYPE_CONSTANT:
            return _process_xml_node_constant(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_CONTAINER:
            return _process_xml_node_container(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_DCAPP:
            return _process_xml_node_dcapp(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_DEFAULT:
            return _process_xml_node_default(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_EDGE_FROM:
            return _process_xml_node_edge_from(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_EDGE_IO:
            return _process_xml_node_edge_io(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_EDGE_TO:
            return _process_xml_node_edge_to(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_EDGE_VARIABLE:
            return _process_xml_node_edge_variable(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_ELLIPSE:
            return _process_xml_node_ellipse(app_data, xml_node, parent_node_index, parent_elem_type, directory);

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

        case DC_APP_ELEM_TYPE_PLANET:
            return _process_xml_node_planet(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_DATA:
            return _process_xml_node_planet_data(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_TEXTURE:
            return _process_xml_node_planet_texture(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_SHADER:
            return _process_xml_node_planet_shader(app_data, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_VIEW:
            return _process_xml_node_planet_view(app_data, xml_node, parent_node_index, parent_elem_type, directory);

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

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.arc.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.arc.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.arc.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.arc.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (raw_x_align) {
        dc_node.arc.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (raw_y_align) {
        dc_node.arc.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // rotation (where the center of the arc points, 0 = top)
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.arc.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.arc.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.arc.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.arc.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.arc.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Arc", "PivotX and PivotY must both be specified, or neither");
    }

    // radius
    xmlChar *raw_radius = xmlGetProp(xml_node, BAD_CAST "Radius");
    if (raw_radius) {
        dc_node.arc.radius = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius);
        xmlFree(raw_radius);
    }

    // angle (span of the arc in degrees)
    xmlChar *raw_angle = xmlGetProp(xml_node, BAD_CAST "Angle");
    if (raw_angle) {
        dc_node.arc.angle = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_angle);
        xmlFree(raw_angle);
    } else {
        // default to 360 degrees if not specified (matches legacy Circle behavior)
        dc_node.arc.angle = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, "360");
    }

    // segments
    xmlChar *raw_segments = xmlGetProp(xml_node, BAD_CAST "Segments");
    if (raw_segments) {
        dc_node.arc.num_segments = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_segments);
        xmlFree(raw_segments);
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.arc.line_width = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // line color
    _load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.arc.line_color));

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.arc.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.arc.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    return node_index;
}

static _NodeIndex _process_xml_node_ellipse(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_ELLIPSE;
    dc_node.parent = parent_node_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.ellipse.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.ellipse.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.ellipse.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.ellipse.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.ellipse.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.ellipse.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.ellipse.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

        dc_node.ellipse.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.ellipse.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.ellipse.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.ellipse.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.ellipse.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.ellipse.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Ellipse", "PivotX and PivotY must both be specified, or neither");
    }

    // angle (span of the wedge in degrees, 360 = full ellipse)
    xmlChar *raw_angle = xmlGetProp(xml_node, BAD_CAST "Angle");
    if (raw_angle) {
        dc_node.ellipse.angle = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_angle);
        xmlFree(raw_angle);
    }

    // radius (shorthand for both RadiusX and RadiusY)
    xmlChar *raw_radius = xmlGetProp(xml_node, BAD_CAST "Radius");
    DcAppValIndex radius_val = DC_APP_VAL_INDEX_UNDEFINED;
    if (raw_radius) {
        radius_val = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius);
        xmlFree(raw_radius);
    }

    // radius x (overrides Radius if specified)
    xmlChar *raw_radius_x = xmlGetProp(xml_node, BAD_CAST "RadiusX");
    if (raw_radius_x) {
        dc_node.ellipse.radius_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius_x);
        xmlFree(raw_radius_x);
    } else {
        dc_node.ellipse.radius_x = radius_val;  // fallback to Radius
    }

    // radius y (overrides Radius if specified)
    xmlChar *raw_radius_y = xmlGetProp(xml_node, BAD_CAST "RadiusY");
    if (raw_radius_y) {
        dc_node.ellipse.radius_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius_y);
        xmlFree(raw_radius_y);
    } else {
        dc_node.ellipse.radius_y = radius_val;  // fallback to Radius
    }

    // segments
    xmlChar *raw_segments = xmlGetProp(xml_node, BAD_CAST "Segments");
    if (raw_segments) {
        dc_node.ellipse.num_segments = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_segments);
        xmlFree(raw_segments);
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.ellipse.line_width = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // colors
    dc_node.ellipse.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(app_data, xml_node, "FillColor", &(dc_node.ellipse.fill_color)))
        dc_node.ellipse.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
    if (_load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.ellipse.line_color)))
        dc_node.ellipse.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.ellipse.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.ellipse.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children (must store result first to avoid stale pointer after sb reallocation)
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);
    _get_node(app_data, node_index)->ellipse.child = first_child_index;

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

    _Node dc_node = {0};
    dc_node.type   = NODE_TYPE_BLINK;
    dc_node.parent = parent_node_index;

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

    // edge-triggered blink
    xmlChar *raw_fire_blink = xmlGetProp(xml_node, BAD_CAST "FireBlink");
    if (raw_fire_blink) {
        dc_node.blink.fire_blink = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_fire_blink);
        xmlFree(raw_fire_blink);
    }

    // initialize child

    // initialize runtime state
    dc_node.blink.remaining_duration  = 0.0;
    dc_node.blink.last_frame_time     = 0.0;
    // last_fire_blink_value is zero-initialized by _Node dc_node = {0}

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

    // initialize child and state flags
    dc_node.button.state_flags = 0;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.button.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.button.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.button.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.button.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // virtual x dimension
    xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
    if (!raw_x_virtual_dimension) {
        raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
    }
    if (raw_x_virtual_dimension) {
        dc_node.button.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    }

    // virtual y dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.button.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.button.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.button.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.button.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.button.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.button.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.button.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.button.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.button.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.button.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Button", "PivotX and PivotY must both be specified, or neither");
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
        xmlChar *raw_enabled_on   = xmlGetProp(xml_node, BAD_CAST "EnableOn");
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
        xmlChar *raw_enabled_variable   = xmlGetProp(xml_node, BAD_CAST "EnableVariable");
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
            } else if (raw_indicator_variable) {
                // IndicatorVariable is explicitly specified with no Variable/TargetVariable,
                // so skip creating an anonymous target variable. An anonymous target would
                // never match the indicator variable, causing permanent transitioning.
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
            const char *initial_value_str = dc_app_lookup_get_value(app_data->lookup, dc_node.button.val_enabled_on)->value_string;
            dc_node.button.var_enabled    = _register_anonymous_variable(app_data, DC_VALUE_TYPE_STRING, initial_value_str);
        }

        // cleanup
        xmlFree(raw_default_variable);
        xmlFree(raw_enabled_variable);
        xmlFree(raw_target_variable);
        xmlFree(raw_indicator_variable);
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.button.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.button.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children (state conditional nodes become regular children)
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);
    _Node *node = _get_node(app_data, node_index);
    node->button.child = first_child_index;

    return node_index;
}

// Helper to create a state event node
static _NodeIndex _create_state_event_node(_AppData *app_data, _NodeType node_type, _NodeIndex parent_node_index, _NodeIndex child_index) {
    _Node dc_node = {};
    dc_node.type = node_type;
    dc_node.parent = parent_node_index;
    dc_node.state_event.child = child_index;
    return _register_node(app_data, &dc_node);
}

// Helper to set HAS_MOUSE_HANDLERS flag on parent node
static void _set_parent_has_mouse_handlers(_AppData *app_data, _NodeIndex parent_node_index) {
    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_CONTAINER:
            parent_node->container.config_flags |= NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS;
            break;
        case NODE_TYPE_ELLIPSE:
            parent_node->ellipse.config_flags |= NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS;
            break;
        case NODE_TYPE_IMAGE:
            parent_node->image.config_flags |= NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS;
            break;
        case NODE_TYPE_PIXELSTREAM:
            parent_node->pixelstream.config_flags |= NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS;
            break;
        case NODE_TYPE_POLYGON:
            parent_node->polygon.config_flags |= NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS;
            break;
        case NODE_TYPE_RECTANGLE:
            parent_node->rectangle.config_flags |= NODE_CONFIG_FLAG_HAS_MOUSE_HANDLERS;
            break;
        default:
            // Button and other types don't need this flag
            break;
    }
}

static _NodeIndex _process_xml_node_button_disabled(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_BUTTON_DISABLED, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("Disabled", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_button_enabled(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_BUTTON_ENABLED, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("Enabled", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_button_indicator_off(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_BUTTON_INDICATOR_OFF, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("Off", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_button_indicator_on(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_BUTTON_INDICATOR_ON, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("On", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_button_transition(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_BUTTON_TRANSITION, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("Transition", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
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

    _Node dc_node = {0};
    dc_node.type   = NODE_TYPE_CONTAINER;
    dc_node.parent = parent_node_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.container.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.container.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.container.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.container.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // virtual x dimension
    xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
    if (!raw_x_virtual_dimension) {
        raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
    }
    if (raw_x_virtual_dimension) {
        dc_node.container.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    }

    // virtual y virtual_dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.container.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.container.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.container.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.container.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.container.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.container.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.container.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.container.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.container.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.container.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Container", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.container.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.container.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
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

static _NodeIndex _process_xml_node_edge_from(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_EDGE_IO: {
            _process_xml_node_children(app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
            break;
        }
        default: {
            DC_LOG_ERROR("EdgeFrom", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
            break;
        }
    }

    // return
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_edge_io(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    // host
    xmlChar *raw_host = xmlGetProp(xml_node, BAD_CAST "Host");
    char     host[DC_VALUE_STRING_BUFFER_SIZE];
    if (raw_host) {
        strncpy(host, (const char *)raw_host, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_host);
    } else {
        strncpy(host, "localhost", DC_VALUE_STRING_BUFFER_SIZE - 1);
    }

    // port (default 5451 for EDGE RCS)
    xmlChar *raw_port = xmlGetProp(xml_node, BAD_CAST "Port");
    int      port     = 5451;
    if (raw_port) {
        port = (int)dc_utils_string_to_double((const char *)raw_port);
        xmlFree(raw_port);
    }

    // data rate
    xmlChar *raw_data_rate = xmlGetProp(xml_node, BAD_CAST "DataRate");
    double   data_rate     = 1.0;
    if (raw_data_rate) {
        data_rate = dc_utils_string_to_double((const char *)raw_data_rate);
        xmlFree(raw_data_rate);
    }

    // connected variable (optional)
    xmlChar      *raw_connected_var   = xmlGetProp(xml_node, BAD_CAST "ConnectedVariable");
    DcAppVarIndex connected_var_index = DC_APP_VAR_INDEX_UNDEFINED;
    if (raw_connected_var) {
        connected_var_index = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_connected_var);
        xmlFree(raw_connected_var);
    }

    // create edge instance
    _EdgeContext edge_context = {};
    edge_context.edge                = dc_edge_create(host, port, (float)data_rate, 2);
    edge_context.connected_var_index = connected_var_index;
    sbpush(app_data->sb_edges, edge_context);

    // process children
    _process_xml_node_children(app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);

    // return
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_edge_to(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_EDGE_IO: {
            _process_xml_node_children(app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
            break;
        }
        default: {
            DC_LOG_ERROR("EdgeTo", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
            break;
        }
    }

    // return
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_edge_variable(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    // check for invalid elem type
    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_EDGE_FROM:
        case DC_APP_ELEM_TYPE_EDGE_TO:
            break;
        default:
            DC_LOG_ERROR("EdgeVariable", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }

    // edge command
    xmlChar *raw_edge_cmd = xmlGetProp(xml_node, BAD_CAST "Command");
    char     edge_cmd[DC_VALUE_STRING_BUFFER_SIZE];
    if (raw_edge_cmd) {
        strncpy(edge_cmd, (const char *)raw_edge_cmd, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_edge_cmd);
    } else {
        DC_LOG_ERROR("EdgeVariable", "Missing 'Command' attribute");
        edge_cmd[0] = '\0';
    }

    // dcapp var
    xmlChar *raw_dcapp_var = xmlNodeGetContent(xml_node);
    char     dcapp_var[DC_VALUE_STRING_BUFFER_SIZE];
    if (raw_dcapp_var) {
        strncpy(dcapp_var, (const char *)raw_dcapp_var, DC_VALUE_STRING_BUFFER_SIZE - 1);
        dcapp_var[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        xmlFree(raw_dcapp_var);
        dc_utils_trim_whitespace_inplace(dcapp_var);
        if (dcapp_var[0] == '\0') {
            DC_LOG_ERROR("EdgeVariable", "Empty variable name");
        }
    } else {
        DC_LOG_ERROR("EdgeVariable", "Missing variable name");
        dcapp_var[0] = '\0';
    }

    // get current edge context
    _EdgeContext *edge_context = &(app_data->sb_edges[sbcount(app_data->sb_edges) - 1]);

    // handle depending on parent
    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_EDGE_FROM: {

            // create + add rx var
            _EdgeRxVarContext var = {};
            var.edge_var_index    = dc_edge_add_rx_var(edge_context->edge, edge_cmd);
            var.dcapp_var_index   = dc_app_lookup_get_var_index(app_data->lookup, dcapp_var);
            if (var.dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) {
                DC_LOG_ERROR("EdgeVariable", "Unknown variable '%s' in EdgeFrom", dcapp_var);
            }
            sbpush(edge_context->sb_rx_var_contexts, var);
            break;
        }
        case DC_APP_ELEM_TYPE_EDGE_TO: {

            // create + add tx var
            _EdgeTxVarContext var = {};
            var.dcapp_var_index   = dc_app_lookup_get_var_index(app_data->lookup, dcapp_var);
            var.edge_var_index    = dc_edge_add_tx_var(edge_context->edge, edge_cmd);
            if (var.dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) {
                DC_LOG_ERROR("EdgeVariable", "Unknown variable '%s' in EdgeTo", dcapp_var);
            } else {
                DcValue *dc_var_value = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, var.dcapp_var_index)->value_index);
                var.prev_value        = *dc_var_value;
            }
            sbpush(edge_context->sb_tx_var_contexts, var);
            break;
        }
        default:
            // should never reach here
            DC_LOG_ERROR("EdgeVariable", "Invalid parent node");
            break;
    }

    // return
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_false(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_IF: {
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_IF_FALSE, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("False", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_function(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_FUNCTION;
    dc_node.parent = parent_node_index;

    // get function name
    xmlChar *raw_name = xmlGetProp(xml_node, BAD_CAST "Name");
    if (raw_name) {
        if (app_data->logic_lib) {
            dc_node.function.callback = (void (*)(void))_ext_library->load_function(app_data->logic_lib, (const char *)raw_name);
            if (!dc_node.function.callback) {
                DC_LOG_ERROR("Function", "Failed to load function '%s' from logic library", (const char *)raw_name);
            }
        } else {
            DC_LOG_ERROR("Function", "No logic library loaded, cannot load function '%s'", (const char *)raw_name);
        }
        xmlFree(raw_name);
    } else {
        DC_LOG_ERROR("Function", "Missing 'Name' attribute");
    }

    // edge-triggered call
    xmlChar *raw_fire_call = xmlGetProp(xml_node, BAD_CAST "FireCall");
    if (raw_fire_call) {
        dc_node.function.fire_call = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_fire_call);
        xmlFree(raw_fire_call);
    }

    // register node
    return _register_node(app_data, &dc_node);
}

static _NodeIndex _process_xml_node_if(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node                     = {};
    dc_node.type                      = NODE_TYPE_CONDITIONAL;
    dc_node.parent                    = parent_node_index;
    dc_node.conditional.state_flags   = NODE_STATE_FLAG_NONE;

    // conditional type
    xmlChar *raw_type = xmlGetProp(xml_node, BAD_CAST "Operator");
    if (raw_type) {
        dc_node.conditional.type = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_type);
        xmlFree(raw_type);
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
        DC_LOG_ERROR("If", "No value specified for conditional");
    }

    // value2
    xmlChar *raw_value2 = xmlGetProp(xml_node, BAD_CAST "Value2");
    if (raw_value2) {
        dc_node.conditional.value2 = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_STRING, (const char *)raw_value2);
        xmlFree(raw_value2);
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children (True/False become state event nodes)
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);

    // update child index
    _Node *node           = _get_node(app_data, node_index);
    node->conditional.child = first_child_index;

    return node_index;
}

static _NodeIndex _process_xml_node_image(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node = {0};
    dc_node.type   = NODE_TYPE_IMAGE;
    dc_node.parent = parent_node_index;

    // get filepath
    xmlChar *filepath = xmlGetProp(xml_node, BAD_CAST "File");
    if (!filepath || xmlStrlen(filepath) == 0) {
        filepath = xmlNodeGetContent(xml_node);
    }
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
        for (int ii = TEXTURE_FIRST_INDEX; ii < sbcount(app_data->sb_textures); ii++) {
            const char *comp_name = &(app_data->sb_texture_names[app_data->sb_texture_name_offsets[ii]]);
            if (strcmp(canon_filepath, comp_name) == 0) {
                texture_index = ii;
                break;
            }
        }

        // load new texture if it doesn't exist
        if (texture_index == TEXTURE_INDEX_UNDEFINED) {

            // get device
            plDevice *device = _ext_starter->get_device();

            // load raw data
            size_t         file_data_size;
            unsigned char *file_data = dc_utils_load_binary_file(canon_filepath, &file_data_size);
            if (!file_data) {
                DC_LOG_ERROR("Image", "Failed to load file: %s", canon_filepath);
                return _register_node(app_data, &dc_node);
            }

            // load as image data
            int            image_width, image_height, channels;
            unsigned char *image_data = _ext_image->load(file_data, (int)file_data_size, &image_width, &image_height, &channels, 4);
            free(file_data);
            if (!image_data) {
                DC_LOG_ERROR("Image", "Failed to decode file: %s", canon_filepath);
                return _register_node(app_data, &dc_node);
            }

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

        dc_node.image.texture_index = texture_index;
    } else {
        DC_LOG_ERROR("Image", "Missing 'File' attribute");
    }

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.image.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.image.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.image.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.image.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.image.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.image.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.image.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.image.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.image.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.image.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.image.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.image.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.image.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Image", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.image.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.image.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children (must store result first to avoid stale pointer after sb reallocation)
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);
    _get_node(app_data, node_index)->image.child = first_child_index;

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_line(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_LINE;
    dc_node.parent = parent_node_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.line.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.line.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.line.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        if (raw_pivot_parent_align_x) {
            dc_node.line.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
            xmlFree(raw_pivot_parent_align_x);
        }
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_y) {
            dc_node.line.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
            xmlFree(raw_pivot_parent_align_y);
        }
    } else if (raw_pivot_position_x || raw_pivot_position_y) {
        DC_LOG_ERROR("Line", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.line.line_width = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // colors
    dc_node.line.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.line.line_color)))
        dc_node.line.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.line.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.line.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

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
        DC_LOG_ERROR("Logic", "Duplicate <Logic> definitions");
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
            if (!dc_utils_file_exists(try_filepath)) continue;
            const plLibraryDesc logic_so_desc = {
                .tFlags = PL_LIBRARY_FLAGS_NONE,
                .pcName = try_filepath,
            };
            if (_ext_library->load(logic_so_desc, &app_data->logic_lib) == PL_LIBRARY_RESULT_SUCCESS) {
                loaded = true;
            }
        }

        if (!loaded) {
            DC_LOG_ERROR("Logic", "Failed to load library: %s.so/.dylib/.dll", base_filepath);
        } else {
            // load functions (only if library loaded successfully)
            app_data->logic_pre_init = (void (*)(_GetVariableValueAddr))_ext_library->load_function(app_data->logic_lib, "display_pre_init");
            app_data->logic_init     = (void (*)(void))_ext_library->load_function(app_data->logic_lib, "display_init");
            app_data->logic_draw     = (void (*)(void))_ext_library->load_function(app_data->logic_lib, "display_draw");
            app_data->logic_close    = (void (*)(void))_ext_library->load_function(app_data->logic_lib, "display_close");
        }
    } else {
        DC_LOG_ERROR("Logic", "Missing 'File' attribute");
    }

    // return
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_mouse_active(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
        case NODE_TYPE_CONTAINER:
        case NODE_TYPE_ELLIPSE:
        case NODE_TYPE_IMAGE:
        case NODE_TYPE_PIXELSTREAM:
        case NODE_TYPE_POLYGON:
        case NODE_TYPE_RECTANGLE: {
            _set_parent_has_mouse_handlers(app_data, parent_node_index);
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_MOUSE_ACTIVE, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("MouseActive", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_mouse_hovered(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
        case NODE_TYPE_CONTAINER:
        case NODE_TYPE_ELLIPSE:
        case NODE_TYPE_IMAGE:
        case NODE_TYPE_PIXELSTREAM:
        case NODE_TYPE_POLYGON:
        case NODE_TYPE_RECTANGLE: {
            _set_parent_has_mouse_handlers(app_data, parent_node_index);
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_MOUSE_HOVERED, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("MouseHovered", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_mouse_inactive(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
        case NODE_TYPE_CONTAINER:
        case NODE_TYPE_ELLIPSE:
        case NODE_TYPE_IMAGE:
        case NODE_TYPE_PIXELSTREAM:
        case NODE_TYPE_POLYGON:
        case NODE_TYPE_RECTANGLE: {
            _set_parent_has_mouse_handlers(app_data, parent_node_index);
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_MOUSE_INACTIVE, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("MouseInactive", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_mouse_motion(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_MOUSE_MOTION;
    dc_node.parent = parent_node_index;

    // VariableX
    xmlChar *raw_var_x = xmlGetProp(xml_node, BAD_CAST "VariableX");
    if (raw_var_x) {
        dc_node.mouse_motion.var_x = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_var_x);
        xmlFree(raw_var_x);
    }

    // VariableY
    xmlChar *raw_var_y = xmlGetProp(xml_node, BAD_CAST "VariableY");
    if (raw_var_y) {
        dc_node.mouse_motion.var_y = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_var_y);
        xmlFree(raw_var_y);
    }

    return _register_node(app_data, &dc_node);
}

static _NodeIndex _process_xml_node_mouse_pressed(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
        case NODE_TYPE_CONTAINER:
        case NODE_TYPE_ELLIPSE:
        case NODE_TYPE_IMAGE:
        case NODE_TYPE_PIXELSTREAM:
        case NODE_TYPE_POLYGON:
        case NODE_TYPE_RECTANGLE: {
            _set_parent_has_mouse_handlers(app_data, parent_node_index);
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_MOUSE_PRESSED, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("MousePressed", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_mouse_released(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    _Node *parent_node = _get_node(app_data, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
        case NODE_TYPE_CONTAINER:
        case NODE_TYPE_ELLIPSE:
        case NODE_TYPE_IMAGE:
        case NODE_TYPE_PIXELSTREAM:
        case NODE_TYPE_POLYGON:
        case NODE_TYPE_RECTANGLE: {
            _set_parent_has_mouse_handlers(app_data, parent_node_index);
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, parent_elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_MOUSE_RELEASED, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("MouseReleased", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_panel(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_PANEL;
    dc_node.parent = parent_node_index;

    // virtual x dimension
    xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
    if (!raw_x_virtual_dimension) {
        raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
    }
    if (raw_x_virtual_dimension) {
        dc_node.panel.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    }

    // virtual y virtual_dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.panel.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    }

    // display index (matched against Window's ActiveDisplay)
    xmlChar *raw_display_index = xmlGetProp(xml_node, BAD_CAST "DisplayIndex");
    if (raw_display_index) {
        dc_node.panel.index = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_display_index);
        xmlFree(raw_display_index);
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

    _Node dc_node = {0};
    dc_node.type   = NODE_TYPE_PIXELSTREAM;
    dc_node.parent = parent_node_index;

    dc_node.pixelstream.frame = NULL;

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
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.pixelstream.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.pixelstream.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.pixelstream.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.pixelstream.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.pixelstream.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.pixelstream.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.pixelstream.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.pixelstream.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.pixelstream.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.pixelstream.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.pixelstream.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.pixelstream.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("PixelStream", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
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
        DC_LOG_ERROR("PixelStream", "Missing 'Type' attribute");
    }

    // iterate over type
    switch (dc_node.pixelstream.type) {

        case DC_APP_PIXELSTREAM_TYPE_SHMEM: {

            // parse filepath (also used as shmem key via ftok)
            xmlChar *raw_filepath = xmlGetProp(xml_node, BAD_CAST "File");
            if (!raw_filepath) {
                raw_filepath = xmlGetProp(xml_node, BAD_CAST "URL");  // fallback
            }
            char filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
            if (raw_filepath) {
                strncpy(filepath, (const char *)raw_filepath, DC_UTILS_FILEPATH_BUFFER_SIZE);
                filepath[DC_UTILS_FILEPATH_BUFFER_SIZE - 1] = '\0';
                xmlFree(raw_filepath);
            } else {
                DC_LOG_ERROR("PixelStream", "Missing 'File' attribute for shmem type");
                filepath[0] = '\0';
            }

            // set shmem field
            dc_node.pixelstream.shmem.handle = dc_ps_shmem_add_source(filepath);
            break;
        }

        case DC_APP_PIXELSTREAM_TYPE_MJPEG: {

            // parse URL
            xmlChar *raw_url = xmlGetProp(xml_node, BAD_CAST "URL");
            char     cleaned_url[2048];
            if (raw_url) {
                strncpy(cleaned_url, (const char *)raw_url, 2048);
                xmlFree(raw_url);
            } else {
                DC_LOG_ERROR("PixelStream", "Missing 'URL' attribute");
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
            DC_LOG_ERROR("PixelStream", "Unknown pixelstream type");
            break;
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.pixelstream.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.pixelstream.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // test pattern
    {
        // get filepath (default to assets/testpattern.png)
        xmlChar *raw_test_pattern = xmlGetProp(xml_node, BAD_CAST "TestPattern");
        char     test_pattern_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
        if (raw_test_pattern) {
            if (dc_utils_is_relative_path((const char *)raw_test_pattern)) {
                char abs_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
                dc_utils_join_paths(directory, (const char *)raw_test_pattern, abs_filepath, sizeof(abs_filepath));
                dc_utils_canonicalize_path(abs_filepath, test_pattern_path, sizeof(test_pattern_path));
            } else {
                dc_utils_canonicalize_path((const char *)raw_test_pattern, test_pattern_path, sizeof(test_pattern_path));
            }
            xmlFree(raw_test_pattern);
        } else {
            snprintf(test_pattern_path, sizeof(test_pattern_path), "%s/assets/testpattern.png", app_data->config->dcapp_dir_path);
        }

        // canonicalize path
        char canon_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
        if (dc_utils_canonicalize_path(test_pattern_path, canon_filepath, DC_UTILS_FILEPATH_BUFFER_SIZE) != 0) {
            DC_LOG_ERROR("PixelStream", "TestPattern file not found: %s", test_pattern_path);
        } else {
            // check for existing texture
            _TextureIndex texture_index = TEXTURE_INDEX_UNDEFINED;
            for (int ii = TEXTURE_FIRST_INDEX; ii < sbcount(app_data->sb_textures); ii++) {
                const char *comp_name = &(app_data->sb_texture_names[app_data->sb_texture_name_offsets[ii]]);
                if (strcmp(canon_filepath, comp_name) == 0) {
                    texture_index = ii;
                    break;
                }
            }

            // load new texture if it doesn't exist
            if (texture_index == TEXTURE_INDEX_UNDEFINED) {
                plDevice *device = _ext_starter->get_device();

                size_t         file_data_size;
                unsigned char *file_data = dc_utils_load_binary_file(canon_filepath, &file_data_size);
                if (!file_data) {
                    DC_LOG_ERROR("PixelStream", "Failed to load test pattern file: %s", canon_filepath);
                    goto skip_test_pattern;
                }

                int            image_width, image_height, channels;
                unsigned char *image_data = _ext_image->load(file_data, (int)file_data_size, &image_width, &image_height, &channels, 4);
                free(file_data);
                if (!image_data) {
                    DC_LOG_ERROR("PixelStream", "Failed to decode test pattern: %s", canon_filepath);
                    goto skip_test_pattern;
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
            skip_test_pattern:

            dc_node.pixelstream.test_pattern_texture_index = texture_index;
        }
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children (must store result first to avoid stale pointer after sb reallocation)
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);
    _get_node(app_data, node_index)->pixelstream.child = first_child_index;

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_polygon(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_POLYGON;
    dc_node.parent = parent_node_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.polygon.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.polygon.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.polygon.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.polygon.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.polygon.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        if (raw_pivot_parent_align_x) {
            dc_node.polygon.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
            xmlFree(raw_pivot_parent_align_x);
        }
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_y) {
            dc_node.polygon.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
            xmlFree(raw_pivot_parent_align_y);
        }
    } else if (raw_pivot_position_x || raw_pivot_position_y) {
        DC_LOG_ERROR("Polygon", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.polygon.line_width = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // colors
    dc_node.polygon.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(app_data, xml_node, "FillColor", &(dc_node.polygon.fill_color)))
        dc_node.polygon.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
    if (_load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.polygon.line_color)))
        dc_node.polygon.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.polygon.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.polygon.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children (must store result first to avoid stale pointer after sb reallocation)
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);
    _get_node(app_data, node_index)->polygon.child = first_child_index;

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_rectangle(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_RECTANGLE;
    dc_node.parent = parent_node_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.rectangle.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.rectangle.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.rectangle.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.rectangle.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.rectangle.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.rectangle.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.rectangle.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.rectangle.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.rectangle.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.rectangle.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.rectangle.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.rectangle.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.rectangle.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Rectangle", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.rectangle.line_width = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // colors
    dc_node.rectangle.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(app_data, xml_node, "FillColor", &(dc_node.rectangle.fill_color)))
        dc_node.rectangle.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
    if (_load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.rectangle.line_color)))
        dc_node.rectangle.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.rectangle.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.rectangle.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // process children (must store result first to avoid stale pointer after sb reallocation)
    _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, node_index, elem_type, directory);
    _get_node(app_data, node_index)->rectangle.child = first_child_index;

    // return
    return node_index;
}

static _NodeIndex _process_xml_node_set(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_SET;
    dc_node.parent = parent_node_index;

    // variable
    xmlChar *raw_variable = xmlGetProp(xml_node, BAD_CAST "Variable");
    if (raw_variable) {
        dc_node.set.var_index = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_variable);
        xmlFree(raw_variable);
    } else {
        DC_LOG_ERROR("Set", "Missing 'Variable' attribute");
    }

    // operand
    xmlChar *raw_operand = xmlNodeGetContent(xml_node);
    if (raw_operand) {
        char operand[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(operand, (const char *)raw_operand, DC_VALUE_STRING_BUFFER_SIZE - 1);
        operand[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        xmlFree(raw_operand);
        dc_utils_trim_whitespace_inplace(operand);
        if (operand[0] == '\0') {
        } else {
            dc_node.set.operand = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_STRING, operand);
        }
    } else {
        DC_LOG_WARN("Set", "Missing node content, skipping");
    }

    // operator
    xmlChar *raw_operator = xmlGetProp(xml_node, BAD_CAST "Operator");
    if (raw_operator) {
        dc_node.set.operation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_operator);
        xmlFree(raw_operator);
    }

    // defer flag
    xmlChar *raw_defer = xmlGetProp(xml_node, BAD_CAST "Defer");
    if (raw_defer) {
        dc_node.set.deferred = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_defer);
        xmlFree(raw_defer);
    }

    // register node
    return _register_node(app_data, &dc_node);
}

static _NodeIndex _process_xml_node_sphere(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_SPHERE;
    dc_node.parent = parent_node_index;

    // default texture to undefined

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.sphere.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.sphere.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.sphere.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.sphere.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.sphere.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.sphere.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // pivot parent x align
    xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
    if (raw_pivot_parent_align_x) {
        dc_node.sphere.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
        xmlFree(raw_pivot_parent_align_x);
    }

    // pivot parent y align
    xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
    if (raw_pivot_parent_align_y) {
        dc_node.sphere.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
        xmlFree(raw_pivot_parent_align_y);
    }

    // pivot local x align
    xmlChar *raw_pivot_local_x_align = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
    if (raw_pivot_local_x_align) {
        dc_node.sphere.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_local_x_align);
        xmlFree(raw_pivot_local_x_align);
    }

    // pivot local y align
    xmlChar *raw_pivot_local_y_align = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
    if (raw_pivot_local_y_align) {
        dc_node.sphere.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_local_y_align);
        xmlFree(raw_pivot_local_y_align);
    }

    // pivot x position
    xmlChar *raw_pivot_x_position = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
    if (raw_pivot_x_position) {
        dc_node.sphere.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_x_position);
        xmlFree(raw_pivot_x_position);
    }

    // pivot y position
    xmlChar *raw_pivot_y_position = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
    if (raw_pivot_y_position) {
        dc_node.sphere.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_y_position);
        xmlFree(raw_pivot_y_position);
    }

    // rotation (external 2D rotation)
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (raw_rotation) {
        dc_node.sphere.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
    }

    // radius
    xmlChar *raw_radius = xmlGetProp(xml_node, BAD_CAST "Radius");
    if (raw_radius) {
        dc_node.sphere.radius = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius);
        xmlFree(raw_radius);
    }

    // fill color
    _load_color_from_string(app_data, xml_node, "FillColor", &dc_node.sphere.fill_color);

    // internal rotation (roll, pitch, yaw)
    xmlChar *raw_roll = xmlGetProp(xml_node, BAD_CAST "Roll");
    if (raw_roll) {
        dc_node.sphere.rpy.roll = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_roll);
        xmlFree(raw_roll);
    }

    xmlChar *raw_pitch = xmlGetProp(xml_node, BAD_CAST "Pitch");
    if (raw_pitch) {
        dc_node.sphere.rpy.pitch = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pitch);
        xmlFree(raw_pitch);
    }

    xmlChar *raw_yaw = xmlGetProp(xml_node, BAD_CAST "Yaw");
    if (raw_yaw) {
        dc_node.sphere.rpy.yaw = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_yaw);
        xmlFree(raw_yaw);
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
        for (int ii = TEXTURE_FIRST_INDEX; ii < sbcount(app_data->sb_textures); ii++) {
            const char *comp_name = &(app_data->sb_texture_names[app_data->sb_texture_name_offsets[ii]]);
            if (strcmp(canon_filepath, comp_name) == 0) {
                texture_index = ii;
                break;
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
                DC_LOG_ERROR("Sphere", "Failed to load image: %s", canon_filepath);
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

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.sphere.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.sphere.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    return _register_node(app_data, &dc_node);
}

static _NodeIndex _process_xml_node_stencil(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node               = {};
    dc_node.type                = NODE_TYPE_STENCIL;
    dc_node.parent              = parent_node_index;
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
            DC_LOG_ERROR("StencilAdd", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
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
            DC_LOG_ERROR("StencilDraw", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
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
            DC_LOG_ERROR("StencilRemove", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_style(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    // ignore at this point
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_planet(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);
    (void)parent_node_index;
    (void)parent_elem_type;

    // planet definition (top-level resource, no drawable node)
    _PlanetDef def = {0};

    // Name
    xmlChar *raw_name = xmlGetProp(xml_node, BAD_CAST "Name");
    def.name = strdup((const char *)raw_name);
    xmlFree(raw_name);

    // ShaderIndex (optional — selects active PlanetShader by index at runtime)
    def.shader_index = DC_APP_VAL_INDEX_UNDEFINED;
    xmlChar *raw_shader_index = xmlGetProp(xml_node, BAD_CAST "ShaderIndex");
    if (raw_shader_index) {
        def.shader_index = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_shader_index);
        xmlFree(raw_shader_index);
    }

    // collect definition
    sbpush(app_data->sb_planet_defs, def);

    // process children (PlanetData, PlanetTexture, PlanetShader store into this def)
    _process_xml_node_children(app_data, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);

    // no drawable node — return undefined
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_planet_data(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    (void)parent_node_index;

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET) {
        DC_LOG_ERROR("PlanetData", "Invalid parent element type: %s", dc_app_elem_type_to_string(parent_elem_type));
        return NODE_INDEX_UNDEFINED;
    }

    _PlanetDef *def = &app_data->sb_planet_defs[sbcount(app_data->sb_planet_defs) - 1];

    // file
    xmlChar *raw_filepath = xmlGetProp(xml_node, BAD_CAST "File");
    if (raw_filepath) {

        // clean filepath
        char cleaned_filepath[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned_filepath, (const char *)raw_filepath, DC_VALUE_STRING_BUFFER_SIZE - 1);
        cleaned_filepath[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        xmlFree(raw_filepath);

        // convert to absolute path
        char abs_filepath[DC_VALUE_STRING_BUFFER_SIZE];
        if (dc_utils_is_relative_path(cleaned_filepath)) {
            dc_utils_join_paths(directory, cleaned_filepath, abs_filepath, sizeof(abs_filepath));
        } else {
            strcpy(abs_filepath, cleaned_filepath);
        }

        sbpush(def->sb_data_files, strdup(abs_filepath));
    } else {
        DC_LOG_ERROR("PlanetData", "Missing 'File' attribute");
    }

    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_planet_texture(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    (void)parent_node_index;

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET) {
        DC_LOG_ERROR("PlanetTexture", "Invalid parent element type: %s", dc_app_elem_type_to_string(parent_elem_type));
        return NODE_INDEX_UNDEFINED;
    }

    _PlanetDef *def = &app_data->sb_planet_defs[sbcount(app_data->sb_planet_defs) - 1];

    if (sbcount(def->sb_textures) >= 1) {
        DC_LOG_ERROR("PlanetTexture", "Only one <PlanetTexture> per <Planet> is currently supported (line %ld)", xmlGetLineNo(xml_node));
        return NODE_INDEX_UNDEFINED;
    }

    _PlanetTextureEntry entry = {0};

    // file path
    xmlChar *raw_file = xmlGetProp(xml_node, BAD_CAST "File");
    if (raw_file) {
        char cleaned[DC_UTILS_FILEPATH_BUFFER_SIZE];
        strncpy(cleaned, (const char *)raw_file, DC_UTILS_FILEPATH_BUFFER_SIZE - 1);
        cleaned[DC_UTILS_FILEPATH_BUFFER_SIZE - 1] = '\0';
        xmlFree(raw_file);

        char abs_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
        if (dc_utils_is_relative_path(cleaned)) {
            dc_utils_join_paths(directory, cleaned, abs_path, sizeof(abs_path));
        } else {
            strcpy(abs_path, cleaned);
        }
        char vfs_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
        _planet_shader_abs_to_vfs(abs_path, vfs_path, sizeof(vfs_path));
        entry.source = strdup(vfs_path);
    }

    // meters per pixel
    xmlChar *raw_mpp = xmlGetProp(xml_node, BAD_CAST "MetersPerPixel");
    if (raw_mpp) {
        entry.mpp = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_mpp);
        xmlFree(raw_mpp);
    }

    // latitude
    xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "Latitude");
    if (raw_lat) {
        entry.lat = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
        xmlFree(raw_lat);
    }

    // longitude
    xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
    if (raw_lon) {
        entry.lon = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
        xmlFree(raw_lon);
    }

    // edge-triggered refresh
    xmlChar *raw_fire_refresh = xmlGetProp(xml_node, BAD_CAST "FireRefresh");
    if (raw_fire_refresh) {
        entry.fire_refresh = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_fire_refresh);
        xmlFree(raw_fire_refresh);
    }

    sbpush(def->sb_textures, entry);

    return NODE_INDEX_UNDEFINED;
}

// Mounts the directory containing abs_path under a stable VFS mount point
// (derived by hashing the directory), then writes the VFS path into vfs_out.
static void _planet_shader_abs_to_vfs(const char *abs_path, char *vfs_out, size_t vfs_out_size) {
    char dir[DC_VALUE_STRING_BUFFER_SIZE];
    dc_utils_get_directory(abs_path, dir, sizeof(dir));

    char hash[32];
    dc_utils_string_to_hash(dir, hash, sizeof(hash));

    char vfs_mount[32];
    snprintf(vfs_mount, sizeof(vfs_mount), "/%s", hash);

    // idempotent — VFS silently ignores duplicate mounts to the same virtual path
    _ext_vfs->mount_directory(vfs_mount, dir, PL_VFS_MOUNT_FLAGS_NONE);

    // extract filename (handle both / and \ separators)
    const char *fslash = strrchr(abs_path, '/');
    const char *bslash = strrchr(abs_path, '\\');
    const char *filename = (fslash > bslash) ? fslash + 1 : (bslash ? bslash + 1 : abs_path);

    snprintf(vfs_out, vfs_out_size, "%s/%s", vfs_mount, filename);
}

static _NodeIndex _process_xml_node_planet_shader(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    (void)parent_node_index;

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET) {
        DC_LOG_ERROR("PlanetShader", "Invalid parent element type: %s", dc_app_elem_type_to_string(parent_elem_type));
        return NODE_INDEX_UNDEFINED;
    }

    _PlanetDef *def = &app_data->sb_planet_defs[sbcount(app_data->sb_planet_defs) - 1];

    // Index (required)
    xmlChar *raw_index = xmlGetProp(xml_node, BAD_CAST "Index");
    if (!raw_index) {
        DC_LOG_ERROR("PlanetShader", "Missing required 'Index' attribute");
        return NODE_INDEX_UNDEFINED;
    }
    _PlanetShaderEntry entry = {0};
    entry.index = atoi((const char *)raw_index);
    xmlFree(raw_index);

    // VertexSource (optional)
    xmlChar *raw_vert = xmlGetProp(xml_node, BAD_CAST "VertexSource");
    if (raw_vert) {
        char cleaned[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned, (const char *)raw_vert, DC_VALUE_STRING_BUFFER_SIZE - 1);
        cleaned[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        xmlFree(raw_vert);
        char abs_path[DC_VALUE_STRING_BUFFER_SIZE];
        if (dc_utils_is_relative_path(cleaned)) {
            dc_utils_join_paths(directory, cleaned, abs_path, sizeof(abs_path));
        } else {
            strncpy(abs_path, cleaned, sizeof(abs_path) - 1);
        }
        char vfs_path[DC_VALUE_STRING_BUFFER_SIZE];
        _planet_shader_abs_to_vfs(abs_path, vfs_path, sizeof(vfs_path));
        entry.vertex_path = strdup(vfs_path);
    }

    // FragmentSource (optional)
    xmlChar *raw_frag = xmlGetProp(xml_node, BAD_CAST "FragmentSource");
    if (raw_frag) {
        char cleaned[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned, (const char *)raw_frag, DC_VALUE_STRING_BUFFER_SIZE - 1);
        cleaned[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        xmlFree(raw_frag);
        char abs_path[DC_VALUE_STRING_BUFFER_SIZE];
        if (dc_utils_is_relative_path(cleaned)) {
            dc_utils_join_paths(directory, cleaned, abs_path, sizeof(abs_path));
        } else {
            strncpy(abs_path, cleaned, sizeof(abs_path) - 1);
        }
        char vfs_path[DC_VALUE_STRING_BUFFER_SIZE];
        _planet_shader_abs_to_vfs(abs_path, vfs_path, sizeof(vfs_path));
        entry.fragment_path = strdup(vfs_path);
    }

    sbpush(def->sb_shaders, entry);

    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_planet_view(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);
    (void)elem_type;
    (void)parent_elem_type;
    (void)directory;

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_PLANET_VIEW;
    dc_node.parent = parent_node_index;

    // Planet reference (required) — resolve def index at parse time
    xmlChar *raw_planet = xmlGetProp(xml_node, BAD_CAST "Planet");
    dc_node.planet_view.planet_def_index = UINT8_MAX;
    if (raw_planet) {
        int def_count = sbcount(app_data->sb_planet_defs);
        for (int j = 0; j < def_count; j++) {
            if (app_data->sb_planet_defs[j].name && strcmp(app_data->sb_planet_defs[j].name, (const char *)raw_planet) == 0) {
                dc_node.planet_view.planet_def_index = (uint8_t)j;
                break;
            }
        }
        if (dc_node.planet_view.planet_def_index == UINT8_MAX) {
            DC_LOG_ERROR("PlanetView", "Planet '%s' not found", (const char *)raw_planet);
        }
        xmlFree(raw_planet);
    }

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.planet_view.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.planet_view.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.planet_view.dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.planet_view.dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.planet_view.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.planet_view.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.planet_view.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.planet_view.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.planet_view.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

        dc_node.planet_view.pivot_position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.planet_view.pivot_position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.planet_view.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.planet_view.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.planet_view.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.planet_view.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("PlanetView", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // LLE camera mode (orthogonal to surface)
    xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "CameraLatitude");
    xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "CameraLongitude");
    xmlChar *raw_ele = xmlGetProp(xml_node, BAD_CAST "CameraElevation");
    bool has_lle = raw_lat || raw_lon || raw_ele;

    if (has_lle) {
        if (raw_lat && raw_lon && raw_ele) {
            dc_node.planet_view.lle.lat = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
            dc_node.planet_view.lle.lon = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
            dc_node.planet_view.lle.ele = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_ele);
        } else {
            DC_LOG_ERROR("PlanetView", "Incomplete LLE: must specify all of CameraLatitude, CameraLongitude, and CameraElevation");
        }
    }
    if (raw_lat) xmlFree(raw_lat);
    if (raw_lon) xmlFree(raw_lon);
    if (raw_ele) xmlFree(raw_ele);

    // heading (LLE mode: azimuth from north, CW, degrees)
    xmlChar *raw_heading = xmlGetProp(xml_node, BAD_CAST "CameraHeading");
    if (raw_heading) {
        dc_node.planet_view.heading = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_heading);
        xmlFree(raw_heading);
    }

    // XYZ/RPY camera mode (raw world coordinates)
    xmlChar *raw_cam_x = xmlGetProp(xml_node, BAD_CAST "CameraX");
    xmlChar *raw_cam_y = xmlGetProp(xml_node, BAD_CAST "CameraY");
    xmlChar *raw_cam_z = xmlGetProp(xml_node, BAD_CAST "CameraZ");
    xmlChar *raw_roll  = xmlGetProp(xml_node, BAD_CAST "CameraRoll");
    xmlChar *raw_pitch = xmlGetProp(xml_node, BAD_CAST "CameraPitch");
    xmlChar *raw_yaw   = xmlGetProp(xml_node, BAD_CAST "CameraYaw");
    bool has_xyz = raw_cam_x || raw_cam_y || raw_cam_z || raw_roll || raw_pitch || raw_yaw;

    if (has_lle && has_xyz) {
        DC_LOG_WARN("PlanetView", "Both LLE and XYZ/RPY specified; using XYZ/RPY");
    }

    if (has_xyz) {
        if (raw_cam_x && raw_cam_y && raw_cam_z && raw_roll && raw_pitch && raw_yaw) {
            dc_node.planet_view.xyz.x     = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_cam_x);
            dc_node.planet_view.xyz.y     = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_cam_y);
            dc_node.planet_view.xyz.z     = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_cam_z);
            dc_node.planet_view.rpy.roll  = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_roll);
            dc_node.planet_view.rpy.pitch = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_pitch);
            dc_node.planet_view.rpy.yaw   = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_yaw);
        } else {
            DC_LOG_ERROR("PlanetView", "Incomplete XYZ/RPY: must specify all of CameraX, CameraY, CameraZ, CameraRoll, CameraPitch, and CameraYaw");
        }
    }
    if (raw_cam_x) xmlFree(raw_cam_x);
    if (raw_cam_y) xmlFree(raw_cam_y);
    if (raw_cam_z) xmlFree(raw_cam_z);
    if (raw_roll)  xmlFree(raw_roll);
    if (raw_pitch) xmlFree(raw_pitch);
    if (raw_yaw)   xmlFree(raw_yaw);

    if (!has_lle && !has_xyz) {
        DC_LOG_ERROR("PlanetView", "Must specify either LLE (CameraLatitude/CameraLongitude/CameraElevation) or XYZ/RPY (CameraX/CameraY/CameraZ/CameraRoll/CameraPitch/CameraYaw)");
    }

    // orthographic projection
    xmlChar *raw_ortho = xmlGetProp(xml_node, BAD_CAST "CameraOrthographic");
    if (raw_ortho) {
        dc_node.planet_view.orthographic = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_ortho);
        xmlFree(raw_ortho);
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.planet_view.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.planet_view.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    _NodeIndex node_index = _register_node(app_data, &dc_node);

    // collect planet view node index for post-tree initialization
    sbpush(app_data->sb_planet_view_node_indices, node_index);

    // leaf element — no children
    return node_index;
}

static _NodeIndex _process_xml_node_text(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node  = {};
    dc_node.type   = NODE_TYPE_TEXT;
    dc_node.parent = parent_node_index;

    // text
    xmlChar *raw_text = xmlNodeGetContent(xml_node);
    if (raw_text) {
        char cleaned_text[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned_text, (const char *)raw_text, DC_VALUE_STRING_BUFFER_SIZE - 1);
        cleaned_text[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        xmlFree(raw_text);
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

                DcAppVarIndex   var_index = dc_app_lookup_get_var_index(app_data->lookup, var);
                DcAppLookupVar *var_var   = dc_app_lookup_get_var(app_data->lookup, var_index);
                if (var_var) {
                    sbpush(dc_node.text.sb_vals, var_var->value_index);
                } else {
                    DC_LOG_ERROR("Text", "Unknown variable '%s'", var);
                    sbpush(dc_node.text.sb_vals, DC_APP_VAL_INDEX_UNDEFINED);
                }
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
                            DC_LOG_ERROR("Text", "Unknown format specifier: %s", format_spec);
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
        DC_LOG_ERROR("Text", "Missing node content");
    }

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.text.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.text.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.text.local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.text.local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.text.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.text.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.text.rotation = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
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

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.text.pivot_parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.text.pivot_parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.text.pivot_local_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.text.pivot_local_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Text", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // size
    xmlChar *raw_size = xmlGetProp(xml_node, BAD_CAST "Size");
    if (raw_size) {
        dc_node.text.size = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_size);
        xmlFree(raw_size);
    }

    dc_node.text.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(app_data, xml_node, "FillColor", &(dc_node.text.fill_color)))
        dc_node.text.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
    if (_load_color_from_string(app_data, xml_node, "LineColor", &(dc_node.text.line_color)))
        dc_node.text.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    // shadow offset
    xmlChar *raw_shadow_offset = xmlGetProp(xml_node, BAD_CAST "ShadowOffset");
    if (raw_shadow_offset) {
        dc_node.text.shadow_offset = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_shadow_offset);
        xmlFree(raw_shadow_offset);
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.text.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.text.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // log
    xmlChar *raw_log = xmlGetProp(xml_node, BAD_CAST "Log");
    if (raw_log) {
        dc_node.text.log = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_STRING, (const char *)raw_log);
        xmlFree(raw_log);
    }

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
            DC_LOG_ERROR("TrickFrom", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
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
        DC_LOG_ERROR("TrickIO", "Missing 'Port' attribute");
    }

    // port
    xmlChar *raw_port = xmlGetProp(xml_node, BAD_CAST "Port");
    int      port     = 0;
    if (raw_port) {
        port = (int)dc_utils_string_to_double((const char *)raw_port);
        xmlFree(raw_port);
    } else {
        DC_LOG_ERROR("TrickIO", "Missing 'Port' attribute");
    }

    // data rate
    xmlChar *raw_data_rate = xmlGetProp(xml_node, BAD_CAST "DataRate");
    double   data_rate     = 0.1;
    if (raw_data_rate) {
        data_rate = dc_utils_string_to_double((const char *)raw_data_rate);
        xmlFree(raw_data_rate);
    }

    // connected variable (optional)
    xmlChar      *raw_connected_var     = xmlGetProp(xml_node, BAD_CAST "ConnectedVariable");
    DcAppVarIndex connected_var_index   = DC_APP_VAR_INDEX_UNDEFINED;
    if (raw_connected_var) {
        connected_var_index = dc_app_lookup_get_var_index(app_data->lookup, (const char *)raw_connected_var);
        xmlFree(raw_connected_var);
    }

    // create trick instance
    _TrickContext trick_context = {};
    trick_context.trick               = dc_trick_create(host, port, (float)data_rate, 1);
    trick_context.connected_var_index = connected_var_index;
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
            DC_LOG_ERROR("TrickTo", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
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
            DC_LOG_ERROR("TrickVariable", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }

    // var path
    xmlChar *raw_trick_path = xmlGetProp(xml_node, BAD_CAST "Name");
    char     trick_path[DC_VALUE_STRING_BUFFER_SIZE];
    if (raw_trick_path) {
        strncpy(trick_path, (const char *)raw_trick_path, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_trick_path);
    } else {
        DC_LOG_ERROR("TrickVariable", "Missing trick variable path");
    }

    // dcapp var
    xmlChar *raw_dcapp_var = xmlNodeGetContent(xml_node);
    char     dcapp_var[DC_VALUE_STRING_BUFFER_SIZE];
    if (raw_dcapp_var) {
        strncpy(dcapp_var, (const char *)raw_dcapp_var, DC_VALUE_STRING_BUFFER_SIZE - 1);
        dcapp_var[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        xmlFree(raw_dcapp_var);
        dc_utils_trim_whitespace_inplace(dcapp_var);
        if (dcapp_var[0] == '\0') {
            DC_LOG_ERROR("TrickVariable", "Empty dcapp variable path");
        }
    } else {
        DC_LOG_ERROR("TrickVariable", "Missing dcapp variable path");
        dcapp_var[0] = '\0';
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
            if (var.dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) {
                DC_LOG_ERROR("TrickVariable", "Unknown variable '%s' in TrickFrom", dcapp_var);
            }
            sbpush(trick_context->sb_rx_var_contexts, var);
            break;
        }
        case DC_APP_ELEM_TYPE_TRICK_TO: {

            // create + add tx var
            _TrickTxVarContext var = {};
            var.dcapp_var_index    = dc_app_lookup_get_var_index(app_data->lookup, dcapp_var);
            var.trick_var_index    = dc_trick_add_tx_var(trick_context->trick, trick_path, units, false); // default to non-string if var undefined
            if (var.dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) {
                DC_LOG_ERROR("TrickVariable", "Unknown variable '%s' in TrickTo", dcapp_var);
            } else {
                DcValue *dc_var_value  = dc_app_lookup_get_value(app_data->lookup, dc_app_lookup_get_var(app_data->lookup, var.dcapp_var_index)->value_index);
                var.trick_var_index    = dc_trick_add_tx_var(trick_context->trick, trick_path, units, dc_var_value->type == DC_VALUE_TYPE_STRING);
                var.prev_value         = *dc_var_value;
            }
            sbpush(trick_context->sb_tx_var_contexts, var);
            break;
        }
        default:
            // should never reach here
            DC_LOG_ERROR("TrickVariable", "Invalid parent node");
            break;
    }

    // return
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_true(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_IF: {
            _NodeIndex first_child_index = _process_xml_node_children(app_data, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(app_data, NODE_TYPE_STATE_IF_TRUE, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("True", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }

    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_variable(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    // name
    xmlChar *raw_name = xmlNodeGetContent(xml_node);
    char     name[DC_VALUE_STRING_BUFFER_SIZE];
    if (raw_name) {
        strncpy(name, (const char *)raw_name, DC_VALUE_STRING_BUFFER_SIZE - 1);
        name[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        xmlFree(raw_name);
        dc_utils_trim_whitespace_inplace(name);
        if (name[0] == '\0') {
            DC_LOG_ERROR("Variable", "Empty variable name");
        }
    } else {
        DC_LOG_ERROR("Variable", "Missing node content");
        name[0] = '\0';
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

            // vertex data
            _VertexData vertex = {};
            vertex.position.x = DC_APP_VAL_INDEX_UNDEFINED;
            vertex.position.y = DC_APP_VAL_INDEX_UNDEFINED;
            vertex.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            vertex.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                vertex.position.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
                xmlFree(raw_x_position);
            } else {
                DC_LOG_ERROR("Vertex", "Missing 'X' attribute");
            }

            // y position
            xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
            if (!raw_y_position) {
                raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
            }
            if (raw_y_position) {
                vertex.position.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
                xmlFree(raw_y_position);
            } else {
                DC_LOG_ERROR("Vertex", "Missing 'Y' attribute");
            }

            // parent x align
            xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
            if (raw_parent_x_align) {
                vertex.parent_align.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
                xmlFree(raw_parent_x_align);
            }

            // parent y align
            xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
            if (raw_parent_y_align) {
                vertex.parent_align.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
                xmlFree(raw_parent_y_align);
            }

            // negate x
            xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
            if (raw_negate_x) {
                vertex.negate_x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
                xmlFree(raw_negate_x);
            } else {
                vertex.negate_x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // negate y
            xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
            if (raw_negate_y) {
                vertex.negate_y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
                xmlFree(raw_negate_y);
            } else {
                vertex.negate_y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            switch (parent_node->type) {
                case NODE_TYPE_LINE:
                    // add to parent
                    sbpush(parent_node->line.sb_vertices, vertex);

                    // check point count
                    if (sbcount(parent_node->line.sb_vertices) > _NODE_LINE_MAX_POINTS) {
                        DC_LOG_ERROR("Line", "Maximum number of points exceeded");
                    }
                    break;
                case NODE_TYPE_POLYGON:
                    // add to parent
                    sbpush(parent_node->polygon.sb_vertices, vertex);

                    // check point count
                    if (sbcount(parent_node->polygon.sb_vertices) > _NODE_POLYGON_MAX_POINTS) {
                        DC_LOG_ERROR("Polygon", "Maximum number of points exceeded");
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            DC_LOG_ERROR("Vertex", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
    }

    // return
    return NODE_INDEX_UNDEFINED;
}

static _NodeIndex _process_xml_node_window(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(xml_node);

    _Node dc_node = {0};
    dc_node.type   = NODE_TYPE_WINDOW;
    dc_node.parent = parent_node_index;

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
        DC_LOG_ERROR("Window", "Missing 'Width' attribute");
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
        DC_LOG_ERROR("Window", "Missing 'Height' attribute");
    }

    // virtual x dimension
    xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
    if (!raw_x_virtual_dimension) {
        raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
    }
    if (raw_x_virtual_dimension) {
        dc_node.window.virtual_dimension.x = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    }

    // virtual y virtual_dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.window.virtual_dimension.y = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    }

    // active display (for Panel DisplayIndex matching)
    xmlChar *raw_active_display = xmlGetProp(xml_node, BAD_CAST "ActiveDisplay");
    if (raw_active_display) {
        dc_node.window.active_display = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_INTEGER, (const char *)raw_active_display);
        xmlFree(raw_active_display);
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

static const char *_node_type_to_string(_NodeType type) {
    switch (type) {
        case NODE_TYPE_CONTAINER:
            return "Container";
        case NODE_TYPE_CONDITIONAL:
            return "Conditional";
        case NODE_TYPE_LINE:
            return "Line";
        case NODE_TYPE_PANEL:
            return "Panel";
        case NODE_TYPE_PIXELSTREAM:
            return "PixelStream";
        case NODE_TYPE_POLYGON:
            return "Polygon";
        case NODE_TYPE_RECTANGLE:
            return "Rectangle";
        case NODE_TYPE_SET:
            return "Set";
        case NODE_TYPE_PLANET_VIEW:
            return "PlanetView";
        case NODE_TYPE_TEXT:
            return "Text";
        case NODE_TYPE_WINDOW:
            return "Window";
        default:
            DC_LOG_WARN("NodeType", "Unknown type: %d", type);
            return "";
    }
}

static _NodeIndex _register_node(_AppData *app_data, _Node *node) {
    sbpush(app_data->sb_nodes, *node);
    return sbcount(app_data->sb_nodes) - 1;
}

static void _init_stencil_pipelines(_AppData* app_data, plDevice* device, plRenderPassHandle render_pass)
{
    plRenderPassLayoutHandle render_pass_layout = _ext_gfx->get_render_pass(device, render_pass)->tDesc.tLayout;
    uint32_t sample_count = _ext_gfx->get_swapchain_info(_ext_starter->get_swapchain()).tSampleCount;

    // common vertex buffer layout for 2D drawing
    const plVertexBufferLayout vertex_layout = {
        .uByteStride = sizeof(float) * 5,
        .atAttributes = {
            {.uByteOffset = 0,                 .tFormat = PL_VERTEX_FORMAT_FLOAT2},
            {.uByteOffset = sizeof(float) * 2, .tFormat = PL_VERTEX_FORMAT_FLOAT2},
            {.uByteOffset = sizeof(float) * 4, .tFormat = PL_VERTEX_FORMAT_UINT},
        }
    };

    // common bind group layouts
    const plBindGroupLayoutDesc sampler_layout = {
        .atSamplerBindings = {
            {.uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT}
        }
    };
    const plBindGroupLayoutDesc texture_layout = {
        .atTextureBindings = {
            {.uSlot = 0, .tStages = PL_SHADER_STAGE_FRAGMENT, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED}
        }
    };

    // stencil create graphics state: increment stencil buffer
    const plGraphicsState stencil_create_state = {
        .ulDepthWriteEnabled  = 0,
        .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
        .ulCullMode           = PL_CULL_MODE_NONE,
        .ulStencilTestEnabled = 1,
        .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
        .ulStencilRef         = 1,
        .ulStencilMask        = 0xFF,
        .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
        .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
        .ulStencilOpPass      = PL_STENCIL_OP_INCREMENT_AND_CLAMP
    };

    // stencil remove graphics state: decrement stencil buffer
    const plGraphicsState stencil_remove_state = {
        .ulDepthWriteEnabled  = 0,
        .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
        .ulCullMode           = PL_CULL_MODE_NONE,
        .ulStencilTestEnabled = 1,
        .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
        .ulStencilRef         = 0,
        .ulStencilMask        = 0xFF,
        .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
        .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
        .ulStencilOpPass      = PL_STENCIL_OP_DECREMENT_AND_CLAMP
    };

    // stencil cleanup graphics state: decrement stencil buffer (same as remove but separate pipeline)
    const plGraphicsState stencil_cleanup_state = {
        .ulDepthWriteEnabled  = 0,
        .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
        .ulCullMode           = PL_CULL_MODE_NONE,
        .ulStencilTestEnabled = 1,
        .ulStencilMode        = PL_COMPARE_MODE_ALWAYS,
        .ulStencilRef         = 0,
        .ulStencilMask        = 0xFF,
        .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
        .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
        .ulStencilOpPass      = PL_STENCIL_OP_DECREMENT_AND_CLAMP
    };

    // blend states
    const plBlendState stencil_only_blend = {
        .bBlendEnabled   = false,
        .uColorWriteMask = PL_COLOR_WRITE_MASK_NONE  // don't write to color buffer
    };
    const plBlendState alpha_blend = {
        .bBlendEnabled   = true,
        .uColorWriteMask = PL_COLOR_WRITE_MASK_ALL,
        .tSrcColorFactor = PL_BLEND_FACTOR_SRC_ALPHA,
        .tDstColorFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .tColorOp        = PL_BLEND_OP_ADD,
        .tSrcAlphaFactor = PL_BLEND_FACTOR_SRC_ALPHA,
        .tDstAlphaFactor = PL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .tAlphaOp        = PL_BLEND_OP_ADD
    };

    // load shaders (pilotlight draw shaders)
    plShaderModule vert_2d  = _ext_shader->load_glsl("draw_2d.vert", "main", NULL, NULL);
    plShaderModule frag_2d  = _ext_shader->load_glsl("draw_2d.frag", "main", NULL, NULL);
    plShaderModule frag_sdf = _ext_shader->load_glsl("draw_2d_sdf.frag", "main", NULL, NULL);

    // load stencil shaders (dcapp-specific, use discard for transparent pixels)
    plShaderModule frag_2d_stencil  = _ext_shader->load_glsl("dc_draw_2d_stencil.frag", "main", NULL, NULL);
    plShaderModule frag_sdf_stencil = _ext_shader->load_glsl("dc_draw_2d_sdf_stencil.frag", "main", NULL, NULL);

    //-------------------------------------------------------------------------
    // 2D stencil shaders
    //-------------------------------------------------------------------------

    // Stencil create 2D (uses discard for transparent pixels, colorWriteMask=0)
    const plShaderDesc stencil_create_2d_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_2d_stencil,
        .tGraphicsState        = stencil_create_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_create_2d_shader = _ext_gfx->create_shader(device, &stencil_create_2d_desc);

    // Stencil remove 2D (uses discard for transparent pixels, colorWriteMask=0)
    const plShaderDesc stencil_remove_2d_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_2d_stencil,
        .tGraphicsState        = stencil_remove_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_remove_2d_shader = _ext_gfx->create_shader(device, &stencil_remove_2d_desc);

    // Stencil cleanup 2D (decrement stencil, no color write)
    const plShaderDesc stencil_cleanup_2d_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_2d_stencil,
        .tGraphicsState        = stencil_cleanup_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_cleanup_2d_shader = _ext_gfx->create_shader(device, &stencil_cleanup_2d_desc);

    // Stencil draw 2D (one per depth level)
    for (int i = 0; i < DC_STENCIL_MAX_DEPTH; i++) {
        const plGraphicsState stencil_draw_state = {
            .ulDepthWriteEnabled  = 0,
            .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
            .ulCullMode           = PL_CULL_MODE_NONE,
            .ulStencilTestEnabled = 1,
            .ulStencilMode        = PL_COMPARE_MODE_LESS_OR_EQUAL,
            .ulStencilRef         = i + 1,
            .ulStencilMask        = 0xFF,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP
        };
        const plShaderDesc stencil_draw_2d_desc = {
            .tVertexShader         = vert_2d,
            .tFragmentShader       = frag_2d,
            .tGraphicsState        = stencil_draw_state,
            .atVertexBufferLayouts = { vertex_layout },
            .atBlendStates         = { alpha_blend },
            .atBindGroupLayouts    = { sampler_layout, texture_layout },
            .tRenderPassLayout     = render_pass_layout,
            .uSubpassIndex         = 0,
            .tMSAASampleCount      = sample_count
        };
        app_data->stencil_draw_2d_shader[i] = _ext_gfx->create_shader(device, &stencil_draw_2d_desc);
    }

    //-------------------------------------------------------------------------
    // SDF stencil shaders
    //-------------------------------------------------------------------------

    // Stencil create SDF (uses discard for non-glyph pixels, colorWriteMask=0)
    const plShaderDesc stencil_create_sdf_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_sdf_stencil,
        .tGraphicsState        = stencil_create_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_create_sdf_shader = _ext_gfx->create_shader(device, &stencil_create_sdf_desc);

    // Stencil remove SDF (uses discard for non-glyph pixels, colorWriteMask=0)
    const plShaderDesc stencil_remove_sdf_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_sdf_stencil,
        .tGraphicsState        = stencil_remove_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_remove_sdf_shader = _ext_gfx->create_shader(device, &stencil_remove_sdf_desc);

    // Stencil cleanup SDF (decrement stencil, no color write)
    const plShaderDesc stencil_cleanup_sdf_desc = {
        .tVertexShader         = vert_2d,
        .tFragmentShader       = frag_sdf_stencil,
        .tGraphicsState        = stencil_cleanup_state,
        .atVertexBufferLayouts = { vertex_layout },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_cleanup_sdf_shader = _ext_gfx->create_shader(device, &stencil_cleanup_sdf_desc);

    // Stencil draw SDF (one per depth level)
    for (int i = 0; i < DC_STENCIL_MAX_DEPTH; i++) {
        const plGraphicsState stencil_draw_state = {
            .ulDepthWriteEnabled  = 0,
            .ulDepthMode          = PL_COMPARE_MODE_ALWAYS,
            .ulCullMode           = PL_CULL_MODE_NONE,
            .ulStencilTestEnabled = 1,
            .ulStencilMode        = PL_COMPARE_MODE_LESS_OR_EQUAL,
            .ulStencilRef         = i + 1,
            .ulStencilMask        = 0xFF,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP
        };
        const plShaderDesc stencil_draw_sdf_desc = {
            .tVertexShader         = vert_2d,
            .tFragmentShader       = frag_sdf,
            .tGraphicsState        = stencil_draw_state,
            .atVertexBufferLayouts = { vertex_layout },
            .atBlendStates         = { alpha_blend },
            .atBindGroupLayouts    = { sampler_layout, texture_layout },
            .tRenderPassLayout     = render_pass_layout,
            .uSubpassIndex         = 0,
            .tMSAASampleCount      = sample_count
        };
        app_data->stencil_draw_sdf_shader[i] = _ext_gfx->create_shader(device, &stencil_draw_sdf_desc);
    }

    //-------------------------------------------------------------------------
    // 3D solid stencil shaders
    //-------------------------------------------------------------------------

    // 3D solid vertex buffer layout (pos3 + color)
    const plVertexBufferLayout vertex_layout_3d_solid = {
        .uByteStride = sizeof(float) * 4,
        .atAttributes = {
            {.uByteOffset = 0,                 .tFormat = PL_VERTEX_FORMAT_FLOAT3},
            {.uByteOffset = sizeof(float) * 3, .tFormat = PL_VERTEX_FORMAT_UINT},
        }
    };

    // load 3D shaders
    plShaderModule vert_3d          = _ext_shader->load_glsl("dc_draw_3d.vert", "main", NULL, NULL);
    plShaderModule frag_3d          = _ext_shader->load_glsl("dc_draw_3d.frag", "main", NULL, NULL);
    plShaderModule vert_3d_textured = _ext_shader->load_glsl("dc_draw_3d_textured.vert", "main", NULL, NULL);
    plShaderModule frag_3d_textured = _ext_shader->load_glsl("dc_draw_3d_textured.frag", "main", NULL, NULL);

    // Stencil create 3D solid (increment stencil, no color write)
    const plShaderDesc stencil_create_3d_solid_desc = {
        .tVertexShader         = vert_3d,
        .tFragmentShader       = frag_3d,
        .tGraphicsState        = stencil_create_state,
        .atVertexBufferLayouts = { vertex_layout_3d_solid },
        .atBlendStates         = { stencil_only_blend },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_create_3d_solid_shader = _ext_gfx->create_shader(device, &stencil_create_3d_solid_desc);

    // Stencil remove 3D solid (decrement stencil, no color write)
    const plShaderDesc stencil_remove_3d_solid_desc = {
        .tVertexShader         = vert_3d,
        .tFragmentShader       = frag_3d,
        .tGraphicsState        = stencil_remove_state,
        .atVertexBufferLayouts = { vertex_layout_3d_solid },
        .atBlendStates         = { stencil_only_blend },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_remove_3d_solid_shader = _ext_gfx->create_shader(device, &stencil_remove_3d_solid_desc);

    // Stencil cleanup 3D solid (decrement stencil, no color write)
    const plShaderDesc stencil_cleanup_3d_solid_desc = {
        .tVertexShader         = vert_3d,
        .tFragmentShader       = frag_3d,
        .tGraphicsState        = stencil_cleanup_state,
        .atVertexBufferLayouts = { vertex_layout_3d_solid },
        .atBlendStates         = { stencil_only_blend },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_cleanup_3d_solid_shader = _ext_gfx->create_shader(device, &stencil_cleanup_3d_solid_desc);

    // Stencil draw 3D solid (one per depth level, with depth test)
    for (int i = 0; i < DC_STENCIL_MAX_DEPTH; i++) {
        const plGraphicsState stencil_draw_3d_state = {
            .ulDepthWriteEnabled  = 1,
            .ulDepthMode          = PL_COMPARE_MODE_LESS,
            .ulCullMode           = PL_CULL_MODE_NONE,
            .ulStencilTestEnabled = 1,
            .ulStencilMode        = PL_COMPARE_MODE_LESS_OR_EQUAL,
            .ulStencilRef         = i + 1,
            .ulStencilMask        = 0xFF,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP
        };
        const plShaderDesc stencil_draw_3d_solid_desc = {
            .tVertexShader         = vert_3d,
            .tFragmentShader       = frag_3d,
            .tGraphicsState        = stencil_draw_3d_state,
            .atVertexBufferLayouts = { vertex_layout_3d_solid },
            .atBlendStates         = { alpha_blend },
            .tRenderPassLayout     = render_pass_layout,
            .uSubpassIndex         = 0,
            .tMSAASampleCount      = sample_count
        };
        app_data->stencil_draw_3d_solid_shader[i] = _ext_gfx->create_shader(device, &stencil_draw_3d_solid_desc);
    }

    //-------------------------------------------------------------------------
    // 3D textured stencil shaders
    //-------------------------------------------------------------------------

    // 3D textured vertex buffer layout (pos3 + uv2 + color)
    const plVertexBufferLayout vertex_layout_3d_textured = {
        .uByteStride = sizeof(float) * 6,
        .atAttributes = {
            {.uByteOffset = 0,                 .tFormat = PL_VERTEX_FORMAT_FLOAT3},
            {.uByteOffset = sizeof(float) * 3, .tFormat = PL_VERTEX_FORMAT_FLOAT2},
            {.uByteOffset = sizeof(float) * 5, .tFormat = PL_VERTEX_FORMAT_UINT},
        }
    };

    // Stencil create 3D textured (increment stencil, no color write)
    const plShaderDesc stencil_create_3d_textured_desc = {
        .tVertexShader         = vert_3d_textured,
        .tFragmentShader       = frag_3d_textured,
        .tGraphicsState        = stencil_create_state,
        .atVertexBufferLayouts = { vertex_layout_3d_textured },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_create_3d_textured_shader = _ext_gfx->create_shader(device, &stencil_create_3d_textured_desc);

    // Stencil remove 3D textured (decrement stencil, no color write)
    const plShaderDesc stencil_remove_3d_textured_desc = {
        .tVertexShader         = vert_3d_textured,
        .tFragmentShader       = frag_3d_textured,
        .tGraphicsState        = stencil_remove_state,
        .atVertexBufferLayouts = { vertex_layout_3d_textured },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_remove_3d_textured_shader = _ext_gfx->create_shader(device, &stencil_remove_3d_textured_desc);

    // Stencil cleanup 3D textured (decrement stencil, no color write)
    const plShaderDesc stencil_cleanup_3d_textured_desc = {
        .tVertexShader         = vert_3d_textured,
        .tFragmentShader       = frag_3d_textured,
        .tGraphicsState        = stencil_cleanup_state,
        .atVertexBufferLayouts = { vertex_layout_3d_textured },
        .atBlendStates         = { stencil_only_blend },
        .atBindGroupLayouts    = { sampler_layout, texture_layout },
        .tRenderPassLayout     = render_pass_layout,
        .uSubpassIndex         = 0,
        .tMSAASampleCount      = sample_count
    };
    app_data->stencil_cleanup_3d_textured_shader = _ext_gfx->create_shader(device, &stencil_cleanup_3d_textured_desc);

    // Stencil draw 3D textured (one per depth level, with depth test)
    for (int i = 0; i < DC_STENCIL_MAX_DEPTH; i++) {
        const plGraphicsState stencil_draw_3d_state = {
            .ulDepthWriteEnabled  = 1,
            .ulDepthMode          = PL_COMPARE_MODE_LESS,
            .ulCullMode           = PL_CULL_MODE_NONE,
            .ulStencilTestEnabled = 1,
            .ulStencilMode        = PL_COMPARE_MODE_LESS_OR_EQUAL,
            .ulStencilRef         = i + 1,
            .ulStencilMask        = 0xFF,
            .ulStencilOpFail      = PL_STENCIL_OP_KEEP,
            .ulStencilOpDepthFail = PL_STENCIL_OP_KEEP,
            .ulStencilOpPass      = PL_STENCIL_OP_KEEP
        };
        const plShaderDesc stencil_draw_3d_textured_desc = {
            .tVertexShader         = vert_3d_textured,
            .tFragmentShader       = frag_3d_textured,
            .tGraphicsState        = stencil_draw_3d_state,
            .atVertexBufferLayouts = { vertex_layout_3d_textured },
            .atBlendStates         = { alpha_blend },
            .atBindGroupLayouts    = { sampler_layout, texture_layout },
            .tRenderPassLayout     = render_pass_layout,
            .uSubpassIndex         = 0,
            .tMSAASampleCount      = sample_count
        };
        app_data->stencil_draw_3d_textured_shader[i] = _ext_gfx->create_shader(device, &stencil_draw_3d_textured_desc);
    }
}

static void _init_app_data(_AppData *app_data, _Node *window_node) {

    // mount VFS dirs
    _ext_vfs->mount_directory("/shaders-terrain", "../../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/assets", "../../data", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/cache", "cache", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/shaders", "../shaders", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/shader-temp", "../shader-temp", PL_VFS_MOUNT_FLAGS_NONE);
    _ext_vfs->mount_directory("/tiles", "../../data", PL_VFS_MOUNT_FLAGS_NONE);

    // set initial window params
    plWindowDesc window_desc = {};
    window_desc.pcTitle      = window_node->window.title;
    window_desc.uWidth       = (uint32_t)window_node->window.init_dimension.x;
    window_desc.uHeight      = (uint32_t)window_node->window.init_dimension.y;
    window_desc.iXPos        = (int)window_node->window.init_position.x;
    window_desc.iYPos        = (int)window_node->window.init_position.y;
    _ext_windows->create(window_desc, &(app_data->pl_window));
    _ext_windows->show(app_data->pl_window);

    // initialize the starter API (handles alot of boilerplate)
    plStarterInit tStarterInit = {
        .tFlags   = PL_STARTER_FLAGS_ALL_EXTENSIONS & (~PL_STARTER_FLAGS_SHADER_EXT) | PL_STARTER_FLAGS_MSAA | PL_STARTER_FLAGS_DEPTH_BUFFER,
        .ptWindow = app_data->pl_window};
    _ext_starter->initialize(tStarterInit);

    // get device
    plDevice *device = _ext_starter->get_device();

    // initialize dc_draw_ext and dc_draw_backend_ext (pl_starter doesn't do this since we use dcDrawI)
    plDrawInit tDrawInit = {0};
    _ext_draw->initialize(&tDrawInit);
    _ext_draw_backend->initialize(device);

    // initialize GPU memory allocators
    app_data->gpu_local_dedicated_allocator  = _ext_gpu_allocators->get_local_dedicated_allocator(device);
    app_data->gpu_local_buddy_allocator      = _ext_gpu_allocators->get_local_buddy_allocator(device);
    app_data->gpu_staging_uncached_allocator = _ext_gpu_allocators->get_staging_uncached_allocator(device);

    // init default staging buffer
    {
        // set size to 10 MB
        app_data->pl_staging_buffer_size = 100 * 1048576;

        // description
        const plBufferDesc staging_buffer_desc = {
            .tUsage      = PL_BUFFER_USAGE_STAGING,
            .szByteSize  = app_data->pl_staging_buffer_size,
            .pcDebugName = "staging buffer"};
        app_data->pl_staging_buffer_handle = _ext_gfx->create_buffer(device, &staging_buffer_desc, NULL);

        // retrieve buffer to get memory allocation requirements
        plBuffer *staging_buffer = _ext_gfx->get_buffer(device, app_data->pl_staging_buffer_handle);

        // allocate memory using staging allocator
        plDeviceMemoryAllocatorI *allocator = app_data->gpu_staging_uncached_allocator;
        const plDeviceMemoryAllocation staging_buffer_allocation = allocator->allocate(
            allocator->ptInst,
            staging_buffer->tMemoryRequirements.uMemoryTypeBits,
            staging_buffer->tMemoryRequirements.ulSize,
            staging_buffer->tMemoryRequirements.ulAlignment,
            "staging buffer memory");

        // bind the buffer to the new memory allocation
        _ext_gfx->bind_buffer_to_memory(device, app_data->pl_staging_buffer_handle, &staging_buffer_allocation);
    }

    // create font atlas
    {
        // create and set font atlas
        plFontAtlas* font_atlas = _ext_draw->create_font_atlas();
        _ext_draw->set_font_atlas(font_atlas);

        // typical font range (you can also add individual characters)
        const plFontRange font_range = {
            .iFirstCodePoint = 0x0020,
            .uCharCount      = 0x00FF - 0x0020};

        // adding previous font but as a signed distance field (SDF)
        plFontConfig font_config   = {};
        font_config.bSdf           = true; // only works with ttf
        font_config.fSize          = 25.0f;
        font_config.uHOverSampling = 1;
        font_config.uVOverSampling = 1;
        font_config.ucOnEdgeValue  = 180;
        font_config.iSdfPadding    = 1;
        font_config.uRangeCount    = 1;
        font_config.ptRanges       = &font_range;

        app_data->pl_vera_sdf_font = _ext_draw->add_font_from_file_ttf(font_atlas, font_config, "../../assets/fonts/bitstream-vera-sans/Vera.ttf");

        // build font atlas (CPU prepare + GPU upload - backend handles prepare internally)
        plCommandBuffer* command_buffer = _ext_gfx->request_command_buffer(_ext_starter->get_current_command_pool(), "dcapp font atlas");
        _ext_draw_backend->build_font_atlas(command_buffer, font_atlas);
        _ext_gfx->wait_on_command_buffer(command_buffer);
        _ext_gfx->return_command_buffer(command_buffer);
    }
    // Note: don't call set_default_font - pl_starter uses plDrawI with its own font atlas,
    // while dcapp uses dcDrawI with a separate font atlas. Let pl_starter create its own default font.

    // initialize shader compiler
    plShaderOptions shader_options          = {};
    shader_options.apcIncludeDirectories[0] = "/shaders/";
    shader_options.apcIncludeDirectories[1] = "/shaders-terrain/";
    shader_options.apcDirectories[0]        = "/shaders/";
    shader_options.apcDirectories[1]        = "/shaders-terrain/";
    shader_options.pcCacheOutputDirectory   = "/shader-temp/";
    shader_options.tFlags                   = PL_SHADER_FLAGS_AUTO_OUTPUT | PL_SHADER_FLAGS_INCLUDE_DEBUG | PL_SHADER_FLAGS_ALWAYS_COMPILE;
    _ext_shader->initialize(&shader_options);

    // wraps up
    _ext_starter->finalize();

    // initialize stencil pipelines for clipping
    _init_stencil_pipelines(app_data, _ext_starter->get_device(), _ext_starter->get_render_pass());

    // register our app drawlist
    app_data->pl_draw_list = _ext_draw->request_2d_drawlist();

    // request layers (allows drawing out of order)
    app_data->pl_layer = _ext_draw->request_2d_layer(app_data->pl_draw_list);

    // initialize frame data
    app_data->frame_data.pressed_node      = NODE_INDEX_UNDEFINED;
    app_data->frame_data.next_pressed_node = NODE_INDEX_UNDEFINED;
    app_data->frame_data.hovered_node      = NODE_INDEX_UNDEFINED;
    app_data->frame_data.next_hovered_node = NODE_INDEX_UNDEFINED;
    app_data->frame_data.released_node     = NODE_INDEX_UNDEFINED;
    app_data->frame_data.active_node       = NODE_INDEX_UNDEFINED;
}

static _Texture _create_texture(_AppData *app_data, uint32_t texture_width, uint32_t texture_height, const char *texture_name) {

    // get device
    plDevice *device = _ext_starter->get_device();

    // create new texture desc
    plTextureDesc pl_texture_desc;
    memset(&pl_texture_desc, 0, sizeof(plTextureDesc));
    pl_texture_desc.tDimensions = (plVec3){(float)texture_width, (float)texture_height, 1.0f};
    pl_texture_desc.tFormat     = PL_FORMAT_R8G8B8A8_UNORM;
    pl_texture_desc.uLayers     = 1;
    pl_texture_desc.uMips       = 1;
    pl_texture_desc.tType       = PL_TEXTURE_TYPE_2D;
    pl_texture_desc.tUsage      = PL_TEXTURE_USAGE_SAMPLED;
    pl_texture_desc.pcDebugName = texture_name;

    // create texture
    plTexture      *pl_texture;
    plTextureHandle pl_texture_handle = _ext_gfx->create_texture(device, &pl_texture_desc, &pl_texture);

    // choose allocator based on texture size
    // use buddy allocator for smaller textures, dedicated for large ones
    plDeviceMemoryAllocatorI *allocator = app_data->gpu_local_buddy_allocator;
    if (pl_texture->tMemoryRequirements.ulSize > _ext_gpu_allocators->get_buddy_block_size())
        allocator = app_data->gpu_local_dedicated_allocator;

    const plDeviceMemoryAllocation pl_texture_allocation = allocator->allocate(
        allocator->ptInst,
        pl_texture->tMemoryRequirements.uMemoryTypeBits,
        pl_texture->tMemoryRequirements.ulSize,
        pl_texture->tMemoryRequirements.ulAlignment,
        texture_name);

    // bind memory
    _ext_gfx->bind_texture_to_memory(device, pl_texture_handle, &pl_texture_allocation);

    // create bind group
    plBindGroupHandle pl_bind_group_handle = _ext_draw_backend->create_bind_group_for_texture(pl_texture_handle);

    // create _Texture struct
    _Texture texture = {
        pl_texture_handle,
        pl_bind_group_handle};
    return texture;
}

static bool _load_color_from_string(_AppData *app_data, xmlNodePtr xml_node, const char *attr_name, _ValIndex4 *color_out) {

    xmlChar *raw_color = xmlGetProp(xml_node, BAD_CAST attr_name);
    if (raw_color) {

        // clean raw string
        char cleaned_color[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned_color, (const char *)(const char *)raw_color, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_color);

        // split by whitespace
        // assume no more than 20 splits
        size_t       index_buffer[20];
        size_t       index_count;
        dc_utils_split_string_inplace(cleaned_color, dc_utils_whitespace, index_buffer, 20, &index_count);

        // if empty, assume no color
        if (index_count == 0) {
            return false;
        }

        // process each color
        if (index_count > 0) {
            color_out->r = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[0]]));
        } else {
            color_out->r = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 1) {
            color_out->g = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[1]]));
        } else {
            color_out->g = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 2) {
            color_out->b = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[2]]));
        } else {
            color_out->b = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 3) {
            color_out->a = dc_app_create_and_register_typed_value_from_string(app_data->lookup, DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[3]]));
        } else {
            color_out->a = DC_APP_VAL_INDEX_UNDEFINED;
        }

        return true;
    } else {
        return false;
    }
}
