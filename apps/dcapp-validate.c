#include "../src/app/config.h"
#include "../src/app/elem.h"
#include "../src/app/lookup.h"
#include "../src/utils/env.h"
#include "../src/utils/file.h"

#include <libxml/parser.h>

#include <stdio.h>
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

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "Usage: dcapp-validate <config.xml> [CONSTANT=value ...]\n");
        return 1;
    }

    // create config
    DcAppConfig *config;
    const char  *config_filepath = argv[1];
    if (argc < 3) {
        config = dc_app_config_create(config_filepath, NULL, 0);
    } else {
        config = dc_app_config_create(config_filepath, &(argv[2]), argc - 2);
    }

    // set environment
    dc_utils_set_env("dcappDisplayHome", config->config_dir_path, 1);

    // create lookup (needed for config_clean_xml)
    DcAppLookup *lookup = dc_app_lookup_create();

    // preprocess XML file (expands includes, constants, staticifs)
    dc_app_config_preprocess_xml(config, lookup);

    // dump preprocessed XML for debugging
    char log_file[256];
    dc_utils_join_paths(config->cache_dir_path, "xml.log", log_file, sizeof(log_file));
    dc_app_config_save_to_file(config, log_file);

    // validate
    ValidationContext ctx       = {0};
    xmlNodePtr        root_node = xmlDocGetRootElement(config->xml_doc);
    _validate_node(&ctx, root_node, DC_APP_ELEM_TYPE_NONELEM);

    // report summary
    printf("\n");
    printf("Validation complete: %d error(s), %d warning(s)\n", ctx.error_count, ctx.warning_count);

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
        fprintf(stderr, "ERROR: <%s> is not a valid child of <%s> (line %ld)\n",
                node->name, node->parent ? node->parent->name : (xmlChar *)"root", xmlGetLineNo(node));
        ctx->error_count++;
    }

    // validate required attributes for this element type
    _validate_required_attributes(ctx, node, elem_type);

    // validate that all attributes are valid for this element type
    _validate_attribute_names(ctx, node, elem_type);

    // validate attribute values are valid
    _validate_attribute_values(ctx, node, elem_type);

    // recurse into children
    _validate_children(ctx, node, elem_type);
}

