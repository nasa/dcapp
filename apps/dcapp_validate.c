#include "../src/app/xml_preprocessor.h"
#include "../src/app/xml_element.h"
#include "../src/app/planet_types.h"
#include "../src/utils/log.h"
#include "../src/utils/string.h"

#include <libxml/parser.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//~ validation state

typedef struct {
    int error_count;
    int warning_count;
} ValidationContext;

//~ attribute schemas

static const char *_valid_attrs_common[] = {"Style", "_Directory", NULL};
static const char *_valid_attrs_position[] = {"X", "Y", "PositionX", "PositionY", NULL};
static const char *_valid_attrs_negate[] = {"NegateX", "NegateY", NULL};
static const char *_valid_attrs_dimension[] = {"Width", "Height", "DimensionX", "DimensionY", NULL};
static const char *_valid_attrs_virtual_dimension[] = {"VirtualWidth", "VirtualHeight", "VirtualDimensionX", "VirtualDimensionY", NULL};
static const char *_valid_attrs_align[] = {"LocalAlignX", "LocalAlignY", "HorizontalAlign", "VerticalAlign", "ParentAlignX", "ParentAlignY", NULL};
static const char *_valid_attrs_pivot[] = {"PivotX", "PivotY", "PivotPositionX", "PivotPositionY", "PivotLocalAlignX", "PivotLocalAlignY", "PivotParentAlignX", "PivotParentAlignY", NULL};
static const char *_valid_attrs_rotation[] = {"Rotation", "Rotate", NULL};
static const char *_valid_attrs_color[] = {"FillColor", "LineColor", "BackgroundColor", NULL};
static const char *_valid_attrs_line[] = {"LineWidth", NULL};

static const char *_valid_attrs_arc[] = {"Radius", "Angle", "Segments", "LineColor", NULL};
static const char *_valid_attrs_arg[] = {"Type", "Value", NULL};
static const char *_valid_attrs_blink[] = {"FireBlink", "Frequency", "DutyCycle", "Duration", NULL};
static const char *_valid_attrs_button[] = {"Type", "Variable", "EnableVariable", "EnableOn", "TargetVariable", "TargetOn", "TargetOff", "On", "Off", "IndicatorVariable", "IndicatorOn", NULL};
static const char *_valid_attrs_ellipse[] = {"Radius", "RadiusX", "RadiusY", "Segments", "Angle", NULL};
static const char *_valid_attrs_constant[] = {"Name", NULL};
static const char *_valid_attrs_function[] = {"Name", "FireCall", NULL};
static const char *_valid_attrs_draw_function[] = {"Name", NULL};
static const char *_valid_attrs_if[] = {"Value", "Value1", "Value2", "Operator", "Static", NULL};
static const char *_valid_attrs_image[] = {"File", NULL};
static const char *_valid_attrs_logic[] = {"File", NULL};
static const char *_valid_attrs_mouse_motion[] = {"VariableX", "VariableY", NULL};
static const char *_valid_attrs_panel[] = {"DisplayIndex", NULL};
static const char *_valid_attrs_pixelstream[] = {"Type", "URL", "Protocol", "Timeout", "TestPattern", NULL};
static const char *_valid_attrs_set[] = {"Variable", "Operator", "Defer", NULL};
static const char *_valid_attrs_sphere[] = {"Radius", "Image", "Roll", "Pitch", "Yaw", NULL};
static const char *_valid_attrs_style[] = {"Name", NULL};
static const char *_valid_attrs_planet[] = {"Name", "CRS", "LightDirectionX", "LightDirectionY", "LightDirectionZ", "MeshCacheSize", NULL};
static const char *_valid_attrs_planet_view[] = {"Planet", "CRS", "AttitudeFrame", "ShaderIndex", "Tau", "Flatten", "PositionX", "X", "PositionY", "Y", "DimensionX", "Width", "DimensionY", "Height", "LocalAlignX", "HorizontalAlign", "LocalAlignY", "VerticalAlign", "ParentAlignX", "ParentAlignY", "Rotation", "Rotate", "PivotPositionX", "PivotX", "PivotPositionY", "PivotY", "PivotParentAlignX", "PivotParentAlignY", "PivotLocalAlignX", "PivotLocalAlignY", "CameraLatitude", "CameraLongitude", "CameraElevation", "CameraHeading", "CameraFOV", "CameraX", "CameraY", "CameraZ", "CameraRoll", "CameraPitch", "CameraYaw", "CameraOrthographic", "NegateX", "NegateY", NULL};
static const char *_valid_attrs_planet_container[] = {"Latitude", "Longitude", "HeightAboveTerrain", "Rotation", "Scale", "Enabled", NULL};
static const char *_valid_attrs_planet_data[] = {"File", NULL};
static const char *_valid_attrs_planet_texture[] = {"File", "CRS", "MetersPerPixel", "Latitude", "Longitude", "X", "Y", "Z", "OriginX", "OriginY", "Enabled", "FireRefresh", NULL};
static const char *_valid_attrs_planet_shader[] = {"Index", "VertexShader", "FragmentShader", NULL};
static const char *_valid_attrs_planet_overlay[] = {"Planet", "CRS", "HeightAboveTerrain", "Latitude", "Longitude", "X", "Y", "Z", "Radius", "RadiusX", "RadiusY", "Rotation", "Segments", "Size", "Enabled", NULL};
static const char *_valid_attrs_planet_image[] = {"File", "Width", "Height", "DimensionX", "DimensionY", "TintColor", "Color", NULL};
static const char *_valid_attrs_planet_breadcrumbs[] = {"Altitude", "PointSpacing", "MaxPoints", "Clear", "Enabled", NULL};
static const char *_valid_attrs_planet_geojson[] = {"File", "Planet", "CRS", "HeightAboveTerrain", "Enabled", NULL};
static const char *_valid_attrs_planet_vertex[] = {"Latitude", "Longitude", "Altitude", "X", "Y", "Z", NULL};
static const char *_valid_attrs_rounded[] = {"Rounded", NULL};
static const char *_valid_attrs_text[] = {"Size", "ShadowOffset", "UpdateRate", "Font", "Color", NULL};
static const char *_valid_attrs_trick_io[] = {"Host", "Port", "DataRate", "ConnectedVariable", NULL};
static const char *_valid_attrs_trick_variable[] = {"Name", "Units", NULL};
static const char *_valid_attrs_edge_io[] = {"Host", "Port", "DataRate", "ConnectedVariable", NULL};
static const char *_valid_attrs_edge_variable[] = {"Command", NULL};
static const char *_valid_attrs_variable[] = {"Type", "InitialValue", NULL};
static const char *_valid_attrs_vertex[] = {NULL};
static const char *_valid_attrs_window[] = {"Title", "ActiveDisplay", "UpdateRate", "Fullscreen", NULL};

//~ declarations

static void _validate_node(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType parent_type);
static void _validate_children(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType parent_type);
static bool _is_valid_child(DcAppXmlElementType parent_type, DcAppXmlElementType child_type);
static bool _is_window_render_parent(DcAppXmlElementType parent_type);
static void _validate_required_attributes(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType elem_type);
static void _validate_planet_local_attributes(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType elem_type, DcAppXmlElementType parent_type);
static bool _attr_in_list(const char *attr_name, const char **list);
static bool _is_valid_attr_for_elem(const char *attr_name, DcAppXmlElementType elem_type);
static void _validate_attribute_names(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType elem_type);
static bool _is_variable_ref(const char *value);
static bool _is_valid_int_in_range(const char *value, int min, int max);
static void _validate_enum_attr(ValidationContext *ctx, xmlNodePtr node, const char *attr_name,
                                int min_val, int max_val, const char *valid_values_desc);
