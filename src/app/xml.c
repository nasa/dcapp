#include "xml.h"

#define PL_EXPERIMENTAL
#include "pl.h"
#include "pl_vfs_ext.h"

#include "data_link.h"
#include "font.h"
#include "geojson.h"
#include "logic_runtime.h"
#include "node.h"
#include "pixelstream.h"
#include "planet_api.h"
#include "scene.h"
#include "texture.h"

#include "app/elem.h"
#include "app/lookup.h"
#include "value.h"
#include "utils/file.h"
#include "utils/library.h"
#include "utils/log.h"
#include "utils/stb_sb.h"
#include "utils/string.h"
#include "libxml/tree.h"
#include "libxml/xmlstring.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Subsystem pointers are borrowed; this context owns only its root copy and scratch storage.
struct DcAppXmlContext {
    DcAppContext *app_context;
    DcAppSceneContext *scene;
    DcAppFontContext *fonts;
    DcAppTextureContext *textures;
    DcAppLogicContext *logic;
    DcAppDataLinkContext *data_link;
    DcAppPixelstreamContext *pixelstreams;
    char *dcapp_root;
    DcAppXmlBootstrapFn bootstrap;
    unsigned int registered_anonymous_variable_count;
    unsigned int created_anonymous_variable_count;
    char *sb_text_filler;
};

static const plMemoryI *_ext_memory = NULL;
static const plVfsI    *_ext_vfs    = NULL;

#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_FREE(x)  _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

void dc_app_xml_init(plApiRegistryI *api_registry) {
    _ext_memory = pl_get_api_latest(api_registry, plMemoryI);
    _ext_vfs    = pl_get_api_latest(api_registry, plVfsI);
}

DcAppXmlContext *dc_app_xml_context_create(
    DcAppContext *app_context,
    DcAppSceneContext *scene,
    DcAppLogicContext *logic,
    DcAppDataLinkContext *data_link,
    const char *dcapp_root,
    DcAppXmlBootstrapFn bootstrap) {
    if (!scene) return NULL;

    DcAppXmlContext *xml_ctx = (DcAppXmlContext *)PL_ALLOC(sizeof(*xml_ctx));
    if (!xml_ctx) return NULL;
    memset(xml_ctx, 0, sizeof(*xml_ctx));

    xml_ctx->app_context = app_context;
    xml_ctx->scene = scene;
    xml_ctx->logic = logic;
    xml_ctx->data_link = data_link;
    xml_ctx->bootstrap = bootstrap;

    if (dcapp_root && dcapp_root[0] != '\0') {
        size_t length = strlen(dcapp_root) + 1;
        xml_ctx->dcapp_root = (char *)PL_ALLOC(length);
        memcpy(xml_ctx->dcapp_root, dcapp_root, length);
    }
    return xml_ctx;
}

void dc_app_xml_context_destroy(DcAppXmlContext *xml_ctx) {
    if (!xml_ctx) return;
    sbfree(xml_ctx->sb_text_filler);
    if (xml_ctx->dcapp_root) PL_FREE(xml_ctx->dcapp_root);
    PL_FREE(xml_ctx);
}

void dc_app_xml_set_fonts(DcAppXmlContext *xml_ctx, DcAppFontContext *fonts) {
    if (xml_ctx) xml_ctx->fonts = fonts;
}

void dc_app_xml_set_textures(DcAppXmlContext *xml_ctx, DcAppTextureContext *textures) {
    if (xml_ctx) xml_ctx->textures = textures;
}

void dc_app_xml_set_pixelstreams(DcAppXmlContext *xml_ctx, DcAppPixelstreamContext *pixelstreams) {
    if (xml_ctx) xml_ctx->pixelstreams = pixelstreams;
}

static DcAppLookup *_lookup(DcAppXmlContext *xml_ctx) {
    return dc_app_scene_lookup(xml_ctx->scene);
}

