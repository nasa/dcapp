#include "../src/app/config.h"
#include "../src/app/elem.h"
#include "../src/app/lookup.h"
#include "../src/utils/env.h"
#include "../src/utils/file.h"
#include "../src/utils/log.h"

#include <libxml/parser.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// validation context
typedef struct {
    int error_count;
    int warning_count;
} ValidationContext;

// forward declarations
static void _validate_node(ValidationContext *ctx, xmlNodePtr node, DcAppElemType parent_type);
static void _validate_children(ValidationContext *ctx, xmlNodePtr node, DcAppElemType parent_type);
static bool _is_valid_child(DcAppElemType parent_type, DcAppElemType child_type);
static void _validate_required_attributes(ValidationContext *ctx, xmlNodePtr node, DcAppElemType elem_type);
static void _validate_attribute_names(ValidationContext *ctx, xmlNodePtr node, DcAppElemType elem_type);
static void _validate_attribute_values(ValidationContext *ctx, xmlNodePtr node, DcAppElemType elem_type);
static void _validate_variable_references(ValidationContext *ctx, xmlNodePtr node, DcAppElemType elem_type);
static bool _is_variable_ref(const char *value);

int main(int argc, char **argv) {

    if (argc < 2) {
        DC_LOG_ERROR("Validate", "Usage: dcapp-validate <config.xml> [--preprocessed <output.xml>] [CONSTANT=value ...]");
        return 1;
    }

    // parse --preprocessed flag (before constants)
    const char *preprocessed_output = NULL;
    int         const_count         = 0;
    char      **const_args          = NULL;

    for (int ii = 2; ii < argc; ii++) {
        if (strcmp(argv[ii], "--preprocessed") == 0 && ii + 1 < argc) {
            preprocessed_output = argv[++ii];
        }
    }

    // collect constant args (skip --preprocessed and its value)
    if (argc > 2) {
        const_args = (char **)malloc(sizeof(char *) * (argc - 2));
        for (int ii = 2; ii < argc; ii++) {
            if (strcmp(argv[ii], "--preprocessed") == 0 && ii + 1 < argc) {
                ii++; // skip value
                continue;
            }
            const_args[const_count++] = argv[ii];
        }
    }

    // create config
    DcAppConfig *config;
    const char  *config_filepath = argv[1];
    if (const_count > 0) {
        config = dc_app_config_create(config_filepath, const_args, const_count);
    } else {
        config = dc_app_config_create(config_filepath, NULL, 0);
    }
    free(const_args);

    // set environment
    dc_utils_set_env("dcappDisplayHome", config->config_dir_path, 1);
    dc_utils_set_env("dcappHome", config->dcapp_dir_path, 1);

    // create lookup (needed for config_clean_xml)
    DcAppLookup *lookup = dc_app_lookup_create();

    // preprocess XML file (expands includes, constants, staticifs)
    dc_app_config_preprocess_xml(config, lookup);

    // dump preprocessed XML for debugging
    dc_app_config_save_preprocessed(config, preprocessed_output);

    // validate
    ValidationContext ctx       = {0};
    xmlNodePtr        root_node = xmlDocGetRootElement(config->xml_doc);

    _validate_node(&ctx, root_node, DC_APP_ELEM_TYPE_NONELEM);

    // report summary
    DC_LOG_INFO("Validate", "Complete: %d error(s), %d warning(s)", ctx.error_count, ctx.warning_count);

    return ctx.error_count > 0 ? 1 : 0;
}

void _validate_children(ValidationContext *ctx, xmlNodePtr node, DcAppElemType parent_type) {
    xmlNodePtr child = node->children;
    while (child) {
        _validate_node(ctx, child, parent_type);
        child = child->next;
    }
}

void _validate_node(ValidationContext *ctx, xmlNodePtr node, DcAppElemType parent_type) {

    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(node);

    // skip non-element nodes
    if (elem_type == DC_APP_ELEM_TYPE_NONELEM) {
        return;
    }

    // check parent-child relationship
    if (!_is_valid_child(parent_type, elem_type)) {
        DC_LOG_ERROR("Validate", "<%s> is not a valid child of <%s> (line %ld)",
                     node->name, node->parent ? node->parent->name : (xmlChar *)"root", xmlGetLineNo(node));
        ctx->error_count++;
    }

    // validate required attributes for this element type
    _validate_required_attributes(ctx, node, elem_type);

    // validate that all attributes are valid for this element type
    _validate_attribute_names(ctx, node, elem_type);

    // validate attribute values are valid
    _validate_attribute_values(ctx, node, elem_type);

    // validate variable references point to declared variables
    _validate_variable_references(ctx, node, elem_type);

    // recurse into children
    _validate_children(ctx, node, elem_type);
}