static void _validate_attribute_values(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType elem_type);
static void _check_var_attr(ValidationContext *ctx, xmlNodePtr node, const char *attr_name);
static void _validate_variable_references(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType elem_type);

//~ validation entry point

int main(int argc, char **argv) {

    if (argc < 2) {
        DC_LOG_ERROR("Validate", "Usage: dcapp-validate <config.xml> [--preprocessed <output.xml>] [CONSTANT=value ...]");
        return 1;
    }

    //- parse validator options

    // find the preprocessed output before collecting constants
    const char *preprocessed_output = NULL;
    int const_count = 0;
    char **const_args = NULL;

    for (int ii = 2; ii < argc; ii++) {
        if (strcmp(argv[ii], "--preprocessed") == 0 && ii + 1 < argc) {
            preprocessed_output = argv[++ii];
        }
    }

    // collect constants apart from the output option
    if (argc > 2) {
        const_args = (char **)malloc(sizeof(char *) * (argc - 2));
        for (int ii = 2; ii < argc; ii++) {
            if (strcmp(argv[ii], "--preprocessed") == 0 && ii + 1 < argc) {
                ii++; // skip the output path
                continue;
            }
            const_args[const_count++] = argv[ii];
        }
    }

    //- preprocess the display configuration

    DcAppXmlPreprocessorContext *config;
    const char *config_filepath = argv[1];
    if (const_count > 0) {
        config = dc_app_xml_preprocessor_context_create(config_filepath, const_args, const_count);
    } else {
        config = dc_app_xml_preprocessor_context_create(config_filepath, NULL, 0);
    }
    free(const_args);

    // export the same path roots used by the runtime
    dc_app_xml_preprocessor_export_environment(config);

    // expand includes, constants, and static conditions
    dc_app_xml_preprocessor_preprocess(config);

    // save the expanded xml when requested
    dc_app_xml_preprocessor_save_preprocessed(config, preprocessed_output);

    //- validate the expanded tree
    ValidationContext ctx = {0};
    xmlNodePtr root_node = dc_app_xml_preprocessor_root(config);

    _validate_node(&ctx, root_node, DC_APP_XML_ELEMENT_TYPE_NONELEM);

    DC_LOG_INFO("Validate", "Complete: %d error(s), %d warning(s)", ctx.error_count, ctx.warning_count);

    return ctx.error_count > 0 ? 1 : 0;
}

//~ tree validation

void _validate_children(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType parent_type) {
    xmlNodePtr child = node->children;
    while (child) {
        _validate_node(ctx, child, parent_type);
        child = child->next;
    }
}

void _validate_node(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType parent_type) {

    DcAppXmlElementType elem_type = dc_app_xml_element_type_from_xml_node(node);

    if (elem_type == DC_APP_XML_ELEMENT_TYPE_NONELEM) {
        return;
    }

    //- validate node placement
    if (!_is_valid_child(parent_type, elem_type)) {
        DC_LOG_ERROR("Validate", "<%s> is not a valid child of <%s> (line %ld)",
                     node->name, node->parent ? node->parent->name : (xmlChar *)"root", xmlGetLineNo(node));
        ctx->error_count++;
    }

    //- validate node attributes

    _validate_required_attributes(ctx, node, elem_type);

    // enforce local planet primitive placement
    _validate_planet_local_attributes(ctx, node, elem_type, parent_type);

    _validate_attribute_names(ctx, node, elem_type);

    _validate_attribute_values(ctx, node, elem_type);

    _validate_variable_references(ctx, node, elem_type);

    _validate_children(ctx, node, elem_type);
}