// Forward declarations
static DcAppVarIndex _register_anonymous_variable(DcAppXmlContext *xml_ctx, DcValueType type, const char *initial_value_str);
static bool          _load_color_from_string(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, const char *attr_name, DcAppValIndex4 *color_out);
DcAppNodeIndex    dc_app_process_xml_node(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_arc(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_blink(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_button(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_button_disabled(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_button_enabled(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_button_indicator_off(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_button_indicator_on(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_button_pressed(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_button_released(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_button_transition(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_constant(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_container(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_dcapp(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_default(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_edge_from(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_edge_io(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_edge_to(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_edge_variable(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_draw_function(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_ellipse(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_false(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_function(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_if(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_image(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_line(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_logic(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_mouse_active(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_mouse_hovered(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_mouse_inactive(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_mouse_motion(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_mouse_pressed(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_mouse_released(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_nonelem(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_panel(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_pixelstream(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_breadcrumbs(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_container(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_data(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_ellipse(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_geo_json(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_image(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_line(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_polygon(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_shader(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static void          _planet_abs_path_to_vfs(const char *abs_path, char *vfs_out, size_t vfs_out_size);
static DcAppNodeIndex    _process_xml_node_planet_sphere(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_text(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_texture(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_planet_view(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_polygon(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_rectangle(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_set(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_sphere(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_stencil(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_stencil_add(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_stencil_draw(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_stencil_remove(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_style(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_text(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_trick_from(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_trick_io(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_trick_to(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_trick_variable(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_true(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_variable(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_vertex(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);
static DcAppNodeIndex    _process_xml_node_window(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);

// utils (definitions at bottom of file)
static const char *_node_type_to_string(DcAppNodeType type);
static DcAppNodeIndex  _register_node(DcAppXmlContext *xml_ctx, DcAppNode *node);
static DcAppNode       *_get_node(DcAppXmlContext *xml_ctx, DcAppNodeIndex index);

static int _register_font(DcAppXmlContext *xml_ctx, const char *path) {
    return dc_app_font_register(xml_ctx->fonts, path);
}

static DcAppVarIndex _register_anonymous_variable(DcAppXmlContext *xml_ctx, DcValueType type, const char *initial_value_str) {
    // create anon name
    char anon_name[32];
    snprintf(anon_name, sizeof(anon_name), "__anon_%u__", xml_ctx->registered_anonymous_variable_count++);

    // register variable
    DcAppValIndex value_index = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), type, initial_value_str);
    return dc_app_lookup_register_var(_lookup(xml_ctx), anon_name, value_index);
}

static DcAppNodeIndex _process_xml_node_children(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex node_index, DcAppElemType elem_type, const char *directory) {
    xmlNodePtr xml_child_node = xml_node->children;

    DcAppNodeIndex first_child_index         = NODE_INDEX_UNDEFINED;
    DcAppNodeIndex previous_child_node_index = NODE_INDEX_UNDEFINED;
    while (xml_child_node) {

        DcAppNodeIndex child_node_index = dc_app_process_xml_node(xml_ctx, xml_child_node, node_index, elem_type, directory);

        if (child_node_index != NODE_INDEX_UNDEFINED) {

            // get node addresses here since the address could change per node process
            DcAppNode *node                = _get_node(xml_ctx, node_index);
            DcAppNode *child_node          = _get_node(xml_ctx, child_node_index);
            DcAppNode *previous_child_node = _get_node(xml_ctx, previous_child_node_index);

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
            DcAppNodeIndex last_child_node_index = child_node_index;
            DcAppNode     *last_child_node       = _get_node(xml_ctx, last_child_node_index);
            while (last_child_node->next != NODE_INDEX_UNDEFINED) {
                last_child_node_index = last_child_node->next;
                last_child_node       = _get_node(xml_ctx, last_child_node_index);
            }
            previous_child_node_index = last_child_node_index;
        }

        // increment pointer
        xml_child_node = xml_child_node->next;
    }

    return first_child_index;
}
DcAppNodeIndex dc_app_process_xml_node(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    char     directory_buffer[DC_UTILS_FILEPATH_BUFFER_SIZE];
    // Included nodes retain their source directory through this private preprocessing attribute.
    xmlChar *dir_attr = xmlGetProp(xml_node, BAD_CAST "_Directory");
    if (dir_attr) {
        strncpy(directory_buffer, (const char *)dir_attr, sizeof(directory_buffer) - 1);
        directory_buffer[sizeof(directory_buffer) - 1] = '\0';
        directory                                      = directory_buffer;
        xmlFree(dir_attr);
    }

    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);
    switch (elem_type) {
        case DC_APP_ELEM_TYPE_NONELEM:
            return _process_xml_node_nonelem(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_ARC:
            return _process_xml_node_arc(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_ARG:
            return NODE_INDEX_UNDEFINED;

        case DC_APP_ELEM_TYPE_BLINK:
            return _process_xml_node_blink(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON:
            return _process_xml_node_button(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON_DISABLED:
            return _process_xml_node_button_disabled(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON_ENABLED:
            return _process_xml_node_button_enabled(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_OFF:
            return _process_xml_node_button_indicator_off(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_ON:
            return _process_xml_node_button_indicator_on(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_BUTTON_TRANSITION:
            return _process_xml_node_button_transition(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_CONSTANT:
            return _process_xml_node_constant(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_CONTAINER:
            return _process_xml_node_container(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_DCAPP:
            return _process_xml_node_dcapp(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_DEFAULT:
            return _process_xml_node_default(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_EDGE_FROM:
            return _process_xml_node_edge_from(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_EDGE_IO:
            return _process_xml_node_edge_io(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_EDGE_TO:
            return _process_xml_node_edge_to(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_EDGE_VARIABLE:
            return _process_xml_node_edge_variable(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            return _process_xml_node_draw_function(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_ELLIPSE:
            return _process_xml_node_ellipse(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_FALSE:
            return _process_xml_node_false(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_FUNCTION:
            return _process_xml_node_function(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_IF:
            return _process_xml_node_if(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_IMAGE:
            return _process_xml_node_image(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_LINE:
            return _process_xml_node_line(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_LOGIC:
            return _process_xml_node_logic(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_ACTIVE:
            return _process_xml_node_mouse_active(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_HOVERED:
            return _process_xml_node_mouse_hovered(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_INACTIVE:
            return _process_xml_node_mouse_inactive(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_MOTION:
            return _process_xml_node_mouse_motion(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_PRESSED:
            return _process_xml_node_mouse_pressed(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_MOUSE_RELEASED:
            return _process_xml_node_mouse_released(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PANEL:
            return _process_xml_node_panel(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PIXELSTREAM:
            return _process_xml_node_pixelstream(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_POLYGON:
            return _process_xml_node_polygon(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_RECTANGLE:
            return _process_xml_node_rectangle(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_SET:
            return _process_xml_node_set(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_SPHERE:
            return _process_xml_node_sphere(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_STENCIL:
            return _process_xml_node_stencil(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_STENCIL_ADD:
            return _process_xml_node_stencil_add(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_STENCIL_DRAW:
            return _process_xml_node_stencil_draw(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_STENCIL_REMOVE:
            return _process_xml_node_stencil_remove(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_STYLE:
            return _process_xml_node_style(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET:
            return _process_xml_node_planet(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_BREADCRUMBS:
            return _process_xml_node_planet_breadcrumbs(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_CONTAINER:
            return _process_xml_node_planet_container(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_DATA:
            return _process_xml_node_planet_data(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_ELLIPSE:
            return _process_xml_node_planet_ellipse(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_GEO_JSON:
            return _process_xml_node_planet_geo_json(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_IMAGE:
            return _process_xml_node_planet_image(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_LINE:
            return _process_xml_node_planet_line(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_POLYGON:
            return _process_xml_node_planet_polygon(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_SHADER:
            return _process_xml_node_planet_shader(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_SPHERE:
            return _process_xml_node_planet_sphere(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_TEXT:
            return _process_xml_node_planet_text(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_TEXTURE:
            return _process_xml_node_planet_texture(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_PLANET_VIEW:
            return _process_xml_node_planet_view(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TEXT:
            return _process_xml_node_text(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TRICK_FROM:
            return _process_xml_node_trick_from(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TRICK_IO:
            return _process_xml_node_trick_io(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TRICK_TO:
            return _process_xml_node_trick_to(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TRICK_VARIABLE:
            return _process_xml_node_trick_variable(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_TRUE:
            return _process_xml_node_true(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_VARIABLE:
            return _process_xml_node_variable(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_VERTEX:
            return _process_xml_node_vertex(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        case DC_APP_ELEM_TYPE_WINDOW:
            return _process_xml_node_window(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);

        default:
            return NODE_INDEX_UNDEFINED;
    }
}

static DcAppNodeIndex _process_xml_node_nonelem(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    // ignore non-element nodes
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_arc(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_ARC;
    dc_node.parent = parent_node_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.arc.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.arc.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.arc.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.arc.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (raw_x_align) {
        dc_node.arc.local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (raw_y_align) {
        dc_node.arc.local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // rotation (where the center of the arc points, 0 = top)
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.arc.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
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
        dc_node.arc.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.arc.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.arc.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.arc.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.arc.pivot_local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.arc.pivot_local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Arc", "PivotX and PivotY must both be specified, or neither");
    }

    // radius
    xmlChar *raw_radius = xmlGetProp(xml_node, BAD_CAST "Radius");
    if (raw_radius) {
        dc_node.arc.radius = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius);
        xmlFree(raw_radius);
    }

    // angle (span of the arc in degrees)
    xmlChar *raw_angle = xmlGetProp(xml_node, BAD_CAST "Angle");
    if (raw_angle) {
        dc_node.arc.angle = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_angle);
        xmlFree(raw_angle);
    } else {
        // default to 360 degrees if not specified (matches legacy Circle behavior)
        dc_node.arc.angle = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, "360");
    }

    // segments
    xmlChar *raw_segments = xmlGetProp(xml_node, BAD_CAST "Segments");
    if (raw_segments) {
        dc_node.arc.num_segments = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_segments);
        xmlFree(raw_segments);
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.arc.line_width = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // line dash pattern
    xmlChar *raw_line_pattern = xmlGetProp(xml_node, BAD_CAST "LinePattern");
    if (raw_line_pattern) {
        dc_node.arc.line_pattern = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_line_pattern);
        xmlFree(raw_line_pattern);
    }


    // line color
    _load_color_from_string(xml_ctx, xml_node, "LineColor", &(dc_node.arc.line_color));

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.arc.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.arc.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    return node_index;
}

static DcAppNodeIndex _process_xml_node_ellipse(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_ELLIPSE;
    dc_node.parent = parent_node_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.ellipse.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.ellipse.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.ellipse.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.ellipse.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.ellipse.local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.ellipse.local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.ellipse.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
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

        dc_node.ellipse.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.ellipse.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.ellipse.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.ellipse.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.ellipse.pivot_local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.ellipse.pivot_local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Ellipse", "PivotX and PivotY must both be specified, or neither");
    }

    // angle (span of the wedge in degrees, 360 = full ellipse)
    xmlChar *raw_angle = xmlGetProp(xml_node, BAD_CAST "Angle");
    if (raw_angle) {
        dc_node.ellipse.angle = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_angle);
        xmlFree(raw_angle);
    }

    // radius (shorthand for both RadiusX and RadiusY)
    xmlChar      *raw_radius = xmlGetProp(xml_node, BAD_CAST "Radius");
    DcAppValIndex radius_val = DC_APP_VAL_INDEX_UNDEFINED;
    if (raw_radius) {
        radius_val = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius);
        xmlFree(raw_radius);
    }

    // radius x (overrides Radius if specified)
    xmlChar *raw_radius_x = xmlGetProp(xml_node, BAD_CAST "RadiusX");
    if (raw_radius_x) {
        dc_node.ellipse.radius_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius_x);
        xmlFree(raw_radius_x);
    } else {
        dc_node.ellipse.radius_x = radius_val; // fallback to Radius
    }

    // radius y (overrides Radius if specified)
    xmlChar *raw_radius_y = xmlGetProp(xml_node, BAD_CAST "RadiusY");
    if (raw_radius_y) {
        dc_node.ellipse.radius_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius_y);
        xmlFree(raw_radius_y);
    } else {
        dc_node.ellipse.radius_y = radius_val; // fallback to Radius
    }

    // segments
    xmlChar *raw_segments = xmlGetProp(xml_node, BAD_CAST "Segments");
    if (raw_segments) {
        dc_node.ellipse.num_segments = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_segments);
        xmlFree(raw_segments);
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.ellipse.line_width = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // line dash pattern
    xmlChar *raw_line_pattern = xmlGetProp(xml_node, BAD_CAST "LinePattern");
    if (raw_line_pattern) {
        dc_node.ellipse.line_pattern = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_line_pattern);
        xmlFree(raw_line_pattern);
    }


    // colors
    dc_node.ellipse.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "FillColor", &(dc_node.ellipse.fill_color)))
        dc_node.ellipse.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
    if (_load_color_from_string(xml_ctx, xml_node, "LineColor", &(dc_node.ellipse.line_color)))
        dc_node.ellipse.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.ellipse.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.ellipse.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children (must store result first to avoid stale pointer after sb reallocation)
    DcAppNodeIndex first_child_index                   = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);
    _get_node(xml_ctx, node_index)->ellipse.child = first_child_index;

    // return
    return node_index;
}

// Counter for generating unique anonymous variable names
// Helper: Create an anonymous variable and return its index
static DcAppVarIndex _create_anonymous_variable(DcAppXmlContext *xml_ctx, DcValueType type, const char *initial_value_str) {
    char name[DC_VALUE_STRING_BUFFER_SIZE];
    snprintf(name, sizeof(name), "__anon_%u", xml_ctx->created_anonymous_variable_count++);

    DcValue initial_value = dc_value_create_value_string(initial_value_str);
    initial_value.type    = type;

    DcAppValIndex value_index = dc_app_lookup_register_value(_lookup(xml_ctx), &initial_value);
    return dc_app_lookup_register_var(_lookup(xml_ctx), name, value_index);
}

static DcAppNodeIndex _process_xml_node_blink(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {0};
    dc_node.type   = NODE_TYPE_BLINK;
    dc_node.parent = parent_node_index;

    // frequency (blinks per second)
    xmlChar *raw_frequency = xmlGetProp(xml_node, BAD_CAST "Frequency");
    if (raw_frequency) {
        dc_node.blink.frequency = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_frequency);
        xmlFree(raw_frequency);
    } else {
        DcValue temp_value      = dc_value_create_value_double(1.0);
        dc_node.blink.frequency = dc_app_lookup_register_value(_lookup(xml_ctx), &temp_value);
    }

    // duty cycle (fraction on, 0.0 to 1.0)
    xmlChar *raw_duty_cycle = xmlGetProp(xml_node, BAD_CAST "DutyCycle");
    if (raw_duty_cycle) {
        dc_node.blink.duty_cycle = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_duty_cycle);
        xmlFree(raw_duty_cycle);
    } else {
        DcValue temp_value       = dc_value_create_value_double(0.5);
        dc_node.blink.duty_cycle = dc_app_lookup_register_value(_lookup(xml_ctx), &temp_value);
    }

    // duration (seconds, <= 0 = indefinite toggle)
    xmlChar *raw_duration = xmlGetProp(xml_node, BAD_CAST "Duration");
    if (raw_duration) {
        dc_node.blink.duration = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_duration);
        xmlFree(raw_duration);
    } else {
        DcValue temp_value     = dc_value_create_value_double(-1.0);
        dc_node.blink.duration = dc_app_lookup_register_value(_lookup(xml_ctx), &temp_value);
    }

    // edge-triggered blink
    xmlChar *raw_fire_blink = xmlGetProp(xml_node, BAD_CAST "FireBlink");
    if (raw_fire_blink) {
        dc_node.blink.fire_blink = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_fire_blink);
        xmlFree(raw_fire_blink);
    }

    // initialize child

    // initialize runtime state
    dc_node.blink.remaining_duration = 0.0;
    dc_node.blink.last_frame_time    = 0.0;
    // last_fire_blink_value is zero-initialized by DcAppNode dc_node = {0}

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children
    DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);

    // update child index
    DcAppNode *node       = _get_node(xml_ctx, node_index);
    node->blink.child = first_child_index;

    // return
    return node_index;
}
static DcAppNodeIndex _process_xml_node_button(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {};
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
        dc_node.button.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.button.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.button.dimension.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.button.dimension.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // virtual x dimension
    xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
    if (!raw_x_virtual_dimension) {
        raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
    }
    if (raw_x_virtual_dimension) {
        dc_node.button.virtual_dimension.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    }

    // virtual y dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.button.virtual_dimension.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.button.local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.button.local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.button.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.button.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.button.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
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

        dc_node.button.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.button.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.button.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.button.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.button.pivot_local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.button.pivot_local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
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
            default_on_val = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_STRING, (const char *)raw_default_on);
        }
        DcAppValIndex default_off_val = DC_APP_VAL_INDEX_UNDEFINED;
        if (raw_default_off) {
            default_off_val = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_STRING, (const char *)raw_default_off);
        }

        // process target
        if (raw_target_on) {
            dc_node.button.val_target_on = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_STRING, (const char *)raw_target_on);
        } else {
            if (default_on_val != DC_APP_VAL_INDEX_UNDEFINED) {
                dc_node.button.val_target_on = default_on_val;
            } else {
                DcValue temp_value           = dc_value_create_value_integer(1);
                dc_node.button.val_target_on = dc_app_lookup_register_value(_lookup(xml_ctx), &temp_value);
            }
        }
        if (raw_target_off) {
            dc_node.button.val_target_off = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_STRING, (const char *)raw_target_off);
        } else {
            if (default_off_val != DC_APP_VAL_INDEX_UNDEFINED) {
                dc_node.button.val_target_off = default_off_val;
            } else {
                DcValue temp_value            = dc_value_create_value_integer(0);
                dc_node.button.val_target_off = dc_app_lookup_register_value(_lookup(xml_ctx), &temp_value);
            }
        }

        // process indicator
        if (raw_indicator_on) {
            dc_node.button.val_indicator_on = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_STRING, (const char *)raw_indicator_on);
        } else {
            dc_node.button.val_indicator_on = dc_node.button.val_target_on;
        }

        // process enabled
        if (raw_enabled_on) {
            dc_node.button.val_enabled_on = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_STRING, (const char *)raw_enabled_on);
        } else {
            DcValue temp_value            = dc_value_create_value_integer(1);
            dc_node.button.val_enabled_on = dc_app_lookup_register_value(_lookup(xml_ctx), &temp_value);
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
            default_variable_index = dc_app_lookup_get_var_index(_lookup(xml_ctx), (const char *)raw_default_variable);
        }

        // target variable
        if (raw_target_variable) {
            dc_node.button.var_target = dc_app_lookup_get_var_index(_lookup(xml_ctx), (const char *)raw_target_variable);
        } else {
            if (default_variable_index != DC_APP_VAR_INDEX_UNDEFINED) {
                dc_node.button.var_target = default_variable_index;
            } else if (raw_indicator_variable) {
                // IndicatorVariable is explicitly specified with no Variable/TargetVariable,
                // so skip creating an anonymous target variable. An anonymous target would
                // never match the indicator variable, causing permanent transitioning.
            } else {
                const char *initial_value_str = dc_app_lookup_get_value(_lookup(xml_ctx), dc_node.button.val_target_off)->value_string;
                dc_node.button.var_target     = _register_anonymous_variable(xml_ctx, DC_VALUE_TYPE_STRING, initial_value_str);
            }
        }

        // indicator variable
        if (raw_indicator_variable) {
            dc_node.button.var_indicator = dc_app_lookup_get_var_index(_lookup(xml_ctx), (const char *)raw_indicator_variable);
        } else {
            dc_node.button.var_indicator = dc_node.button.var_target;
        }

        // enabled
        if (raw_enabled_variable) {
            dc_node.button.var_enabled = dc_app_lookup_get_var_index(_lookup(xml_ctx), (const char *)raw_enabled_variable);
        } else {
            const char *initial_value_str = dc_app_lookup_get_value(_lookup(xml_ctx), dc_node.button.val_enabled_on)->value_string;
            dc_node.button.var_enabled    = _register_anonymous_variable(xml_ctx, DC_VALUE_TYPE_STRING, initial_value_str);
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
        dc_node.button.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.button.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children (state conditional nodes become regular children)
    DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);
    DcAppNode     *node              = _get_node(xml_ctx, node_index);
    node->button.child           = first_child_index;

    return node_index;
}

// Helper to create a state event node
static DcAppNodeIndex _create_state_event_node(DcAppXmlContext *xml_ctx, DcAppNodeType node_type, DcAppNodeIndex parent_node_index, DcAppNodeIndex child_index) {
    DcAppNode dc_node             = {};
    dc_node.type              = node_type;
    dc_node.parent            = parent_node_index;
    dc_node.state_event.child = child_index;
    return _register_node(xml_ctx, &dc_node);
}

// Helper to set HAS_MOUSE_HANDLERS flag on parent node
static void _set_parent_has_mouse_handlers(DcAppXmlContext *xml_ctx, DcAppNodeIndex parent_node_index) {
    DcAppNode *parent_node = _get_node(xml_ctx, parent_node_index);
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

static DcAppNodeIndex _process_xml_node_button_disabled(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_BUTTON_DISABLED, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("Disabled", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_button_enabled(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_BUTTON_ENABLED, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("Enabled", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_button_indicator_off(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_BUTTON_INDICATOR_OFF, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("Off", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_button_indicator_on(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_BUTTON_INDICATOR_ON, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("On", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_button_transition(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_BUTTON: {
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_BUTTON_TRANSITION, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("Transition", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_constant(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    // ignore at this point
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_container(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {0};
    dc_node.type   = NODE_TYPE_CONTAINER;
    dc_node.parent = parent_node_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.container.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.container.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.container.dimension.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.container.dimension.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // virtual x dimension
    xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
    if (!raw_x_virtual_dimension) {
        raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
    }
    if (raw_x_virtual_dimension) {
        dc_node.container.virtual_dimension.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    }

    // virtual y virtual_dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.container.virtual_dimension.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.container.local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.container.local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.container.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.container.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.container.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
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

        dc_node.container.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.container.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.container.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.container.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.container.pivot_local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.container.pivot_local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Container", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.container.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.container.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children
    DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);

    // update child index
    DcAppNode *node           = _get_node(xml_ctx, node_index);
    node->container.child = first_child_index;

    // return
    return node_index;
}

static DcAppNodeIndex _process_xml_node_dcapp(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    // Logic is a root-level declaration used while executable nodes are
    // parsed. Load it first so sibling order does not affect symbol lookup.
    xmlNodePtr xml_child_node = xml_node->children;
    while (xml_child_node) {
        if (dc_app_elem_type_from_xml_node(xml_child_node) == DC_APP_ELEM_TYPE_LOGIC) {
            dc_app_process_xml_node(
                xml_ctx,
                xml_child_node,
                NODE_INDEX_UNDEFINED,
                elem_type,
                directory);
        }
        xml_child_node = xml_child_node->next;
    }

    xml_child_node = xml_node->children;
    while (xml_child_node) {
        if (dc_app_elem_type_from_xml_node(xml_child_node) != DC_APP_ELEM_TYPE_LOGIC) {
            dc_app_process_xml_node(
                xml_ctx,
                xml_child_node,
                NODE_INDEX_UNDEFINED,
                elem_type,
                directory);
        }
        xml_child_node = xml_child_node->next;
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_default(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    // ignore at this point
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_edge_from(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_EDGE_IO: {
            _process_xml_node_children(xml_ctx, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
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

static DcAppNodeIndex _process_xml_node_edge_io(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

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
        connected_var_index = dc_app_lookup_get_var_index(_lookup(xml_ctx), (const char *)raw_connected_var);
        xmlFree(raw_connected_var);
    }

    // create edge instance
    dc_app_data_link_add_edge(xml_ctx->data_link, host, port, (float)data_rate, connected_var_index);

    // process children
    _process_xml_node_children(xml_ctx, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);

    // return
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_edge_to(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_EDGE_IO: {
            _process_xml_node_children(xml_ctx, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
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

static DcAppNodeIndex _process_xml_node_edge_variable(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

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

    // handle depending on parent
    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_EDGE_FROM: {
            // create + add rx var
            DcAppVarIndex dcapp_var_index = dc_app_lookup_get_var_index(_lookup(xml_ctx), dcapp_var);
            if (dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) {
                DC_LOG_ERROR("EdgeVariable", "Unknown variable '%s' in EdgeFrom", dcapp_var);
            }
            dc_app_data_link_add_edge_rx(xml_ctx->data_link, edge_cmd, dcapp_var_index);
            break;
        }
        case DC_APP_ELEM_TYPE_EDGE_TO: {
            // create + add tx var
            DcAppVarIndex dcapp_var_index = dc_app_lookup_get_var_index(_lookup(xml_ctx), dcapp_var);
            const DcValue *initial_value = NULL;
            if (dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) {
                DC_LOG_ERROR("EdgeVariable", "Unknown variable '%s' in EdgeTo", dcapp_var);
            } else {
                initial_value = dc_app_lookup_get_value(
                    _lookup(xml_ctx),
                    dc_app_lookup_get_var_value_index(_lookup(xml_ctx), dcapp_var_index));
            }
            dc_app_data_link_add_edge_tx(xml_ctx->data_link, edge_cmd, dcapp_var_index, initial_value);
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

static DcAppNodeIndex _process_xml_node_false(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_IF: {
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_IF_FALSE, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("False", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_function(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    if (parent_node_index == NODE_INDEX_UNDEFINED) {
        DC_LOG_ERROR("Function", "<Function> must be inside the <Window> render tree");
        return NODE_INDEX_UNDEFINED;
    }

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_FUNCTION;
    dc_node.parent = parent_node_index;
    bool is_valid  = true;

    // get function name
    xmlChar *raw_name = xmlGetProp(xml_node, BAD_CAST "Name");
    if (raw_name) {
        if (dc_app_logic_is_loaded(xml_ctx->logic)) {
            dc_node.function.callback = (DcAppLogicFunctionFn)dc_app_logic_symbol(xml_ctx->logic, (const char *)raw_name);
            if (!dc_node.function.callback) {
                DC_LOG_ERROR("Function", "Failed to load function '%s' from logic library: %s", (const char *)raw_name, dc_utils_library_last_error());
                is_valid = false;
            }
        } else {
            DC_LOG_ERROR("Function", "No logic library loaded, cannot load function '%s'", (const char *)raw_name);
            is_valid = false;
        }
        xmlFree(raw_name);
    } else {
        DC_LOG_ERROR("Function", "Missing 'Name' attribute");
        is_valid = false;
    }

    // edge-triggered call
    xmlChar *raw_fire_call = xmlGetProp(xml_node, BAD_CAST "FireCall");
    if (raw_fire_call) {
        dc_node.function.fire_call = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_fire_call);
        xmlFree(raw_fire_call);
    }

    if (!is_valid) return NODE_INDEX_UNDEFINED;

    // register node
    return _register_node(xml_ctx, &dc_node);
}

static DcAppNodeIndex _process_xml_node_draw_function(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);
    (void)elem_type;
    (void)parent_elem_type;
    (void)directory;

    if (parent_node_index == NODE_INDEX_UNDEFINED) {
        DC_LOG_ERROR("DrawFunction", "<DrawFunction> must be inside the <Window> render tree");
        return NODE_INDEX_UNDEFINED;
    }

    DcAppNode dc_node = {};
    dc_node.type = NODE_TYPE_DRAW_FUNCTION;
    dc_node.parent = parent_node_index;
    bool is_valid = true;

    xmlChar *raw_name = xmlGetProp(xml_node, BAD_CAST "Name");
    if (raw_name) {
        if (dc_app_logic_is_loaded(xml_ctx->logic)) {
            dc_node.draw_function.callback = (DcAppLogicDrawFunctionFn)dc_app_logic_symbol(xml_ctx->logic, (const char *)raw_name);
            if (!dc_node.draw_function.callback) {
                DC_LOG_ERROR("DrawFunction", "Failed to load function '%s' from logic library: %s", (const char *)raw_name, dc_utils_library_last_error());
                is_valid = false;
            }
        } else {
            DC_LOG_ERROR("DrawFunction", "No logic library loaded, cannot load function '%s'", (const char *)raw_name);
            is_valid = false;
        }
        xmlFree(raw_name);
    } else {
        DC_LOG_ERROR("DrawFunction", "Missing 'Name' attribute");
        is_valid = false;
    }

    xmlNodePtr xml_child_node = xml_node->children;
    while (xml_child_node) {
        if (dc_app_elem_type_from_xml_node(xml_child_node) == DC_APP_ELEM_TYPE_ARG) {
            xmlChar *raw_type = xmlGetProp(xml_child_node, BAD_CAST "Type");
            xmlChar *raw_value = xmlGetProp(xml_child_node, BAD_CAST "Value");
            if (!raw_type) {
                DC_LOG_ERROR("DrawFunction", "<Arg> missing 'Type' attribute");
                is_valid = false;
            }
            if (!raw_value) {
                DC_LOG_ERROR("DrawFunction", "<Arg> missing 'Value' attribute");
                is_valid = false;
            }
            if (raw_type && raw_value) {
                DcValueType type = (DcValueType)dc_utils_string_to_integer((const char *)raw_type);
                if (type < DC_VALUE_TYPE_STRING || type > DC_VALUE_TYPE_BOOLEAN) {
                    DC_LOG_ERROR("DrawFunction", "<Arg> has invalid Type '%s'", (const char *)raw_type);
                    is_valid = false;
                } else {
                    DcAppDrawFunctionArg arg = {
                        .type = type,
                        .value = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), type, (const char *)raw_value),
                    };
                    if (arg.value == DC_APP_VAL_INDEX_UNDEFINED) {
                        DC_LOG_ERROR("DrawFunction", "<Arg> Value '%s' could not be resolved", (const char *)raw_value);
                        is_valid = false;
                    } else {
                        sbpush(dc_node.draw_function.sb_args, arg);
                    }
                }
            }
            if (raw_type) xmlFree(raw_type);
            if (raw_value) xmlFree(raw_value);
        }
        xml_child_node = xml_child_node->next;
    }

    if (!is_valid) {
        sbfree(dc_node.draw_function.sb_args);
        return NODE_INDEX_UNDEFINED;
    }

    return _register_node(xml_ctx, &dc_node);
}

static DcAppNodeIndex _process_xml_node_if(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node                   = {};
    dc_node.type                    = NODE_TYPE_CONDITIONAL;
    dc_node.parent                  = parent_node_index;
    dc_node.conditional.state_flags = NODE_STATE_FLAG_NONE;

    // conditional type
    xmlChar *raw_type = xmlGetProp(xml_node, BAD_CAST "Operator");
    if (raw_type) {
        dc_node.conditional.type = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_type);
        xmlFree(raw_type);
    }

    // value1
    xmlChar *raw_value1 = xmlGetProp(xml_node, BAD_CAST "Value");
    if (!raw_value1) {
        raw_value1 = xmlGetProp(xml_node, BAD_CAST "Value1");
    }
    if (raw_value1) {
        dc_node.conditional.value1 = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_STRING, (const char *)raw_value1);
        xmlFree(raw_value1);
    } else {
        DC_LOG_ERROR("If", "No value specified for conditional");
    }

    // value2
    xmlChar *raw_value2 = xmlGetProp(xml_node, BAD_CAST "Value2");
    if (raw_value2) {
        dc_node.conditional.value2 = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_STRING, (const char *)raw_value2);
        xmlFree(raw_value2);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children (True/False become state event nodes)
    DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);

    // update child index
    DcAppNode *node             = _get_node(xml_ctx, node_index);
    node->conditional.child = first_child_index;

    return node_index;
}

static DcAppNodeIndex _process_xml_node_image(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {0};
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
        dc_node.image.texture_index = dc_app_texture_load_image_index(xml_ctx->textures, cleaned_filepath, directory);
    } else {
        DC_LOG_ERROR("Image", "Missing 'File' attribute");
    }

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.image.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.image.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.image.dimension.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.image.dimension.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.image.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.image.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.image.local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.image.local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.image.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
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

        dc_node.image.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.image.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.image.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.image.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.image.pivot_local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.image.pivot_local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Image", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.image.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.image.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children (must store result first to avoid stale pointer after sb reallocation)
    DcAppNodeIndex first_child_index                 = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);
    _get_node(xml_ctx, node_index)->image.child = first_child_index;

    // return
    return node_index;
}

static DcAppNodeIndex _process_xml_node_line(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_LINE;
    dc_node.parent = parent_node_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.line.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.line.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.line.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
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

        dc_node.line.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.line.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        if (raw_pivot_parent_align_x) {
            dc_node.line.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
            xmlFree(raw_pivot_parent_align_x);
        }
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_y) {
            dc_node.line.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
            xmlFree(raw_pivot_parent_align_y);
        }
    } else if (raw_pivot_position_x || raw_pivot_position_y) {
        DC_LOG_ERROR("Line", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.line.line_width = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // line dash pattern
    xmlChar *raw_line_pattern = xmlGetProp(xml_node, BAD_CAST "LinePattern");
    if (raw_line_pattern) {
        dc_node.line.line_pattern = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_line_pattern);
        xmlFree(raw_line_pattern);
    }


    // colors
    dc_node.line.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "LineColor", &(dc_node.line.line_color)))
        dc_node.line.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.line.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.line.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children
    _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);

    // return
    return node_index;
}

static DcAppNodeIndex _process_xml_node_logic(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);
    (void)elem_type;
    (void)parent_node_index;

    if (parent_elem_type != DC_APP_ELEM_TYPE_DCAPP) {
        DC_LOG_ERROR("Logic", "<Logic> must be a direct child of <DCAPP>");
        return NODE_INDEX_UNDEFINED;
    }

    xmlChar *raw_filepath = xmlGetProp(xml_node, BAD_CAST "File");
    if (raw_filepath) {
        // clean filepath
        char cleaned_filepath[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned_filepath, (const char *)raw_filepath, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_filepath);
        dc_app_logic_load(xml_ctx->logic, cleaned_filepath, directory);
    } else {
        DC_LOG_ERROR("Logic", "Missing 'File' attribute");
    }

    // return
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_mouse_active(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppNode *parent_node = _get_node(xml_ctx, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
        case NODE_TYPE_CONTAINER:
        case NODE_TYPE_ELLIPSE:
        case NODE_TYPE_IMAGE:
        case NODE_TYPE_PIXELSTREAM:
        case NODE_TYPE_POLYGON:
        case NODE_TYPE_RECTANGLE: {
            _set_parent_has_mouse_handlers(xml_ctx, parent_node_index);
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_MOUSE_ACTIVE, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("MouseActive", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_mouse_hovered(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppNode *parent_node = _get_node(xml_ctx, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
        case NODE_TYPE_CONTAINER:
        case NODE_TYPE_ELLIPSE:
        case NODE_TYPE_IMAGE:
        case NODE_TYPE_PIXELSTREAM:
        case NODE_TYPE_POLYGON:
        case NODE_TYPE_RECTANGLE: {
            _set_parent_has_mouse_handlers(xml_ctx, parent_node_index);
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_MOUSE_HOVERED, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("MouseHovered", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_mouse_inactive(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppNode *parent_node = _get_node(xml_ctx, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
        case NODE_TYPE_CONTAINER:
        case NODE_TYPE_ELLIPSE:
        case NODE_TYPE_IMAGE:
        case NODE_TYPE_PIXELSTREAM:
        case NODE_TYPE_POLYGON:
        case NODE_TYPE_RECTANGLE: {
            _set_parent_has_mouse_handlers(xml_ctx, parent_node_index);
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_MOUSE_INACTIVE, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("MouseInactive", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_mouse_motion(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_MOUSE_MOTION;
    dc_node.parent = parent_node_index;

    // VariableX
    xmlChar *raw_var_x = xmlGetProp(xml_node, BAD_CAST "VariableX");
    if (raw_var_x) {
        dc_node.mouse_motion.var_x = dc_app_lookup_get_var_index(_lookup(xml_ctx), (const char *)raw_var_x);
        xmlFree(raw_var_x);
    }

    // VariableY
    xmlChar *raw_var_y = xmlGetProp(xml_node, BAD_CAST "VariableY");
    if (raw_var_y) {
        dc_node.mouse_motion.var_y = dc_app_lookup_get_var_index(_lookup(xml_ctx), (const char *)raw_var_y);
        xmlFree(raw_var_y);
    }

    return _register_node(xml_ctx, &dc_node);
}

static DcAppNodeIndex _process_xml_node_mouse_pressed(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppNode *parent_node = _get_node(xml_ctx, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
        case NODE_TYPE_CONTAINER:
        case NODE_TYPE_ELLIPSE:
        case NODE_TYPE_IMAGE:
        case NODE_TYPE_PIXELSTREAM:
        case NODE_TYPE_POLYGON:
        case NODE_TYPE_RECTANGLE: {
            _set_parent_has_mouse_handlers(xml_ctx, parent_node_index);
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_MOUSE_PRESSED, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("MousePressed", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_mouse_released(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppNode *parent_node = _get_node(xml_ctx, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_BUTTON:
        case NODE_TYPE_CONTAINER:
        case NODE_TYPE_ELLIPSE:
        case NODE_TYPE_IMAGE:
        case NODE_TYPE_PIXELSTREAM:
        case NODE_TYPE_POLYGON:
        case NODE_TYPE_RECTANGLE: {
            _set_parent_has_mouse_handlers(xml_ctx, parent_node_index);
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, parent_elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_MOUSE_RELEASED, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("MouseReleased", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
            break;
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_panel(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_PANEL;
    dc_node.parent = parent_node_index;

    // virtual x dimension
    xmlChar *raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionX");
    if (!raw_x_virtual_dimension) {
        raw_x_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualWidth");
    }
    if (raw_x_virtual_dimension) {
        dc_node.panel.virtual_dimension.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    }

    // virtual y virtual_dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.panel.virtual_dimension.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    }

    // display index (matched against Window's ActiveDisplay)
    xmlChar *raw_display_index = xmlGetProp(xml_node, BAD_CAST "DisplayIndex");
    if (raw_display_index) {
        dc_node.panel.index = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_display_index);
        xmlFree(raw_display_index);
    }

    // background color
    dc_node.panel.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "BackgroundColor", &(dc_node.panel.background_color)))
        dc_node.panel.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children
    DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);

    // update child index
    DcAppNode *node       = _get_node(xml_ctx, node_index);
    node->panel.child = first_child_index;

    // return
    return node_index;
}

static DcAppNodeIndex _process_xml_node_pixelstream(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {0};
    dc_node.type   = NODE_TYPE_PIXELSTREAM;
    dc_node.parent = parent_node_index;

    // parse type
    DcAppPixelstreamType ps_type = 0;
    xmlChar *raw_type = xmlGetProp(xml_node, BAD_CAST "Type");
    if (!raw_type) {
        raw_type = xmlGetProp(xml_node, BAD_CAST "Protocol");
    }
    if (raw_type) {
        ps_type = dc_utils_string_to_integer((const char *)raw_type);
        xmlFree(raw_type);
    } else {
        DC_LOG_ERROR("PixelStream", "Missing 'Type' attribute");
    }

    // get source key for deduplication (filepath for shmem, URL for mjpeg)
    char source_key[DC_UTILS_FILEPATH_BUFFER_SIZE] = {0};
    {
        xmlChar *raw_key = NULL;
        if (ps_type == DC_APP_PIXELSTREAM_TYPE_SHMEM) {
            raw_key = xmlGetProp(xml_node, BAD_CAST "SharedMemoryKey");
            if (!raw_key) raw_key = xmlGetProp(xml_node, BAD_CAST "File");
            if (!raw_key) raw_key = xmlGetProp(xml_node, BAD_CAST "URL");
        } else if (ps_type == DC_APP_PIXELSTREAM_TYPE_MJPEG) {
            raw_key = xmlGetProp(xml_node, BAD_CAST "URL");
        }
        if (raw_key) {
            strncpy(source_key, (const char *)raw_key, DC_UTILS_FILEPATH_BUFFER_SIZE - 1);
            xmlFree(raw_key);
        }
    }

    // find or create pixelstream source
    DcAppPixelstreamSourceIndex source_index = dc_app_pixelstream_find(xml_ctx->pixelstreams, ps_type, source_key);
    if (source_index == DC_APP_PIXELSTREAM_SOURCE_INDEX_UNDEFINED) {
        int timeout = 5;
        xmlChar *raw_timeout = xmlGetProp(xml_node, BAD_CAST "Timeout");
        if (raw_timeout) {
            timeout = dc_utils_string_to_integer((const char *)raw_timeout);
            xmlFree(raw_timeout);
        }
        if (source_key[0] == '\0') {
            if (ps_type == DC_APP_PIXELSTREAM_TYPE_SHMEM)
                DC_LOG_ERROR("PixelStream", "Missing 'SharedMemoryKey' or 'File' attribute for shmem type");
            else if (ps_type == DC_APP_PIXELSTREAM_TYPE_MJPEG)
                DC_LOG_ERROR("PixelStream", "Missing 'URL' attribute for mjpeg type");
        }
        source_index = dc_app_pixelstream_add(xml_ctx->pixelstreams, ps_type, source_key, timeout);
        DC_LOG_INFO("PixelStream", "New source [%d]: %s", source_index, source_key);
    } else {
        DC_LOG_INFO("PixelStream", "Reusing source [%d]: %s", source_index, source_key);
    }
    dc_node.pixelstream.source_index = source_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.pixelstream.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.pixelstream.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.pixelstream.dimension.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.pixelstream.dimension.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.pixelstream.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.pixelstream.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.pixelstream.local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.pixelstream.local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.pixelstream.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
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

        dc_node.pixelstream.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.pixelstream.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.pixelstream.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.pixelstream.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.pixelstream.pivot_local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.pixelstream.pivot_local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("PixelStream", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.pixelstream.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.pixelstream.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
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
            snprintf(test_pattern_path, sizeof(test_pattern_path), "%s/assets/testpattern.png", xml_ctx->dcapp_root ? xml_ctx->dcapp_root : "");
        }

        dc_node.pixelstream.test_pattern_texture_index =
            dc_app_texture_load_image_index(xml_ctx->textures, test_pattern_path, NULL);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children (must store result first to avoid stale pointer after sb reallocation)
    DcAppNodeIndex first_child_index                       = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);
    _get_node(xml_ctx, node_index)->pixelstream.child = first_child_index;

    // return
    return node_index;
}

static DcAppNodeIndex _process_xml_node_polygon(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_POLYGON;
    dc_node.parent = parent_node_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.polygon.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.polygon.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.polygon.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.polygon.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.polygon.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
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

        dc_node.polygon.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.polygon.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        if (raw_pivot_parent_align_x) {
            dc_node.polygon.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
            xmlFree(raw_pivot_parent_align_x);
        }
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_y) {
            dc_node.polygon.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
            xmlFree(raw_pivot_parent_align_y);
        }
    } else if (raw_pivot_position_x || raw_pivot_position_y) {
        DC_LOG_ERROR("Polygon", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.polygon.line_width = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // line dash pattern
    xmlChar *raw_line_pattern = xmlGetProp(xml_node, BAD_CAST "LinePattern");
    if (raw_line_pattern) {
        dc_node.polygon.line_pattern = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_line_pattern);
        xmlFree(raw_line_pattern);
    }


    // colors
    dc_node.polygon.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "FillColor", &(dc_node.polygon.fill_color)))
        dc_node.polygon.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
    if (_load_color_from_string(xml_ctx, xml_node, "LineColor", &(dc_node.polygon.line_color)))
        dc_node.polygon.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.polygon.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.polygon.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // rounded
    xmlChar *raw_rounded = xmlGetProp(xml_node, BAD_CAST "Rounded");
    if (raw_rounded) {
        dc_node.polygon.rounded = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_rounded);
        xmlFree(raw_rounded);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children (must store result first to avoid stale pointer after sb reallocation)
    DcAppNodeIndex first_child_index                   = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);
    _get_node(xml_ctx, node_index)->polygon.child = first_child_index;

    // return
    return node_index;
}

static DcAppNodeIndex _process_xml_node_rectangle(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_RECTANGLE;
    dc_node.parent = parent_node_index;

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.rectangle.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.rectangle.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.rectangle.dimension.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.rectangle.dimension.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.rectangle.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.rectangle.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.rectangle.local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.rectangle.local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.rectangle.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
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

        dc_node.rectangle.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.rectangle.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.rectangle.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.rectangle.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.rectangle.pivot_local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.rectangle.pivot_local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Rectangle", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.rectangle.line_width = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // line dash pattern
    xmlChar *raw_line_pattern = xmlGetProp(xml_node, BAD_CAST "LinePattern");
    if (raw_line_pattern) {
        dc_node.rectangle.line_pattern = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_line_pattern);
        xmlFree(raw_line_pattern);
    }


    // colors
    dc_node.rectangle.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "FillColor", &(dc_node.rectangle.fill_color)))
        dc_node.rectangle.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
    if (_load_color_from_string(xml_ctx, xml_node, "LineColor", &(dc_node.rectangle.line_color)))
        dc_node.rectangle.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.rectangle.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.rectangle.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // rounded
    xmlChar *raw_rounded = xmlGetProp(xml_node, BAD_CAST "Rounded");
    if (raw_rounded) {
        dc_node.rectangle.rounded = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_rounded);
        xmlFree(raw_rounded);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children (must store result first to avoid stale pointer after sb reallocation)
    DcAppNodeIndex first_child_index                     = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);
    _get_node(xml_ctx, node_index)->rectangle.child = first_child_index;

    // return
    return node_index;
}

static DcAppNodeIndex _process_xml_node_set(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_SET;
    dc_node.parent = parent_node_index;

    // variable
    xmlChar *raw_variable = xmlGetProp(xml_node, BAD_CAST "Variable");
    if (raw_variable) {
        dc_node.set.var_index = dc_app_lookup_get_var_index(_lookup(xml_ctx), (const char *)raw_variable);
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
            dc_node.set.operand = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_STRING, operand);
        }
    } else {
        DC_LOG_WARN("Set", "Missing node content, skipping");
    }

    // operator
    xmlChar *raw_operator = xmlGetProp(xml_node, BAD_CAST "Operator");
    if (raw_operator) {
        dc_node.set.operation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_operator);
        xmlFree(raw_operator);
    }

    // defer flag
    xmlChar *raw_defer = xmlGetProp(xml_node, BAD_CAST "Defer");
    if (raw_defer) {
        dc_node.set.deferred = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_defer);
        xmlFree(raw_defer);
    }

    // register node
    return _register_node(xml_ctx, &dc_node);
}

static DcAppNodeIndex _process_xml_node_sphere(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_SPHERE;
    dc_node.parent = parent_node_index;

    // default texture to undefined

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.sphere.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.sphere.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.sphere.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.sphere.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.sphere.local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.sphere.local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // pivot parent x align
    xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
    if (raw_pivot_parent_align_x) {
        dc_node.sphere.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
        xmlFree(raw_pivot_parent_align_x);
    }

    // pivot parent y align
    xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
    if (raw_pivot_parent_align_y) {
        dc_node.sphere.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
        xmlFree(raw_pivot_parent_align_y);
    }

    // pivot local x align
    xmlChar *raw_pivot_local_x_align = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
    if (raw_pivot_local_x_align) {
        dc_node.sphere.pivot_local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_local_x_align);
        xmlFree(raw_pivot_local_x_align);
    }

    // pivot local y align
    xmlChar *raw_pivot_local_y_align = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
    if (raw_pivot_local_y_align) {
        dc_node.sphere.pivot_local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_local_y_align);
        xmlFree(raw_pivot_local_y_align);
    }

    // pivot x position
    xmlChar *raw_pivot_x_position = xmlGetProp(xml_node, BAD_CAST "PivotPositionX");
    if (raw_pivot_x_position) {
        dc_node.sphere.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_x_position);
        xmlFree(raw_pivot_x_position);
    }

    // pivot y position
    xmlChar *raw_pivot_y_position = xmlGetProp(xml_node, BAD_CAST "PivotPositionY");
    if (raw_pivot_y_position) {
        dc_node.sphere.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_y_position);
        xmlFree(raw_pivot_y_position);
    }

    // rotation (external 2D rotation)
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (raw_rotation) {
        dc_node.sphere.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
    }

    // radius
    xmlChar *raw_radius = xmlGetProp(xml_node, BAD_CAST "Radius");
    if (raw_radius) {
        dc_node.sphere.radius = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius);
        xmlFree(raw_radius);
    }

    // fill color
    _load_color_from_string(xml_ctx, xml_node, "FillColor", &dc_node.sphere.fill_color);

    // internal rotation (roll, pitch, yaw)
    xmlChar *raw_roll = xmlGetProp(xml_node, BAD_CAST "Roll");
    if (raw_roll) {
        dc_node.sphere.rpy.roll = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_roll);
        xmlFree(raw_roll);
    }

    xmlChar *raw_pitch = xmlGetProp(xml_node, BAD_CAST "Pitch");
    if (raw_pitch) {
        dc_node.sphere.rpy.pitch = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pitch);
        xmlFree(raw_pitch);
    }

    xmlChar *raw_yaw = xmlGetProp(xml_node, BAD_CAST "Yaw");
    if (raw_yaw) {
        dc_node.sphere.rpy.yaw = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_yaw);
        xmlFree(raw_yaw);
    }

    // optional texture (Image attribute)
    xmlChar *filepath = xmlGetProp(xml_node, BAD_CAST "Image");
    if (filepath) {
        char cleaned_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
        strncpy(cleaned_filepath, (const char *)filepath, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(filepath);
        dc_node.sphere.texture_index = dc_app_texture_load_image_index(xml_ctx->textures, cleaned_filepath, directory);
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.sphere.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.sphere.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // register node
    return _register_node(xml_ctx, &dc_node);
}

static DcAppNodeIndex _process_xml_node_stencil(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node               = {};
    dc_node.type                = NODE_TYPE_STENCIL;
    dc_node.parent              = parent_node_index;
    dc_node.stencil.sb_children = NULL;

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process children (StencilAdd, StencilRemove, StencilDraw)
    _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);

    return node_index;
}

static DcAppNodeIndex _process_xml_node_stencil_add(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_STENCIL: {
            // process children
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, elem_type, directory);

            // add entry to stencil's children buffer
            DcAppStencilChild stencil_child = {
                .child = first_child_index,
                .type  = STENCIL_CHILD_TYPE_ADD,
            };
            DcAppNode *parent_node = _get_node(xml_ctx, parent_node_index);
            sbpush(parent_node->stencil.sb_children, stencil_child);
            break;
        }
        default:
            DC_LOG_ERROR("StencilAdd", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_stencil_draw(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_STENCIL: {
            // process children
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, elem_type, directory);

            // add entry to stencil's children buffer
            DcAppStencilChild stencil_child = {
                .child = first_child_index,
                .type  = STENCIL_CHILD_TYPE_DRAW,
            };
            DcAppNode *parent_node = _get_node(xml_ctx, parent_node_index);
            sbpush(parent_node->stencil.sb_children, stencil_child);
            break;
        }
        default:
            DC_LOG_ERROR("StencilDraw", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_stencil_remove(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_STENCIL: {
            // process children
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, elem_type, directory);

            // add entry to stencil's children buffer
            DcAppStencilChild stencil_child = {
                .child = first_child_index,
                .type  = STENCIL_CHILD_TYPE_REMOVE,
            };
            DcAppNode *parent_node = _get_node(xml_ctx, parent_node_index);
            sbpush(parent_node->stencil.sb_children, stencil_child);
            break;
        }
        default:
            DC_LOG_ERROR("StencilRemove", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_style(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    // ignore at this point
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_planet(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);
    (void)parent_node_index;
    (void)parent_elem_type;

    // planet definition (top-level resource, no drawable node)
    DcAppPlanetDefinition def = {0};
    def.crs = DC_APP_PLANET_CRS_GEODETIC;

    // Name
    xmlChar *raw_name = xmlGetProp(xml_node, BAD_CAST "Name");
    def.name          = strdup((const char *)raw_name);
    xmlFree(raw_name);

    // coordinate reference system inherited by PlanetTexture
    xmlChar *raw_crs = xmlGetProp(xml_node, BAD_CAST "CRS");
    if (raw_crs) {
        def.crs = (DcAppPlanetCrs)atoi((const char *)raw_crs);
        xmlFree(raw_crs);
    }

    // light direction
    xmlChar *raw_ldx = xmlGetProp(xml_node, BAD_CAST "LightDirectionX");
    if (raw_ldx) {
        def.light_direction.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_ldx);
        xmlFree(raw_ldx);
    }
    xmlChar *raw_ldy = xmlGetProp(xml_node, BAD_CAST "LightDirectionY");
    if (raw_ldy) {
        def.light_direction.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_ldy);
        xmlFree(raw_ldy);
    }
    xmlChar *raw_ldz = xmlGetProp(xml_node, BAD_CAST "LightDirectionZ");
    if (raw_ldz) {
        def.light_direction.z = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_ldz);
        xmlFree(raw_ldz);
    }

    // combined cache size in MiB, split evenly between vertex and index buffers.
    xmlChar *raw_mesh_cache = xmlGetProp(xml_node, BAD_CAST "MeshCacheSize");
    if (raw_mesh_cache) {
        def.mesh_cache_size_mb = (uint32_t)atof((const char *)raw_mesh_cache);
        xmlFree(raw_mesh_cache);
    }

    // collect definition
    // Register first so depth-first child handlers can extend this definition.
    dc_app_scene_add_planet_definition(xml_ctx->scene, &def);

    // process children (PlanetData, PlanetTexture, PlanetShader store into this def)
    _process_xml_node_children(xml_ctx, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);

    // no drawable node — return undefined
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_planet_data(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    (void)parent_node_index;

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET) {
        DC_LOG_ERROR("PlanetData", "Invalid parent element type: %s", dc_app_elem_type_to_string(parent_elem_type));
        return NODE_INDEX_UNDEFINED;
    }

    uint32_t definition_count = dc_app_scene_get_planet_definition_count(xml_ctx->scene);
    DcAppPlanetDefinition *def = dc_app_scene_get_planet_definition(xml_ctx->scene, definition_count - 1);

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

static DcAppNodeIndex _process_xml_node_planet_breadcrumbs(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    (void)directory;

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET_VIEW) {
        DC_LOG_ERROR("PlanetBreadcrumbs", "PlanetBreadcrumbs must be a child of PlanetView");
        return NODE_INDEX_UNDEFINED;
    }

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_PLANET_BREADCRUMBS;
    dc_node.parent = parent_node_index;

    DcAppNode *parent = _get_node(xml_ctx, parent_node_index);
    dc_node.planet_breadcrumbs.planet_def_index = parent->planet_view.planet_def_index;
    dc_node.planet_breadcrumbs.crs              = parent->planet_view.crs;

    xmlChar *raw_crs = xmlGetProp(xml_node, BAD_CAST "CRS");
    if (raw_crs) {
        dc_node.planet_breadcrumbs.crs = (DcAppPlanetCrs)atoi((const char *)raw_crs);
        xmlFree(raw_crs);
    }

    xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "Latitude");
    if (raw_lat) {
        dc_node.planet_breadcrumbs.lat = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
        xmlFree(raw_lat);
    }

    xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
    if (raw_lon) {
        dc_node.planet_breadcrumbs.lon = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
        xmlFree(raw_lon);
    }

    xmlChar *raw_alt = xmlGetProp(xml_node, BAD_CAST "Altitude");
    if (raw_alt) {
        dc_node.planet_breadcrumbs.alt = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_alt);
        xmlFree(raw_alt);
    }

    xmlChar *raw_x = xmlGetProp(xml_node, BAD_CAST "X");
    if (raw_x) {
        dc_node.planet_breadcrumbs.xyz.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x);
        xmlFree(raw_x);
    }

    xmlChar *raw_y = xmlGetProp(xml_node, BAD_CAST "Y");
    if (raw_y) {
        dc_node.planet_breadcrumbs.xyz.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y);
        xmlFree(raw_y);
    }

    xmlChar *raw_z = xmlGetProp(xml_node, BAD_CAST "Z");
    if (raw_z) {
        dc_node.planet_breadcrumbs.xyz.z = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_z);
        xmlFree(raw_z);
    }

    xmlChar *raw_height = xmlGetProp(xml_node, BAD_CAST "HeightAboveTerrain");
    if (raw_height) {
        dc_node.planet_breadcrumbs.height_above_terrain = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_height);
        xmlFree(raw_height);
    }

    xmlChar *raw_spacing = xmlGetProp(xml_node, BAD_CAST "PointSpacing");
    if (raw_spacing) {
        dc_node.planet_breadcrumbs.point_spacing = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_spacing);
        xmlFree(raw_spacing);
    }

    xmlChar *raw_max_points = xmlGetProp(xml_node, BAD_CAST "MaxPoints");
    if (raw_max_points) {
        dc_node.planet_breadcrumbs.max_points = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_max_points);
        xmlFree(raw_max_points);
    }

    xmlChar *raw_clear = xmlGetProp(xml_node, BAD_CAST "Clear");
    if (raw_clear) {
        dc_node.planet_breadcrumbs.clear = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_clear);
        xmlFree(raw_clear);
    }

    xmlChar *raw_enabled = xmlGetProp(xml_node, BAD_CAST "Enabled");
    if (raw_enabled) {
        dc_node.planet_breadcrumbs.enabled = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_enabled);
        xmlFree(raw_enabled);
    }

    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.planet_breadcrumbs.line_width = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    xmlChar *raw_line_pattern = xmlGetProp(xml_node, BAD_CAST "LinePattern");
    if (raw_line_pattern) {
        dc_node.planet_breadcrumbs.line_pattern = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_line_pattern);
        xmlFree(raw_line_pattern);
    }

    dc_node.planet_breadcrumbs.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "LineColor", &(dc_node.planet_breadcrumbs.line_color)))
        dc_node.planet_breadcrumbs.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    return _register_node(xml_ctx, &dc_node);
}

static DcAppNodeIndex _process_xml_node_planet_container(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET_VIEW) {
        DC_LOG_ERROR("PlanetContainer", "PlanetContainer must be a child of PlanetView");
        return NODE_INDEX_UNDEFINED;
    }

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_PLANET_CONTAINER;
    dc_node.parent = parent_node_index;

    DcAppNode *parent = _get_node(xml_ctx, parent_node_index);
    dc_node.planet_container.planet_def_index = parent->planet_view.planet_def_index;

    xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "Latitude");
    if (raw_lat) {
        dc_node.planet_container.lat = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
        xmlFree(raw_lat);
    } else {
        DC_LOG_ERROR("PlanetContainer", "Missing 'Latitude' attribute");
    }

    xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
    if (raw_lon) {
        dc_node.planet_container.lon = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
        xmlFree(raw_lon);
    } else {
        DC_LOG_ERROR("PlanetContainer", "Missing 'Longitude' attribute");
    }

    xmlChar *raw_height = xmlGetProp(xml_node, BAD_CAST "HeightAboveTerrain");
    if (raw_height) {
        dc_node.planet_container.height_above_terrain = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_height);
        xmlFree(raw_height);
    }

    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (raw_rotation) {
        dc_node.planet_container.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
    }

    xmlChar *raw_scale = xmlGetProp(xml_node, BAD_CAST "Scale");
    if (raw_scale) {
        dc_node.planet_container.scale = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_scale);
        xmlFree(raw_scale);
    }

    xmlChar *raw_enabled = xmlGetProp(xml_node, BAD_CAST "Enabled");
    if (raw_enabled) {
        dc_node.planet_container.enabled = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_enabled);
        xmlFree(raw_enabled);
    }

    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);
    DcAppNodeIndex child_index = _process_xml_node_children(xml_ctx, xml_node, node_index, DC_APP_ELEM_TYPE_PLANET_CONTAINER, directory);
    _get_node(xml_ctx, node_index)->planet_container.child = child_index;
    return node_index;
}

static DcAppNodeIndex _process_xml_node_planet_ellipse(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    (void)directory;

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET_VIEW) {
        DC_LOG_ERROR("PlanetEllipse", "PlanetEllipse must be a child of PlanetView");
        return NODE_INDEX_UNDEFINED;
    }

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_PLANET_ELLIPSE;
    dc_node.parent = parent_node_index;

    // inherit planet_def_index from parent PlanetView
    DcAppNode *parent = _get_node(xml_ctx, parent_node_index);
    dc_node.planet_ellipse.planet_def_index = parent->planet_view.planet_def_index;
    dc_node.planet_ellipse.crs              = parent->planet_view.crs;

    // coordinate reference system
    xmlChar *raw_crs = xmlGetProp(xml_node, BAD_CAST "CRS");
    if (raw_crs) {
        dc_node.planet_ellipse.crs = (DcAppPlanetCrs)atoi((const char *)raw_crs);
        xmlFree(raw_crs);
    }

    // latitude
    xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "Latitude");
    if (raw_lat) {
        dc_node.planet_ellipse.lat = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
        xmlFree(raw_lat);
    }

    // longitude
    xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
    if (raw_lon) {
        dc_node.planet_ellipse.lon = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
        xmlFree(raw_lon);
    }

    // cartesian position
    xmlChar *raw_x = xmlGetProp(xml_node, BAD_CAST "X");
    if (raw_x) {
        dc_node.planet_ellipse.xyz.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x);
        xmlFree(raw_x);
    }
    xmlChar *raw_y = xmlGetProp(xml_node, BAD_CAST "Y");
    if (raw_y) {
        dc_node.planet_ellipse.xyz.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y);
        xmlFree(raw_y);
    }
    xmlChar *raw_z = xmlGetProp(xml_node, BAD_CAST "Z");
    if (raw_z) {
        dc_node.planet_ellipse.xyz.z = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_z);
        xmlFree(raw_z);
    }

    // radius (shorthand for both RadiusX and RadiusY)
    xmlChar      *raw_radius = xmlGetProp(xml_node, BAD_CAST "Radius");
    DcAppValIndex radius_val = DC_APP_VAL_INDEX_UNDEFINED;
    if (raw_radius) {
        radius_val = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius);
        xmlFree(raw_radius);
    }

    // radius x (overrides Radius if specified)
    xmlChar *raw_radius_x = xmlGetProp(xml_node, BAD_CAST "RadiusX");
    if (raw_radius_x) {
        dc_node.planet_ellipse.radius_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius_x);
        xmlFree(raw_radius_x);
    } else {
        dc_node.planet_ellipse.radius_x = radius_val;
    }

    // radius y (overrides Radius if specified)
    xmlChar *raw_radius_y = xmlGetProp(xml_node, BAD_CAST "RadiusY");
    if (raw_radius_y) {
        dc_node.planet_ellipse.radius_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius_y);
        xmlFree(raw_radius_y);
    } else {
        dc_node.planet_ellipse.radius_y = radius_val;
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (raw_rotation) {
        dc_node.planet_ellipse.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
        xmlFree(raw_rotation);
    }

    // height above terrain
    xmlChar *raw_height = xmlGetProp(xml_node, BAD_CAST "HeightAboveTerrain");
    if (raw_height) {
        dc_node.planet_ellipse.height_above_terrain = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_height);
        xmlFree(raw_height);
    }

    // segments
    xmlChar *raw_segments = xmlGetProp(xml_node, BAD_CAST "Segments");
    if (raw_segments) {
        dc_node.planet_ellipse.segments = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_segments);
        xmlFree(raw_segments);
    }

    // line width
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.planet_ellipse.line_width = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // colors
    dc_node.planet_ellipse.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "FillColor", &(dc_node.planet_ellipse.fill_color)))
        dc_node.planet_ellipse.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
    if (_load_color_from_string(xml_ctx, xml_node, "LineColor", &(dc_node.planet_ellipse.line_color)))
        dc_node.planet_ellipse.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    return _register_node(xml_ctx, &dc_node);
}

static void _create_geojson_nodes(
    DcAppXmlContext *xml_ctx, const DcGeojsonFeature *feat,
    DcAppNodeIndex parent, uint8_t planet_def_index,
    DcAppValIndex height, DcAppValIndex line_width,
    DcAppValIndex4 line_color, DcAppValIndex4 fill_color, uint8_t flags,
    DcAppNodeIndex *first_index, DcAppNodeIndex *prev_index);

static void _create_geojson_nodes(
    DcAppXmlContext *xml_ctx, const DcGeojsonFeature *feat,
    DcAppNodeIndex parent, uint8_t planet_def_index,
    DcAppValIndex height, DcAppValIndex line_width,
    DcAppValIndex4 line_color, DcAppValIndex4 fill_color, uint8_t flags,
    DcAppNodeIndex *first_index, DcAppNodeIndex *prev_index)
{
    DcAppNode dc_node = {};
    dc_node.parent = parent;
    DcAppNodeIndex node_index;

    switch (feat->type) {

        case DC_GEOJSON_FEATURE_POINT: {
            const DcGeojsonPosition *pos = &feat->geom.point.position;
            DcAppValIndex point_height = height;
            if (pos->has_alt) {
                DcValue altitude = dc_value_create_value_double(pos->alt);
                point_height = dc_app_lookup_register_value(_lookup(xml_ctx), &altitude);
            }
            char lat_buf[32], lon_buf[32];
            snprintf(lat_buf, sizeof(lat_buf), "%f", pos->lat);
            snprintf(lon_buf, sizeof(lon_buf), "%f", pos->lon);
            dc_node.type = NODE_TYPE_PLANET_SPHERE;
            dc_node.planet_sphere.lat                  = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, lat_buf);
            dc_node.planet_sphere.lon                  = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, lon_buf);
            dc_node.planet_sphere.height_above_terrain = point_height;
            dc_node.planet_sphere.radius               = DC_APP_VAL_INDEX_UNDEFINED;
            dc_node.planet_sphere.fill_color           = line_color;
            dc_node.planet_sphere.config_flags         = NODE_CONFIG_FLAG_FILL_ENABLED;
            dc_node.planet_sphere.planet_def_index     = planet_def_index;
            dc_node.planet_sphere.crs                  = DC_APP_PLANET_CRS_GEODETIC;
            node_index = _register_node(xml_ctx, &dc_node);
            if (*first_index == NODE_INDEX_UNDEFINED) *first_index = node_index;
            if (*prev_index != NODE_INDEX_UNDEFINED) _get_node(xml_ctx, *prev_index)->next = node_index;
            *prev_index = node_index;
            break;
        }

        case DC_GEOJSON_FEATURE_MULTI_POINT:
            for (uint32_t i = 0; i < feat->geom.multi_point.count; i++) {
                const DcGeojsonPosition *pos = &feat->geom.multi_point.positions[i];
                DcAppValIndex point_height = height;
                if (pos->has_alt) {
                    DcValue altitude = dc_value_create_value_double(pos->alt);
                    point_height = dc_app_lookup_register_value(_lookup(xml_ctx), &altitude);
                }
                char lat_buf[32], lon_buf[32];
                snprintf(lat_buf, sizeof(lat_buf), "%f", pos->lat);
                snprintf(lon_buf, sizeof(lon_buf), "%f", pos->lon);
                memset(&dc_node, 0, sizeof(DcAppNode));
                dc_node.parent = parent;
                dc_node.type = NODE_TYPE_PLANET_SPHERE;
                dc_node.planet_sphere.lat                  = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, lat_buf);
                dc_node.planet_sphere.lon                  = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, lon_buf);
                dc_node.planet_sphere.height_above_terrain = point_height;
                dc_node.planet_sphere.radius               = DC_APP_VAL_INDEX_UNDEFINED;
                dc_node.planet_sphere.fill_color           = line_color;
                dc_node.planet_sphere.config_flags         = NODE_CONFIG_FLAG_FILL_ENABLED;
                dc_node.planet_sphere.planet_def_index     = planet_def_index;
                dc_node.planet_sphere.crs                  = DC_APP_PLANET_CRS_GEODETIC;
                node_index = _register_node(xml_ctx, &dc_node);
                if (*first_index == NODE_INDEX_UNDEFINED) *first_index = node_index;
                if (*prev_index != NODE_INDEX_UNDEFINED) _get_node(xml_ctx, *prev_index)->next = node_index;
                *prev_index = node_index;
            }
            break;

        case DC_GEOJSON_FEATURE_LINE_STRING: {
            DcAppPlanetVertexStatic *sb_pts = NULL;
            for (uint32_t p = 0; p < feat->geom.line_string.count; p++) {
                DcAppPlanetVertexStatic v = { .lon = feat->geom.line_string.positions[p].lon, .lat = feat->geom.line_string.positions[p].lat, .alt = feat->geom.line_string.positions[p].alt, .has_alt = feat->geom.line_string.positions[p].has_alt };
                sbpush(sb_pts, v);
            }
            dc_node.type = NODE_TYPE_PLANET_LINE;
            dc_node.planet_line.sb_points_static     = sb_pts;
            dc_node.planet_line.sb_points_dynamic    = NULL;
            dc_node.planet_line.is_dynamic           = false;
            dc_node.planet_line.height_above_terrain = height;
            dc_node.planet_line.line_color           = line_color;
            dc_node.planet_line.line_width           = line_width;
            dc_node.planet_line.config_flags         = flags & NODE_CONFIG_FLAG_LINE_ENABLED ? NODE_CONFIG_FLAG_LINE_ENABLED : NODE_CONFIG_FLAG_NONE;
            dc_node.planet_line.planet_def_index     = planet_def_index;
            dc_node.planet_line.crs                  = DC_APP_PLANET_CRS_GEODETIC;
            node_index = _register_node(xml_ctx, &dc_node);
            if (*first_index == NODE_INDEX_UNDEFINED) *first_index = node_index;
            if (*prev_index != NODE_INDEX_UNDEFINED) _get_node(xml_ctx, *prev_index)->next = node_index;
            *prev_index = node_index;
            break;
        }

        case DC_GEOJSON_FEATURE_MULTI_LINE_STRING:
            for (uint32_t i = 0; i < feat->geom.multi_line_string.count; i++) {
                DcAppPlanetVertexStatic *sb_pts = NULL;
                for (uint32_t p = 0; p < feat->geom.multi_line_string.line_strings[i].count; p++) {
                    DcAppPlanetVertexStatic v = { .lon = feat->geom.multi_line_string.line_strings[i].positions[p].lon, .lat = feat->geom.multi_line_string.line_strings[i].positions[p].lat, .alt = feat->geom.multi_line_string.line_strings[i].positions[p].alt, .has_alt = feat->geom.multi_line_string.line_strings[i].positions[p].has_alt };
                    sbpush(sb_pts, v);
                }
                memset(&dc_node, 0, sizeof(DcAppNode));
                dc_node.parent = parent;
                dc_node.type = NODE_TYPE_PLANET_LINE;
                dc_node.planet_line.sb_points_static     = sb_pts;
                dc_node.planet_line.sb_points_dynamic    = NULL;
                dc_node.planet_line.is_dynamic           = false;
                dc_node.planet_line.height_above_terrain = height;
                dc_node.planet_line.line_color           = line_color;
                dc_node.planet_line.line_width           = line_width;
                dc_node.planet_line.config_flags         = flags & NODE_CONFIG_FLAG_LINE_ENABLED ? NODE_CONFIG_FLAG_LINE_ENABLED : NODE_CONFIG_FLAG_NONE;
                dc_node.planet_line.planet_def_index     = planet_def_index;
                dc_node.planet_line.crs                  = DC_APP_PLANET_CRS_GEODETIC;
                node_index = _register_node(xml_ctx, &dc_node);
                if (*first_index == NODE_INDEX_UNDEFINED) *first_index = node_index;
                if (*prev_index != NODE_INDEX_UNDEFINED) _get_node(xml_ctx, *prev_index)->next = node_index;
                *prev_index = node_index;
            }
            break;

        case DC_GEOJSON_FEATURE_POLYGON: {
            if (feat->geom.polygon.ring_count == 0) break;
            const DcGeojsonCoordArray *ring = &feat->geom.polygon.rings[0];
            DcAppPlanetVertexStatic *sb_pts = NULL;
            for (uint32_t p = 0; p < ring->count; p++) {
                DcAppPlanetVertexStatic v = { .lon = ring->positions[p].lon, .lat = ring->positions[p].lat, .alt = ring->positions[p].alt, .has_alt = ring->positions[p].has_alt };
                sbpush(sb_pts, v);
            }
            dc_node.type = NODE_TYPE_PLANET_POLYGON;
            dc_node.planet_polygon.sb_points_static     = sb_pts;
            dc_node.planet_polygon.sb_points_dynamic    = NULL;
            dc_node.planet_polygon.is_dynamic           = false;
            dc_node.planet_polygon.height_above_terrain = height;
            dc_node.planet_polygon.line_color           = line_color;
            dc_node.planet_polygon.line_width           = line_width;
            dc_node.planet_polygon.fill_color           = fill_color;
            dc_node.planet_polygon.config_flags         = flags;
            dc_node.planet_polygon.planet_def_index     = planet_def_index;
            dc_node.planet_polygon.crs                  = DC_APP_PLANET_CRS_GEODETIC;
            node_index = _register_node(xml_ctx, &dc_node);
            if (*first_index == NODE_INDEX_UNDEFINED) *first_index = node_index;
            if (*prev_index != NODE_INDEX_UNDEFINED) _get_node(xml_ctx, *prev_index)->next = node_index;
            *prev_index = node_index;
            break;
        }

        case DC_GEOJSON_FEATURE_MULTI_POLYGON:
            for (uint32_t i = 0; i < feat->geom.multi_polygon.count; i++) {
                if (feat->geom.multi_polygon.polygons[i].ring_count == 0) continue;
                const DcGeojsonCoordArray *ring = &feat->geom.multi_polygon.polygons[i].rings[0];
                DcAppPlanetVertexStatic *sb_pts = NULL;
                for (uint32_t p = 0; p < ring->count; p++) {
                    DcAppPlanetVertexStatic v = { .lon = ring->positions[p].lon, .lat = ring->positions[p].lat, .alt = ring->positions[p].alt, .has_alt = ring->positions[p].has_alt };
                    sbpush(sb_pts, v);
                }
                memset(&dc_node, 0, sizeof(DcAppNode));
                dc_node.parent = parent;
                dc_node.type = NODE_TYPE_PLANET_POLYGON;
                dc_node.planet_polygon.sb_points_static     = sb_pts;
                dc_node.planet_polygon.sb_points_dynamic    = NULL;
                dc_node.planet_polygon.is_dynamic           = false;
                dc_node.planet_polygon.height_above_terrain = height;
                dc_node.planet_polygon.line_color           = line_color;
                dc_node.planet_polygon.line_width           = line_width;
                dc_node.planet_polygon.fill_color           = fill_color;
                dc_node.planet_polygon.config_flags         = flags;
                dc_node.planet_polygon.planet_def_index     = planet_def_index;
                dc_node.planet_polygon.crs                  = DC_APP_PLANET_CRS_GEODETIC;
                node_index = _register_node(xml_ctx, &dc_node);
                if (*first_index == NODE_INDEX_UNDEFINED) *first_index = node_index;
                if (*prev_index != NODE_INDEX_UNDEFINED) _get_node(xml_ctx, *prev_index)->next = node_index;
                *prev_index = node_index;
            }
            break;

        case DC_GEOJSON_FEATURE_GEOMETRY_COLLECTION:
            for (uint32_t i = 0; i < feat->geom.geometry_collection.count; i++)
                _create_geojson_nodes(xml_ctx, &feat->geom.geometry_collection.features[i], parent, planet_def_index, height, line_width, line_color, fill_color, flags, first_index, prev_index);
            break;

        case DC_GEOJSON_FEATURE_UNDEFINED:
            break;
    }
}

static DcAppNodeIndex _process_xml_node_planet_geo_json(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET_VIEW) {
        DC_LOG_ERROR("PlanetGeoJSON", "PlanetGeoJSON must be a child of PlanetView");
        return NODE_INDEX_UNDEFINED;
    }

    // file (required)
    xmlChar *raw_filepath = xmlGetProp(xml_node, BAD_CAST "File");
    if (!raw_filepath) {
        DC_LOG_ERROR("PlanetGeoJSON", "Missing 'File' attribute");
        return NODE_INDEX_UNDEFINED;
    }

    char cleaned_filepath[DC_VALUE_STRING_BUFFER_SIZE];
    strncpy(cleaned_filepath, (const char *)raw_filepath, DC_VALUE_STRING_BUFFER_SIZE - 1);
    cleaned_filepath[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
    xmlFree(raw_filepath);

    char abs_filepath[DC_VALUE_STRING_BUFFER_SIZE];
    if (dc_utils_is_relative_path(cleaned_filepath)) {
        dc_utils_join_paths(directory, cleaned_filepath, abs_filepath, sizeof(abs_filepath));
    } else {
        strcpy(abs_filepath, cleaned_filepath);
    }

    // load and parse geojson
    DcGeojson *geojson = dc_geojson_load(abs_filepath);
    if (!geojson) {
        DC_LOG_ERROR("PlanetGeoJSON", "Failed to load GeoJSON: %s", abs_filepath);
        return NODE_INDEX_UNDEFINED;
    }

    uint32_t feature_count = dc_geojson_feature_count(geojson);
    if (feature_count == 0) {
        dc_geojson_free(geojson);
        return NODE_INDEX_UNDEFINED;
    }

    // parse default attributes from XML
    DcAppNode *parent = _get_node(xml_ctx, parent_node_index);
    uint8_t planet_def_index = parent->planet_view.planet_def_index;

    xmlChar *raw_crs = xmlGetProp(xml_node, BAD_CAST "CRS");
    if (raw_crs) {
        DcAppPlanetCrs crs = (DcAppPlanetCrs)atoi((const char *)raw_crs);
        if (crs != DC_APP_PLANET_CRS_GEODETIC) {
            DC_LOG_WARN("PlanetGeoJSON", "Only geodetic CRS is currently supported; using geodetic");
        }
        xmlFree(raw_crs);
    }

    DcAppValIndex default_height = DC_APP_VAL_INDEX_UNDEFINED;
    xmlChar *raw_height = xmlGetProp(xml_node, BAD_CAST "HeightAboveTerrain");
    if (raw_height) {
        default_height = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_height);
        xmlFree(raw_height);
    }

    DcAppValIndex default_line_width = DC_APP_VAL_INDEX_UNDEFINED;
    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        default_line_width = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    // default colors from XML
    DcAppValIndex4 default_line_color = {};
    DcAppValIndex4 default_fill_color = {};
    uint8_t    default_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "LineColor", &default_line_color))
        default_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;
    if (_load_color_from_string(xml_ctx, xml_node, "FillColor", &default_fill_color))
        default_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;

    // create nodes for each feature, chained via .next
    DcAppNodeIndex first_index = NODE_INDEX_UNDEFINED;
    DcAppNodeIndex prev_index  = NODE_INDEX_UNDEFINED;

    for (uint32_t f = 0; f < feature_count; f++) {
        const DcGeojsonFeature *feat = dc_geojson_feature(geojson, f);

        // determine per-feature style overrides
        DcAppValIndex4 feat_line_color = default_line_color;
        DcAppValIndex4 feat_fill_color = default_fill_color;
        uint8_t    feat_flags      = default_flags;

        if (feat->style.stroke.has_value) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%f", (double)feat->style.stroke.r);
            feat_line_color.r = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, buf);
            snprintf(buf, sizeof(buf), "%f", (double)feat->style.stroke.g);
            feat_line_color.g = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, buf);
            snprintf(buf, sizeof(buf), "%f", (double)feat->style.stroke.b);
            feat_line_color.b = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, buf);
            snprintf(buf, sizeof(buf), "%f", (double)feat->style.stroke.a);
            feat_line_color.a = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, buf);
            feat_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;
        }
        if (feat->style.fill.has_value) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%f", (double)feat->style.fill.r);
            feat_fill_color.r = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, buf);
            snprintf(buf, sizeof(buf), "%f", (double)feat->style.fill.g);
            feat_fill_color.g = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, buf);
            snprintf(buf, sizeof(buf), "%f", (double)feat->style.fill.b);
            feat_fill_color.b = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, buf);
            snprintf(buf, sizeof(buf), "%f", (double)feat->style.fill.a);
            feat_fill_color.a = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, buf);
            feat_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
        }

        DcAppValIndex feat_line_width = default_line_width;
        if (feat->style.has_stroke_width) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%f", (double)feat->style.stroke_width);
            feat_line_width = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, buf);
        }

        _create_geojson_nodes(xml_ctx, feat, parent_node_index, planet_def_index, default_height, feat_line_width, feat_line_color, feat_fill_color, feat_flags, &first_index, &prev_index);
    }

    dc_geojson_free(geojson);
    return first_index;
}

static DcAppNodeIndex _process_xml_node_planet_image(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET_VIEW) {
        DC_LOG_ERROR("PlanetImage", "PlanetImage must be a child of PlanetView");
        return NODE_INDEX_UNDEFINED;
    }

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_PLANET_IMAGE;
    dc_node.parent = parent_node_index;

    DcAppNode *parent = _get_node(xml_ctx, parent_node_index);
    dc_node.planet_image.planet_def_index = parent->planet_view.planet_def_index;
    dc_node.planet_image.crs              = parent->planet_view.crs;

    xmlChar *raw_file = xmlGetProp(xml_node, BAD_CAST "File");
    if (!raw_file || xmlStrlen(raw_file) == 0) {
        if (raw_file) xmlFree(raw_file);
        raw_file = xmlNodeGetContent(xml_node);
    }
    if (raw_file) {
        char cleaned_file[DC_UTILS_FILEPATH_BUFFER_SIZE];
        strncpy(cleaned_file, (const char *)raw_file, sizeof(cleaned_file) - 1);
        cleaned_file[sizeof(cleaned_file) - 1] = '\0';
        xmlFree(raw_file);
        dc_utils_trim_whitespace_inplace(cleaned_file);
        dc_node.planet_image.texture_index = dc_app_texture_load_image_index(xml_ctx->textures, cleaned_file, directory);
    } else {
        DC_LOG_ERROR("PlanetImage", "Missing 'File' attribute");
    }

    xmlChar *raw_crs = xmlGetProp(xml_node, BAD_CAST "CRS");
    if (raw_crs) {
        dc_node.planet_image.crs = (DcAppPlanetCrs)atoi((const char *)raw_crs);
        xmlFree(raw_crs);
    }

    xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "Latitude");
    if (raw_lat) {
        dc_node.planet_image.lat = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
        xmlFree(raw_lat);
    }

    xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
    if (raw_lon) {
        dc_node.planet_image.lon = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
        xmlFree(raw_lon);
    }

    xmlChar *raw_x = xmlGetProp(xml_node, BAD_CAST "X");
    if (raw_x) {
        dc_node.planet_image.xyz.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x);
        xmlFree(raw_x);
    }

    xmlChar *raw_y = xmlGetProp(xml_node, BAD_CAST "Y");
    if (raw_y) {
        dc_node.planet_image.xyz.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y);
        xmlFree(raw_y);
    }

    xmlChar *raw_z = xmlGetProp(xml_node, BAD_CAST "Z");
    if (raw_z) {
        dc_node.planet_image.xyz.z = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_z);
        xmlFree(raw_z);
    }

    xmlChar *raw_height = xmlGetProp(xml_node, BAD_CAST "HeightAboveTerrain");
    if (raw_height) {
        dc_node.planet_image.height_above_terrain = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_height);
        xmlFree(raw_height);
    }

    xmlChar *raw_width = xmlGetProp(xml_node, BAD_CAST "Width");
    if (!raw_width) raw_width = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_width) raw_width = xmlGetProp(xml_node, BAD_CAST "Size");
    if (raw_width) {
        dc_node.planet_image.dimension.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_width);
        xmlFree(raw_width);
    }

    xmlChar *raw_height_px = xmlGetProp(xml_node, BAD_CAST "Height");
    if (!raw_height_px) raw_height_px = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (raw_height_px) {
        dc_node.planet_image.dimension.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_height_px);
        xmlFree(raw_height_px);
    }

    dc_node.planet_image.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "TintColor", &(dc_node.planet_image.tint_color)) ||
        _load_color_from_string(xml_ctx, xml_node, "Color", &(dc_node.planet_image.tint_color)) ||
        _load_color_from_string(xml_ctx, xml_node, "FillColor", &(dc_node.planet_image.tint_color))) {
        dc_node.planet_image.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
    }

    return _register_node(xml_ctx, &dc_node);
}

static DcAppNodeIndex _process_xml_node_planet_line(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET_VIEW &&
        parent_elem_type != DC_APP_ELEM_TYPE_PLANET_CONTAINER) {
        DC_LOG_ERROR("PlanetLine", "PlanetLine must be a child of PlanetView or PlanetContainer");
        return NODE_INDEX_UNDEFINED;
    }

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_PLANET_LINE;
    dc_node.parent = parent_node_index;

    DcAppNode *parent = _get_node(xml_ctx, parent_node_index);
    if (parent_elem_type == DC_APP_ELEM_TYPE_PLANET_CONTAINER) {
        dc_node.planet_line.planet_def_index = parent->planet_container.planet_def_index;
        dc_node.planet_line.crs              = DC_APP_PLANET_CRS_GEODETIC;
    } else {
        dc_node.planet_line.planet_def_index = parent->planet_view.planet_def_index;
        dc_node.planet_line.crs              = parent->planet_view.crs;
    }
    dc_node.planet_line.sb_points_static  = NULL;
    dc_node.planet_line.sb_points_dynamic = NULL;
    dc_node.planet_line.is_dynamic        = true;

    xmlChar *raw_crs = xmlGetProp(xml_node, BAD_CAST "CRS");
    if (raw_crs) {
        dc_node.planet_line.crs = (DcAppPlanetCrs)atoi((const char *)raw_crs);
        xmlFree(raw_crs);
    }

    xmlChar *raw_height = xmlGetProp(xml_node, BAD_CAST "HeightAboveTerrain");
    if (raw_height) {
        dc_node.planet_line.height_above_terrain = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_height);
        xmlFree(raw_height);
    }

    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.planet_line.line_width = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    xmlChar *raw_line_pattern = xmlGetProp(xml_node, BAD_CAST "LinePattern");
    if (raw_line_pattern) {
        dc_node.planet_line.line_pattern = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_line_pattern);
        xmlFree(raw_line_pattern);
    }

    dc_node.planet_line.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "LineColor", &(dc_node.planet_line.line_color)))
        dc_node.planet_line.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    xmlChar *raw_enabled = xmlGetProp(xml_node, BAD_CAST "Enabled");
    if (raw_enabled) {
        dc_node.planet_line.enabled = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_enabled);
        xmlFree(raw_enabled);
    }

    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process Vertex children
    _process_xml_node_children(xml_ctx, xml_node, node_index, DC_APP_ELEM_TYPE_PLANET_LINE, directory);

    return node_index;
}

static DcAppNodeIndex _process_xml_node_planet_polygon(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET_VIEW &&
        parent_elem_type != DC_APP_ELEM_TYPE_PLANET_CONTAINER) {
        DC_LOG_ERROR("PlanetPolygon", "PlanetPolygon must be a child of PlanetView or PlanetContainer");
        return NODE_INDEX_UNDEFINED;
    }

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_PLANET_POLYGON;
    dc_node.parent = parent_node_index;

    DcAppNode *parent = _get_node(xml_ctx, parent_node_index);
    if (parent_elem_type == DC_APP_ELEM_TYPE_PLANET_CONTAINER) {
        dc_node.planet_polygon.planet_def_index = parent->planet_container.planet_def_index;
        dc_node.planet_polygon.crs              = DC_APP_PLANET_CRS_GEODETIC;
    } else {
        dc_node.planet_polygon.planet_def_index = parent->planet_view.planet_def_index;
        dc_node.planet_polygon.crs              = parent->planet_view.crs;
    }
    dc_node.planet_polygon.sb_points_static  = NULL;
    dc_node.planet_polygon.sb_points_dynamic = NULL;
    dc_node.planet_polygon.is_dynamic        = true;

    xmlChar *raw_crs = xmlGetProp(xml_node, BAD_CAST "CRS");
    if (raw_crs) {
        dc_node.planet_polygon.crs = (DcAppPlanetCrs)atoi((const char *)raw_crs);
        xmlFree(raw_crs);
    }

    xmlChar *raw_height = xmlGetProp(xml_node, BAD_CAST "HeightAboveTerrain");
    if (raw_height) {
        dc_node.planet_polygon.height_above_terrain = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_height);
        xmlFree(raw_height);
    }

    xmlChar *raw_line_width = xmlGetProp(xml_node, BAD_CAST "LineWidth");
    if (raw_line_width) {
        dc_node.planet_polygon.line_width = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_line_width);
        xmlFree(raw_line_width);
    }

    xmlChar *raw_line_pattern = xmlGetProp(xml_node, BAD_CAST "LinePattern");
    if (raw_line_pattern) {
        dc_node.planet_polygon.line_pattern = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_line_pattern);
        xmlFree(raw_line_pattern);
    }

    dc_node.planet_polygon.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "FillColor", &(dc_node.planet_polygon.fill_color)))
        dc_node.planet_polygon.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
    if (_load_color_from_string(xml_ctx, xml_node, "LineColor", &(dc_node.planet_polygon.line_color)))
        dc_node.planet_polygon.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;

    xmlChar *raw_enabled = xmlGetProp(xml_node, BAD_CAST "Enabled");
    if (raw_enabled) {
        dc_node.planet_polygon.enabled = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_enabled);
        xmlFree(raw_enabled);
    }

    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // process Vertex children
    _process_xml_node_children(xml_ctx, xml_node, node_index, DC_APP_ELEM_TYPE_PLANET_POLYGON, directory);

    return node_index;
}

// Mounts the directory containing abs_path under a stable VFS mount point
// (derived by hashing the directory), then writes the VFS path into vfs_out.
static void _planet_abs_path_to_vfs(const char *abs_path, char *vfs_out, size_t vfs_out_size) {
    char dir[DC_VALUE_STRING_BUFFER_SIZE];
    dc_utils_get_directory(abs_path, dir, sizeof(dir));

    char hash[32];
    dc_utils_string_to_hash(dir, hash, sizeof(hash));

    char vfs_mount[33];
    snprintf(vfs_mount, sizeof(vfs_mount), "/%s", hash);

    // idempotent — VFS silently ignores duplicate mounts to the same virtual path
    _ext_vfs->mount_directory(vfs_mount, dir, PL_VFS_MOUNT_FLAGS_NONE);

    // extract filename (handle both / and \ separators)
    const char *fslash   = strrchr(abs_path, '/');
    const char *bslash   = strrchr(abs_path, '\\');
    const char *filename = (fslash > bslash) ? fslash + 1 : (bslash ? bslash + 1 : abs_path);

    snprintf(vfs_out, vfs_out_size, "%s/%s", vfs_mount, filename);
}

static DcAppNodeIndex _process_xml_node_planet_shader(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    (void)parent_node_index;

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET) {
        DC_LOG_ERROR("PlanetShader", "Invalid parent element type: %s", dc_app_elem_type_to_string(parent_elem_type));
        return NODE_INDEX_UNDEFINED;
    }

    uint32_t definition_count = dc_app_scene_get_planet_definition_count(xml_ctx->scene);
    DcAppPlanetDefinition *def = dc_app_scene_get_planet_definition(xml_ctx->scene, definition_count - 1);

    // Index (required)
    xmlChar *raw_index = xmlGetProp(xml_node, BAD_CAST "Index");
    if (!raw_index) {
        DC_LOG_ERROR("PlanetShader", "Missing required 'Index' attribute");
        return NODE_INDEX_UNDEFINED;
    }
    DcAppPlanetShaderEntry entry = {0};
    entry.index              = atoi((const char *)raw_index);
    xmlFree(raw_index);

    // VertexShader (optional)
    xmlChar *raw_vert = xmlGetProp(xml_node, BAD_CAST "VertexShader");
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
        _planet_abs_path_to_vfs(abs_path, vfs_path, sizeof(vfs_path));
        entry.vertex_path = strdup(vfs_path);
    }

    // FragmentShader (optional)
    xmlChar *raw_frag = xmlGetProp(xml_node, BAD_CAST "FragmentShader");
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
        _planet_abs_path_to_vfs(abs_path, vfs_path, sizeof(vfs_path));
        entry.fragment_path = strdup(vfs_path);
    }

    sbpush(def->sb_shaders, entry);

    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_planet_sphere(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    (void)directory;

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET_VIEW) {
        DC_LOG_ERROR("PlanetSphere", "PlanetSphere must be a child of PlanetView");
        return NODE_INDEX_UNDEFINED;
    }

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_PLANET_SPHERE;
    dc_node.parent = parent_node_index;

    DcAppNode *parent = _get_node(xml_ctx, parent_node_index);
    dc_node.planet_sphere.planet_def_index = parent->planet_view.planet_def_index;
    dc_node.planet_sphere.crs              = parent->planet_view.crs;

    xmlChar *raw_crs = xmlGetProp(xml_node, BAD_CAST "CRS");
    if (raw_crs) {
        dc_node.planet_sphere.crs = (DcAppPlanetCrs)atoi((const char *)raw_crs);
        xmlFree(raw_crs);
    }

    xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "Latitude");
    if (raw_lat) {
        dc_node.planet_sphere.lat = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
        xmlFree(raw_lat);
    }

    xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
    if (raw_lon) {
        dc_node.planet_sphere.lon = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
        xmlFree(raw_lon);
    }

    xmlChar *raw_x = xmlGetProp(xml_node, BAD_CAST "X");
    if (raw_x) {
        dc_node.planet_sphere.xyz.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x);
        xmlFree(raw_x);
    }

    xmlChar *raw_y = xmlGetProp(xml_node, BAD_CAST "Y");
    if (raw_y) {
        dc_node.planet_sphere.xyz.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y);
        xmlFree(raw_y);
    }

    xmlChar *raw_z = xmlGetProp(xml_node, BAD_CAST "Z");
    if (raw_z) {
        dc_node.planet_sphere.xyz.z = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_z);
        xmlFree(raw_z);
    }

    xmlChar *raw_height = xmlGetProp(xml_node, BAD_CAST "HeightAboveTerrain");
    if (raw_height) {
        dc_node.planet_sphere.height_above_terrain = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_height);
        xmlFree(raw_height);
    }

    xmlChar *raw_radius = xmlGetProp(xml_node, BAD_CAST "Radius");
    if (raw_radius) {
        dc_node.planet_sphere.radius = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_radius);
        xmlFree(raw_radius);
    }

    dc_node.planet_sphere.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "FillColor", &(dc_node.planet_sphere.fill_color)))
        dc_node.planet_sphere.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;

    return _register_node(xml_ctx, &dc_node);
}

static DcAppNodeIndex _process_xml_node_planet_text(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    (void)directory;

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET_VIEW) {
        DC_LOG_ERROR("PlanetText", "PlanetText must be a child of PlanetView");
        return NODE_INDEX_UNDEFINED;
    }

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_PLANET_TEXT;
    dc_node.parent = parent_node_index;

    // inherit planet_def_index from parent PlanetView
    DcAppNode *parent = _get_node(xml_ctx, parent_node_index);
    dc_node.planet_text.planet_def_index = parent->planet_view.planet_def_index;
    dc_node.planet_text.crs              = parent->planet_view.crs;

    xmlChar *raw_crs = xmlGetProp(xml_node, BAD_CAST "CRS");
    if (raw_crs) {
        dc_node.planet_text.crs = (DcAppPlanetCrs)atoi((const char *)raw_crs);
        xmlFree(raw_crs);
    }

    // text content (same parsing as _process_xml_node_text)
    xmlChar *raw_text = xmlNodeGetContent(xml_node);
    if (raw_text) {
        char cleaned_text[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned_text, (const char *)raw_text, DC_VALUE_STRING_BUFFER_SIZE - 1);
        cleaned_text[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        xmlFree(raw_text);
        dc_utils_trim_whitespace_inplace(cleaned_text);

        sbclear(xml_ctx->sb_text_filler);
        for (size_t ii = 0; ii < strlen(cleaned_text);) {
            if (cleaned_text[ii] == '\\') {
                if (ii + 1 < strlen(cleaned_text)) {
                    char next_char = cleaned_text[ii + 1];
                    if (next_char == 'n') {
                        sbpush(xml_ctx->sb_text_filler, '\n');
                    } else if (next_char == 'r') {
                        sbpush(xml_ctx->sb_text_filler, '\r');
                    } else if (next_char == 't') {
                        sbpush(xml_ctx->sb_text_filler, '\t');
                    } else if (next_char == '\\') {
                        sbpush(xml_ctx->sb_text_filler, '\\');
                    } else if (next_char == '"') {
                        sbpush(xml_ctx->sb_text_filler, '"');
                    } else if (next_char == '\'') {
                        sbpush(xml_ctx->sb_text_filler, '\'');
                    } else if (dc_utils_char_in(next_char, "@#$%")) {
                        sbpush(xml_ctx->sb_text_filler, next_char);
                    } else {
                        sbpush(xml_ctx->sb_text_filler, '\\');
                        sbpush(xml_ctx->sb_text_filler, next_char);
                    }
                    ii += 2;
                } else {
                    sbpush(xml_ctx->sb_text_filler, cleaned_text[ii++]);
                }
                continue;
            }

            if (cleaned_text[ii] == '@') {
                size_t start = ii;
                ii++;

                char var[DC_VALUE_STRING_BUFFER_SIZE] = {0};

                if (ii < strlen(cleaned_text) && cleaned_text[ii] == '{') {
                    ii++;
                    int end = dc_utils_str_find_first(&(cleaned_text[ii]), '}');
                    if (end == -1) {
                        sbpushn(xml_ctx->sb_text_filler, &(cleaned_text[start]), (int)(ii - start));
                        continue;
                    }
                    strncpy(var, &(cleaned_text[ii]), end);
                    var[end] = '\0';
                    ii += end + 1;
                } else {
                    size_t start_var = ii;
                    while (ii < strlen(cleaned_text) && !isspace(cleaned_text[ii]) && cleaned_text[ii] != '(') {
                        ii++;
                    }
                    size_t len = ii - start_var;
                    strncpy(var, &(cleaned_text[start_var]), len);
                    var[len] = '\0';
                }

                DcAppVarIndex var_index = dc_app_lookup_get_var_index(_lookup(xml_ctx), var);
                if (var_index != DC_APP_VAR_INDEX_UNDEFINED) {
                    sbpush(dc_node.planet_text.sb_vals, dc_app_lookup_get_var_value_index(_lookup(xml_ctx), var_index));
                } else {
                    DC_LOG_ERROR("PlanetText", "Unknown variable '%s'", var);
                    sbpush(dc_node.planet_text.sb_vals, DC_APP_VAL_INDEX_UNDEFINED);
                }

                char format_spec[DC_VALUE_STRING_BUFFER_SIZE] = {0};
                if (ii < strlen(cleaned_text) && cleaned_text[ii] == '(') {
                    int close = dc_utils_str_find_first(&(cleaned_text[ii]), ')');
                    if (close > 1) {
                        size_t len = close - 1;
                        strncpy(format_spec, &(cleaned_text[ii + 1]), len);
                        format_spec[len] = '\0';
                        ii += close + 1;

                        if (dc_utils_is_format_specifier_int(format_spec)) {
                            sbpush(dc_node.planet_text.sb_format_types, DC_VALUE_TYPE_INTEGER);
                            sbpush(dc_node.planet_text.sb_format_indices, (uint8_t)sbcount(dc_node.planet_text.sb_formats));
                            sbpushn(dc_node.planet_text.sb_formats, format_spec, (int)strlen(format_spec) + 1);
                        } else if (dc_utils_is_format_specifier_double(format_spec)) {
                            sbpush(dc_node.planet_text.sb_format_types, DC_VALUE_TYPE_DOUBLE);
                            sbpush(dc_node.planet_text.sb_format_indices, (uint8_t)sbcount(dc_node.planet_text.sb_formats));
                            sbpushn(dc_node.planet_text.sb_formats, format_spec, (int)strlen(format_spec) + 1);
                        } else if (dc_utils_is_format_specifier_string(format_spec)) {
                            sbpush(dc_node.planet_text.sb_format_types, DC_VALUE_TYPE_STRING);
                            sbpush(dc_node.planet_text.sb_format_indices, (uint8_t)sbcount(dc_node.planet_text.sb_formats));
                            sbpushn(dc_node.planet_text.sb_formats, format_spec, (int)strlen(format_spec) + 1);
                        } else if (dc_utils_is_format_specifier_bool(format_spec)) {
                            sbpush(dc_node.planet_text.sb_format_types, DC_VALUE_TYPE_BOOLEAN);
                            sbpush(dc_node.planet_text.sb_format_indices, (uint8_t)sbcount(dc_node.planet_text.sb_formats));
                            sbpushn(dc_node.planet_text.sb_formats, format_spec, (int)strlen(format_spec) + 1);
                        } else {
                            DC_LOG_ERROR("PlanetText", "Unknown format specifier: %s", format_spec);
                            sbpush(dc_node.planet_text.sb_format_types, DC_VALUE_TYPE_STRING);
                            sbpush(dc_node.planet_text.sb_format_indices, (uint8_t)sbcount(dc_node.planet_text.sb_formats));
                            sbpushn(dc_node.planet_text.sb_formats, "%s", 3);
                        }
                    } else {
                        sbpush(dc_node.planet_text.sb_format_types, DC_VALUE_TYPE_STRING);
                        sbpush(dc_node.planet_text.sb_format_indices, (uint8_t)sbcount(dc_node.planet_text.sb_formats));
                        sbpushn(dc_node.planet_text.sb_formats, "%s", 3);
                    }
                } else {
                    sbpush(dc_node.planet_text.sb_format_types, DC_VALUE_TYPE_STRING);
                    sbpush(dc_node.planet_text.sb_format_indices, (uint8_t)sbcount(dc_node.planet_text.sb_formats));
                    sbpushn(dc_node.planet_text.sb_formats, "%s", 3);
                }

                sbpush(dc_node.planet_text.sb_filler_indices, (uint8_t)sbcount(dc_node.planet_text.sb_fillers));
                sbpushn(dc_node.planet_text.sb_fillers, xml_ctx->sb_text_filler, sbcount(xml_ctx->sb_text_filler));
                sbpush(dc_node.planet_text.sb_fillers, '\0');
                sbclear(xml_ctx->sb_text_filler);

                continue;
            }

            sbpush(xml_ctx->sb_text_filler, cleaned_text[ii++]);
        }

        sbpush(dc_node.planet_text.sb_filler_indices, (uint8_t)sbcount(dc_node.planet_text.sb_fillers));
        sbpushn(dc_node.planet_text.sb_fillers, xml_ctx->sb_text_filler, sbcount(xml_ctx->sb_text_filler));
        sbpush(dc_node.planet_text.sb_fillers, '\0');
        sbclear(xml_ctx->sb_text_filler);
    } else {
        DC_LOG_ERROR("PlanetText", "Missing node content");
    }

    // latitude
    xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "Latitude");
    if (raw_lat) {
        dc_node.planet_text.lat = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
        xmlFree(raw_lat);
    }

    // longitude
    xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
    if (raw_lon) {
        dc_node.planet_text.lon = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
        xmlFree(raw_lon);
    }

    // cartesian position
    xmlChar *raw_x = xmlGetProp(xml_node, BAD_CAST "X");
    if (raw_x) {
        dc_node.planet_text.xyz.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x);
        xmlFree(raw_x);
    }
    xmlChar *raw_y = xmlGetProp(xml_node, BAD_CAST "Y");
    if (raw_y) {
        dc_node.planet_text.xyz.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y);
        xmlFree(raw_y);
    }
    xmlChar *raw_z = xmlGetProp(xml_node, BAD_CAST "Z");
    if (raw_z) {
        dc_node.planet_text.xyz.z = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_z);
        xmlFree(raw_z);
    }

    // height above terrain
    xmlChar *raw_height = xmlGetProp(xml_node, BAD_CAST "HeightAboveTerrain");
    if (raw_height) {
        dc_node.planet_text.height_above_terrain = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_height);
        xmlFree(raw_height);
    }

    // size
    xmlChar *raw_size = xmlGetProp(xml_node, BAD_CAST "Size");
    if (raw_size) {
        dc_node.planet_text.size = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_size);
        xmlFree(raw_size);
    }

    // color
    dc_node.planet_text.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "FillColor", &(dc_node.planet_text.fill_color)))
        dc_node.planet_text.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;

    return _register_node(xml_ctx, &dc_node);
}

static DcAppNodeIndex _process_xml_node_planet_texture(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    (void)parent_node_index;

    if (parent_elem_type != DC_APP_ELEM_TYPE_PLANET) {
        DC_LOG_ERROR("PlanetTexture", "Invalid parent element type: %s", dc_app_elem_type_to_string(parent_elem_type));
        return NODE_INDEX_UNDEFINED;
    }

    uint32_t definition_count = dc_app_scene_get_planet_definition_count(xml_ctx->scene);
    DcAppPlanetDefinition *def = dc_app_scene_get_planet_definition(xml_ctx->scene, definition_count - 1);

    if (sbcount(def->sb_textures) >= DC_APP_PLANET_TEXTURE_SLOT_COUNT) {
        DC_LOG_ERROR("PlanetTexture", "A <Planet> supports at most %u <PlanetTexture> elements (line %ld)", DC_APP_PLANET_TEXTURE_SLOT_COUNT, xmlGetLineNo(xml_node));
        return NODE_INDEX_UNDEFINED;
    }

    DcAppPlanetTextureEntry entry = {0};
    entry.crs  = def->crs;
    // XML order assigns each texture its zero-based slot.
    entry.slot = (uint8_t)sbcount(def->sb_textures);

    // coordinate reference system
    xmlChar *raw_crs = xmlGetProp(xml_node, BAD_CAST "CRS");
    if (raw_crs) {
        entry.crs = (DcAppPlanetCrs)atoi((const char *)raw_crs);
        xmlFree(raw_crs);
    }

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
        _planet_abs_path_to_vfs(abs_path, vfs_path, sizeof(vfs_path));
        entry.source = strdup(vfs_path);
    }

    // meters per pixel
    xmlChar *raw_mpp = xmlGetProp(xml_node, BAD_CAST "MetersPerPixel");
    if (raw_mpp) {
        entry.mpp = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_mpp);
        xmlFree(raw_mpp);
    }

    // geodetic center
    xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "Latitude");
    if (raw_lat) {
        entry.lle.lat = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
        xmlFree(raw_lat);
    }
    xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
    if (raw_lon) {
        entry.lle.lon = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
        xmlFree(raw_lon);
    }

    // projected origin x
    xmlChar *raw_origin_x = xmlGetProp(xml_node, BAD_CAST "OriginX");
    if (raw_origin_x) {
        entry.originX = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_origin_x);
        xmlFree(raw_origin_x);
    }

    // projected origin y
    xmlChar *raw_origin_y = xmlGetProp(xml_node, BAD_CAST "OriginY");
    if (raw_origin_y) {
        entry.originY = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_origin_y);
        xmlFree(raw_origin_y);
    }

    // cartesian center
    xmlChar *raw_x = xmlGetProp(xml_node, BAD_CAST "X");
    if (raw_x) {
        entry.xyz.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x);
        xmlFree(raw_x);
    }
    xmlChar *raw_y = xmlGetProp(xml_node, BAD_CAST "Y");
    if (raw_y) {
        entry.xyz.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y);
        xmlFree(raw_y);
    }
    xmlChar *raw_z = xmlGetProp(xml_node, BAD_CAST "Z");
    if (raw_z) {
        entry.xyz.z = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_z);
        xmlFree(raw_z);
    }

    // load/remove this texture slot
    xmlChar *raw_enabled = xmlGetProp(xml_node, BAD_CAST "Enabled");
    if (raw_enabled) {
        entry.enabled = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_enabled);
        xmlFree(raw_enabled);
    }

    // edge-triggered refresh
    xmlChar *raw_fire_refresh = xmlGetProp(xml_node, BAD_CAST "FireRefresh");
    if (raw_fire_refresh) {
        entry.fire_refresh = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_fire_refresh);
        xmlFree(raw_fire_refresh);
    }

    sbpush(def->sb_textures, entry);

    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_planet_view(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);
    (void)elem_type;
    (void)parent_elem_type;
    (void)directory;

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_PLANET_VIEW;
    dc_node.parent = parent_node_index;

    // resolves the planet reference at parse time.
    xmlChar *raw_planet                  = xmlGetProp(xml_node, BAD_CAST "Planet");
    dc_node.planet_view.planet_def_index = UINT8_MAX;
    if (raw_planet) {
        int def_count = (int)dc_app_scene_get_planet_definition_count(xml_ctx->scene);
        for (int j = 0; j < def_count; j++) {
            DcAppPlanetDefinition *definition = dc_app_scene_get_planet_definition(xml_ctx->scene, (uint32_t)j);
            if (definition->name && strcmp(definition->name, (const char *)raw_planet) == 0) {
                dc_node.planet_view.planet_def_index = (uint8_t)j;
                break;
            }
        }
        if (dc_node.planet_view.planet_def_index == UINT8_MAX) {
            DC_LOG_ERROR("PlanetView", "Planet '%s' not found", (const char *)raw_planet);
        }
        xmlFree(raw_planet);
    }
    dc_node.planet_view.crs = DC_APP_PLANET_CRS_GEODETIC;
    xmlChar *raw_crs        = xmlGetProp(xml_node, BAD_CAST "CRS");
    if (raw_crs) {
        dc_node.planet_view.crs = (DcAppPlanetCrs)atoi((const char *)raw_crs);
        xmlFree(raw_crs);
    } else {
        DC_LOG_ERROR("PlanetView", "CRS is required; use #_planet_crs_geodetic_ or #_planet_crs_cartesian_");
    }

    dc_node.planet_view.attitude_frame = DC_APP_PLANET_ATTITUDE_FRAME_UNDEFINED;
    xmlChar *raw_attitude_frame = xmlGetProp(xml_node, BAD_CAST "AttitudeFrame");
    if (raw_attitude_frame) {
        dc_node.planet_view.attitude_frame = (DcAppPlanetAttitudeFrame)atoi((const char *)raw_attitude_frame);
        xmlFree(raw_attitude_frame);
    } else {
        // defaults attitude frame from crs to preserve concise xml.
        dc_node.planet_view.attitude_frame = dc_node.planet_view.crs == DC_APP_PLANET_CRS_GEODETIC
            ? DC_APP_PLANET_ATTITUDE_FRAME_LOCAL_NED
            : DC_APP_PLANET_ATTITUDE_FRAME_CARTESIAN_RPY;
    }

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.planet_view.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.planet_view.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // x dimension
    xmlChar *raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionX");
    if (!raw_x_dimension) {
        raw_x_dimension = xmlGetProp(xml_node, BAD_CAST "Width");
    }
    if (raw_x_dimension) {
        dc_node.planet_view.dimension.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_dimension);
        xmlFree(raw_x_dimension);
    }

    // y dimension
    xmlChar *raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "DimensionY");
    if (!raw_y_dimension) {
        raw_y_dimension = xmlGetProp(xml_node, BAD_CAST "Height");
    }
    if (raw_y_dimension) {
        dc_node.planet_view.dimension.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_dimension);
        xmlFree(raw_y_dimension);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.planet_view.local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.planet_view.local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.planet_view.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.planet_view.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.planet_view.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
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

        dc_node.planet_view.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.planet_view.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.planet_view.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.planet_view.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.planet_view.pivot_local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.planet_view.pivot_local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("PlanetView", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // parses geodetic camera position.
    xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "CameraLatitude");
    xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "CameraLongitude");
    xmlChar *raw_ele = xmlGetProp(xml_node, BAD_CAST "CameraElevation");
    bool     has_lle = raw_lat || raw_lon || raw_ele;

    if (has_lle) {
        if (raw_lat && raw_lon && raw_ele) {
            dc_node.planet_view.lle.lat = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
            dc_node.planet_view.lle.lon = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
            dc_node.planet_view.lle.ele = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_ele);
        } else {
            DC_LOG_ERROR("PlanetView", "Incomplete geodetic camera position: must specify all of CameraLatitude, CameraLongitude, and CameraElevation");
        }
    }
    if (raw_lat) xmlFree(raw_lat);
    if (raw_lon) xmlFree(raw_lon);
    if (raw_ele) xmlFree(raw_ele);

    // field of view (vertical, degrees)
    xmlChar *raw_fov = xmlGetProp(xml_node, BAD_CAST "CameraFOV");
    if (raw_fov) {
        dc_node.planet_view.fov = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_fov);
        xmlFree(raw_fov);
    }

    // parses roll pitch yaw in the selected attitude frame.
    xmlChar *raw_roll  = xmlGetProp(xml_node, BAD_CAST "CameraRoll");
    xmlChar *raw_pitch = xmlGetProp(xml_node, BAD_CAST "CameraPitch");
    xmlChar *raw_yaw   = xmlGetProp(xml_node, BAD_CAST "CameraYaw");
    xmlChar *raw_heading = xmlGetProp(xml_node, BAD_CAST "CameraHeading");

    if (raw_heading && raw_yaw) {
        DC_LOG_ERROR("PlanetView", "CameraHeading is a legacy alias for CameraYaw; do not specify both");
    }
    if (raw_roll) {
        dc_node.planet_view.rpy.roll = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_roll);
        xmlFree(raw_roll);
    }
    if (raw_pitch) {
        dc_node.planet_view.rpy.pitch = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pitch);
        xmlFree(raw_pitch);
    }
    if (raw_yaw) {
        dc_node.planet_view.rpy.yaw = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_yaw);
        xmlFree(raw_yaw);
    } else if (raw_heading && dc_node.planet_view.crs == DC_APP_PLANET_CRS_GEODETIC) {
        // treats cameraheading as a legacy alias for local-ned yaw.
        dc_node.planet_view.rpy.yaw = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_heading);
    } else if (raw_heading) {
        DC_LOG_ERROR("PlanetView", "CameraHeading is only valid for geodetic PlanetView; use CameraYaw for cartesian CRS");
    }
    if (raw_heading) xmlFree(raw_heading);

    // parses cartesian camera position.
    xmlChar *raw_cam_x = xmlGetProp(xml_node, BAD_CAST "CameraX");
    xmlChar *raw_cam_y = xmlGetProp(xml_node, BAD_CAST "CameraY");
    xmlChar *raw_cam_z = xmlGetProp(xml_node, BAD_CAST "CameraZ");
    bool     has_xyz   = raw_cam_x || raw_cam_y || raw_cam_z;

    if (has_lle && has_xyz)
        DC_LOG_ERROR("PlanetView", "Cannot mix geodetic and cartesian camera position attributes");

    if (has_xyz) {
        if (raw_cam_x && raw_cam_y && raw_cam_z) {
            dc_node.planet_view.xyz.x     = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_cam_x);
            dc_node.planet_view.xyz.y     = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_cam_y);
            dc_node.planet_view.xyz.z     = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_cam_z);
        } else {
            DC_LOG_ERROR("PlanetView", "Incomplete cartesian camera position: must specify all of CameraX, CameraY, and CameraZ");
        }
    }
    if (raw_cam_x) xmlFree(raw_cam_x);
    if (raw_cam_y) xmlFree(raw_cam_y);
    if (raw_cam_z) xmlFree(raw_cam_z);

    if (dc_node.planet_view.crs == DC_APP_PLANET_CRS_GEODETIC) {
        // validates that the declared crs and attitude frame form a supported pair.
        if (!has_lle)
            DC_LOG_ERROR("PlanetView", "Geodetic CRS requires CameraLatitude, CameraLongitude, and CameraElevation");
        if (has_xyz)
            DC_LOG_ERROR("PlanetView", "Geodetic CRS cannot use CameraX/CameraY/CameraZ");
        if (dc_node.planet_view.attitude_frame != DC_APP_PLANET_ATTITUDE_FRAME_LOCAL_NED)
            DC_LOG_ERROR("PlanetView", "Geodetic CRS requires AttitudeFrame=#_planet_attitude_frame_local_ned_");
    } else if (dc_node.planet_view.crs == DC_APP_PLANET_CRS_CARTESIAN) {
        if (!has_xyz)
            DC_LOG_ERROR("PlanetView", "Cartesian CRS requires CameraX, CameraY, and CameraZ");
        if (has_lle)
            DC_LOG_ERROR("PlanetView", "Cartesian CRS cannot use CameraLatitude/CameraLongitude/CameraElevation");
        if (dc_node.planet_view.attitude_frame != DC_APP_PLANET_ATTITUDE_FRAME_CARTESIAN_RPY)
            DC_LOG_ERROR("PlanetView", "Cartesian CRS requires AttitudeFrame=#_planet_attitude_frame_cartesian_rpy_");
    } else {
        DC_LOG_ERROR("PlanetView", "Unknown CRS value: %d", dc_node.planet_view.crs);
    }

    // orthographic projection
    xmlChar *raw_ortho = xmlGetProp(xml_node, BAD_CAST "CameraOrthographic");
    if (raw_ortho) {
        dc_node.planet_view.orthographic = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_ortho);
        xmlFree(raw_ortho);
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.planet_view.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.planet_view.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // shader index (optional — selects active PlanetShader by index at runtime)
    xmlChar *raw_shader_index = xmlGetProp(xml_node, BAD_CAST "ShaderIndex");
    if (raw_shader_index) {
        dc_node.planet_view.shader_index = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_shader_index);
        xmlFree(raw_shader_index);
    }

    // LOD error threshold (lower = more aggressive chunk loading, default 0.3)
    xmlChar *raw_tau = xmlGetProp(xml_node, BAD_CAST "Tau");
    if (raw_tau) {
        dc_node.planet_view.tau = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_tau);
        xmlFree(raw_tau);
    }

    // flatten to sphere
    xmlChar *raw_flatten = xmlGetProp(xml_node, BAD_CAST "Flatten");
    if (raw_flatten) {
        dc_node.planet_view.flatten = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_flatten);
        xmlFree(raw_flatten);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // collect planet view node index for post-tree initialization
    dc_app_scene_add_planet_view_node(xml_ctx->scene, node_index);

    // process children (PlanetEllipse, etc.)
    DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);
    _get_node(xml_ctx, node_index)->planet_view.child = first_child_index;

    return node_index;
}