bool _is_valid_child(DcAppElemType parent_type, DcAppElemType child_type) {

    // These elements should have been removed during preprocessing
    // If they still exist, the preprocessor failed or they're in an invalid location
    // Note: True/False are valid inside <If> elements (runtime conditionals)
    switch (child_type) {
        case DC_APP_ELEM_TYPE_CONSTANT:
        case DC_APP_ELEM_TYPE_STYLE:
        case DC_APP_ELEM_TYPE_INCLUDE:
        case DC_APP_ELEM_TYPE_DUMMY:
            return false; // These should never exist after preprocessing
        default:
            break;
    }

    // DCAPP root can contain top-level elements
    if (parent_type == DC_APP_ELEM_TYPE_DCAPP) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_WINDOW:
            case DC_APP_ELEM_TYPE_VARIABLE:
            case DC_APP_ELEM_TYPE_TRICK_IO:
            case DC_APP_ELEM_TYPE_EDGE_IO:
            case DC_APP_ELEM_TYPE_LOGIC:
            case DC_APP_ELEM_TYPE_FUNCTION:
            case DC_APP_ELEM_TYPE_PLANET:
                return true;
            default:
                return false;
        }
    }

    // Window can contain panels, drawing elements, and config elements
    if (parent_type == DC_APP_ELEM_TYPE_WINDOW) {
        switch (child_type) {
            // legacy support
            case DC_APP_ELEM_TYPE_PANEL:
            // config elements
            case DC_APP_ELEM_TYPE_VARIABLE:
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            // input elements
            case DC_APP_ELEM_TYPE_BUTTON:
            case DC_APP_ELEM_TYPE_MOUSE_MOTION:
                return true;
            default:
                return false;
        }
    }

    // Panel can contain drawing elements
    if (parent_type == DC_APP_ELEM_TYPE_PANEL) {
        switch (child_type) {
            // config elements
            case DC_APP_ELEM_TYPE_VARIABLE:
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            // input elements
            case DC_APP_ELEM_TYPE_BUTTON:
            case DC_APP_ELEM_TYPE_MOUSE_MOTION:
                return true;
            default:
                return false;
        }
    }

    // Container can contain same as Panel, plus mouse events
    if (parent_type == DC_APP_ELEM_TYPE_CONTAINER) {
        switch (child_type) {
            // config elements
            case DC_APP_ELEM_TYPE_VARIABLE:
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            // input elements
            case DC_APP_ELEM_TYPE_BUTTON:
            case DC_APP_ELEM_TYPE_MOUSE_MOTION:
            // mouse events
            case DC_APP_ELEM_TYPE_MOUSE_ACTIVE:
            case DC_APP_ELEM_TYPE_MOUSE_INACTIVE:
            case DC_APP_ELEM_TYPE_MOUSE_HOVERED:
            case DC_APP_ELEM_TYPE_MOUSE_PRESSED:
            case DC_APP_ELEM_TYPE_MOUSE_RELEASED:
                return true;
            default:
                return false;
        }
    }

    // Blink can contain drawable content
    if (parent_type == DC_APP_ELEM_TYPE_BLINK) {
        switch (child_type) {
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_BUTTON:
            case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
                return true;
            default:
                return false;
        }
    }

    // DrawFunction can contain positional arguments
    if (parent_type == DC_APP_ELEM_TYPE_DRAW_FUNCTION) {
        return child_type == DC_APP_ELEM_TYPE_ARG;
    }

    // Button can contain drawing elements and button state elements
    if (parent_type == DC_APP_ELEM_TYPE_BUTTON) {
        switch (child_type) {
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            // button state elements
            case DC_APP_ELEM_TYPE_BUTTON_PRESSED:
            case DC_APP_ELEM_TYPE_BUTTON_RELEASED:
            case DC_APP_ELEM_TYPE_BUTTON_ENABLED:
            case DC_APP_ELEM_TYPE_BUTTON_DISABLED:
            case DC_APP_ELEM_TYPE_BUTTON_TRANSITION:
            case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_ON:
            case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_OFF:
            // mouse events
            case DC_APP_ELEM_TYPE_MOUSE_ACTIVE:
            case DC_APP_ELEM_TYPE_MOUSE_INACTIVE:
            case DC_APP_ELEM_TYPE_MOUSE_HOVERED:
            case DC_APP_ELEM_TYPE_MOUSE_PRESSED:
            case DC_APP_ELEM_TYPE_MOUSE_RELEASED:
                return true;
            default:
                return false;
        }
    }

    // Button state elements can contain drawable content
    if (parent_type == DC_APP_ELEM_TYPE_BUTTON_PRESSED ||
        parent_type == DC_APP_ELEM_TYPE_BUTTON_RELEASED ||
        parent_type == DC_APP_ELEM_TYPE_BUTTON_ENABLED ||
        parent_type == DC_APP_ELEM_TYPE_BUTTON_DISABLED ||
        parent_type == DC_APP_ELEM_TYPE_BUTTON_TRANSITION ||
        parent_type == DC_APP_ELEM_TYPE_BUTTON_INDICATOR_ON ||
        parent_type == DC_APP_ELEM_TYPE_BUTTON_INDICATOR_OFF) {
        switch (child_type) {
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_BUTTON:
            case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
                return true;
            default:
                return false;
        }
    }

    // If can contain True/False branches and drawable content
    if (parent_type == DC_APP_ELEM_TYPE_IF) {
        switch (child_type) {
            // conditional branches
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            // config elements
            case DC_APP_ELEM_TYPE_VARIABLE:
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_BUTTON:
            case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            // input elements
            case DC_APP_ELEM_TYPE_MOUSE_MOTION:
                return true;
            default:
                return false;
        }
    }

    // True/False (inside If) can contain drawable content
    if (parent_type == DC_APP_ELEM_TYPE_TRUE || parent_type == DC_APP_ELEM_TYPE_FALSE) {
        switch (child_type) {
            // config elements
            case DC_APP_ELEM_TYPE_VARIABLE:
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_BUTTON:
            case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            case DC_APP_ELEM_TYPE_FUNCTION:
            case DC_APP_ELEM_TYPE_MOUSE_MOTION:
                return true;
            default:
                return false;
        }
    }

    // Stencil can contain stencil operations and drawing elements
    if (parent_type == DC_APP_ELEM_TYPE_STENCIL) {
        switch (child_type) {
            // stencil operations
            case DC_APP_ELEM_TYPE_STENCIL_ADD:
            case DC_APP_ELEM_TYPE_STENCIL_REMOVE:
            case DC_APP_ELEM_TYPE_STENCIL_DRAW:
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_BUTTON:
            case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
                return true;
            default:
                return false;
        }
    }

    // Stencil sub-elements
    if (parent_type == DC_APP_ELEM_TYPE_STENCIL_ADD ||
        parent_type == DC_APP_ELEM_TYPE_STENCIL_REMOVE ||
        parent_type == DC_APP_ELEM_TYPE_STENCIL_DRAW) {
        switch (child_type) {
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_BUTTON:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
                return true;
            default:
                return false;
        }
    }

    // Default/Style can contain element templates
    if (parent_type == DC_APP_ELEM_TYPE_DEFAULT || parent_type == DC_APP_ELEM_TYPE_STYLE) {
        // Allow any element type as a template
        return true;
    }

    // TrickIO can contain TrickFrom/TrickTo
    if (parent_type == DC_APP_ELEM_TYPE_TRICK_IO) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_TRICK_FROM:
            case DC_APP_ELEM_TYPE_TRICK_TO:
                return true;
            default:
                return false;
        }
    }

    // TrickFrom/TrickTo can contain TrickVariable
    if (parent_type == DC_APP_ELEM_TYPE_TRICK_FROM ||
        parent_type == DC_APP_ELEM_TYPE_TRICK_TO) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_TRICK_VARIABLE:
                return true;
            default:
                return false;
        }
    }

    // EdgeIO can contain EdgeFrom/EdgeTo
    if (parent_type == DC_APP_ELEM_TYPE_EDGE_IO) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_EDGE_FROM:
            case DC_APP_ELEM_TYPE_EDGE_TO:
                return true;
            default:
                return false;
        }
    }

    // EdgeFrom/EdgeTo can contain EdgeVariable
    if (parent_type == DC_APP_ELEM_TYPE_EDGE_FROM ||
        parent_type == DC_APP_ELEM_TYPE_EDGE_TO) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_EDGE_VARIABLE:
                return true;
            default:
                return false;
        }
    }

    // Planet can contain PlanetData, PlanetTexture, and PlanetShader
    if (parent_type == DC_APP_ELEM_TYPE_PLANET) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_PLANET_DATA:
            case DC_APP_ELEM_TYPE_PLANET_TEXTURE:
            case DC_APP_ELEM_TYPE_PLANET_SHADER:
                return true;
            default:
                return false;
        }
    }

    // PlanetView can contain planet overlay elements
    if (parent_type == DC_APP_ELEM_TYPE_PLANET_VIEW) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_PLANET_ELLIPSE:
            case DC_APP_ELEM_TYPE_PLANET_GEO_JSON:
            case DC_APP_ELEM_TYPE_PLANET_LINE:
            case DC_APP_ELEM_TYPE_PLANET_POLYGON:
            case DC_APP_ELEM_TYPE_PLANET_SPHERE:
            case DC_APP_ELEM_TYPE_PLANET_TEXT:
                return true;
            default:
                return false;
        }
    }

    // Polygon can contain Vertex, drawable content, and mouse events
    if (parent_type == DC_APP_ELEM_TYPE_POLYGON) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_VERTEX:
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_BUTTON:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            // mouse events
            case DC_APP_ELEM_TYPE_MOUSE_ACTIVE:
            case DC_APP_ELEM_TYPE_MOUSE_INACTIVE:
            case DC_APP_ELEM_TYPE_MOUSE_HOVERED:
            case DC_APP_ELEM_TYPE_MOUSE_PRESSED:
            case DC_APP_ELEM_TYPE_MOUSE_RELEASED:
                return true;
            default:
                return false;
        }
    }

    // Line can contain Vertex
    if (parent_type == DC_APP_ELEM_TYPE_LINE) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_VERTEX:
                return true;
            default:
                return false;
        }
    }

    // Shapes that can contain drawable content and mouse events
    if (parent_type == DC_APP_ELEM_TYPE_ELLIPSE ||
        parent_type == DC_APP_ELEM_TYPE_IMAGE ||
        parent_type == DC_APP_ELEM_TYPE_PIXELSTREAM ||
        parent_type == DC_APP_ELEM_TYPE_RECTANGLE) {
        switch (child_type) {
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_BUTTON:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            // mouse events
            case DC_APP_ELEM_TYPE_MOUSE_ACTIVE:
            case DC_APP_ELEM_TYPE_MOUSE_INACTIVE:
            case DC_APP_ELEM_TYPE_MOUSE_HOVERED:
            case DC_APP_ELEM_TYPE_MOUSE_PRESSED:
            case DC_APP_ELEM_TYPE_MOUSE_RELEASED:
                return true;
            default:
                return false;
        }
    }

    // Mouse event elements can contain drawable content
    if (parent_type == DC_APP_ELEM_TYPE_MOUSE_ACTIVE ||
        parent_type == DC_APP_ELEM_TYPE_MOUSE_INACTIVE ||
        parent_type == DC_APP_ELEM_TYPE_MOUSE_HOVERED ||
        parent_type == DC_APP_ELEM_TYPE_MOUSE_PRESSED ||
        parent_type == DC_APP_ELEM_TYPE_MOUSE_RELEASED) {
        switch (child_type) {
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_PLANET_VIEW:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_BUTTON:
            // logic elements
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
                return true;
            default:
                return false;
        }
    }

    // Primitives without children (Text, Arc, Sphere, Set, Constant, Variable, etc.)
    switch (parent_type) {
        case DC_APP_ELEM_TYPE_TEXT:
        case DC_APP_ELEM_TYPE_ARC:
        case DC_APP_ELEM_TYPE_ARG:
        case DC_APP_ELEM_TYPE_SPHERE:
        case DC_APP_ELEM_TYPE_CONSTANT:
        case DC_APP_ELEM_TYPE_VARIABLE:
        case DC_APP_ELEM_TYPE_SET:
        case DC_APP_ELEM_TYPE_TRICK_FROM:
        case DC_APP_ELEM_TYPE_TRICK_TO:
        case DC_APP_ELEM_TYPE_VERTEX:
        case DC_APP_ELEM_TYPE_PLANET_DATA:
        case DC_APP_ELEM_TYPE_PLANET_TEXTURE:
        case DC_APP_ELEM_TYPE_PLANET_SHADER:
        case DC_APP_ELEM_TYPE_LOGIC:
        case DC_APP_ELEM_TYPE_FUNCTION:
        case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
        case DC_APP_ELEM_TYPE_MOUSE_MOTION:
            return false;
        default:
            break;
    }

    // Root level (before DCAPP)
    if (parent_type == DC_APP_ELEM_TYPE_NONELEM) {
        return child_type == DC_APP_ELEM_TYPE_DCAPP;
    }

    // Unknown parent - be permissive but warn
    return true;
}