static void _validate_planet_local_attributes(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType elem_type, DcAppXmlElementType parent_type) {
    //- direct planet container primitives
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_PLANET_CONTAINER) {
        if (elem_type == DC_APP_XML_ELEMENT_TYPE_PLANET_LINE || elem_type == DC_APP_XML_ELEMENT_TYPE_PLANET_POLYGON) {
            const char *invalid_attrs[] = {"CRS", "HeightAboveTerrain"};
            for (size_t i = 0; i < sizeof(invalid_attrs) / sizeof(invalid_attrs[0]); i++) {
                if (xmlHasProp(node, BAD_CAST invalid_attrs[i])) {
                    DC_LOG_ERROR("Validate", "<%s> inside <PlanetContainer> cannot use '%s' (line %ld)",
                                 node->name, invalid_attrs[i], xmlGetLineNo(node));
                    ctx->error_count++;
                }
            }
            return;
        }

        if (elem_type == DC_APP_XML_ELEMENT_TYPE_PLANET_TEXT) {
            if (!xmlHasProp(node, BAD_CAST "X")) {
                DC_LOG_ERROR("Validate", "<PlanetText> inside <PlanetContainer> requires 'X' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (!xmlHasProp(node, BAD_CAST "Y")) {
                DC_LOG_ERROR("Validate", "<PlanetText> inside <PlanetContainer> requires 'Y' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }

            const char *invalid_attrs[] = {"CRS", "HeightAboveTerrain", "Latitude", "Longitude", "Z"};
            for (size_t i = 0; i < sizeof(invalid_attrs) / sizeof(invalid_attrs[0]); i++) {
                if (xmlHasProp(node, BAD_CAST invalid_attrs[i])) {
                    DC_LOG_ERROR("Validate", "<PlanetText> inside <PlanetContainer> cannot use '%s' (line %ld)",
                                 invalid_attrs[i], xmlGetLineNo(node));
                    ctx->error_count++;
                }
            }
            return;
        }
    }

    //- nested planet container vertices
    if (elem_type != DC_APP_XML_ELEMENT_TYPE_VERTEX ||
        (parent_type != DC_APP_XML_ELEMENT_TYPE_PLANET_LINE && parent_type != DC_APP_XML_ELEMENT_TYPE_PLANET_POLYGON)) return;

    xmlNodePtr container = node->parent ? node->parent->parent : NULL;
    if (!container || dc_app_xml_element_type_from_xml_node(container) != DC_APP_XML_ELEMENT_TYPE_PLANET_CONTAINER) return;

    if (!xmlHasProp(node, BAD_CAST "X")) {
        DC_LOG_ERROR("Validate", "<Vertex> inside <PlanetContainer> requires 'X' (line %ld)", xmlGetLineNo(node));
        ctx->error_count++;
    }
    if (!xmlHasProp(node, BAD_CAST "Y")) {
        DC_LOG_ERROR("Validate", "<Vertex> inside <PlanetContainer> requires 'Y' (line %ld)", xmlGetLineNo(node));
        ctx->error_count++;
    }

    const char *invalid_attrs[] = {"Latitude", "Longitude", "Altitude", "Z"};
    for (size_t i = 0; i < sizeof(invalid_attrs) / sizeof(invalid_attrs[0]); i++) {
        if (xmlHasProp(node, BAD_CAST invalid_attrs[i])) {
            DC_LOG_ERROR("Validate", "<Vertex> inside <PlanetContainer> cannot use '%s' (line %ld)",
                         invalid_attrs[i], xmlGetLineNo(node));
            ctx->error_count++;
        }
    }
}

static bool _is_window_render_parent(DcAppXmlElementType parent_type) {
    switch (parent_type) {
        case DC_APP_XML_ELEMENT_TYPE_WINDOW:
        case DC_APP_XML_ELEMENT_TYPE_PANEL:
        case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
        case DC_APP_XML_ELEMENT_TYPE_BLINK:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_PRESSED:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_RELEASED:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_ENABLED:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_DISABLED:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_TRANSITION:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_INDICATOR_ON:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_INDICATOR_OFF:
        case DC_APP_XML_ELEMENT_TYPE_IF:
        case DC_APP_XML_ELEMENT_TYPE_TRUE:
        case DC_APP_XML_ELEMENT_TYPE_FALSE:
        case DC_APP_XML_ELEMENT_TYPE_STENCIL:
        case DC_APP_XML_ELEMENT_TYPE_STENCIL_ADD:
        case DC_APP_XML_ELEMENT_TYPE_STENCIL_REMOVE:
        case DC_APP_XML_ELEMENT_TYPE_STENCIL_DRAW:
        case DC_APP_XML_ELEMENT_TYPE_POLYGON:
        case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
        case DC_APP_XML_ELEMENT_TYPE_IMAGE:
        case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
        case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
        case DC_APP_XML_ELEMENT_TYPE_MOUSE_ACTIVE:
        case DC_APP_XML_ELEMENT_TYPE_MOUSE_INACTIVE:
        case DC_APP_XML_ELEMENT_TYPE_MOUSE_HOVERED:
        case DC_APP_XML_ELEMENT_TYPE_MOUSE_PRESSED:
        case DC_APP_XML_ELEMENT_TYPE_MOUSE_RELEASED:
            return true;
        default:
            return false;
    }
}

//~ parent and child rules

bool _is_valid_child(DcAppXmlElementType parent_type, DcAppXmlElementType child_type) {

    // reject preprocessing elements that survive expansion
    switch (child_type) {
        case DC_APP_XML_ELEMENT_TYPE_CONSTANT:
        case DC_APP_XML_ELEMENT_TYPE_STYLE:
        case DC_APP_XML_ELEMENT_TYPE_INCLUDE:
        case DC_APP_XML_ELEMENT_TYPE_DUMMY:
            return false; // these should never exist after preprocessing
        default:
            break;
    }

    // callbacks execute only through the window render tree
    if (child_type == DC_APP_XML_ELEMENT_TYPE_FUNCTION ||
        child_type == DC_APP_XML_ELEMENT_TYPE_DRAW_FUNCTION) {
        return _is_window_render_parent(parent_type);
    }

    //- dcapp root children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_DCAPP) {
        switch (child_type) {
            case DC_APP_XML_ELEMENT_TYPE_WINDOW:
            case DC_APP_XML_ELEMENT_TYPE_VARIABLE:
            case DC_APP_XML_ELEMENT_TYPE_TRICK_IO:
            case DC_APP_XML_ELEMENT_TYPE_EDGE_IO:
            case DC_APP_XML_ELEMENT_TYPE_LOGIC:
            case DC_APP_XML_ELEMENT_TYPE_PLANET:
                return true;
            default:
                return false;
        }
    }

    //- window children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_WINDOW) {
        switch (child_type) {
            // legacy support
            case DC_APP_XML_ELEMENT_TYPE_PANEL:
            // configuration elements
            case DC_APP_XML_ELEMENT_TYPE_VARIABLE:
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
            // input elements
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_MOTION:
                return true;
            default:
                return false;
        }
    }

    //- panel children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_PANEL) {
        switch (child_type) {
            // configuration elements
            case DC_APP_XML_ELEMENT_TYPE_VARIABLE:
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
            // input elements
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_MOTION:
                return true;
            default:
                return false;
        }
    }

    //- container children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_CONTAINER) {
        switch (child_type) {
            // configuration elements
            case DC_APP_XML_ELEMENT_TYPE_VARIABLE:
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
            // input elements
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_MOTION:
            // mouse events
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_ACTIVE:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_INACTIVE:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_HOVERED:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_PRESSED:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_RELEASED:
                return true;
            default:
                return false;
        }
    }

    //- blink children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_BLINK) {
        switch (child_type) {
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
                return true;
            default:
                return false;
        }
    }

    //- draw function children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_DRAW_FUNCTION) {
        return child_type == DC_APP_XML_ELEMENT_TYPE_ARG;
    }

    //- button children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_BUTTON) {
        switch (child_type) {
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
            // button state elements
            case DC_APP_XML_ELEMENT_TYPE_BUTTON_PRESSED:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON_RELEASED:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON_ENABLED:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON_DISABLED:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON_TRANSITION:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON_INDICATOR_ON:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON_INDICATOR_OFF:
            // mouse events
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_ACTIVE:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_INACTIVE:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_HOVERED:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_PRESSED:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_RELEASED:
                return true;
            default:
                return false;
        }
    }

    //- button state children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_BUTTON_PRESSED ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_BUTTON_RELEASED ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_BUTTON_ENABLED ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_BUTTON_DISABLED ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_BUTTON_TRANSITION ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_BUTTON_INDICATOR_ON ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_BUTTON_INDICATOR_OFF) {
        switch (child_type) {
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
                return true;
            default:
                return false;
        }
    }

    //- conditional children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_IF) {
        switch (child_type) {
            // conditional branches
            case DC_APP_XML_ELEMENT_TYPE_TRUE:
            case DC_APP_XML_ELEMENT_TYPE_FALSE:
            // configuration elements
            case DC_APP_XML_ELEMENT_TYPE_VARIABLE:
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
            // input elements
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_MOTION:
                return true;
            default:
                return false;
        }
    }

    //- conditional branch children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_TRUE || parent_type == DC_APP_XML_ELEMENT_TYPE_FALSE) {
        switch (child_type) {
            // configuration elements
            case DC_APP_XML_ELEMENT_TYPE_VARIABLE:
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_MOTION:
                return true;
            default:
                return false;
        }
    }

    //- stencil children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_STENCIL) {
        switch (child_type) {
            // stencil operations
            case DC_APP_XML_ELEMENT_TYPE_STENCIL_ADD:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL_REMOVE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL_DRAW:
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
                return true;
            default:
                return false;
        }
    }

    //- stencil operation children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_STENCIL_ADD ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_STENCIL_REMOVE ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_STENCIL_DRAW) {
        switch (child_type) {
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
                return true;
            default:
                return false;
        }
    }

    //- template children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_DEFAULT || parent_type == DC_APP_XML_ELEMENT_TYPE_STYLE) {
        // allow any element type as a template
        return true;
    }

    //- trick io children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_TRICK_IO) {
        switch (child_type) {
            case DC_APP_XML_ELEMENT_TYPE_TRICK_FROM:
            case DC_APP_XML_ELEMENT_TYPE_TRICK_TO:
                return true;
            default:
                return false;
        }
    }

    //- trick direction children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_TRICK_FROM ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_TRICK_TO) {
        switch (child_type) {
            case DC_APP_XML_ELEMENT_TYPE_TRICK_VARIABLE:
                return true;
            default:
                return false;
        }
    }

    //- edge io children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_EDGE_IO) {
        switch (child_type) {
            case DC_APP_XML_ELEMENT_TYPE_EDGE_FROM:
            case DC_APP_XML_ELEMENT_TYPE_EDGE_TO:
                return true;
            default:
                return false;
        }
    }

    //- edge direction children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_EDGE_FROM ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_EDGE_TO) {
        switch (child_type) {
            case DC_APP_XML_ELEMENT_TYPE_EDGE_VARIABLE:
                return true;
            default:
                return false;
        }
    }

    //- planet children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_PLANET) {
        switch (child_type) {
            case DC_APP_XML_ELEMENT_TYPE_PLANET_DATA:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_TEXTURE:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_SHADER:
                return true;
            default:
                return false;
        }
    }

    //- planet view children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW) {
        switch (child_type) {
            case DC_APP_XML_ELEMENT_TYPE_PLANET_BREADCRUMBS:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_GEO_JSON:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_LINE:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_TEXT:
                return true;
            default:
                return false;
        }
    }

    //- planet container children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_PLANET_CONTAINER) {
        return child_type == DC_APP_XML_ELEMENT_TYPE_PLANET_LINE ||
               child_type == DC_APP_XML_ELEMENT_TYPE_PLANET_POLYGON ||
               child_type == DC_APP_XML_ELEMENT_TYPE_PLANET_TEXT;
    }

    //- planet primitive children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_PLANET_LINE ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_PLANET_POLYGON) {
        return child_type == DC_APP_XML_ELEMENT_TYPE_VERTEX;
    }

    //- polygon children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_POLYGON) {
        switch (child_type) {
            case DC_APP_XML_ELEMENT_TYPE_VERTEX:
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
            // mouse events
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_ACTIVE:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_INACTIVE:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_HOVERED:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_PRESSED:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_RELEASED:
                return true;
            default:
                return false;
        }
    }

    //- line children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_LINE) {
        switch (child_type) {
            case DC_APP_XML_ELEMENT_TYPE_VERTEX:
                return true;
            default:
                return false;
        }
    }

    //- shape children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_ELLIPSE ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_IMAGE ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_RECTANGLE) {
        switch (child_type) {
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
            // mouse events
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_ACTIVE:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_INACTIVE:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_HOVERED:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_PRESSED:
            case DC_APP_XML_ELEMENT_TYPE_MOUSE_RELEASED:
                return true;
            default:
                return false;
        }
    }

    //- mouse event children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_MOUSE_ACTIVE ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_MOUSE_INACTIVE ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_MOUSE_HOVERED ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_MOUSE_PRESSED ||
        parent_type == DC_APP_XML_ELEMENT_TYPE_MOUSE_RELEASED) {
        switch (child_type) {
            // drawing elements
            case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            case DC_APP_XML_ELEMENT_TYPE_LINE:
            case DC_APP_XML_ELEMENT_TYPE_ARC:
            case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            case DC_APP_XML_ELEMENT_TYPE_TEXT:
            case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            case DC_APP_XML_ELEMENT_TYPE_STENCIL:
            case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            case DC_APP_XML_ELEMENT_TYPE_BLINK:
            case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            // logic elements
            case DC_APP_XML_ELEMENT_TYPE_IF:
            case DC_APP_XML_ELEMENT_TYPE_SET:
                return true;
            default:
                return false;
        }
    }

    //- leaf elements
    switch (parent_type) {
        case DC_APP_XML_ELEMENT_TYPE_TEXT:
        case DC_APP_XML_ELEMENT_TYPE_ARC:
        case DC_APP_XML_ELEMENT_TYPE_ARG:
        case DC_APP_XML_ELEMENT_TYPE_SPHERE:
        case DC_APP_XML_ELEMENT_TYPE_CONSTANT:
        case DC_APP_XML_ELEMENT_TYPE_VARIABLE:
        case DC_APP_XML_ELEMENT_TYPE_SET:
        case DC_APP_XML_ELEMENT_TYPE_TRICK_FROM:
        case DC_APP_XML_ELEMENT_TYPE_TRICK_TO:
        case DC_APP_XML_ELEMENT_TYPE_VERTEX:
        case DC_APP_XML_ELEMENT_TYPE_PLANET_DATA:
        case DC_APP_XML_ELEMENT_TYPE_PLANET_TEXTURE:
        case DC_APP_XML_ELEMENT_TYPE_PLANET_SHADER:
        case DC_APP_XML_ELEMENT_TYPE_LOGIC:
        case DC_APP_XML_ELEMENT_TYPE_FUNCTION:
        case DC_APP_XML_ELEMENT_TYPE_DRAW_FUNCTION:
        case DC_APP_XML_ELEMENT_TYPE_MOUSE_MOTION:
            return false;
        default:
            break;
    }

    //- document root children
    if (parent_type == DC_APP_XML_ELEMENT_TYPE_NONELEM) {
        return child_type == DC_APP_XML_ELEMENT_TYPE_DCAPP;
    }

    // allow unknown parents for forward compatibility
    return true;
}