static DcAppNodeIndex _process_xml_node_text(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {};
    dc_node.type   = NODE_TYPE_TEXT;
    dc_node.parent = parent_node_index;
    // font
    xmlChar *raw_font = xmlGetProp(xml_node, BAD_CAST "Font");
    if (raw_font) {
        // resolve path relative to display directory
        char font_path[1024];
        if (((const char *)raw_font)[0] == '/') {
            strncpy(font_path, (const char *)raw_font, sizeof(font_path) - 1);
        } else {
            snprintf(font_path, sizeof(font_path), "%s/%s", directory, (const char *)raw_font);
        }
        font_path[sizeof(font_path) - 1] = '\0';
        dc_node.text.font_index = _register_font(xml_ctx, font_path);
        xmlFree(raw_font);
    }

    // text
    xmlChar *raw_text = xmlNodeGetContent(xml_node);
    if (raw_text) {
        char cleaned_text[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned_text, (const char *)raw_text, DC_VALUE_STRING_BUFFER_SIZE - 1);
        cleaned_text[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
        xmlFree(raw_text);
        // dc_utils_trim_whitespace_inplace(cleaned_text);

        sbclear(xml_ctx->sb_text_filler);
        for (size_t ii = 0; ii < strlen(cleaned_text);) {
            if (cleaned_text[ii] == '\\') {
                // handle escape characters
                if (ii + 1 < strlen(cleaned_text)) {

                    // if a C style escape sequence, add the combined character
                    // (e.g. '\\' + 'n' => '\n')
                    char next_char = cleaned_text[ii + 1];
                    if (next_char == 'n') {
                        sbpush(xml_ctx->sb_text_filler, '\n');
                    } else if (next_char == 'r') {
                        sbpush(xml_ctx->sb_text_filler, '\r');
                    } else if (next_char == 't') {
                        sbpush(xml_ctx->sb_text_filler, '\t');
                    } else if (next_char == '\\') {
                        sbpush(xml_ctx->sb_text_filler, '\\');
                    } else if (next_char == '"') {
                        sbpush(xml_ctx->sb_text_filler, '"');
                    } else if (next_char == '\'') {
                        sbpush(xml_ctx->sb_text_filler, '\'');
                    } else if (dc_utils_char_in(next_char, "@#$%")) {
                        // push only the latter character
                        sbpush(xml_ctx->sb_text_filler, next_char);
                    } else {
                        // unknown escape; keep the backslash and next char
                        sbpush(xml_ctx->sb_text_filler, '\\');
                        sbpush(xml_ctx->sb_text_filler, next_char);
                    }

                    // increment to character after
                    ii += 2;
                } else {
                    sbpush(xml_ctx->sb_text_filler, cleaned_text[ii++]);
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
                        sbpushn(xml_ctx->sb_text_filler, &(cleaned_text[start]), (int)(ii - start));
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

                DcAppVarIndex var_index = dc_app_lookup_get_var_index(_lookup(xml_ctx), var);
                if (var_index != DC_APP_VAR_INDEX_UNDEFINED) {
                    sbpush(dc_node.text.sb_vals, dc_app_lookup_get_var_value_index(_lookup(xml_ctx), var_index));
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
                sbpushn(dc_node.text.sb_fillers, xml_ctx->sb_text_filler, sbcount(xml_ctx->sb_text_filler));
                sbpush(dc_node.text.sb_fillers, '\0');
                sbclear(xml_ctx->sb_text_filler);

                continue;
            }

            // Default: append character to result
            sbpush(xml_ctx->sb_text_filler, cleaned_text[ii++]);
        }

        // append the remaining filler
        sbpush(dc_node.text.sb_filler_indices, (uint8_t)sbcount(dc_node.text.sb_fillers));
        sbpushn(dc_node.text.sb_fillers, xml_ctx->sb_text_filler, sbcount(xml_ctx->sb_text_filler));
        sbpush(dc_node.text.sb_fillers, '\0');

        // clear temp buffer
        sbclear(xml_ctx->sb_text_filler);
    } else {
        DC_LOG_ERROR("Text", "Missing node content");
    }

    // x position
    xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
    if (!raw_x_position) {
        raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
    }
    if (raw_x_position) {
        dc_node.text.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
        xmlFree(raw_x_position);
    }

    // y position
    xmlChar *raw_y_position = xmlGetProp(xml_node, BAD_CAST "PositionY");
    if (!raw_y_position) {
        raw_y_position = xmlGetProp(xml_node, BAD_CAST "Y");
    }
    if (raw_y_position) {
        dc_node.text.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
        xmlFree(raw_y_position);
    }

    // local x align
    xmlChar *raw_x_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignX");
    if (!raw_x_align) {
        raw_x_align = xmlGetProp(xml_node, BAD_CAST "HorizontalAlign");
    }
    if (raw_x_align) {
        dc_node.text.local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_x_align);
        xmlFree(raw_x_align);
    }

    // local y align
    xmlChar *raw_y_align = xmlGetProp(xml_node, BAD_CAST "LocalAlignY");
    if (!raw_y_align) {
        raw_y_align = xmlGetProp(xml_node, BAD_CAST "VerticalAlign");
    }
    if (raw_y_align) {
        dc_node.text.local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_y_align);
        xmlFree(raw_y_align);
    }

    // parent x align
    xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
    if (raw_parent_x_align) {
        dc_node.text.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
        xmlFree(raw_parent_x_align);
    }

    // parent y align
    xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
    if (raw_parent_y_align) {
        dc_node.text.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
        xmlFree(raw_parent_y_align);
    }

    // rotation
    xmlChar *raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotation");
    if (!raw_rotation) {
        raw_rotation = xmlGetProp(xml_node, BAD_CAST "Rotate");
    }
    if (raw_rotation) {
        dc_node.text.rotation = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_rotation);
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

        dc_node.text.pivot_position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_x);
        xmlFree(raw_pivot_position_x);

        dc_node.text.pivot_position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_pivot_position_y);
        xmlFree(raw_pivot_position_y);

    } else if (!raw_pivot_position_x && !raw_pivot_position_y) {
        xmlChar *raw_pivot_parent_align_x = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignX");
        xmlChar *raw_pivot_parent_align_y = xmlGetProp(xml_node, BAD_CAST "PivotParentAlignY");
        if (raw_pivot_parent_align_x || raw_pivot_parent_align_y) {
            if (raw_pivot_parent_align_x) {
                dc_node.text.pivot_parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_x);
                xmlFree(raw_pivot_parent_align_x);
            }
            if (raw_pivot_parent_align_y) {
                dc_node.text.pivot_parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_parent_align_y);
                xmlFree(raw_pivot_parent_align_y);
            }
        } else {
            xmlChar *raw_pivot_align_x = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignX");
            if (raw_pivot_align_x) {
                dc_node.text.pivot_local_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_x);
                xmlFree(raw_pivot_align_x);
            }

            xmlChar *raw_pivot_align_y = xmlGetProp(xml_node, BAD_CAST "PivotLocalAlignY");
            if (raw_pivot_align_y) {
                dc_node.text.pivot_local_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_pivot_align_y);
                xmlFree(raw_pivot_align_y);
            }
        }

    } else {
        DC_LOG_ERROR("Text", "Invalid PivotParameters: must use both PivotX and PivotY, or neither");
    }

    // size
    xmlChar *raw_size = xmlGetProp(xml_node, BAD_CAST "Size");
    if (raw_size) {
        dc_node.text.size = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_size);
        xmlFree(raw_size);
    }

    dc_node.text.config_flags = NODE_CONFIG_FLAG_NONE;
    if (_load_color_from_string(xml_ctx, xml_node, "FillColor", &(dc_node.text.fill_color)) ||
        _load_color_from_string(xml_ctx, xml_node, "Color", &(dc_node.text.fill_color)))
        dc_node.text.config_flags |= NODE_CONFIG_FLAG_FILL_ENABLED;
    if (_load_color_from_string(xml_ctx, xml_node, "LineColor", &(dc_node.text.line_color)))
        dc_node.text.config_flags |= NODE_CONFIG_FLAG_LINE_ENABLED;
    if (_load_color_from_string(xml_ctx, xml_node, "BackgroundColor", &(dc_node.text.background_color)))
        dc_node.text.config_flags |= NODE_CONFIG_FLAG_BACKGROUND_ENABLED;


    // bold
    xmlChar *raw_bold = xmlGetProp(xml_node, BAD_CAST "Bold");
    if (raw_bold) {
        dc_node.text.bold = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_bold);
        xmlFree(raw_bold);
    }

    // italic
    xmlChar *raw_italic = xmlGetProp(xml_node, BAD_CAST "Italic");
    if (raw_italic) {
        dc_node.text.italic = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_italic);
        xmlFree(raw_italic);
    }

    // shadow offset
    xmlChar *raw_shadow_offset = xmlGetProp(xml_node, BAD_CAST "ShadowOffset");
    if (raw_shadow_offset) {
        dc_node.text.shadow_offset = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_shadow_offset);
        xmlFree(raw_shadow_offset);
    }

    // update rate
    xmlChar *raw_update_rate = xmlGetProp(xml_node, BAD_CAST "UpdateRate");
    if (raw_update_rate) {
        dc_node.text.update_rate = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_update_rate);
        xmlFree(raw_update_rate);
    }

    // negate x
    xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
    if (raw_negate_x) {
        dc_node.text.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
        xmlFree(raw_negate_x);
    }

    // negate y
    xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
    if (raw_negate_y) {
        dc_node.text.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
        xmlFree(raw_negate_y);
    }

    // log
    xmlChar *raw_log = xmlGetProp(xml_node, BAD_CAST "Log");
    if (raw_log) {
        dc_node.text.log = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_STRING, (const char *)raw_log);
        xmlFree(raw_log);
    }

    // register node
    return _register_node(xml_ctx, &dc_node);
}