bool _is_valid_child(DcAppElemType parent_type, DcAppElemType child_type) {

    // Common elements allowed in most containers
    // (StaticIf, True, False, Include, Dummy are typically post-processed away)

    // DCAPP root can contain top-level elements
    if (parent_type == DC_APP_ELEM_TYPE_DCAPP) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_WINDOW:
            case DC_APP_ELEM_TYPE_CONSTANT:
            case DC_APP_ELEM_TYPE_VARIABLE:
            case DC_APP_ELEM_TYPE_TRICK_IO:
            case DC_APP_ELEM_TYPE_DEFAULT:
            case DC_APP_ELEM_TYPE_STYLE:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
            case DC_APP_ELEM_TYPE_LOGIC:
            case DC_APP_ELEM_TYPE_FUNCTION:
                return true;
            default:
                return false;
        }
    }

    // Window can contain panels and config elements
    if (parent_type == DC_APP_ELEM_TYPE_WINDOW) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_PANEL:
            case DC_APP_ELEM_TYPE_CONSTANT:
            case DC_APP_ELEM_TYPE_VARIABLE:
            case DC_APP_ELEM_TYPE_DEFAULT:
            case DC_APP_ELEM_TYPE_STYLE:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
                return true;
            default:
                return false;
        }
    }

    // Panel can contain drawing elements
    if (parent_type == DC_APP_ELEM_TYPE_PANEL) {
        switch (child_type) {
            // config elements
            case DC_APP_ELEM_TYPE_CONSTANT:
            case DC_APP_ELEM_TYPE_VARIABLE:
            case DC_APP_ELEM_TYPE_DEFAULT:
            case DC_APP_ELEM_TYPE_STYLE:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_CIRCLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_TERRAIN:
            case DC_APP_ELEM_TYPE_BLINK:
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

    // Container can contain same as Panel
    if (parent_type == DC_APP_ELEM_TYPE_CONTAINER) {
        switch (child_type) {
            // config elements
            case DC_APP_ELEM_TYPE_CONSTANT:
            case DC_APP_ELEM_TYPE_VARIABLE:
            case DC_APP_ELEM_TYPE_DEFAULT:
            case DC_APP_ELEM_TYPE_STYLE:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
            // drawing elements
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_CIRCLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_TERRAIN:
            case DC_APP_ELEM_TYPE_BLINK:
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

    // Blink can contain drawable content
    if (parent_type == DC_APP_ELEM_TYPE_BLINK) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_CIRCLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
                return true;
            default:
                return false;
        }
    }

    // Button can contain drawing elements and button state elements
    if (parent_type == DC_APP_ELEM_TYPE_BUTTON) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_CIRCLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
            // button state elements
            case DC_APP_ELEM_TYPE_BUTTON_PRESSED:
            case DC_APP_ELEM_TYPE_BUTTON_RELEASED:
            case DC_APP_ELEM_TYPE_BUTTON_ENABLED:
            case DC_APP_ELEM_TYPE_BUTTON_DISABLED:
            case DC_APP_ELEM_TYPE_BUTTON_TRANSITION:
            case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_ON:
            case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_OFF:
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
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_CIRCLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
                return true;
            default:
                return false;
        }
    }

    // If can contain True, False, and drawable content
    if (parent_type == DC_APP_ELEM_TYPE_IF) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            // also allow implicit content (same as container)
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_CIRCLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_STENCIL:
            case DC_APP_ELEM_TYPE_PIXELSTREAM:
            case DC_APP_ELEM_TYPE_TERRAIN:
            case DC_APP_ELEM_TYPE_BLINK:
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            case DC_APP_ELEM_TYPE_BUTTON:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
                return true;
            default:
                return false;
        }
    }

    // StaticIf can only contain True and False
    if (parent_type == DC_APP_ELEM_TYPE_STATIC_IF) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
                return true;
            default:
                return false;
        }
    }

    // True/False can contain anything their grandparent can
    if (parent_type == DC_APP_ELEM_TYPE_TRUE || parent_type == DC_APP_ELEM_TYPE_FALSE) {
        // Allow everything - the real validation depends on grandparent
        // which is too complex to track here. Just allow it.
        return true;
    }

    // Stencil can contain stencil operations and drawing elements
    if (parent_type == DC_APP_ELEM_TYPE_STENCIL) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_STENCIL_ADD:
            case DC_APP_ELEM_TYPE_STENCIL_REMOVE:
            case DC_APP_ELEM_TYPE_STENCIL_DRAW:
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_CIRCLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
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
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_CIRCLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
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

    // TrickIO can contain Trick elements
    if (parent_type == DC_APP_ELEM_TYPE_TRICK_IO) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_TRICK_VARIABLE:
            case DC_APP_ELEM_TYPE_TRICK_FROM:
            case DC_APP_ELEM_TYPE_TRICK_TO:
                return true;
            default:
                return false;
        }
    }

    // TrickVariable can contain TrickFrom/TrickTo
    if (parent_type == DC_APP_ELEM_TYPE_TRICK_VARIABLE) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_TRICK_FROM:
            case DC_APP_ELEM_TYPE_TRICK_TO:
                return true;
            default:
                return false;
        }
    }

    // Terrain can contain TerrainDEM
    if (parent_type == DC_APP_ELEM_TYPE_TERRAIN) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_TERRAIN_DEM:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
                return true;
            default:
                return false;
        }
    }

    // Polygon can contain Vertex
    if (parent_type == DC_APP_ELEM_TYPE_POLYGON) {
        switch (child_type) {
            case DC_APP_ELEM_TYPE_VERTEX:
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

    // Mouse event children of shapes (Rectangle, Circle, Ellipse, Image)
    if (parent_type == DC_APP_ELEM_TYPE_RECTANGLE ||
        parent_type == DC_APP_ELEM_TYPE_CIRCLE ||
        parent_type == DC_APP_ELEM_TYPE_ELLIPSE ||
        parent_type == DC_APP_ELEM_TYPE_IMAGE) {
        switch (child_type) {
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
            case DC_APP_ELEM_TYPE_CONTAINER:
            case DC_APP_ELEM_TYPE_RECTANGLE:
            case DC_APP_ELEM_TYPE_CIRCLE:
            case DC_APP_ELEM_TYPE_ELLIPSE:
            case DC_APP_ELEM_TYPE_LINE:
            case DC_APP_ELEM_TYPE_ARC:
            case DC_APP_ELEM_TYPE_POLYGON:
            case DC_APP_ELEM_TYPE_TEXT:
            case DC_APP_ELEM_TYPE_IMAGE:
            case DC_APP_ELEM_TYPE_SPHERE:
            case DC_APP_ELEM_TYPE_IF:
            case DC_APP_ELEM_TYPE_SET:
            case DC_APP_ELEM_TYPE_STATIC_IF:
            case DC_APP_ELEM_TYPE_TRUE:
            case DC_APP_ELEM_TYPE_FALSE:
            case DC_APP_ELEM_TYPE_INCLUDE:
            case DC_APP_ELEM_TYPE_DUMMY:
                return true;
            default:
                return false;
        }
    }

    // Primitives without children (Text, Arc, Sphere, Set, Constant, Variable, etc.)
    switch (parent_type) {
        case DC_APP_ELEM_TYPE_TEXT:
        case DC_APP_ELEM_TYPE_ARC:
        case DC_APP_ELEM_TYPE_SPHERE:
        case DC_APP_ELEM_TYPE_CONSTANT:
        case DC_APP_ELEM_TYPE_VARIABLE:
        case DC_APP_ELEM_TYPE_SET:
        case DC_APP_ELEM_TYPE_TRICK_FROM:
        case DC_APP_ELEM_TYPE_TRICK_TO:
        case DC_APP_ELEM_TYPE_VERTEX:
        case DC_APP_ELEM_TYPE_PIXELSTREAM:
        case DC_APP_ELEM_TYPE_TERRAIN_DEM:
        case DC_APP_ELEM_TYPE_LOGIC:
        case DC_APP_ELEM_TYPE_FUNCTION:
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
                fprintf(stderr, "ERROR: <Variable> missing required attribute 'Type' (line %ld)\n", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(type);
            }

            xmlChar *content = xmlNodeGetContent(node);
            if (!content || strlen((char *)content) == 0) {
                fprintf(stderr, "ERROR: <Variable> missing variable name (line %ld)\n", xmlGetLineNo(node));
                ctx->error_count++;
            }
            if (content)
                xmlFree(content);
            break;
        }

        case DC_APP_ELEM_TYPE_CONSTANT: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                fprintf(stderr, "ERROR: <Constant> missing required attribute 'Name' (line %ld)\n", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(name);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_WINDOW: {
            xmlChar *title = xmlGetProp(node, BAD_CAST "Title");
            if (!title) {
                fprintf(stderr, "WARNING: <Window> missing 'Title' attribute (line %ld)\n", xmlGetLineNo(node));
                ctx->warning_count++;
            } else {
                xmlFree(title);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_PANEL: {
            xmlChar *vw = xmlGetProp(node, BAD_CAST "VirtualWidth");
            if (!vw)
                vw = xmlGetProp(node, BAD_CAST "VirtualDimensionX");
            xmlChar *vh = xmlGetProp(node, BAD_CAST "VirtualHeight");
            if (!vh)
                vh = xmlGetProp(node, BAD_CAST "VirtualDimensionY");
            if (!vw || !vh) {
                fprintf(stderr, "WARNING: <Panel> missing 'VirtualWidth' or 'VirtualHeight' attribute (line %ld)\n", xmlGetLineNo(node));
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
                fprintf(stderr, "ERROR: <Set> missing required attribute 'Variable' (line %ld)\n", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(var);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_IF:
        case DC_APP_ELEM_TYPE_STATIC_IF: {
            xmlChar *value = xmlGetProp(node, BAD_CAST "Value");
            if (!value) {
                value = xmlGetProp(node, BAD_CAST "Value1");
            }
            if (!value) {
                fprintf(stderr, "ERROR: <%s> missing required attribute 'Value' or 'Value1' (line %ld)\n",
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
                fprintf(stderr, "ERROR: <Style> missing required attribute 'Name' (line %ld)\n", xmlGetLineNo(node));
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
                fprintf(stderr, "WARNING: <TrickIO> missing 'Host' attribute (line %ld)\n", xmlGetLineNo(node));
                ctx->warning_count++;
            }
            if (!port) {
                fprintf(stderr, "WARNING: <TrickIO> missing 'Port' attribute (line %ld)\n", xmlGetLineNo(node));
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
                fprintf(stderr, "ERROR: <TrickVariable> missing required attribute 'Name' (line %ld)\n", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(name);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_IMAGE: {
            xmlChar *file = xmlGetProp(node, BAD_CAST "File");
            if (!file) {
                fprintf(stderr, "ERROR: <Image> missing required attribute 'File' (line %ld)\n", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(file);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_LOGIC: {
            xmlChar *file = xmlGetProp(node, BAD_CAST "File");
            if (!file) {
                fprintf(stderr, "ERROR: <Logic> missing required attribute 'File' (line %ld)\n", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(file);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_FUNCTION: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                fprintf(stderr, "ERROR: <Function> missing required attribute 'Name' (line %ld)\n", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(name);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_BLINK: {
            xmlChar *var = xmlGetProp(node, BAD_CAST "Variable");
            if (!var) {
                fprintf(stderr, "ERROR: <Blink> missing required attribute 'Variable' (line %ld)\n", xmlGetLineNo(node));
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
                fprintf(stderr, "WARNING: <MouseMotion> has no VariableX or VariableY (line %ld)\n", xmlGetLineNo(node));
                ctx->warning_count++;
            }
            if (vx)
                xmlFree(vx);
            if (vy)
                xmlFree(vy);
            break;
        }

        case DC_APP_ELEM_TYPE_TERRAIN_DEM: {
            xmlChar *file = xmlGetProp(node, BAD_CAST "File");
            if (!file) {
                fprintf(stderr, "ERROR: <TerrainDEM> missing required attribute 'File' (line %ld)\n", xmlGetLineNo(node));
                ctx->error_count++;
            } else {
                xmlFree(file);
            }
            break;
        }

        case DC_APP_ELEM_TYPE_PIXELSTREAM: {
            xmlChar *type = xmlGetProp(node, BAD_CAST "Type");
            if (!type) {
                fprintf(stderr, "ERROR: <PixelStream> missing required attribute 'Type' (line %ld)\n", xmlGetLineNo(node));
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
static const char *_valid_attrs_dimension[]         = {"Width", "Height", "DimensionX", "DimensionY", NULL};
static const char *_valid_attrs_virtual_dimension[] = {"VirtualWidth", "VirtualHeight", "VirtualDimensionX", "VirtualDimensionY", NULL};
static const char *_valid_attrs_align[]             = {"LocalAlignX", "LocalAlignY", "HorizontalAlign", "VerticalAlign", "ParentAlignX", "ParentAlignY", NULL};
static const char *_valid_attrs_pivot[]             = {"PivotX", "PivotY", "PivotPositionX", "PivotPositionY", "PivotLocalAlignX", "PivotLocalAlignY", NULL};
static const char *_valid_attrs_rotation[]          = {"Rotation", "Rotate", NULL};
static const char *_valid_attrs_color[]             = {"FillColor", "LineColor", "BackgroundColor", NULL};
static const char *_valid_attrs_line[]              = {"LineWidth", NULL};

static const char *_valid_attrs_arc[]            = {"Radius", "Angle", "Segments", "Pie", NULL};
static const char *_valid_attrs_blink[]          = {"Variable", "Frequency", "DutyCycle", "Duration", NULL};
static const char *_valid_attrs_button[]         = {"Type", "Variable", "EnabledVariable", "EnabledOn", "TargetVariable", "TargetOn", "TargetOff", "On", "Off", "IndicatorVariable", "IndicatorOn", NULL};
static const char *_valid_attrs_circle[]         = {"Radius", "Segments", NULL};
static const char *_valid_attrs_ellipse[]        = {"RadiusX", "RadiusY", "Segments", NULL};
static const char *_valid_attrs_constant[]       = {"Name", NULL};
static const char *_valid_attrs_function[]       = {"Name", NULL};
static const char *_valid_attrs_if[]             = {"Value", "Value1", "Value2", "Operation", NULL};
static const char *_valid_attrs_image[]          = {"File", NULL};
static const char *_valid_attrs_logic[]          = {"File", NULL};
static const char *_valid_attrs_mouse_motion[]   = {"VariableX", "VariableY", NULL};
static const char *_valid_attrs_panel[]          = {NULL};
static const char *_valid_attrs_pixelstream[]    = {"Type", "URL", "Protocol", "Timeout", NULL};
static const char *_valid_attrs_set[]            = {"Variable", "Operator", NULL};
static const char *_valid_attrs_sphere[]         = {"Radius", "Image", "Roll", "Pitch", "Yaw", NULL};
static const char *_valid_attrs_style[]          = {"Name", NULL};
static const char *_valid_attrs_terrain[]        = {"Latitude", "Longitude", "Elevation", "Roll", "Pitch", "Yaw", NULL};
static const char *_valid_attrs_terrain_dem[]    = {"File", NULL};
static const char *_valid_attrs_text[]           = {"Size", NULL};
static const char *_valid_attrs_trick_io[]       = {"Host", "Port", "DataRate", NULL};
static const char *_valid_attrs_trick_variable[] = {"Name", "Units", NULL};
static const char *_valid_attrs_variable[]       = {"Type", "InitialValue", NULL};
static const char *_valid_attrs_vertex[]         = {NULL};
static const char *_valid_attrs_window[]         = {"Title", NULL};

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
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   _attr_in_list(attr_name, _valid_attrs_arc);

        case DC_APP_ELEM_TYPE_BLINK:
            return _attr_in_list(attr_name, _valid_attrs_blink);

        case DC_APP_ELEM_TYPE_BUTTON:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
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

        case DC_APP_ELEM_TYPE_CIRCLE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line) ||
                   _attr_in_list(attr_name, _valid_attrs_circle);

        case DC_APP_ELEM_TYPE_CONSTANT:
            return _attr_in_list(attr_name, _valid_attrs_constant);

        case DC_APP_ELEM_TYPE_CONTAINER:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_virtual_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation);

        case DC_APP_ELEM_TYPE_DCAPP:
            return true; // DCAPP element allows any attribute (config)

        case DC_APP_ELEM_TYPE_ELLIPSE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
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

        case DC_APP_ELEM_TYPE_IF:
        case DC_APP_ELEM_TYPE_STATIC_IF:
            return _attr_in_list(attr_name, _valid_attrs_if);

        case DC_APP_ELEM_TYPE_IMAGE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_image);

        case DC_APP_ELEM_TYPE_INCLUDE:
            return strcmp(attr_name, "File") == 0 || strcmp(attr_name, "Optional") == 0;

        case DC_APP_ELEM_TYPE_LINE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
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
                   _attr_in_list(attr_name, _valid_attrs_color);

        case DC_APP_ELEM_TYPE_PIXELSTREAM:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_pixelstream);

        case DC_APP_ELEM_TYPE_POLYGON:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line);

        case DC_APP_ELEM_TYPE_RECTANGLE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_color) ||
                   _attr_in_list(attr_name, _valid_attrs_line);

        case DC_APP_ELEM_TYPE_SET:
            return _attr_in_list(attr_name, _valid_attrs_set);

        case DC_APP_ELEM_TYPE_SPHERE:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
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

        case DC_APP_ELEM_TYPE_TERRAIN:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
                   _attr_in_list(attr_name, _valid_attrs_dimension) ||
                   _attr_in_list(attr_name, _valid_attrs_align) ||
                   _attr_in_list(attr_name, _valid_attrs_pivot) ||
                   _attr_in_list(attr_name, _valid_attrs_rotation) ||
                   _attr_in_list(attr_name, _valid_attrs_terrain);

        case DC_APP_ELEM_TYPE_TERRAIN_DEM:
            return _attr_in_list(attr_name, _valid_attrs_terrain_dem);

        case DC_APP_ELEM_TYPE_TEXT:
            return _attr_in_list(attr_name, _valid_attrs_position) ||
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

        case DC_APP_ELEM_TYPE_VARIABLE:
            return _attr_in_list(attr_name, _valid_attrs_variable);

        case DC_APP_ELEM_TYPE_VERTEX:
            return _attr_in_list(attr_name, _valid_attrs_position);

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

        // Check if attribute is valid for this element type
        if (!_is_valid_attr_for_elem(attr_name, elem_type)) {
            fprintf(stderr, "ERROR: <%s> has invalid attribute '%s' (line %ld)\n",
                    node->name, attr_name, xmlGetLineNo(node));
            ctx->error_count++;
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
        fprintf(stderr, "ERROR: <%s> attribute '%s' has invalid value '%s' (line %ld)\n",
                node->name, attr_name, value, xmlGetLineNo(node));
        fprintf(stderr, "       Valid values: %s\n", valid_values_desc);
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
        case DC_APP_ELEM_TYPE_IF:
        case DC_APP_ELEM_TYPE_STATIC_IF:
            _validate_enum_attr(ctx, node, "Operation", 1, 8,
                                "true(1), false(2), eq(3), ne(4), lt(5), gt(6), lte(7), gte(8)");
            break;

        case DC_APP_ELEM_TYPE_SET:
            _validate_enum_attr(ctx, node, "Operator", 1, 7,
                                "equal(1), add(2), subtract(3), multiply(4), divide(5), min(6), max(7)");
            break;

        case DC_APP_ELEM_TYPE_BUTTON:
            _validate_enum_attr(ctx, node, "Type", 1, 3,
                                "momentary(1), standard(2), toggle(3)");
            break;

        case DC_APP_ELEM_TYPE_VARIABLE:
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