//~ required attributes

void _validate_required_attributes(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType elem_type) {

    //- declarations and controls
    switch (elem_type) {

        case DC_APP_XML_ELEMENT_TYPE_VARIABLE: {
            xmlChar *content = xmlNodeGetContent(node);
            if (content) {
                dc_utils_trim_whitespace_inplace((char *)content);
            }
            if (!content || content[0] == '\0') {
                DC_LOG_ERROR("Validate", "<Variable> missing variable name (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else if (!dc_utils_string_is_c_identifier((const char *)content)) {
                DC_LOG_ERROR(
                    "Validate",
                    "<Variable> name '%s' is not a valid C identifier for a generated symbol (line %ld)",
                    (const char *)content,
                    xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (content)
                xmlFree(content);
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_ARG: {
            xmlChar *type = xmlGetProp(node, BAD_CAST "Type");
            if (!type) {
                DC_LOG_ERROR("Validate", "<Arg> missing required attribute 'Type' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(type);
            }
            xmlChar *value = xmlGetProp(node, BAD_CAST "Value");
            if (!value) {
                DC_LOG_ERROR("Validate", "<Arg> missing required attribute 'Value' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(value);
            }
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_CONSTANT: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                DC_LOG_ERROR("Validate", "<Constant> missing required attribute 'Name' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(name);
            }
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_WINDOW:
            // title is optional
            break;

        case DC_APP_XML_ELEMENT_TYPE_PANEL: {
            xmlChar *vw = xmlGetProp(node, BAD_CAST "VirtualWidth");
            if (!vw)
                vw = xmlGetProp(node, BAD_CAST "VirtualDimensionX");
            xmlChar *vh = xmlGetProp(node, BAD_CAST "VirtualHeight");
            if (!vh)
                vh = xmlGetProp(node, BAD_CAST "VirtualDimensionY");
            if (!vw || !vh) {
                DC_LOG_WARN("Validate", "<Panel> missing 'VirtualWidth' or 'VirtualHeight' attribute (line %ld)", xmlGetLineNo(node));
                ctx->warning_count++;
            }
            if (vw)
                xmlFree(vw);
            if (vh)
                xmlFree(vh);
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_SET: {
            xmlChar *var = xmlGetProp(node, BAD_CAST "Variable");
            if (!var) {
                DC_LOG_ERROR("Validate", "<Set> missing required attribute 'Variable' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(var);
            }
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_IF: {
            xmlChar *value = xmlGetProp(node, BAD_CAST "Value");
            if (!value) {
                value = xmlGetProp(node, BAD_CAST "Value1");
            }
            if (!value) {
                DC_LOG_ERROR("Validate", "<%s> missing required attribute 'Value' or 'Value1' (line %ld)",
                             node->name, xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(value);
            }
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_STYLE: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                DC_LOG_ERROR("Validate", "<Style> missing required attribute 'Name' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(name);
            }
            break;
        }

            //- data links

        case DC_APP_XML_ELEMENT_TYPE_TRICK_IO: {
            xmlChar *host = xmlGetProp(node, BAD_CAST "Host");
            xmlChar *port = xmlGetProp(node, BAD_CAST "Port");
            if (!host) {
                DC_LOG_WARN("Validate", "<TrickIO> missing 'Host' attribute (line %ld)", xmlGetLineNo(node));
                ctx->warning_count++;
            }
            if (!port) {
                DC_LOG_WARN("Validate", "<TrickIO> missing 'Port' attribute (line %ld)", xmlGetLineNo(node));
                ctx->warning_count++;
            }
            if (host)
                xmlFree(host);
            if (port)
                xmlFree(port);
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_TRICK_VARIABLE: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                DC_LOG_ERROR("Validate", "<TrickVariable> missing required attribute 'Name' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(name);
            }
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_EDGE_IO: {
            xmlChar *host = xmlGetProp(node, BAD_CAST "Host");
            xmlChar *port = xmlGetProp(node, BAD_CAST "Port");
            if (!host) {
                DC_LOG_WARN("Validate", "<EdgeIO> missing 'Host' attribute (line %ld)", xmlGetLineNo(node));
                ctx->warning_count++;
            }
            if (!port) {
                DC_LOG_WARN("Validate", "<EdgeIO> missing 'Port' attribute (line %ld)", xmlGetLineNo(node));
                ctx->warning_count++;
            }
            if (host)
                xmlFree(host);
            if (port)
                xmlFree(port);
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_EDGE_VARIABLE: {
            // edge variables may use content instead of a command attribute
            break;
        }

            //- resources callbacks and input

        case DC_APP_XML_ELEMENT_TYPE_IMAGE: {
            xmlChar *file = xmlGetProp(node, BAD_CAST "File");
            if (!file) {
                DC_LOG_ERROR("Validate", "<Image> missing required attribute 'File' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(file);
            }
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_LOGIC: {
            xmlChar *file = xmlGetProp(node, BAD_CAST "File");
            if (!file) {
                DC_LOG_ERROR("Validate", "<Logic> missing required attribute 'File' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(file);
            }
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_FUNCTION:
        case DC_APP_XML_ELEMENT_TYPE_DRAW_FUNCTION: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name || name[0] == '\0') {
                DC_LOG_ERROR("Validate", "<%s> missing required attribute 'Name' (line %ld)", (const char *)node->name, xmlGetLineNo(node));
                ctx->error_count++;
            } else if (!dc_utils_string_is_c_identifier((const char *)name)) {
                DC_LOG_ERROR(
                    "Validate",
                    "<%s> Name '%s' is not a valid C identifier for a generated symbol (line %ld)",
                    (const char *)node->name,
                    (const char *)name,
                    xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (name)
                xmlFree(name);
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_BLINK: {
            xmlChar *var = xmlGetProp(node, BAD_CAST "FireBlink");
            if (!var) {
                DC_LOG_ERROR("Validate", "<Blink> missing required attribute 'FireBlink' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(var);
            }
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_MOUSE_MOTION: {
            xmlChar *vx = xmlGetProp(node, BAD_CAST "VariableX");
            xmlChar *vy = xmlGetProp(node, BAD_CAST "VariableY");
            if (!vx && !vy) {
                DC_LOG_WARN("Validate", "<MouseMotion> has no VariableX or VariableY (line %ld)", xmlGetLineNo(node));
                ctx->warning_count++;
            }
            if (vx)
                xmlFree(vx);
            if (vy)
                xmlFree(vy);
            break;
        }

            //- planet and stream nodes

        case DC_APP_XML_ELEMENT_TYPE_PLANET: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                DC_LOG_ERROR("Validate", "<Planet> missing required attribute 'Name' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                if (_is_variable_ref((const char *)name)) {
                    DC_LOG_ERROR("Validate", "<Planet> Name '%s' cannot be a runtime variable (line %ld)", (const char *)name, xmlGetLineNo(node));
                    ctx->error_count++;
                }
                xmlFree(name);
            }

            int texture_count = 0;
            for (xmlNodePtr child = node->children; child; child = child->next) {
                if (dc_app_xml_element_type_from_xml_node(child) == DC_APP_XML_ELEMENT_TYPE_PLANET_TEXTURE && ++texture_count > 5) {
                    DC_LOG_ERROR("Validate", "<Planet> supports at most five <PlanetTexture> elements (line %ld)", xmlGetLineNo(child));
                    ctx->error_count++;
                    break;
                }
            }
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW: {
            xmlChar *planet = xmlGetProp(node, BAD_CAST "Planet");
            if (!planet) {
                DC_LOG_ERROR("Validate", "<PlanetView> missing required attribute 'Planet' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(planet);
            }
            xmlChar *crs = xmlGetProp(node, BAD_CAST "CRS");
            if (!crs) {
                DC_LOG_ERROR("Validate", "<PlanetView> missing required attribute 'CRS' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }

            xmlChar *lat = xmlGetProp(node, BAD_CAST "CameraLatitude");
            xmlChar *lon = xmlGetProp(node, BAD_CAST "CameraLongitude");
            xmlChar *ele = xmlGetProp(node, BAD_CAST "CameraElevation");
            xmlChar *hdg = xmlGetProp(node, BAD_CAST "CameraHeading");
            xmlChar *attitude_frame = xmlGetProp(node, BAD_CAST "AttitudeFrame");
            xmlChar *cam_x = xmlGetProp(node, BAD_CAST "CameraX");
            xmlChar *cam_y = xmlGetProp(node, BAD_CAST "CameraY");
            xmlChar *cam_z = xmlGetProp(node, BAD_CAST "CameraZ");
            xmlChar *roll = xmlGetProp(node, BAD_CAST "CameraRoll");
            xmlChar *pitch = xmlGetProp(node, BAD_CAST "CameraPitch");
            xmlChar *yaw = xmlGetProp(node, BAD_CAST "CameraYaw");
            bool has_lle = lat || lon || ele;
            bool has_xyz = cam_x || cam_y || cam_z;
            bool complete_lle = lat && lon && ele;
            bool complete_xyz = cam_x && cam_y && cam_z;
            if (has_lle && !complete_lle) {
                DC_LOG_ERROR("Validate", "<PlanetView> incomplete geodetic camera; CameraLatitude, CameraLongitude, and CameraElevation are required together (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (has_xyz && !complete_xyz) {
                DC_LOG_ERROR("Validate", "<PlanetView> incomplete cartesian camera; CameraX, CameraY, and CameraZ are required together (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (has_lle && has_xyz) {
                DC_LOG_ERROR("Validate", "<PlanetView> cannot mix geodetic and cartesian camera attributes (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (hdg && yaw) {
                DC_LOG_ERROR("Validate", "<PlanetView> CameraHeading is a legacy alias for CameraYaw; do not specify both (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (crs) {
                int crs_value = atoi((const char *)crs);
                int attitude_frame_value = attitude_frame ? atoi((const char *)attitude_frame) : (crs_value == DC_APP_PLANET_CRS_GEODETIC ? DC_APP_PLANET_ATTITUDE_FRAME_LOCAL_NED : DC_APP_PLANET_ATTITUDE_FRAME_CARTESIAN_RPY);
                if (hdg && crs_value == DC_APP_PLANET_CRS_CARTESIAN) {
                    DC_LOG_ERROR("Validate", "<PlanetView CRS cartesian> cannot use legacy CameraHeading; use CameraYaw instead (line %ld)", xmlGetLineNo(node));
                    ctx->error_count++;
                }
                if (crs_value == DC_APP_PLANET_CRS_GEODETIC) {
                    if (!has_lle) {
                        DC_LOG_ERROR("Validate", "<PlanetView CRS geodetic> requires CameraLatitude, CameraLongitude, and CameraElevation (line %ld)", xmlGetLineNo(node));
                        ctx->error_count++;
                    }
                    if (has_xyz) {
                        DC_LOG_ERROR("Validate", "<PlanetView CRS geodetic> cannot use cartesian camera attributes (line %ld)", xmlGetLineNo(node));
                        ctx->error_count++;
                    }
                    if (attitude_frame_value != DC_APP_PLANET_ATTITUDE_FRAME_LOCAL_NED) {
                        DC_LOG_ERROR("Validate", "<PlanetView CRS geodetic> requires AttitudeFrame #_planet_attitude_frame_local_ned_ (line %ld)", xmlGetLineNo(node));
                        ctx->error_count++;
                    }
                } else if (crs_value == DC_APP_PLANET_CRS_CARTESIAN) {
                    if (!has_xyz) {
                        DC_LOG_ERROR("Validate", "<PlanetView CRS cartesian> requires CameraX, CameraY, and CameraZ (line %ld)", xmlGetLineNo(node));
                        ctx->error_count++;
                    }
                    if (has_lle) {
                        DC_LOG_ERROR("Validate", "<PlanetView CRS cartesian> cannot use geodetic camera attributes (line %ld)", xmlGetLineNo(node));
                        ctx->error_count++;
                    }
                    if (attitude_frame_value != DC_APP_PLANET_ATTITUDE_FRAME_CARTESIAN_RPY) {
                        DC_LOG_ERROR("Validate", "<PlanetView CRS cartesian> requires AttitudeFrame #_planet_attitude_frame_cartesian_rpy_ (line %ld)", xmlGetLineNo(node));
                        ctx->error_count++;
                    }
                } else {
                    DC_LOG_ERROR("Validate", "<PlanetView> invalid CRS '%s' (line %ld)", (const char *)crs, xmlGetLineNo(node));
                    ctx->error_count++;
                }
            }
            if (crs) xmlFree(crs);
            if (attitude_frame) xmlFree(attitude_frame);
            if (lat) xmlFree(lat);
            if (lon) xmlFree(lon);
            if (ele) xmlFree(ele);
            if (hdg) xmlFree(hdg);
            if (cam_x) xmlFree(cam_x);
            if (cam_y) xmlFree(cam_y);
            if (cam_z) xmlFree(cam_z);
            if (roll) xmlFree(roll);
            if (pitch) xmlFree(pitch);
            if (yaw) xmlFree(yaw);
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_PLANET_CONTAINER: {
            xmlChar *lat = xmlGetProp(node, BAD_CAST "Latitude");
            xmlChar *lon = xmlGetProp(node, BAD_CAST "Longitude");
            if (!lat) {
                DC_LOG_ERROR("Validate", "<PlanetContainer> missing required attribute 'Latitude' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (!lon) {
                DC_LOG_ERROR("Validate", "<PlanetContainer> missing required attribute 'Longitude' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (lat) xmlFree(lat);
            if (lon) xmlFree(lon);
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_PLANET_DATA: {
            xmlChar *file = xmlGetProp(node, BAD_CAST "File");
            if (!file) {
                DC_LOG_ERROR("Validate", "<PlanetData> missing required attribute 'File' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(file);
            }
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_PLANET_TEXTURE:
            // all attributes may be supplied dynamically at runtime
            break;

        case DC_APP_XML_ELEMENT_TYPE_PLANET_IMAGE: {
            xmlChar *file = xmlGetProp(node, BAD_CAST "File");
            xmlChar *content = NULL;
            if (!file) {
                content = xmlNodeGetContent(node);
            }
            if ((!file || xmlStrlen(file) == 0) && (!content || xmlStrlen(content) == 0)) {
                DC_LOG_ERROR("Validate", "<PlanetImage> missing required attribute 'File' or text content (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (file) xmlFree(file);
            if (content) xmlFree(content);

            xmlChar *width = xmlGetProp(node, BAD_CAST "Width");
            if (!width) width = xmlGetProp(node, BAD_CAST "DimensionX");
            if (!width) width = xmlGetProp(node, BAD_CAST "Size");
            xmlChar *height = xmlGetProp(node, BAD_CAST "Height");
            if (!height) height = xmlGetProp(node, BAD_CAST "DimensionY");
            if (!width && !height) {
                DC_LOG_ERROR("Validate", "<PlanetImage> missing required attribute 'Width', 'Height', or 'Size' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (width) xmlFree(width);
            if (height) xmlFree(height);
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_PLANET_SHADER: {
            xmlChar *index = xmlGetProp(node, BAD_CAST "Index");
            if (!index) {
                DC_LOG_ERROR("Validate", "<PlanetShader> missing required attribute 'Index' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(index);
            }
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM: {
            xmlChar *type = xmlGetProp(node, BAD_CAST "Type");
            if (!type) {
                DC_LOG_ERROR("Validate", "<PixelStream> missing required attribute 'Type' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(type);
            }
            break;
        }

        default:
            break;
    }
}

//~ allowed attributes

// check whether an attribute appears in a schema list
static bool _attr_in_list(const char *attr_name, const char **list) {
    if (!list)
        return false;
    for (int i = 0; list[i] != NULL; i++) {
        if (strcmp(attr_name, list[i]) == 0)
            return true;
    }
    return false;
}

// check whether an attribute belongs to an element schema
static bool _is_valid_attr_for_elem(const char *attr_name, DcAppXmlElementType elem_type) {

    // accept shared attributes first
    if (_attr_in_list(attr_name, _valid_attrs_common))
        return true;

    //- core and two dimensional nodes
    switch (elem_type) {
        case DC_APP_XML_ELEMENT_TYPE_ARC:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   _attr_in_list(attr_name, _valid_attrs_arc);

        case DC_APP_XML_ELEMENT_TYPE_ARG:
            return _attr_in_list(attr_name, _valid_attrs_arg);

        case DC_APP_XML_ELEMENT_TYPE_BLINK:
            return _attr_in_list(attr_name, _valid_attrs_blink);

        case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_virtual_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_button);

        case DC_APP_XML_ELEMENT_TYPE_BUTTON_PRESSED:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_RELEASED:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_ENABLED:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_DISABLED:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_TRANSITION:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_INDICATOR_ON:
        case DC_APP_XML_ELEMENT_TYPE_BUTTON_INDICATOR_OFF:
            return true; // no specific attributes beyond common ones

        case DC_APP_XML_ELEMENT_TYPE_CONSTANT:
            return _attr_in_list(attr_name, _valid_attrs_constant);

        case DC_APP_XML_ELEMENT_TYPE_CONTAINER:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_virtual_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation);

        case DC_APP_XML_ELEMENT_TYPE_DCAPP:
            return true; // dcapp accepts arbitrary configuration attributes

        case DC_APP_XML_ELEMENT_TYPE_ELLIPSE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   _attr_in_list(attr_name, _valid_attrs_ellipse);

        case DC_APP_XML_ELEMENT_TYPE_DEFAULT:
        case DC_APP_XML_ELEMENT_TYPE_STYLE:
            return true; // templates accept attributes for their target elements

        case DC_APP_XML_ELEMENT_TYPE_FALSE:
        case DC_APP_XML_ELEMENT_TYPE_TRUE:
            return true; // conditional branches only wrap content

        case DC_APP_XML_ELEMENT_TYPE_FUNCTION:
            return _attr_in_list(attr_name, _valid_attrs_function);

        case DC_APP_XML_ELEMENT_TYPE_DRAW_FUNCTION:
            return _attr_in_list(attr_name, _valid_attrs_draw_function);

        case DC_APP_XML_ELEMENT_TYPE_IF:
            return _attr_in_list(attr_name, _valid_attrs_if);

        case DC_APP_XML_ELEMENT_TYPE_IMAGE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_image);

        case DC_APP_XML_ELEMENT_TYPE_INCLUDE:
            return strcmp(attr_name, "File") == 0 || strcmp(attr_name, "Optional") == 0;

        case DC_APP_XML_ELEMENT_TYPE_LINE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line);

        case DC_APP_XML_ELEMENT_TYPE_LOGIC:
            return _attr_in_list(attr_name, _valid_attrs_logic);

        case DC_APP_XML_ELEMENT_TYPE_MOUSE_ACTIVE:
        case DC_APP_XML_ELEMENT_TYPE_MOUSE_HOVERED:
        case DC_APP_XML_ELEMENT_TYPE_MOUSE_INACTIVE:
        case DC_APP_XML_ELEMENT_TYPE_MOUSE_PRESSED:
        case DC_APP_XML_ELEMENT_TYPE_MOUSE_RELEASED:
            return true; // mouse event elements only wrap content

        case DC_APP_XML_ELEMENT_TYPE_MOUSE_MOTION:
            return _attr_in_list(attr_name, _valid_attrs_mouse_motion);

        case DC_APP_XML_ELEMENT_TYPE_PANEL:
            return _attr_in_list(attr_name, _valid_attrs_virtual_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_panel);

        case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_pixelstream);

        case DC_APP_XML_ELEMENT_TYPE_POLYGON:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   _attr_in_list(attr_name, _valid_attrs_rounded);

        case DC_APP_XML_ELEMENT_TYPE_RECTANGLE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   _attr_in_list(attr_name, _valid_attrs_rounded);

        case DC_APP_XML_ELEMENT_TYPE_SET:
            return _attr_in_list(attr_name, _valid_attrs_set);

        case DC_APP_XML_ELEMENT_TYPE_SPHERE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_sphere);

        case DC_APP_XML_ELEMENT_TYPE_STENCIL:
        case DC_APP_XML_ELEMENT_TYPE_STENCIL_ADD:
        case DC_APP_XML_ELEMENT_TYPE_STENCIL_REMOVE:
        case DC_APP_XML_ELEMENT_TYPE_STENCIL_DRAW:
            return true; // stencil elements only wrap content

            //- planet nodes

        case DC_APP_XML_ELEMENT_TYPE_PLANET:
            return _attr_in_list(attr_name, _valid_attrs_planet);

        case DC_APP_XML_ELEMENT_TYPE_PLANET_VIEW:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_planet_view);

        case DC_APP_XML_ELEMENT_TYPE_PLANET_CONTAINER:
            return _attr_in_list(attr_name, _valid_attrs_planet_container);

        case DC_APP_XML_ELEMENT_TYPE_PLANET_DATA:
            return _attr_in_list(attr_name, _valid_attrs_planet_data);

        case DC_APP_XML_ELEMENT_TYPE_PLANET_TEXTURE:
            return _attr_in_list(attr_name, _valid_attrs_planet_texture);

        case DC_APP_XML_ELEMENT_TYPE_PLANET_SHADER:
            return _attr_in_list(attr_name, _valid_attrs_planet_shader);

        case DC_APP_XML_ELEMENT_TYPE_PLANET_BREADCRUMBS:
            return _attr_in_list(attr_name, _valid_attrs_planet_overlay) ||
                   _attr_in_list(attr_name, _valid_attrs_planet_breadcrumbs) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   strcmp(attr_name, "LinePattern") == 0;

        case DC_APP_XML_ELEMENT_TYPE_PLANET_LINE:
        case DC_APP_XML_ELEMENT_TYPE_PLANET_POLYGON:
            return _attr_in_list(attr_name, _valid_attrs_planet_overlay) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   strcmp(attr_name, "LinePattern") == 0;

        case DC_APP_XML_ELEMENT_TYPE_PLANET_ELLIPSE:
        case DC_APP_XML_ELEMENT_TYPE_PLANET_SPHERE:
        case DC_APP_XML_ELEMENT_TYPE_PLANET_TEXT:
            return _attr_in_list(attr_name, _valid_attrs_planet_overlay) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line);

        case DC_APP_XML_ELEMENT_TYPE_PLANET_IMAGE:
            return _attr_in_list(attr_name, _valid_attrs_planet_overlay) ||
                   _attr_in_list(attr_name, _valid_attrs_planet_image) ||
                   _attr_in_list(attr_name, _valid_attrs_color);

        case DC_APP_XML_ELEMENT_TYPE_PLANET_GEO_JSON:
            return _attr_in_list(attr_name, _valid_attrs_planet_geojson) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line);

            //- text and data links

        case DC_APP_XML_ELEMENT_TYPE_TEXT:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_text);

        case DC_APP_XML_ELEMENT_TYPE_TRICK_IO:
            return _attr_in_list(attr_name, _valid_attrs_trick_io);

        case DC_APP_XML_ELEMENT_TYPE_TRICK_FROM:
        case DC_APP_XML_ELEMENT_TYPE_TRICK_TO:
            return true; // direction elements only map variables

        case DC_APP_XML_ELEMENT_TYPE_TRICK_VARIABLE:
            return _attr_in_list(attr_name, _valid_attrs_trick_variable);

        case DC_APP_XML_ELEMENT_TYPE_EDGE_IO:
            return _attr_in_list(attr_name, _valid_attrs_edge_io);

        case DC_APP_XML_ELEMENT_TYPE_EDGE_FROM:
        case DC_APP_XML_ELEMENT_TYPE_EDGE_TO:
            return true; // direction elements only group variables

        case DC_APP_XML_ELEMENT_TYPE_EDGE_VARIABLE:
            return _attr_in_list(attr_name, _valid_attrs_edge_variable);

            //- variables vertices and window

        case DC_APP_XML_ELEMENT_TYPE_VARIABLE:
            return _attr_in_list(attr_name, _valid_attrs_variable);

        case DC_APP_XML_ELEMENT_TYPE_VERTEX:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_planet_vertex);

        case DC_APP_XML_ELEMENT_TYPE_WINDOW:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_virtual_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_window);

        case DC_APP_XML_ELEMENT_TYPE_DUMMY:
        case DC_APP_XML_ELEMENT_TYPE_NONELEM:
        case DC_APP_XML_ELEMENT_TYPE_UNDEFINED:
            return true;

        default:
            return true; // allow unknown elements for forward compatibility
    }
}

//- declared attribute names

void _validate_attribute_names(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType elem_type) {

    // inspect every declared attribute
    xmlAttr *attr = node->properties;
    while (attr) {
        const char *attr_name = (const char *)attr->name;

        // ignore disabled attributes prefixed with an underscore
        if (attr_name[0] == '_') {
            attr = attr->next;
            continue;
        }

        // warn on attributes outside the element schema
        if (!_is_valid_attr_for_elem(attr_name, elem_type)) {
            DC_LOG_WARN("Validate", "<%s> has unrecognized attribute '%s' (line %ld)",
                        node->name, attr_name, xmlGetLineNo(node));
            ctx->warning_count++;
        }

        attr = attr->next;
    }
}

//~ attribute values

// identify runtime variable references by their prefix
static bool _is_variable_ref(const char *value) {
    return value && value[0] == '@';
}

// validate a complete integer within an inclusive range
static bool _is_valid_int_in_range(const char *value, int min, int max) {
    if (!value || value[0] == '\0')
        return false;
    char *end;
    long val = strtol(value, &end, 10);
    if (*end != '\0')
        return false; // trailing text makes the integer invalid
    return val >= min && val <= max;
}

// validate an expanded enum value
static void _validate_enum_attr(ValidationContext *ctx, xmlNodePtr node, const char *attr_name,
                                int min_val, int max_val, const char *valid_values_desc) {
    xmlChar *raw_value = xmlGetProp(node, BAD_CAST attr_name);
    if (!raw_value)
        return; // absent attributes need no validation

    const char *value = (const char *)raw_value;

    // defer runtime variable references
    if (_is_variable_ref(value)) {
        xmlFree(raw_value);
        return;
    }

    // require a valid enum integer
    if (!_is_valid_int_in_range(value, min_val, max_val)) {
        DC_LOG_ERROR("Validate", "<%s> attribute '%s' has invalid value '%s' (line %ld). Valid values: %s",
                     node->name, attr_name, value, xmlGetLineNo(node), valid_values_desc);
        ctx->error_count++;
    }

    xmlFree(raw_value);
}

void _validate_attribute_values(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType elem_type) {

    //- shared alignment values

    // validate horizontal alignment values
    static const char *align_x_attrs[] = {"LocalAlignX", "ParentAlignX", "PivotLocalAlignX", "HorizontalAlign", NULL};
    for (int i = 0; align_x_attrs[i]; i++) {
        _validate_enum_attr(ctx, node, align_x_attrs[i], 1, 3,
                            "left(1), center(2), right(3)");
    }

    // validate vertical alignment values
    static const char *align_y_attrs[] = {"LocalAlignY", "ParentAlignY", "PivotLocalAlignY", "VerticalAlign", NULL};
    for (int i = 0; align_y_attrs[i]; i++) {
        _validate_enum_attr(ctx, node, align_y_attrs[i], 4, 6,
                            "bottom(4), middle(5), top(6)");
    }

    //- element-specific enums
    switch (elem_type) {
        case DC_APP_XML_ELEMENT_TYPE_IF: {
            _validate_enum_attr(ctx, node, "Operator", 1, 8,
                                "true(1), false(2), eq(3), ne(4), lt(5), gt(6), lte(7), gte(8)");

            // static conditions cannot depend on runtime variables
            xmlChar *static_attr = xmlGetProp(node, BAD_CAST "Static");
            if (static_attr && (strcmp((const char *)static_attr, "true") == 0 || strcmp((const char *)static_attr, "1") == 0)) {
                xmlChar *value = xmlGetProp(node, BAD_CAST "Value");
                xmlChar *value1 = xmlGetProp(node, BAD_CAST "Value1");
                xmlChar *value2 = xmlGetProp(node, BAD_CAST "Value2");

                if (value && _is_variable_ref((const char *)value)) {
                    DC_LOG_ERROR("Validate", "<If Static=\"true\"> Value '%s' cannot be a runtime variable (@) (line %ld)",
                                 value, xmlGetLineNo(node));
                    ctx->error_count++;
                }
                if (value1 && _is_variable_ref((const char *)value1)) {
                    DC_LOG_ERROR("Validate", "<If Static=\"true\"> Value1 '%s' cannot be a runtime variable (@) (line %ld)",
                                 value1, xmlGetLineNo(node));
                    ctx->error_count++;
                }
                if (value2 && _is_variable_ref((const char *)value2)) {
                    DC_LOG_ERROR("Validate", "<If Static=\"true\"> Value2 '%s' cannot be a runtime variable (@) (line %ld)",
                                 value2, xmlGetLineNo(node));
                    ctx->error_count++;
                }

                if (value) xmlFree(value);
                if (value1) xmlFree(value1);
                if (value2) xmlFree(value2);
            }
            if (static_attr) xmlFree(static_attr);
            break;
        }

        case DC_APP_XML_ELEMENT_TYPE_SET:
            _validate_enum_attr(ctx, node, "Operator", 1, 10,
                                "equal(1), add(2), subtract(3), multiply(4), divide(5), min(6), max(7), push(8), pop(9), negate(10)");
            break;

        case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            _validate_enum_attr(ctx, node, "Type", 1, 3,
                                "momentary(1), standard(2), toggle(3)");
            break;

        case DC_APP_XML_ELEMENT_TYPE_VARIABLE:
            _validate_enum_attr(ctx, node, "Type", 1, 4,
                                "string(1), integer(2), double(3), boolean(4)");
            break;

        case DC_APP_XML_ELEMENT_TYPE_ARG:
            _validate_enum_attr(ctx, node, "Type", 1, 4,
                                "string(1), integer(2), double(3), boolean(4)");
            break;

        case DC_APP_XML_ELEMENT_TYPE_PIXELSTREAM:
            _validate_enum_attr(ctx, node, "Type", 1, 2,
                                "dynamic_file(1), mjpeg(2)");
            break;

        default:
            break;
    }
}

//~ variable reference validation

// variable name attributes must not use the legacy dereference prefix
static void _check_var_attr(ValidationContext *ctx, xmlNodePtr node, const char *attr_name) {
    xmlChar *value = xmlGetProp(node, BAD_CAST attr_name);
    if (!value) return;

    if (((const char *)value)[0] == '@') {
        DC_LOG_WARN("Validate", "<%s %s=\"%s\"> should be a variable name, not a dereferenced variable (remove leading '@') (line %ld)",
                    node->name, attr_name, value, xmlGetLineNo(node));
        ctx->warning_count++;
    }
    xmlFree(value);
}

void _validate_variable_references(ValidationContext *ctx, xmlNodePtr node, DcAppXmlElementType elem_type) {
    switch (elem_type) {
        case DC_APP_XML_ELEMENT_TYPE_BLINK:
            _check_var_attr(ctx, node, "Variable");
            break;

        case DC_APP_XML_ELEMENT_TYPE_BUTTON:
            _check_var_attr(ctx, node, "Variable");
            _check_var_attr(ctx, node, "TargetVariable");
            _check_var_attr(ctx, node, "IndicatorVariable");
            _check_var_attr(ctx, node, "EnableVariable");
            break;

        case DC_APP_XML_ELEMENT_TYPE_SET:
            _check_var_attr(ctx, node, "Variable");
            break;

        case DC_APP_XML_ELEMENT_TYPE_MOUSE_MOTION:
            _check_var_attr(ctx, node, "VariableX");
            _check_var_attr(ctx, node, "VariableY");
            break;

        case DC_APP_XML_ELEMENT_TYPE_EDGE_IO:
            _check_var_attr(ctx, node, "ConnectedVariable");
            break;

        case DC_APP_XML_ELEMENT_TYPE_TRICK_IO:
            _check_var_attr(ctx, node, "ConnectedVariable");
            break;

        default:
            break;
    }
}