static DcAppNodeIndex _process_xml_node_trick_from(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_TRICK_IO: {
            _process_xml_node_children(xml_ctx, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
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

static DcAppNodeIndex _process_xml_node_trick_io(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

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
    xmlChar      *raw_connected_var   = xmlGetProp(xml_node, BAD_CAST "ConnectedVariable");
    DcAppVarIndex connected_var_index = DC_APP_VAR_INDEX_UNDEFINED;
    if (raw_connected_var) {
        connected_var_index = dc_app_lookup_get_var_index(_lookup(xml_ctx), (const char *)raw_connected_var);
        xmlFree(raw_connected_var);
    }

    // create trick instance
    dc_app_data_link_add_trick(xml_ctx->data_link, host, port, (float)data_rate, connected_var_index);

    // process children
    _process_xml_node_children(xml_ctx, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);

    // return
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_trick_to(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_TRICK_IO: {
            _process_xml_node_children(xml_ctx, xml_node, NODE_INDEX_UNDEFINED, elem_type, directory);
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

static DcAppNodeIndex _process_xml_node_trick_variable(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

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

    // handle depending on parent
    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_TRICK_FROM: {
            // create + add rx var
            DcAppVarIndex dcapp_var_index = dc_app_lookup_get_var_index(_lookup(xml_ctx), dcapp_var);
            if (dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) {
                DC_LOG_ERROR("TrickVariable", "Unknown variable '%s' in TrickFrom", dcapp_var);
            }
            dc_app_data_link_add_trick_rx(xml_ctx->data_link, trick_path, units, dcapp_var_index);
            break;
        }
        case DC_APP_ELEM_TYPE_TRICK_TO: {
            // create + add tx var
            DcAppVarIndex dcapp_var_index = dc_app_lookup_get_var_index(_lookup(xml_ctx), dcapp_var);
            const DcValue *initial_value = NULL;
            if (dcapp_var_index == DC_APP_VAR_INDEX_UNDEFINED) {
                DC_LOG_ERROR("TrickVariable", "Unknown variable '%s' in TrickTo", dcapp_var);
            } else {
                initial_value = dc_app_lookup_get_value(
                    _lookup(xml_ctx),
                    dc_app_lookup_get_var_value_index(_lookup(xml_ctx), dcapp_var_index));
            }
            dc_app_data_link_add_trick_tx(xml_ctx->data_link, trick_path, units, dcapp_var_index, initial_value);
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

static DcAppNodeIndex _process_xml_node_true(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    switch (parent_elem_type) {
        case DC_APP_ELEM_TYPE_IF: {
            DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, parent_node_index, elem_type, directory);
            return _create_state_event_node(xml_ctx, NODE_TYPE_STATE_IF_TRUE, parent_node_index, first_child_index);
        }
        default:
            DC_LOG_ERROR("True", "Invalid parent of type %s", dc_app_elem_type_to_string(parent_elem_type));
    }

    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_variable(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

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
    DcAppValIndex value_index = dc_app_lookup_register_value(_lookup(xml_ctx), &initial_value);
    dc_app_lookup_register_var(_lookup(xml_ctx), name, value_index);

    // return
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_vertex(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode *parent_node = _get_node(xml_ctx, parent_node_index);
    switch (parent_node->type) {
        case NODE_TYPE_LINE:
        case NODE_TYPE_POLYGON: {

            // vertex data
            DcAppVertexData vertex    = {};
            vertex.position.x     = DC_APP_VAL_INDEX_UNDEFINED;
            vertex.position.y     = DC_APP_VAL_INDEX_UNDEFINED;
            vertex.parent_align.x = DC_APP_VAL_INDEX_UNDEFINED;
            vertex.parent_align.y = DC_APP_VAL_INDEX_UNDEFINED;

            // x position
            xmlChar *raw_x_position = xmlGetProp(xml_node, BAD_CAST "PositionX");
            if (!raw_x_position) {
                raw_x_position = xmlGetProp(xml_node, BAD_CAST "X");
            }
            if (raw_x_position) {
                vertex.position.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_position);
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
                vertex.position.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_position);
                xmlFree(raw_y_position);
            } else {
                DC_LOG_ERROR("Vertex", "Missing 'Y' attribute");
            }

            // parent x align
            xmlChar *raw_parent_x_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignX");
            if (raw_parent_x_align) {
                vertex.parent_align.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_x_align);
                xmlFree(raw_parent_x_align);
            }

            // parent y align
            xmlChar *raw_parent_y_align = xmlGetProp(xml_node, BAD_CAST "ParentAlignY");
            if (raw_parent_y_align) {
                vertex.parent_align.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_parent_y_align);
                xmlFree(raw_parent_y_align);
            }

            // negate x
            xmlChar *raw_negate_x = xmlGetProp(xml_node, BAD_CAST "NegateX");
            if (raw_negate_x) {
                vertex.negate_x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_x);
                xmlFree(raw_negate_x);
            } else {
                vertex.negate_x = DC_APP_VAL_INDEX_UNDEFINED;
            }

            // negate y
            xmlChar *raw_negate_y = xmlGetProp(xml_node, BAD_CAST "NegateY");
            if (raw_negate_y) {
                vertex.negate_y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_BOOLEAN, (const char *)raw_negate_y);
                xmlFree(raw_negate_y);
            } else {
                vertex.negate_y = DC_APP_VAL_INDEX_UNDEFINED;
            }

            switch (parent_node->type) {
                case NODE_TYPE_LINE:
                    // add to parent
                    sbpush(parent_node->line.sb_vertices, vertex);

                    // check point count
                    if (sbcount(parent_node->line.sb_vertices) > DC_APP_NODE_LINE_MAX_POINTS) {
                        DC_LOG_ERROR("Line", "Maximum number of points exceeded");
                    }
                    break;
                case NODE_TYPE_POLYGON:
                    // add to parent
                    sbpush(parent_node->polygon.sb_vertices, vertex);

                    // check point count
                    if (sbcount(parent_node->polygon.sb_vertices) > DC_APP_NODE_POLYGON_MAX_POINTS) {
                        DC_LOG_ERROR("Polygon", "Maximum number of points exceeded");
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        case NODE_TYPE_PLANET_LINE:
        case NODE_TYPE_PLANET_POLYGON: {

            DcAppPlanetVertexDynamic pv = {};
            pv.lat = DC_APP_VAL_INDEX_UNDEFINED;
            pv.lon = DC_APP_VAL_INDEX_UNDEFINED;
            pv.alt = DC_APP_VAL_INDEX_UNDEFINED;

            xmlChar *raw_lat = xmlGetProp(xml_node, BAD_CAST "Latitude");
            if (raw_lat) {
                pv.lat = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lat);
                xmlFree(raw_lat);
            }

            xmlChar *raw_lon = xmlGetProp(xml_node, BAD_CAST "Longitude");
            if (raw_lon) {
                pv.lon = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_lon);
                xmlFree(raw_lon);
            }

            xmlChar *raw_alt = xmlGetProp(xml_node, BAD_CAST "Altitude");
            if (raw_alt) {
                pv.alt = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_alt);
                xmlFree(raw_alt);
            }

            xmlChar *raw_x = xmlGetProp(xml_node, BAD_CAST "X");
            if (raw_x) {
                pv.xyz.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x);
                xmlFree(raw_x);
            }

            xmlChar *raw_y = xmlGetProp(xml_node, BAD_CAST "Y");
            if (raw_y) {
                pv.xyz.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y);
                xmlFree(raw_y);
            }

            xmlChar *raw_z = xmlGetProp(xml_node, BAD_CAST "Z");
            if (raw_z) {
                pv.xyz.z = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_z);
                xmlFree(raw_z);
            }

            if (parent_node->type == NODE_TYPE_PLANET_LINE)
                sbpush(parent_node->planet_line.sb_points_dynamic, pv);
            else
                sbpush(parent_node->planet_polygon.sb_points_dynamic, pv);

            break;
        }
        default:
            DC_LOG_ERROR("Vertex", "Invalid parent of type %s", _node_type_to_string(parent_node->type));
    }

    // return
    return NODE_INDEX_UNDEFINED;
}