void _validate_required_attributes(ValidationContext *ctx, xmlNodePtr node, DcAppElemType elem_type) {

    // Check for required attributes based on element type
    switch (elem_type) {

        case DC_APP_ELEM_TYPE_VARIABLE: {
            xmlChar *type = xmlGetProp(node, BAD_CAST "Type");
            if (!type) {
                DC_LOG_ERROR("Validate", "<Variable> missing required attribute 'Type' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(type);
            }

            xmlChar *content = xmlNodeGetContent(node);
            if (!content || strlen((char *)content) == 0) {
                DC_LOG_ERROR("Validate", "<Variable> missing variable name (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (content)
                xmlFree(content);
            break;
        }

        case DC_APP_ELEM_TYPE_ARG: {
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

        case DC_APP_ELEM_TYPE_CONSTANT: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                DC_LOG_ERROR("Validate", "<Constant> missing required attribute 'Name' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(name);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_WINDOW:
            // Title is optional
            break;

        case DC_APP_ELEM_TYPE_PANEL: {
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

        case DC_APP_ELEM_TYPE_SET: {
            xmlChar *var = xmlGetProp(node, BAD_CAST "Variable");
            if (!var) {
                DC_LOG_ERROR("Validate", "<Set> missing required attribute 'Variable' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(var);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_IF: {
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

        case DC_APP_ELEM_TYPE_STYLE: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                DC_LOG_ERROR("Validate", "<Style> missing required attribute 'Name' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(name);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_TRICK_IO: {
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

        case DC_APP_ELEM_TYPE_TRICK_VARIABLE: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                DC_LOG_ERROR("Validate", "<TrickVariable> missing required attribute 'Name' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(name);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_EDGE_IO: {
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

        case DC_APP_ELEM_TYPE_EDGE_VARIABLE: {
            // EdgeVariable uses content for variable name, Command attribute is optional
            break;
        }

        case DC_APP_ELEM_TYPE_IMAGE: {
            xmlChar *file = xmlGetProp(node, BAD_CAST "File");
            if (!file) {
                DC_LOG_ERROR("Validate", "<Image> missing required attribute 'File' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(file);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_LOGIC: {
            xmlChar *file = xmlGetProp(node, BAD_CAST "File");
            if (!file) {
                DC_LOG_ERROR("Validate", "<Logic> missing required attribute 'File' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(file);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_FUNCTION:
        case DC_APP_ELEM_TYPE_DRAW_FUNCTION: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                DC_LOG_ERROR("Validate", "<%s> missing required attribute 'Name' (line %ld)", (const char *)node->name, xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(name);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_BLINK: {
            xmlChar *var = xmlGetProp(node, BAD_CAST "FireBlink");
            if (!var) {
                DC_LOG_ERROR("Validate", "<Blink> missing required attribute 'FireBlink' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(var);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_MOUSE_MOTION: {
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

        case DC_APP_ELEM_TYPE_PLANET: {
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
            break;
        }

        case DC_APP_ELEM_TYPE_PLANET_VIEW: {
            xmlChar *planet = xmlGetProp(node, BAD_CAST "Planet");
            if (!planet) {
                DC_LOG_ERROR("Validate", "<PlanetView> missing required attribute 'Planet' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(planet);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_PLANET_DATA: {
            xmlChar *file = xmlGetProp(node, BAD_CAST "File");
            if (!file) {
                DC_LOG_ERROR("Validate", "<PlanetData> missing required attribute 'File' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(file);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_PLANET_TEXTURE:
            // All attributes are optional (dynamic, can be set via variables at runtime)
            break;

        case DC_APP_ELEM_TYPE_PLANET_SHADER: {
            xmlChar *index = xmlGetProp(node, BAD_CAST "Index");
            if (!index) {
                DC_LOG_ERROR("Validate", "<PlanetShader> missing required attribute 'Index' (line %ld)", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(index);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_PIXELSTREAM: {
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

// Valid attributes for each element type
static const char *_valid_attrs_common[]            = {"Style", "_Directory", NULL};
static const char *_valid_attrs_position[]          = {"X", "Y", "PositionX", "PositionY", NULL};
static const char *_valid_attrs_negate[]            = {"NegateX", "NegateY", NULL};
static const char *_valid_attrs_dimension[]         = {"Width", "Height", "DimensionX", "DimensionY", NULL};
static const char *_valid_attrs_virtual_dimension[] = {"VirtualWidth", "VirtualHeight", "VirtualDimensionX", "VirtualDimensionY", NULL};
static const char *_valid_attrs_align[]             = {"LocalAlignX", "LocalAlignY", "HorizontalAlign", "VerticalAlign", "ParentAlignX", "ParentAlignY", NULL};
static const char *_valid_attrs_pivot[]             = {"PivotX", "PivotY", "PivotPositionX", "PivotPositionY", "PivotLocalAlignX", "PivotLocalAlignY", "PivotParentAlignX", "PivotParentAlignY", NULL};
static const char *_valid_attrs_rotation[]          = {"Rotation", "Rotate", NULL};
static const char *_valid_attrs_color[]             = {"FillColor", "LineColor", "BackgroundColor", NULL};
static const char *_valid_attrs_line[]              = {"LineWidth", NULL};

static const char *_valid_attrs_arc[]            = {"Radius", "Angle", "Segments", "LineColor", NULL};
static const char *_valid_attrs_arg[]            = {"Type", "Value", NULL};
static const char *_valid_attrs_blink[]          = {"FireBlink", "Frequency", "DutyCycle", "Duration", NULL};
static const char *_valid_attrs_button[]         = {"Type", "Variable", "EnableVariable", "EnableOn", "TargetVariable", "TargetOn", "TargetOff", "On", "Off", "IndicatorVariable", "IndicatorOn", NULL};
static const char *_valid_attrs_ellipse[]        = {"Radius", "RadiusX", "RadiusY", "Segments", "Angle", NULL};
static const char *_valid_attrs_constant[]       = {"Name", NULL};
static const char *_valid_attrs_function[]       = {"Name", "FireCall", NULL};
static const char *_valid_attrs_draw_function[]  = {"Name", NULL};
static const char *_valid_attrs_if[]             = {"Value", "Value1", "Value2", "Operator", "Static", NULL};
static const char *_valid_attrs_image[]          = {"File", NULL};
static const char *_valid_attrs_logic[]          = {"File", NULL};
static const char *_valid_attrs_mouse_motion[]   = {"VariableX", "VariableY", NULL};
static const char *_valid_attrs_panel[]          = {"DisplayIndex", NULL};
static const char *_valid_attrs_pixelstream[]    = {"Type", "URL", "Protocol", "Timeout", "TestPattern", NULL};
static const char *_valid_attrs_set[]            = {"Variable", "Operator", "Defer", NULL};
static const char *_valid_attrs_sphere[]         = {"Radius", "Image", "Roll", "Pitch", "Yaw", NULL};
static const char *_valid_attrs_style[]          = {"Name", NULL};
static const char *_valid_attrs_planet[]         = {"Name", "CRS", "LightDirectionX", "LightDirectionY", "LightDirectionZ", "MeshCacheSize", NULL};
static const char *_valid_attrs_planet_view[]    = {"Planet", "CRS", "ShaderIndex", "Tau", "Flatten", "PositionX", "X", "PositionY", "Y", "DimensionX", "Width", "DimensionY", "Height", "LocalAlignX", "HorizontalAlign", "LocalAlignY", "VerticalAlign", "ParentAlignX", "ParentAlignY", "Rotation", "Rotate", "PivotPositionX", "PivotX", "PivotPositionY", "PivotY", "PivotParentAlignX", "PivotParentAlignY", "PivotLocalAlignX", "PivotLocalAlignY", "CameraLatitude", "CameraLongitude", "CameraElevation", "CameraHeading", "CameraFOV", "CameraX", "CameraY", "CameraZ", "CameraRoll", "CameraPitch", "CameraYaw", "CameraOrthographic", "NegateX", "NegateY", NULL};
static const char *_valid_attrs_planet_data[]    = {"File", NULL};
static const char *_valid_attrs_planet_texture[] = {"File", "CRS", "MetersPerPixel", "Latitude", "Longitude", "X", "Y", "Z", "OriginX", "OriginY", "FireRefresh", NULL};
static const char *_valid_attrs_planet_shader[]  = {"Index", "VertexShader", "FragmentShader", NULL};
static const char *_valid_attrs_planet_overlay[] = {"Planet", "CRS", "HeightAboveTerrain", "Latitude", "Longitude", "X", "Y", "Z", "Radius", "RadiusX", "RadiusY", "Rotation", "Segments", "Size", NULL};
static const char *_valid_attrs_planet_geojson[] = {"File", "Planet", "CRS", "HeightAboveTerrain", NULL};
static const char *_valid_attrs_planet_vertex[]  = {"Latitude", "Longitude", "Altitude", "X", "Y", "Z", NULL};
static const char *_valid_attrs_rounded[]        = {"Rounded", NULL};
static const char *_valid_attrs_text[]           = {"Size", "ShadowOffset", "UpdateRate", "Font", "Color", NULL};
static const char *_valid_attrs_trick_io[]       = {"Host", "Port", "DataRate", "ConnectedVariable", NULL};
static const char *_valid_attrs_trick_variable[] = {"Name", "Units", NULL};
static const char *_valid_attrs_edge_io[]        = {"Host", "Port", "DataRate", "ConnectedVariable", NULL};
static const char *_valid_attrs_edge_variable[]  = {"Command", NULL};
static const char *_valid_attrs_variable[]       = {"Type", "InitialValue", NULL};
static const char *_valid_attrs_vertex[]         = {NULL};
static const char *_valid_attrs_window[]         = {"Title", "ActiveDisplay", "FrameRateLimit", "MaxFPS", "MaxFrameRate", NULL};

// Check if an attribute name is in a list
static bool _attr_in_list(const char *attr_name, const char **list) {
    if (!list)
        return false;
    for (int i = 0; list[i] != NULL; i++) {
        if (strcmp(attr_name, list[i]) == 0)
            return true;
    }
    return false;
}

// Check if attribute is valid for element type
static bool _is_valid_attr_for_elem(const char *attr_name, DcAppElemType elem_type) {

    // Common attributes always valid
    if (_attr_in_list(attr_name, _valid_attrs_common))
        return true;

    // Element-specific checks
    switch (elem_type) {
        case DC_APP_ELEM_TYPE_ARC:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   _attr_in_list(attr_name, _valid_attrs_arc);

        case DC_APP_ELEM_TYPE_ARG:
            return _attr_in_list(attr_name, _valid_attrs_arg);

        case DC_APP_ELEM_TYPE_BLINK:
            return _attr_in_list(attr_name, _valid_attrs_blink);

        case DC_APP_ELEM_TYPE_BUTTON:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_virtual_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_button);

        case DC_APP_ELEM_TYPE_BUTTON_PRESSED:
        case DC_APP_ELEM_TYPE_BUTTON_RELEASED:
        case DC_APP_ELEM_TYPE_BUTTON_ENABLED:
        case DC_APP_ELEM_TYPE_BUTTON_DISABLED:
        case DC_APP_ELEM_TYPE_BUTTON_TRANSITION:
        case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_ON:
        case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_OFF:
            return true; // No specific attributes, allow common

        case DC_APP_ELEM_TYPE_CONSTANT:
            return _attr_in_list(attr_name, _valid_attrs_constant);

        case DC_APP_ELEM_TYPE_CONTAINER:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_virtual_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation);

        case DC_APP_ELEM_TYPE_DCAPP:
            return true; // DCAPP element allows any attribute (config)

        case DC_APP_ELEM_TYPE_ELLIPSE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   _attr_in_list(attr_name, _valid_attrs_ellipse);

        case DC_APP_ELEM_TYPE_DEFAULT:
        case DC_APP_ELEM_TYPE_STYLE:
            return true; // Default/Style elements are templates, allow all

        case DC_APP_ELEM_TYPE_FALSE:
        case DC_APP_ELEM_TYPE_TRUE:
            return true; // True/False just wrap content

        case DC_APP_ELEM_TYPE_FUNCTION:
            return _attr_in_list(attr_name, _valid_attrs_function);

        case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            return _attr_in_list(attr_name, _valid_attrs_draw_function);

        case DC_APP_ELEM_TYPE_IF:
            return _attr_in_list(attr_name, _valid_attrs_if);

        case DC_APP_ELEM_TYPE_IMAGE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_image);

        case DC_APP_ELEM_TYPE_INCLUDE:
            return strcmp(attr_name, "File") == 0 || strcmp(attr_name, "Optional") == 0;

        case DC_APP_ELEM_TYPE_LINE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line);

        case DC_APP_ELEM_TYPE_LOGIC:
            return _attr_in_list(attr_name, _valid_attrs_logic);

        case DC_APP_ELEM_TYPE_MOUSE_ACTIVE:
        case DC_APP_ELEM_TYPE_MOUSE_HOVERED:
        case DC_APP_ELEM_TYPE_MOUSE_INACTIVE:
        case DC_APP_ELEM_TYPE_MOUSE_PRESSED:
        case DC_APP_ELEM_TYPE_MOUSE_RELEASED:
            return true; // Mouse event elements are wrappers

        case DC_APP_ELEM_TYPE_MOUSE_MOTION:
            return _attr_in_list(attr_name, _valid_attrs_mouse_motion);

        case DC_APP_ELEM_TYPE_PANEL:
            return _attr_in_list(attr_name, _valid_attrs_virtual_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_panel);

        case DC_APP_ELEM_TYPE_PIXELSTREAM:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_pixelstream);

        case DC_APP_ELEM_TYPE_POLYGON:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   _attr_in_list(attr_name, _valid_attrs_rounded);

        case DC_APP_ELEM_TYPE_RECTANGLE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   _attr_in_list(attr_name, _valid_attrs_rounded);

        case DC_APP_ELEM_TYPE_SET:
            return _attr_in_list(attr_name, _valid_attrs_set);

        case DC_APP_ELEM_TYPE_SPHERE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_sphere);

        case DC_APP_ELEM_TYPE_STENCIL:
        case DC_APP_ELEM_TYPE_STENCIL_ADD:
        case DC_APP_ELEM_TYPE_STENCIL_REMOVE:
        case DC_APP_ELEM_TYPE_STENCIL_DRAW:
            return true; // Stencil elements are wrappers

        case DC_APP_ELEM_TYPE_PLANET:
            return _attr_in_list(attr_name, _valid_attrs_planet);

        case DC_APP_ELEM_TYPE_PLANET_VIEW:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_planet_view);

        case DC_APP_ELEM_TYPE_PLANET_DATA:
            return _attr_in_list(attr_name, _valid_attrs_planet_data);

        case DC_APP_ELEM_TYPE_PLANET_TEXTURE:
            return _attr_in_list(attr_name, _valid_attrs_planet_texture);

        case DC_APP_ELEM_TYPE_PLANET_SHADER:
            return _attr_in_list(attr_name, _valid_attrs_planet_shader);

        case DC_APP_ELEM_TYPE_PLANET_ELLIPSE:
        case DC_APP_ELEM_TYPE_PLANET_LINE:
        case DC_APP_ELEM_TYPE_PLANET_POLYGON:
        case DC_APP_ELEM_TYPE_PLANET_SPHERE:
        case DC_APP_ELEM_TYPE_PLANET_TEXT:
            return _attr_in_list(attr_name, _valid_attrs_planet_overlay) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line);

        case DC_APP_ELEM_TYPE_PLANET_GEO_JSON:
            return _attr_in_list(attr_name, _valid_attrs_planet_geojson) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line);

        case DC_APP_ELEM_TYPE_TEXT:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_text);

        case DC_APP_ELEM_TYPE_TRICK_IO:
            return _attr_in_list(attr_name, _valid_attrs_trick_io);

        case DC_APP_ELEM_TYPE_TRICK_FROM:
        case DC_APP_ELEM_TYPE_TRICK_TO:
            return true; // These just map variables

        case DC_APP_ELEM_TYPE_TRICK_VARIABLE:
            return _attr_in_list(attr_name, _valid_attrs_trick_variable);

        case DC_APP_ELEM_TYPE_EDGE_IO:
            return _attr_in_list(attr_name, _valid_attrs_edge_io);

        case DC_APP_ELEM_TYPE_EDGE_FROM:
        case DC_APP_ELEM_TYPE_EDGE_TO:
            return true; // These just group variables

        case DC_APP_ELEM_TYPE_EDGE_VARIABLE:
            return _attr_in_list(attr_name, _valid_attrs_edge_variable);

        case DC_APP_ELEM_TYPE_VARIABLE:
            return _attr_in_list(attr_name, _valid_attrs_variable);

        case DC_APP_ELEM_TYPE_VERTEX:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_negate) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_planet_vertex);

        case DC_APP_ELEM_TYPE_WINDOW:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_virtual_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_window);

        case DC_APP_ELEM_TYPE_DUMMY:
        case DC_APP_ELEM_TYPE_NONELEM:
        case DC_APP_ELEM_TYPE_UNDEFINED:
            return true;

        default:
            return true; // Unknown element, be permissive
    }
}

void _validate_attribute_names(ValidationContext *ctx, xmlNodePtr node, DcAppElemType elem_type) {

    // Iterate through all attributes on this node
    xmlAttr *attr = node->properties;
    while (attr) {
        const char *attr_name = (const char *)attr->name;

        // Skip attributes starting with '_' (commented out / not yet implemented)
        if (attr_name[0] == '_') {
            attr = attr->next;
            continue;
        }

        // Check if attribute is valid for this element type
        if (!_is_valid_attr_for_elem(attr_name, elem_type)) {
            DC_LOG_WARN("Validate", "<%s> has unrecognized attribute '%s' (line %ld)",
                        node->name, attr_name, xmlGetLineNo(node));
            ctx->warning_count++;
        }

        attr = attr->next;
    }
}

// Check if a value is a variable reference (@name) - these are runtime values
static bool _is_variable_ref(const char *value) {
    return value && value[0] == '@';
}

// Check if value is a valid integer in range
static bool _is_valid_int_in_range(const char *value, int min, int max) {
    if (!value || value[0] == '\0')
        return false;
    char *end;
    long  val = strtol(value, &end, 10);
    if (*end != '\0')
        return false; // not a pure integer
    return val >= min && val <= max;
}

// Validate an enum attribute value (constants are already expanded to integers by preprocess)
static void _validate_enum_attr(ValidationContext *ctx, xmlNodePtr node, const char *attr_name,
                                int min_val, int max_val, const char *valid_values_desc) {
    xmlChar *raw_value = xmlGetProp(node, BAD_CAST attr_name);
    if (!raw_value)
        return; // attribute not present, nothing to validate

    const char *value = (const char *)raw_value;

    // Skip validation for variable references (runtime values)
    if (_is_variable_ref(value)) {
        xmlFree(raw_value);
        return;
    }

    // Check if it's a valid integer value
    if (!_is_valid_int_in_range(value, min_val, max_val)) {
        DC_LOG_ERROR("Validate", "<%s> attribute '%s' has invalid value '%s' (line %ld). Valid values: %s",
                     node->name, attr_name, value, xmlGetLineNo(node), valid_values_desc);
        ctx->error_count++;
    }

    xmlFree(raw_value);
}

void _validate_attribute_values(ValidationContext *ctx, xmlNodePtr node, DcAppElemType elem_type) {

    // Alignment X attributes (left=1, center=2, right=3)
    static const char *align_x_attrs[] = {"LocalAlignX", "ParentAlignX", "PivotLocalAlignX", "HorizontalAlign", NULL};
    for (int i = 0; align_x_attrs[i]; i++) {
        _validate_enum_attr(ctx, node, align_x_attrs[i], 1, 3,
                            "left(1), center(2), right(3)");
    }

    // Alignment Y attributes (bottom=4, middle=5, top=6)
    static const char *align_y_attrs[] = {"LocalAlignY", "ParentAlignY", "PivotLocalAlignY", "VerticalAlign", NULL};
    for (int i = 0; align_y_attrs[i]; i++) {
        _validate_enum_attr(ctx, node, align_y_attrs[i], 4, 6,
                            "bottom(4), middle(5), top(6)");
    }

    // Element-specific enum attributes
    switch (elem_type) {
        case DC_APP_ELEM_TYPE_IF: {
            _validate_enum_attr(ctx, node, "Operator", 1, 8,
                                "true(1), false(2), eq(3), ne(4), lt(5), gt(6), lte(7), gte(8)");

            // Check for Static="true" - Value/Value1/Value2 cannot be runtime variables
            xmlChar *static_attr = xmlGetProp(node, BAD_CAST "Static");
            if (static_attr && (strcmp((const char *)static_attr, "true") == 0 || strcmp((const char *)static_attr, "1") == 0)) {
                xmlChar *value  = xmlGetProp(node, BAD_CAST "Value");
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

        case DC_APP_ELEM_TYPE_SET:
            _validate_enum_attr(ctx, node, "Operator", 1, 10,
                                "equal(1), add(2), subtract(3), multiply(4), divide(5), min(6), max(7), push(8), pop(9), negate(10)");
            break;

        case DC_APP_ELEM_TYPE_BUTTON:
            _validate_enum_attr(ctx, node, "Type", 1, 3,
                                "momentary(1), standard(2), toggle(3)");
            break;

        case DC_APP_ELEM_TYPE_VARIABLE:
            _validate_enum_attr(ctx, node, "Type", 1, 4,
                                "string(1), integer(2), double(3), boolean(4)");
            break;

        case DC_APP_ELEM_TYPE_ARG:
            _validate_enum_attr(ctx, node, "Type", 1, 4,
                                "string(1), integer(2), double(3), boolean(4)");
            break;

        case DC_APP_ELEM_TYPE_PIXELSTREAM:
            _validate_enum_attr(ctx, node, "Type", 1, 2,
                                "dynamic_file(1), mjpeg(2)");
            break;

        default:
            break;
    }
}

// ============================================================================
// Variable reference validation
// ============================================================================

// Attributes that take a variable NAME should not have a leading '@'.
// The '@' prefix is the legacy dereference syntax and should have been
// stripped during conversion.
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

void _validate_variable_references(ValidationContext *ctx, xmlNodePtr node, DcAppElemType elem_type) {
    switch (elem_type) {
        case DC_APP_ELEM_TYPE_BLINK:
            _check_var_attr(ctx, node, "Variable");
            break;

        case DC_APP_ELEM_TYPE_BUTTON:
            _check_var_attr(ctx, node, "Variable");
            _check_var_attr(ctx, node, "TargetVariable");
            _check_var_attr(ctx, node, "IndicatorVariable");
            _check_var_attr(ctx, node, "EnableVariable");
            break;

        case DC_APP_ELEM_TYPE_SET:
            _check_var_attr(ctx, node, "Variable");
            break;

        case DC_APP_ELEM_TYPE_MOUSE_MOTION:
            _check_var_attr(ctx, node, "VariableX");
            _check_var_attr(ctx, node, "VariableY");
            break;

        case DC_APP_ELEM_TYPE_EDGE_IO:
            _check_var_attr(ctx, node, "ConnectedVariable");
            break;

        case DC_APP_ELEM_TYPE_TRICK_IO:
            _check_var_attr(ctx, node, "ConnectedVariable");
            break;

        default:
            break;
    }
}