static DcAppNodeIndex _process_xml_node_window(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory) {
    DcAppElemType elem_type = dc_app_elem_type_from_xml_node(xml_node);

    DcAppNode dc_node  = {0};
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
        dc_node.window.virtual_dimension.x = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_x_virtual_dimension);
        xmlFree(raw_x_virtual_dimension);
    }

    // virtual y virtual_dimension
    xmlChar *raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualDimensionY");
    if (!raw_y_virtual_dimension) {
        raw_y_virtual_dimension = xmlGetProp(xml_node, BAD_CAST "VirtualHeight");
    }
    if (raw_y_virtual_dimension) {
        dc_node.window.virtual_dimension.y = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_y_virtual_dimension);
        xmlFree(raw_y_virtual_dimension);
    }

    // update rate
    xmlChar *raw_update_rate = xmlGetProp(xml_node, BAD_CAST "UpdateRate");
    if (raw_update_rate) {
        dc_node.window.update_rate = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, (const char *)raw_update_rate);
        xmlFree(raw_update_rate);
    }

    // active display (for Panel DisplayIndex matching)
    xmlChar *raw_active_display = xmlGetProp(xml_node, BAD_CAST "ActiveDisplay");
    if (raw_active_display) {
        dc_node.window.active_display = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_INTEGER, (const char *)raw_active_display);
        xmlFree(raw_active_display);
    }

    // fullscreen mode
    xmlChar *raw_fullscreen = xmlGetProp(xml_node, BAD_CAST "Fullscreen");
    if (raw_fullscreen) {

        dc_node.window.fullscreen = xmlStrcmp(raw_fullscreen, BAD_CAST "true") == 0;
        xmlFree(raw_fullscreen);
    }

    // register node
    DcAppNodeIndex node_index = _register_node(xml_ctx, &dc_node);

    // init PL graphics backend
    // TODO really don't like this approach
    if (xml_ctx->bootstrap) {
        xml_ctx->bootstrap(xml_ctx->app_context, xml_ctx, _get_node(xml_ctx, node_index));
    }

    // process children
    DcAppNodeIndex first_child_index = _process_xml_node_children(xml_ctx, xml_node, node_index, elem_type, directory);

    // build font atlas (after children are processed so custom fonts are registered)
    dc_app_font_build(xml_ctx->fonts);

    // update child index
    DcAppNode *node        = _get_node(xml_ctx, node_index);
    node->window.child = first_child_index;

    // set global window
    dc_app_scene_set_window(xml_ctx->scene, node_index);

    // return
    return node_index;
}

static const char *_node_type_to_string(DcAppNodeType type) {
    switch (type) {
        case NODE_TYPE_CONTAINER:
            return "Container";
        case NODE_TYPE_CONDITIONAL:
            return "Conditional";
        case NODE_TYPE_DRAW_FUNCTION:
            return "DrawFunction";
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
        case NODE_TYPE_PLANET_BREADCRUMBS:
            return "PlanetBreadcrumbs";
        case NODE_TYPE_PLANET_CONTAINER:
            return "PlanetContainer";
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

static DcAppNodeIndex _register_node(DcAppXmlContext *xml_ctx, DcAppNode *node) {
    return dc_app_scene_add_node(xml_ctx->scene, node);
}

static DcAppNode *_get_node(DcAppXmlContext *xml_ctx, DcAppNodeIndex index) {
    return dc_app_scene_get_node(xml_ctx->scene, index);
}

static bool _load_color_from_string(DcAppXmlContext *xml_ctx, xmlNodePtr xml_node, const char *attr_name, DcAppValIndex4 *color_out) {

    xmlChar *raw_color = xmlGetProp(xml_node, BAD_CAST attr_name);
    if (raw_color) {

        // clean raw string
        char cleaned_color[DC_VALUE_STRING_BUFFER_SIZE];
        strncpy(cleaned_color, (const char *)(const char *)raw_color, DC_VALUE_STRING_BUFFER_SIZE - 1);
        xmlFree(raw_color);

        // split by whitespace
        // assume no more than 20 splits
        size_t index_buffer[20];
        size_t index_count;
        dc_utils_split_string_inplace(cleaned_color, dc_utils_whitespace, index_buffer, 20, &index_count);

        // if empty, assume no color
        if (index_count == 0) {
            return false;
        }

        // process each color
        if (index_count > 0) {
            color_out->r = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[0]]));
        } else {
            color_out->r = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 1) {
            color_out->g = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[1]]));
        } else {
            color_out->g = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 2) {
            color_out->b = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[2]]));
        } else {
            color_out->b = DC_APP_VAL_INDEX_UNDEFINED;
        }
        if (index_count > 3) {
            color_out->a = dc_app_lookup_register_value_from_string(_lookup(xml_ctx), DC_VALUE_TYPE_DOUBLE, &(cleaned_color[index_buffer[3]]));
        } else {
            color_out->a = DC_APP_VAL_INDEX_UNDEFINED;
        }

        return true;
    } else {
        return false;
    }
}
