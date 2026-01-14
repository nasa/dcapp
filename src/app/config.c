#include "config.h"
#include "elem.h"
#include "enums.h"
#include "libxml/tree.h"
#include "lookup.h"
#include "../value.h"
#include "../utils/env.h"
#include "../utils/file.h"
#include "../utils/math.h"
#include "../utils/stb_sb.h"
#include "../utils/string.h"

#include <libxml/parser.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// style utils
typedef int              _StyleIndex;
static const _StyleIndex _STYLE_INDEX_UNDEFINED = -1;
static const _StyleIndex _STYLE_INDEX_DEFAULT   = 0;

typedef struct __ElemStyle {
    xmlNodePtr xml_nodes[DC_APP_ELEM_TYPE__COUNT];
} _ElemStyle;

typedef struct __Constant {
    char *val;
    bool  is_immutable;
} _Constant;

typedef struct __ConfigContext {

    // constants
    char      *sb_const_names;
    int       *sb_const_name_offsets;
    _Constant *sb_consts;

    // styles
    char       *sb_style_names;
    int        *sb_style_name_offsets;
    _ElemStyle *sb_styles;

} _ConfigContext;
_ConfigContext *_sb_contexts;

// constant functions
static void             _register_const_by_name(_ConfigContext *context, const char *name, const char *new_value, bool is_immutable);
static const char      *_get_const_by_name(_ConfigContext *context, const char *name);
static void             _dereference_constants(_ConfigContext *context, const char *in, char *out, size_t out_size);
static DcAppLookupIndex _get_const_index(_ConfigContext *context, const char *name);
static void             _set_const(_ConfigContext *context, DcAppLookupIndex index, const char *new_value);
static void             _add_const(_ConfigContext *context, const char *name, const char *value, bool is_immutable);
static void             _add_const_int(_ConfigContext *context, const char *name, int value_int, bool is_immutable);

// style functions
static DcAppStyleIndex _get_style_index(_ConfigContext *context, const char *name);
static void            _add_style(_ConfigContext *context, const char *name, DcAppElemType elem_type, xmlNodePtr xml_node);
static xmlChar        *_get_style_attr(_ConfigContext *context, int style_index, DcAppElemType elem_type, const char *name);
static xmlChar        *_get_style_content(_ConfigContext *context, int style_index, DcAppElemType elem_type);

// xml utils
static char *_get_xml_node_attr(_ConfigContext *context, xmlNodePtr xml_node, DcAppElemType elem_type, int style_index, const char *name);
static char *_get_xml_node_content(_ConfigContext *context, xmlNodePtr xml_node, DcAppElemType elem_type, int style_index);
static void  _clean_xml_node(_ConfigContext *context, xmlNodePtr node, char *directory);
void         _dereference_node_attrs_and_content(_ConfigContext *context, xmlNodePtr node);
static void  _splice_children_into_parent_and_free_wrapper(xmlNodePtr node);

// arg utils
static char *_unquote(const char *str);

DcAppConfig *dc_app_config_create(const char *config_path, char **args, int arg_count) {
    DcAppConfig *config = (DcAppConfig *)malloc(sizeof(DcAppConfig));

    // get current working directory
    char cwd[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_get_cwd(cwd, DC_UTILS_FILEPATH_BUFFER_SIZE);

    // get canonical config file path
    config->config_file_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    if (dc_utils_is_relative_path(config_path)) {
        char abs_config_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
        dc_utils_join_paths(cwd, config_path, abs_config_path, sizeof(abs_config_path));
        dc_utils_canonicalize_path(abs_config_path, config->config_file_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    } else {
        dc_utils_canonicalize_path(config_path, config->config_file_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    }

    // get config file dir
    config->config_dir_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_get_directory(config->config_file_path, config->config_dir_path, DC_UTILS_FILEPATH_BUFFER_SIZE);

    // get exe dir
    char exe_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_get_exe_path(exe_path, sizeof(exe_path));
    char exe_dir[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_get_directory(exe_path, exe_dir, sizeof(exe_dir));

    // get dcapp home
    char dcapp_dir_path_abs[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_join_paths(exe_dir, "../..", dcapp_dir_path_abs, DC_UTILS_FILEPATH_BUFFER_SIZE);
    config->dcapp_dir_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_canonicalize_path(dcapp_dir_path_abs, config->dcapp_dir_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_create_directory(config->dcapp_dir_path);

    // get + create cache dir
    config->cache_dir_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_join_paths(config->dcapp_dir_path, "cache", config->cache_dir_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_create_directory(config->cache_dir_path);

    // get + create log dir
    config->log_dir_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_join_paths(config->dcapp_dir_path, "logs", config->log_dir_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_create_directory(config->log_dir_path);

    // get XML doc
    config->xml_doc = xmlReadFile(config->config_file_path, "UTF-8", XML_PARSE_NOBLANKS);
    if (!config->xml_doc) {
        fprintf(stderr, "DCAPP dc_app_config_create: unable to read config file '%s'\n", config->config_file_path);
    }
    config->xml_doc_is_cleaned = false;

    // create internal context
    _ConfigContext context = {};

    // reserve index 0 for default style
    sbpush(context.sb_style_name_offsets, 0);
    sbpushn(context.sb_style_names, "default", (int)(strlen("default") + 1));
    _ElemStyle default_style = {};
    sbpush(context.sb_styles, default_style);

    // process input arguments
    // TODO get rid of all the heap allocation..
    for (int ii = 0; ii < arg_count; ii++) {

        // get eq sign
        const char *eq_addr = strchr(args[ii], '=');
        if (!eq_addr) {
            fprintf(stderr, "DCApp dc_app_config_create(): input argument '%s' has no value set; ignoring", args[ii]);
            continue;
        }

        // extract the argument name (before '=')
        size_t name_len  = eq_addr - args[ii];
        char  *name_part = strndup(args[ii], name_len);

        // extract the value (after '=')
        const char *value_part = eq_addr + 1;

        // unquote both parts
        char *arg_name  = _unquote(name_part);
        char *arg_value = _unquote(value_part);

        // register
        _register_const_by_name(&context, arg_name, arg_value, true);

        // free memory
        free(name_part);
        free(arg_name);
        free(arg_value);
    }

    // add default constants
    // set initial values
    _add_const_int(&context, "_align_left_", DC_APP_ALIGN_TYPE_LEFT, true);
    _add_const_int(&context, "_align_center_", DC_APP_ALIGN_TYPE_CENTER, true);
    _add_const_int(&context, "_align_right_", DC_APP_ALIGN_TYPE_RIGHT, true);
    _add_const_int(&context, "_align_bottom_", DC_APP_ALIGN_TYPE_BOTTOM, true);
    _add_const_int(&context, "_align_middle_", DC_APP_ALIGN_TYPE_MIDDLE, true);
    _add_const_int(&context, "_align_top_", DC_APP_ALIGN_TYPE_TOP, true);
    _add_const_int(&context, "_button_standard_", DC_APP_BUTTON_TYPE_STANDARD, true);
    _add_const_int(&context, "_button_momentary_", DC_APP_BUTTON_TYPE_MOMENTARY, true);
    _add_const_int(&context, "_button_toggle_", DC_APP_BUTTON_TYPE_TOGGLE, true);
    _add_const_int(&context, "_conditional_true_", DC_APP_CONDITIONAL_TYPE_TRUE, true);
    _add_const_int(&context, "_conditional_false_", DC_APP_CONDITIONAL_TYPE_FALSE, true);
    _add_const_int(&context, "_conditional_eq_", DC_APP_CONDITIONAL_TYPE_EQ, true);
    _add_const_int(&context, "_conditional_ne_", DC_APP_CONDITIONAL_TYPE_NE, true);
    _add_const_int(&context, "_conditional_lt_", DC_APP_CONDITIONAL_TYPE_LT, true);
    _add_const_int(&context, "_conditional_gt_", DC_APP_CONDITIONAL_TYPE_GT, true);
    _add_const_int(&context, "_conditional_lte_", DC_APP_CONDITIONAL_TYPE_LTE, true);
    _add_const_int(&context, "_conditional_gte_", DC_APP_CONDITIONAL_TYPE_GTE, true);
    _add_const_int(&context, "_pixelstream_dynamic_file_", DC_APP_PIXELSTREAM_TYPE_DYNAMIC_FILE, true);
    _add_const_int(&context, "_pixelstream_mjpeg_", DC_APP_PIXELSTREAM_TYPE_MJPEG, true);
    _add_const_int(&context, "_set_equal_", DC_APP_SET_TYPE_EQUAL, true);
    _add_const_int(&context, "_set_add_", DC_APP_SET_TYPE_ADD, true);
    _add_const_int(&context, "_set_subtract_", DC_APP_SET_TYPE_SUBTRACT, true);
    _add_const_int(&context, "_set_multiply_", DC_APP_SET_TYPE_MULTIPLY, true);
    _add_const_int(&context, "_set_divide_", DC_APP_SET_TYPE_DIVIDE, true);
    _add_const_int(&context, "_set_min_", DC_APP_SET_TYPE_MIN, true);
    _add_const_int(&context, "_set_max_", DC_APP_SET_TYPE_MAX, true);
    _add_const_int(&context, "_variable_string_", DC_VALUE_TYPE_STRING, true);
    _add_const_int(&context, "_variable_integer_", DC_VALUE_TYPE_INTEGER, true);
    _add_const_int(&context, "_variable_double_", DC_VALUE_TYPE_DOUBLE, true);
    _add_const_int(&context, "_variable_boolean_", DC_VALUE_TYPE_BOOLEAN, true);
    // Reds & Pinks
    _add_const(&context, "_color_red_", "1.0 0.0 0.0", false);
    _add_const(&context, "_color_crimson_", "0.86 0.08 0.24", false);
    _add_const(&context, "_color_maroon_", "0.5 0.0 0.0", false);
    _add_const(&context, "_color_burgundy_", "0.6 0.0 0.13", false);
    _add_const(&context, "_color_ruby_", "0.88 0.07 0.37", false);
    _add_const(&context, "_color_cherry_", "0.87 0.19 0.39", false);
    _add_const(&context, "_color_rose_", "1.0 0.0 0.5", false);
    _add_const(&context, "_color_pink_", "1.0 0.75 0.8", false);
    _add_const(&context, "_color_salmon_", "0.98 0.5 0.45", false);
    _add_const(&context, "_color_coral_", "1.0 0.5 0.31", false);
    _add_const(&context, "_color_peach_", "1.0 0.85 0.73", false);
    _add_const(&context, "_color_fuchsia_", "1.0 0.0 1.0", false);
    _add_const(&context, "_color_hot_pink_", "1.0 0.41 0.71", false);
    _add_const(&context, "_color_light_pink_", "1.0 0.71 0.76", false);
    _add_const(&context, "_color_mulberry_", "0.77 0.29 0.55", false);
    // Oranges
    _add_const(&context, "_color_orange_", "1.0 0.5 0.0", false);
    _add_const(&context, "_color_tangerine_", "1.0 0.6 0.0", false);
    _add_const(&context, "_color_pumpkin_", "1.0 0.46 0.1", false);
    _add_const(&context, "_color_apricot_", "0.98 0.81 0.69", false);
    _add_const(&context, "_color_cantaloupe_", "1.0 0.71 0.55", false);
    _add_const(&context, "_color_amber_", "1.0 0.75 0.0", false);
    _add_const(&context, "_color_burnt_orange_", "0.8 0.33 0.0", false);
    _add_const(&context, "_color_rust_", "0.72 0.25 0.05", false);
    _add_const(&context, "_color_terracotta_", "0.89 0.45 0.36", false);
    // Yellows
    _add_const(&context, "_color_yellow_", "1.0 1.0 0.0", false);
    _add_const(&context, "_color_lemon_", "1.0 1.0 0.31", false);
    _add_const(&context, "_color_mustard_", "1.0 0.86 0.35", false);
    _add_const(&context, "_color_gold_", "1.0 0.84 0.0", false);
    _add_const(&context, "_color_butter_", "1.0 0.94 0.75", false);
    _add_const(&context, "_color_champagne_", "0.97 0.91 0.81", false);
    _add_const(&context, "_color_sunflower_", "1.0 0.8 0.0", false);
    _add_const(&context, "_color_flax_", "0.93 0.87 0.51", false);
    // Greens
    _add_const(&context, "_color_green_", "0.0 1.0 0.0", false);
    _add_const(&context, "_color_lime_", "0.75 1.0 0.0", false);
    _add_const(&context, "_color_olive_", "0.5 0.5 0.0", false);
    _add_const(&context, "_color_moss_", "0.53 0.6 0.42", false);
    _add_const(&context, "_color_forest_green_", "0.13 0.55 0.13", false);
    _add_const(&context, "_color_emerald_", "0.31 0.78 0.47", false);
    _add_const(&context, "_color_jade_", "0.0 0.66 0.42", false);
    _add_const(&context, "_color_mint_", "0.74 0.99 0.79", false);
    _add_const(&context, "_color_pistachio_", "0.58 0.77 0.45", false);
    _add_const(&context, "_color_seafoam_", "0.62 0.89 0.76", false);
    _add_const(&context, "_color_chartreuse_", "0.5 1.0 0.0", false);
    // Blues
    _add_const(&context, "_color_blue_", "0.0 0.0 1.0", false);
    _add_const(&context, "_color_navy_", "0.0 0.0 0.5", false);
    _add_const(&context, "_color_sky_blue_", "0.53 0.81 0.92", false);
    _add_const(&context, "_color_baby_blue_", "0.87 0.92 1.0", false);
    _add_const(&context, "_color_azure_", "0.0 0.5 1.0", false);
    _add_const(&context, "_color_denim_", "0.08 0.38 0.65", false);
    _add_const(&context, "_color_sapphire_", "0.08 0.15 0.39", false);
    _add_const(&context, "_color_steel_blue_", "0.27 0.51 0.71", false);
    _add_const(&context, "_color_powder_blue_", "0.69 0.88 0.9", false);
    _add_const(&context, "_color_cerulean_", "0.0 0.48 0.65", false);
    _add_const(&context, "_color_teal_", "0.0 0.5 0.5", false);
    // Purples & Violets
    _add_const(&context, "_color_purple_", "0.5 0.0 0.5", false);
    _add_const(&context, "_color_indigo_", "0.29 0.0 0.51", false);
    _add_const(&context, "_color_lavender_", "0.9 0.9 0.98", false);
    _add_const(&context, "_color_plum_", "0.56 0.27 0.52", false);
    _add_const(&context, "_color_violet_", "0.93 0.51 0.93", false);
    _add_const(&context, "_color_amethyst_", "0.6 0.4 0.8", false);
    _add_const(&context, "_color_orchid_", "0.85 0.44 0.84", false);
    _add_const(&context, "_color_thistle_", "0.85 0.75 0.85", false);
    _add_const(&context, "_color_eggplant_", "0.38 0.25 0.32", false);
    // Browns
    _add_const(&context, "_color_brown_", "0.6 0.4 0.2", false);
    _add_const(&context, "_color_chocolate_", "0.82 0.41 0.12", false);
    _add_const(&context, "_color_saddle_brown_", "0.55 0.27 0.07", false);
    _add_const(&context, "_color_umber_", "0.39 0.32 0.28", false);
    _add_const(&context, "_color_mahogany_", "0.65 0.19 0.19", false);
    _add_const(&context, "_color_copper_", "0.72 0.45 0.2", false);
    _add_const(&context, "_color_tan_", "0.82 0.71 0.55", false);
    _add_const(&context, "_color_walnut_", "0.39 0.26 0.13", false);
    _add_const(&context, "_color_espresso_", "0.36 0.25 0.2", false);
    _add_const(&context, "_color_caramel_", "0.87 0.58 0.36", false);
    _add_const(&context, "_color_mocha_", "0.44 0.31 0.22", false);
    _add_const(&context, "_color_pecan_", "0.78 0.52 0.25", false);
    _add_const(&context, "_color_wood_", "0.76 0.6 0.42", false);
    _add_const(&context, "_color_bronze_", "0.8 0.5 0.2", false);
    _add_const(&context, "_color_russet_", "0.5 0.27 0.23", false);
    // Neutrals & Grays
    _add_const(&context, "_color_white_", "1.0 1.0 1.0", false);
    _add_const(&context, "_color_black_", "0.0 0.0 0.0", false);
    _add_const(&context, "_color_gray_", "0.5 0.5 0.5", false);
    _add_const(&context, "_color_light_gray_", "0.83 0.83 0.83", false);
    _add_const(&context, "_color_dark_gray_", "0.33 0.33 0.33", false);
    _add_const(&context, "_color_charcoal_", "0.21 0.27 0.31", false);
    _add_const(&context, "_color_silver_", "0.75 0.75 0.75", false);
    _add_const(&context, "_color_ash_", "0.7 0.75 0.71", false);
    _add_const(&context, "_color_slate_", "0.44 0.5 0.56", false);
    _add_const(&context, "_color_eggshell_", "0.94 0.92 0.84", false);
    _add_const(&context, "_color_alabaster_", "0.98 0.98 0.95", false);
    _add_const(&context, "_color_beige_", "0.96 0.96 0.86", false);
    _add_const(&context, "_color_khaki_", "0.76 0.69 0.57", false);
    _add_const(&context, "_color_sand_", "0.94 0.87 0.73", false);
    _add_const(&context, "_color_taupe_", "0.56 0.52 0.51", false);

    // add to contexts
    sbpush(_sb_contexts, context);
    config->_index = sbcount(_sb_contexts) - 1;

    return config;
}

void dc_app_config_cleanup(DcAppConfig *config) {
    free(config->config_file_path);
    free(config->config_dir_path);
    free(config->dcapp_dir_path);
    free(config->cache_dir_path);
    free(config->log_dir_path);
    xmlFreeDoc(config->xml_doc);

    _ConfigContext *context = &(_sb_contexts[config->_index]);
    sbfree(context->sb_style_name_offsets);
    sbfree(context->sb_style_names);

    // free style xml nodes
    for (int ii = 0; ii < sbcount(context->sb_styles); ii++) {
        for (int jj = 0; jj < DC_APP_ELEM_TYPE__COUNT; jj++) {
            if (context->sb_styles[ii].xml_nodes[jj]) {
                xmlFreeNode(context->sb_styles[ii].xml_nodes[jj]);
            }
        }
    }
    sbfree(context->sb_styles);

    sbfree(context->sb_const_names);
    sbfree(context->sb_const_name_offsets);
    for (int ii = 0; ii < sbcount(context->sb_consts); ii++) {
        sbfree(context->sb_consts[ii].val);
    }
    sbfree(context->sb_consts);
    free(config);
}

void dc_app_config_clean_xml(DcAppConfig *config, DcAppLookup *lookup) {

    _ConfigContext *context = &(_sb_contexts[config->_index]);

    // get root element
    xmlNodePtr node = xmlDocGetRootElement(config->xml_doc);
    if (node == NULL) {
        fprintf(stderr, "DCAPP dc_app_config_clean_xml(): unable to get root element of config file\n");
    }

    // verify root node is valid
    if (dc_app_xml_node_to_elem_type(node) != DC_APP_ELEM_TYPE_DCAPP) {
        fprintf(stderr, "DCAPP dc_app_config_clean_xml(): configuration root element is not DCAPP\n");
    }

    // clean XML file
    _clean_xml_node(context, node, config->config_dir_path);
    config->xml_doc_is_cleaned = true;
}

// Helper function to splice a node's children into its parent and free the wrapper node.
// This is the common "Dummy pattern" used by Dummy, Include, and StaticIf.
// The caller is responsible for processing children (preferrably before calling this).
static void _splice_children_into_parent_and_free_wrapper(xmlNodePtr node) {
    if (node->children) {
        xmlNodePtr first_child = node->children;
        xmlNodePtr last_child  = node->last;

        // update parent
        if (node->parent) {
            if (node->parent->children == node) {
                node->parent->children = first_child;
            }
            if (node->parent->last == node) {
                node->parent->last = last_child;
            }
        }

        // update siblings
        if (node->prev) {
            node->prev->next = first_child;
        }
        if (node->next) {
            node->next->prev = last_child;
        }

        // update children
        for (xmlNodePtr curr_child = first_child; curr_child; curr_child = curr_child->next) {
            curr_child->parent = node->parent;
        }
        first_child->prev = node->prev;
        last_child->next  = node->next;

        // unlink node
        node->children = NULL;
        node->last     = NULL;
        node->parent   = NULL;
        node->next     = NULL;
        node->prev     = NULL;
    } else {
        xmlUnlinkNode(node);
    }

    // free wrapper node
    xmlFreeNode(node);
}

void _clean_xml_node(_ConfigContext *context, xmlNodePtr node, char *directory) {

    // remove if not an element
    if (node->type != XML_ELEMENT_NODE && node->type != XML_TEXT_NODE && node->type != XML_ATTRIBUTE_NODE) {
        xmlUnlinkNode(node);
        xmlFreeNode(node);
        return;
    }

    // only process nodes
    if (node->type != XML_ELEMENT_NODE) {
        return;
    }

    // check for _Directory attribute and use it if present
    char directory_buffer[DC_UTILS_FILEPATH_BUFFER_SIZE];
    xmlChar *dir_attr = xmlGetProp(node, BAD_CAST "_Directory");
    if (dir_attr) {
        strncpy(directory_buffer, (const char *)dir_attr, sizeof(directory_buffer) - 1);
        directory_buffer[sizeof(directory_buffer) - 1] = '\0';
        directory = directory_buffer;
        xmlFree(dir_attr);
    }

    // get element type
    DcAppElemType elem_type = dc_app_xml_node_to_elem_type(node);

    // dereference attributes/content
    _dereference_node_attrs_and_content(context, node);

    // load style attributes, if it exists
    {
        xmlChar *style_name = xmlGetProp(node, BAD_CAST "Style");
        if (style_name) {
            DcAppStyleIndex style_index    = _get_style_index(context, (char *)style_name);
            xmlNodePtr      style_xml_node = context->sb_styles[style_index].xml_nodes[elem_type];

            if (style_xml_node) {
                for (xmlAttrPtr attr = style_xml_node->properties; attr; attr = attr->next) {
                    xmlAttrPtr existing = xmlHasProp(node, attr->name);
                    if (!existing) {
                        xmlChar *value = xmlGetProp(style_xml_node, attr->name);
                        if (value) {
                            xmlSetProp(node, attr->name, value);
                            xmlFree(value);
                        }
                    }
                }
            }

            xmlFree(style_name);
        }
    }

    // load default attributes
    xmlNodePtr style_xml_node = context->sb_styles[_STYLE_INDEX_DEFAULT].xml_nodes[elem_type];
    if (style_xml_node) {
        for (xmlAttrPtr attr = style_xml_node->properties; attr; attr = attr->next) {
            xmlAttrPtr existing = xmlHasProp(node, attr->name);
            if (!existing) {
                xmlChar *value = xmlGetProp(style_xml_node, attr->name);
                if (value) {
                    xmlSetProp(node, attr->name, value);
                    xmlFree(value);
                }
            }
        }
    }

    // processing before targeting children
    char new_directory[DC_UTILS_FILEPATH_BUFFER_SIZE];
    switch (elem_type) {

        case DC_APP_ELEM_TYPE_CONSTANT: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                fprintf(stderr, "DCAPP _clean_xml_node(): 'Name' attribute missing in <Constant> definition\n");
            }
            char cleaned_name[DC_VALUE_STRING_BUFFER_SIZE];
            strncpy(cleaned_name, (const char *)name, DC_VALUE_STRING_BUFFER_SIZE - 1);
            cleaned_name[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
            xmlFree(name);

            xmlChar *value = xmlNodeGetContent(node);
            if (!value) {
                fprintf(stderr, "DCAPP _clean_xml_node(): Node content missing in <Constant> definition\n");
            }
            char cleaned_value[DC_VALUE_STRING_BUFFER_SIZE];
            strncpy(cleaned_value, (const char *)value, DC_VALUE_STRING_BUFFER_SIZE - 1);
            cleaned_value[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
            xmlFree(value);

            bool     is_immutable     = false;
            xmlChar *raw_is_immutable = xmlGetProp(node, BAD_CAST "Immutable");
            if (raw_is_immutable) {
                is_immutable = dc_utils_string_to_boolean((const char *)raw_is_immutable);
                xmlFree(raw_is_immutable);
            }

            _register_const_by_name(context, cleaned_name, cleaned_value, is_immutable);
            return;
        }

        // remove "Dummy" level, keep children
        case DC_APP_ELEM_TYPE_DUMMY: {
            // process children first
            xmlNodePtr child = node->children;
            while (child) {
                xmlNodePtr child_next = child->next;
                _clean_xml_node(context, child, directory);
                child = child_next;
            }

            // splice children into parent and free wrapper
            _splice_children_into_parent_and_free_wrapper(node);
            return;
        }

        case DC_APP_ELEM_TYPE_STATIC_IF: {
            // Parse Operation (already dereferenced by _dereference_node_attrs_and_content)
            xmlChar *raw_operation = xmlGetProp(node, BAD_CAST "Operation");
            int cond_type = DC_APP_CONDITIONAL_TYPE_TRUE;
            if (raw_operation) {
                cond_type = dc_utils_string_to_integer((const char *)raw_operation);
                xmlFree(raw_operation);
            }

            // Parse Value1 (already dereferenced)
            xmlChar *raw_value1 = xmlGetProp(node, BAD_CAST "Value");
            if (!raw_value1) {
                raw_value1 = xmlGetProp(node, BAD_CAST "Value1");
            }
            if (!raw_value1) {
                fprintf(stderr, "DCAPP _clean_xml_node(): StaticIf: no value specified\n");
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                return;
            }

            // Parse Value2 (optional, already dereferenced)
            xmlChar *raw_value2 = xmlGetProp(node, BAD_CAST "Value2");

            // Copy to stack buffers and free xmlChar* immediately
            char value1_buf[256];
            char value2_buf[256];
            strncpy(value1_buf, (const char *)raw_value1, sizeof(value1_buf) - 1);
            value1_buf[sizeof(value1_buf) - 1] = '\0';
            xmlFree(raw_value1);

            const char *value1 = value1_buf;
            const char *value2 = NULL;
            if (raw_value2) {
                strncpy(value2_buf, (const char *)raw_value2, sizeof(value2_buf) - 1);
                value2_buf[sizeof(value2_buf) - 1] = '\0';
                value2 = value2_buf;
                xmlFree(raw_value2);
            }

            // Check for runtime variables (not allowed in StaticIf)
            if (value1[0] == '@') {
                fprintf(stderr, "DCAPP _clean_xml_node(): StaticIf: Value1 '%s' uses runtime variable (@), only constants (#) are allowed in StaticIf\n", value1);
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                return;
            }
            if (value2 && value2[0] == '@') {
                fprintf(stderr, "DCAPP _clean_xml_node(): StaticIf: Value2 '%s' uses runtime variable (@), only constants (#) are allowed in StaticIf\n", value2);
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                return;
            }

            // Evaluate the condition
            bool result = false;
            if (value2) {
                // Two-value comparison - determine type from value1
                // Note: Integer check is skipped because dc_utils_string_is_double() accepts integers.
                // Both "1" and "1.5" will be treated as doubles, which is fine for constant comparisons.
                bool is_double = dc_utils_string_is_double(value1);
                bool is_bool = !is_double && dc_utils_string_is_boolean(value1);

                if (is_double) {
                    // Numeric comparison (handles both integers and doubles)
                    if (!dc_utils_string_is_double(value2)) {
                        fprintf(stderr, "DCAPP _clean_xml_node(): StaticIf: Value1 is numeric but Value2 '%s' is not, ignoring StaticIf\n", value2);
                        xmlUnlinkNode(node);
                        xmlFreeNode(node);
                        return;
                    }

                    double num1 = dc_utils_string_to_double(value1);
                    double num2 = dc_utils_string_to_double(value2);

                    switch (cond_type) {
                        case DC_APP_CONDITIONAL_TYPE_EQ:
                            result = dc_utils_double_equals(num1, num2, 1e-9);
                            break;
                        case DC_APP_CONDITIONAL_TYPE_NE:
                            result = !dc_utils_double_equals(num1, num2, 1e-9);
                            break;
                        case DC_APP_CONDITIONAL_TYPE_LT:
                            result = (num1 < num2);
                            break;
                        case DC_APP_CONDITIONAL_TYPE_GT:
                            result = (num1 > num2);
                            break;
                        case DC_APP_CONDITIONAL_TYPE_LTE:
                            result = (num1 <= num2);
                            break;
                        case DC_APP_CONDITIONAL_TYPE_GTE:
                            result = (num1 >= num2);
                            break;
                        default:
                            fprintf(stderr, "DCAPP _clean_xml_node(): StaticIf: invalid Operation %d, expected EQ/NE/LT/GT/LTE/GTE (3-8)\n", cond_type);
                            break;
                    }
                } else if (is_bool) {
                    // Boolean comparison
                    int bool1 = dc_utils_string_to_boolean(value1);
                    int bool2 = dc_utils_string_to_boolean(value2);

                    switch (cond_type) {
                        case DC_APP_CONDITIONAL_TYPE_EQ:
                            result = (bool1 == bool2);
                            break;
                        case DC_APP_CONDITIONAL_TYPE_NE:
                            result = (bool1 != bool2);
                            break;
                        case DC_APP_CONDITIONAL_TYPE_LT:
                        case DC_APP_CONDITIONAL_TYPE_GT:
                        case DC_APP_CONDITIONAL_TYPE_LTE:
                        case DC_APP_CONDITIONAL_TYPE_GTE:
                            fprintf(stderr, "DCAPP _clean_xml_node(): StaticIf: operator %d not supported for boolean values, ignoring StaticIf\n", cond_type);
                            xmlUnlinkNode(node);
                            xmlFreeNode(node);
                            return;
                        default:
                            fprintf(stderr, "DCAPP _clean_xml_node(): StaticIf: invalid Operation %d for boolean comparison, expected EQ/NE (3-4)\n", cond_type);
                            break;
                    }
                } else {
                    // String comparison
                    switch (cond_type) {
                        case DC_APP_CONDITIONAL_TYPE_EQ:
                            result = (strcmp(value1, value2) == 0);
                            break;
                        case DC_APP_CONDITIONAL_TYPE_NE:
                            result = (strcmp(value1, value2) != 0);
                            break;
                        case DC_APP_CONDITIONAL_TYPE_LT:
                        case DC_APP_CONDITIONAL_TYPE_GT:
                        case DC_APP_CONDITIONAL_TYPE_LTE:
                        case DC_APP_CONDITIONAL_TYPE_GTE:
                            fprintf(stderr, "DCAPP _clean_xml_node(): StaticIf: operator %d not supported for string values, ignoring StaticIf\n", cond_type);
                            xmlUnlinkNode(node);
                            xmlFreeNode(node);
                            return;
                        default:
                            fprintf(stderr, "DCAPP _clean_xml_node(): StaticIf: invalid Operation %d for string comparison, expected EQ/NE (3-4)\n", cond_type);
                            break;
                    }
                }
            } else {
                // Single value boolean evaluation
                bool bool_val = dc_utils_string_to_boolean(value1);
                switch (cond_type) {
                    case DC_APP_CONDITIONAL_TYPE_TRUE:
                        result = bool_val;
                        break;
                    case DC_APP_CONDITIONAL_TYPE_FALSE:
                        result = !bool_val;
                        break;
                    default:
                        fprintf(stderr, "DCAPP _clean_xml_node(): StaticIf: Operation %d (EQ/NE/LT/GT/LTE/GTE) requires Value2 attribute\n", cond_type);
                        break;
                }
            }

            // Step 1b: Process ALL children first (critical for Include expansion)
            xmlNodePtr child = node->children;
            while (child) {
                xmlNodePtr child_next = child->next;
                _clean_xml_node(context, child, directory);
                child = child_next;
            }

            // Step 2: Check if ANY <True> element exists (don't store address)
            bool has_true_node = false;
            for (xmlNodePtr scan = node->children; scan != NULL; scan = scan->next) {
                if (scan->type == XML_ELEMENT_NODE) {
                    DcAppElemType scan_type = dc_app_xml_node_to_elem_type(scan);
                    if (scan_type == DC_APP_ELEM_TYPE_TRUE) {
                        has_true_node = true;
                        break;
                    }
                }
            }

            // Step 3: Handle the 4 cases
            if (has_true_node && result) {
                // Case 1: Explicit mode, true branch
                // Remove all children NOT of element type <True>
                child = node->children;
                while (child) {
                    xmlNodePtr next = child->next;
                    bool should_remove = true;
                    if (child->type == XML_ELEMENT_NODE) {
                        DcAppElemType child_type = dc_app_xml_node_to_elem_type(child);
                        if (child_type == DC_APP_ELEM_TYPE_TRUE) {
                            should_remove = false;
                        }
                    }
                    if (should_remove) {
                        xmlUnlinkNode(child);
                        xmlFreeNode(child);
                    }
                    child = next;
                }

                // Collect all children from ALL <True> elements
                xmlNodePtr collected_first = NULL;
                xmlNodePtr collected_last = NULL;

                child = node->children;
                while (child) {
                    xmlNodePtr next = child->next;
                    // child is guaranteed to be a <True> element

                    // Steal all children from this True wrapper
                    if (child->children) {
                        if (!collected_first) {
                            // First True wrapper - take its children as-is
                            collected_first = child->children;
                            collected_last = child->last;
                        } else {
                            // Subsequent True wrappers - append their children
                            collected_last->next = child->children;
                            child->children->prev = collected_last;
                            collected_last = child->last;
                        }

                        // Update parent pointers for stolen children
                        for (xmlNodePtr stolen = child->children; stolen; stolen = stolen->next) {
                            stolen->parent = node;
                        }

                        // Unlink children from True wrapper before freeing it
                        child->children = NULL;
                        child->last = NULL;
                    }

                    // Free the now-empty True wrapper
                    xmlUnlinkNode(child);
                    xmlFreeNode(child);
                    child = next;
                }

                // Make collected children the direct children of StaticIf
                node->children = collected_first;
                node->last = collected_last;

            } else if (has_true_node && !result) {
                // Case 2: Explicit mode, false branch
                // Remove all children NOT of element type <False>
                child = node->children;
                while (child) {
                    xmlNodePtr next = child->next;
                    bool should_remove = true;
                    if (child->type == XML_ELEMENT_NODE) {
                        DcAppElemType child_type = dc_app_xml_node_to_elem_type(child);
                        if (child_type == DC_APP_ELEM_TYPE_FALSE) {
                            should_remove = false;
                        }
                    }
                    if (should_remove) {
                        xmlUnlinkNode(child);
                        xmlFreeNode(child);
                    }
                    child = next;
                }

                // Collect all children from ALL <False> elements
                xmlNodePtr collected_first = NULL;
                xmlNodePtr collected_last = NULL;

                child = node->children;
                while (child) {
                    xmlNodePtr next = child->next;
                    // child is guaranteed to be a <False> element

                    // Steal all children from this False wrapper
                    if (child->children) {
                        if (!collected_first) {
                            // First False wrapper - take its children as-is
                            collected_first = child->children;
                            collected_last = child->last;
                        } else {
                            // Subsequent False wrappers - append their children
                            collected_last->next = child->children;
                            child->children->prev = collected_last;
                            collected_last = child->last;
                        }

                        // Update parent pointers for stolen children
                        for (xmlNodePtr stolen = child->children; stolen; stolen = stolen->next) {
                            stolen->parent = node;
                        }

                        // Unlink children from False wrapper before freeing it
                        child->children = NULL;
                        child->last = NULL;
                    }

                    // Free the now-empty False wrapper
                    xmlUnlinkNode(child);
                    xmlFreeNode(child);
                    child = next;
                }

                // Make collected children the direct children of StaticIf
                node->children = collected_first;
                node->last = collected_last;

            } else if (!has_true_node && result) {
                // Case 3: Implicit mode, true branch
                // Remove all children of element type <True> or <False>
                child = node->children;
                while (child) {
                    xmlNodePtr next = child->next;
                    if (child->type == XML_ELEMENT_NODE) {
                        DcAppElemType child_type = dc_app_xml_node_to_elem_type(child);
                        if (child_type == DC_APP_ELEM_TYPE_TRUE || child_type == DC_APP_ELEM_TYPE_FALSE) {
                            xmlUnlinkNode(child);
                            xmlFreeNode(child);
                        }
                    }
                    child = next;
                }

            } else {
                // Case 4: Implicit mode, false branch
                // Remove all children
                child = node->children;
                while (child) {
                    xmlNodePtr next = child->next;
                    xmlUnlinkNode(child);
                    xmlFreeNode(child);
                    child = next;
                }
                node->children = NULL;
                node->last = NULL;
            }

            // Step 4: Splice StaticIf's remaining children into parent (Dummy pattern)
            if (node->children) {
                xmlNodePtr first_child = node->children;
                xmlNodePtr last_child = node->last;

                // Update parent
                if (node->parent) {
                    if (node->parent->children == node) {
                        node->parent->children = first_child;
                    }
                    if (node->parent->last == node) {
                        node->parent->last = last_child;
                    }
                }

                // Update siblings
                if (node->prev) {
                    node->prev->next = first_child;
                }
                if (node->next) {
                    node->next->prev = last_child;
                }

                // Update children
                for (xmlNodePtr curr_child = first_child; curr_child; curr_child = curr_child->next) {
                    curr_child->parent = node->parent;
                }
                first_child->prev = node->prev;
                last_child->next = node->next;

                // Unlink StaticIf node
                node->children = NULL;
                node->last = NULL;
                node->parent = NULL;
                node->next = NULL;
                node->prev = NULL;

                // Free StaticIf node
                xmlFreeNode(node);
            } else {
                // No children to splice, just remove StaticIf node
                xmlUnlinkNode(node);
                xmlFreeNode(node);
            }
            return;
        }

        case DC_APP_ELEM_TYPE_INCLUDE: {

            // check if include is optional
            xmlChar *optional_str = xmlGetProp(node, BAD_CAST "Optional");
            int optional = optional_str ? dc_utils_string_to_boolean((const char *)optional_str) : 0;
            if (optional_str) {
                xmlFree(optional_str);
            }

            // get include file name
            xmlChar *filepath = xmlNodeGetContent(node);
            if (!filepath) {
                filepath = xmlGetProp(node, BAD_CAST "File");
                if (!filepath) {
                    fprintf(stderr, "DCAPP _clean_xml_node(): Node content missing in <Include> definition\n");
                }
            }
            char cleaned_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
            strncpy(cleaned_filepath, (const char *)filepath, sizeof(cleaned_filepath) - 1);
            cleaned_filepath[sizeof(cleaned_filepath) - 1] = '\0';
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

            // check if file exists when optional
            if (!dc_utils_file_exists(canon_filepath)) {
                if (optional) {
                    // file doesn't exist and include is optional, skip it
                    fprintf(stderr, "DCAPP _clean_xml_node(): Optional include file '%s' not found, skipping\n", canon_filepath);
                    xmlUnlinkNode(node);
                    xmlFreeNode(node);
                    return;
                }
            }

            // get canonical directory from filepath
            char include_directory[DC_UTILS_FILEPATH_BUFFER_SIZE];
            dc_utils_get_directory(canon_filepath, include_directory, sizeof(include_directory));

            // read XML file
            xmlDocPtr sub_doc = xmlReadFile(canon_filepath, NULL, XML_PARSE_NOBLANKS);
            if (!sub_doc) {
                fprintf(stderr, "DCAPP _clean_xml_node(): Unable to read config file %s\n", canon_filepath);
            }

            // get root element
            xmlNodePtr sub_node = xmlDocGetRootElement(sub_doc);
            if (!sub_node) {
                fprintf(stderr, "DCAPP _clean_xml_node(): Unable to get root element of config file %s\n", canon_filepath);
            }

            // copy loaded content
            xmlNodePtr new_node = xmlDocCopyNode(sub_node, node->doc, 1);

            // add _Directory attribute to all direct children
            for (xmlNodePtr child = new_node->children; child; child = child->next) {
                if (child->type == XML_ELEMENT_NODE) {
                    xmlSetProp(child, BAD_CAST "_Directory", BAD_CAST include_directory);
                }
            }

            // replace Include's children with loaded content
            xmlNodePtr old_child = node->children;
            node->children = new_node->children;
            node->last = new_node->last;

            // update parent pointers for all children
            for (xmlNodePtr child = node->children; child; child = child->next) {
                child->parent = node;
            }

            // unlink loaded content (now owned by Include node)
            new_node->children = NULL;
            new_node->last = NULL;
            xmlFreeNode(new_node);

            // free old child (text node)
            if (old_child) {
                xmlUnlinkNode(old_child);
                xmlFreeNode(old_child);
            }

            // free loaded document
            xmlFreeDoc(sub_doc);

            // process children first with include directory
            xmlNodePtr child = node->children;
            while (child) {
                xmlNodePtr child_next = child->next;
                _clean_xml_node(context, child, include_directory);
                child = child_next;
            }

            // splice children into parent and free wrapper
            _splice_children_into_parent_and_free_wrapper(node);
            return;
        }

        case DC_APP_ELEM_TYPE_DEFAULT:
        case DC_APP_ELEM_TYPE_STYLE: {

            // get style name
            char style_name[DC_VALUE_STRING_BUFFER_SIZE];
            if (elem_type == DC_APP_ELEM_TYPE_DEFAULT) {
                strcpy(style_name, "default");
            } else {
                xmlChar *raw_style_name = xmlGetProp(node, BAD_CAST "Name");
                if (!raw_style_name) {
                    fprintf(stderr, "DCAPP _clean_xml_node(): Style name missing in <Style> definition\n");
                }
                strncpy(style_name, (const char *)raw_style_name, DC_VALUE_STRING_BUFFER_SIZE - 1);
                style_name[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
                xmlFree(raw_style_name);
            }

            // clean children, add styles
            xmlNodePtr child = node->children;
            while (child) {

                xmlNodePtr child_next = child->next;

                // clone child node
                xmlNodePtr orphan_child = xmlCopyNode(child, 1); // shallow: no children

                // dereference constants
                _dereference_node_attrs_and_content(context, orphan_child);

                // add to styles
                DcAppElemType child_type = dc_app_xml_node_to_elem_type(orphan_child);
                _add_style(context, style_name, child_type, orphan_child);

                // increment
                child = child_next;
            }
            return;
        }

        case DC_APP_ELEM_TYPE_PANEL:
            break;

        default:
            break;
    }

    // save to file
    // xmlSaveFormatFile("/data/nreagan/dcapp-pl/cache/xml.log", node->doc, 1);

    // clean children
    xmlNodePtr child = node->children;
    while (child) {
        xmlNodePtr child_next = child->next;
        _clean_xml_node(context, child, directory);
        child = child_next;
    }
}

void dc_app_config_save_to_file(DcAppConfig *config, const char *filepath) {
    xmlSaveFormatFile(filepath, config->xml_doc, 1);
}

DcAppLookupIndex _get_const_index(_ConfigContext *context, const char *name) {

    for (int ii = 0; ii < sbcount(context->sb_const_name_offsets); ii++) {
        const char *lookup_name = &(context->sb_const_names[context->sb_const_name_offsets[ii]]);
        if (strcmp(name, lookup_name) == 0) {
            return ii;
        }
    }
    return DC_APP_LOOKUP_INDEX_UNDEFINED;
}

// sets an existing constant
void _set_const(_ConfigContext *context, DcAppLookupIndex index, const char *new_value) {

    // set const value at index
    char **addr = &(context->sb_consts[index].val);
    sbclear(*addr);
    sbpushn(*addr, new_value, (int)(strlen(new_value) + 1));
}

// adds a new constant
void _add_const(_ConfigContext *context, const char *name, const char *value, bool is_immutable) {

    // add const name to buffer
    sbpush(context->sb_const_name_offsets, sbcount(context->sb_const_names));
    sbpushn(context->sb_const_names, name, (int)(strlen(name) + 1));

    // set const to value
    _Constant constant;
    constant.val = NULL;
    sbpushn(constant.val, value, (int)(strlen(value) + 1));
    constant.is_immutable = is_immutable;

    // register
    sbpush(context->sb_consts, constant);
}

static void _add_const_int(_ConfigContext *context, const char *name, int value_int, bool is_immutable) {
    char value_str[20];
    snprintf(value_str, 20, "%d", value_int);
    _add_const(context, name, value_str, is_immutable);
}

// set a consts value
void _register_const_by_name(_ConfigContext *context, const char *name, const char *new_value, bool is_immutable) {
    DcAppLookupIndex const_index = _get_const_index(context, name);
    if (const_index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        _add_const(context, name, new_value, is_immutable);
    } else {
        if (context->sb_consts[const_index].is_immutable) {
            fprintf(stderr, "DCApp _register_const_by_name(): ignoring constant registration for '%s': immutable\n", name);
        } else {
            _set_const(context, const_index, new_value);
        }
    }
}

// set a consts value (public)
void dc_app_config_register_const_by_name(DcAppConfig *config, const char *name, const char *new_value, bool is_immutable) {
    _ConfigContext *context = &(_sb_contexts[config->_index]);
    _register_const_by_name(context, name, new_value, is_immutable);
}

// get a consts value
const char *_get_const_by_name(_ConfigContext *context, const char *name) {
    DcAppLookupIndex const_index = _get_const_index(context, name);
    if (const_index == DC_APP_LOOKUP_INDEX_UNDEFINED) {
        fprintf(stderr, "DCAPP _get_const_by_name(): constant '%s' does not exist\n", name);
        return NULL;
    } else {
        return context->sb_consts[const_index].val;
    }
}

// expand a string using consts
void _dereference_constants(_ConfigContext *context, const char *in, char *out, size_t out_size) {

    // return string if no '$'/'#'
    if (dc_utils_str_find_first_of(in, "#$") == -1) {
        strncpy(out, in, out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    // iterate through each character
    size_t in_length = strlen(in);
    int    out_index = 0;
    for (int in_index = 0; in_index < in_length && out_index < out_size - 1; in_index++) {

        // skip backslash-escaped # and $
        if (in[in_index] == '\\' && in_index + 1 < in_length && (in[in_index + 1] == '#' || in[in_index + 1] == '$')) {
            out[out_index++] = in[++in_index];
            continue;
        }

        // if a character is a constant '#' or env variable '$', recursively
        // expand the values within.
        if (in[in_index] == '#' || in[in_index] == '$') {
            // error if ending on a #/$
            if (in_index + 1 >= in_length) {
                fprintf(stderr, "DCAPP _dereference_constants(): Cannot have string ending on an unescaped #/$: '%s'\n", in);
                return;
            }

            // find ending index for constant/env variable reference (account for both non-squigglied and squigglied references)
            size_t subtext_start_index;
            size_t subtext_length;
            size_t subtext_length_with_symbols;
            if (in[in_index + 1] == '{') {
                int num_open_brackets = 1;
                int subtext_end_index;
                for (subtext_end_index = in_index + 2; subtext_end_index < in_length; subtext_end_index++) {
                    if (in[subtext_end_index] == '{') {
                        num_open_brackets++;
                    } else if (in[subtext_end_index] == '}') {
                        num_open_brackets--;
                    }

                    if (num_open_brackets == 0) {
                        break;
                    }
                }

                if (num_open_brackets > 0) {
                    fprintf(stderr, "DCAPP _dereference_constants(): mismatch with squiggly braces: '%s'\n", in);
                    return;
                }

                subtext_start_index         = in_index + 2;
                subtext_length              = subtext_end_index - subtext_start_index;
                subtext_length_with_symbols = subtext_length + 2;
            } else {
                static const char *valid_chars       = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#$";
                int                subtext_end_index = dc_utils_str_find_first_not_of(&(in[in_index]), valid_chars);
                if (subtext_end_index == -1) {
                    subtext_end_index = (int)in_length;
                } else {
                    subtext_end_index += in_index;  // Convert from relative to absolute position
                }

                subtext_start_index         = in_index + 1;
                subtext_length              = subtext_end_index - subtext_start_index;
                subtext_length_with_symbols = subtext_length;
            }

            // get substring, ensure no strange values within
            char subtext[DC_VALUE_STRING_BUFFER_SIZE];
            strncpy(subtext, &in[subtext_start_index], subtext_length);
            subtext[subtext_length] = '\0';
            if (dc_utils_str_find_first(subtext, '@') != -1) {
                fprintf(stderr, "DCAPP _dereference_constants(): cannot have variable nested inside variable/constant expansion: '%s'\n", in);
                return;
            }

            // recursion to clean the inner text
            char subtext_cleaned[DC_VALUE_STRING_BUFFER_SIZE];
            _dereference_constants(context, subtext, subtext_cleaned, sizeof(subtext_cleaned));

            // if constant, pull value from list of constants. otherwise use the environment
            if (in[in_index] == '#') {
                const char *const_value = _get_const_by_name(context, subtext_cleaned);
                if (const_value) {
                    for (int ii = 0; ii < strlen(const_value) && out_index < out_size - 1;) {
                        out[out_index++] = const_value[ii++];
                    }
                }
            } else if (in[in_index] == '$') {
                const char *env_value = dc_utils_get_env(subtext_cleaned);
                if (env_value) {
                    for (int ii = 0; ii < strlen(env_value) && out_index < out_size - 1;) {
                        out[out_index++] = env_value[ii++];
                    }
                }
            }

            // increment in_index by the cleaned amount
            in_index += (int)subtext_length_with_symbols;
        } else {
            out[out_index++] = in[in_index];
        }
    }

    if (out_index < out_size) {
        out[out_index] = '\0';
    }
}

static DcAppStyleIndex _get_style_index(_ConfigContext *context, const char *name) {
    if (name) {
        for (int ii = 0; ii < sbcount(context->sb_styles); ii++) {
            const char *comp_name = &(context->sb_style_names[context->sb_style_name_offsets[ii]]);
            if (strcmp(name, comp_name) == 0) {
                return ii;
            }
        }
    }
    return -1;
}

static void _add_style(_ConfigContext *context, const char *name, DcAppElemType elem_type, xmlNodePtr xml_node) {
    if (name) {

        DcAppStyleIndex style_index = _get_style_index(context, name);

        // create new style if it doesn't exist
        if (style_index == _STYLE_INDEX_UNDEFINED) {
            sbpush(context->sb_style_name_offsets, sbcount(context->sb_style_names));
            sbpushn(context->sb_style_names, name, (int)(strlen(name) + 1));
            _ElemStyle style = {};
            sbpush(context->sb_styles, style);
            style_index = sbcount(context->sb_styles) - 1;
        }

        // update style
        _ElemStyle *style           = &(context->sb_styles[style_index]);
        style->xml_nodes[elem_type] = xml_node;
    } else {
        fprintf(stderr, "DCAPP _set_style(): name %s is undefined\n", name);
    }
}

static xmlChar *_get_style_attr(_ConfigContext *context, int style_index, DcAppElemType elem_type, const char *name) {
    xmlNodePtr style_xml_node = context->sb_styles[style_index].xml_nodes[elem_type];
    xmlChar   *value          = xmlGetProp(style_xml_node, BAD_CAST name);
    if (value) {
        return value;
    }
    return NULL;
}

static xmlChar *_get_style_content(_ConfigContext *context, int style_index, DcAppElemType elem_type) {
    xmlNodePtr style_xml_node = context->sb_styles[style_index].xml_nodes[elem_type];
    xmlChar   *value          = xmlNodeGetContent(style_xml_node);
    if (value) {
        return value;
    }
    return NULL;
}

void _dereference_node_attrs_and_content(_ConfigContext *context, xmlNodePtr node) {

    // expands constants on each attribute
    xmlAttrPtr attr = node->properties;
    while (attr) {
        xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
        if (value) {
            char cleaned_value[DC_VALUE_STRING_BUFFER_SIZE];
            _dereference_constants(context, (char *)value, cleaned_value, sizeof(cleaned_value));
            xmlSetProp(node, attr->name, BAD_CAST cleaned_value);
            xmlFree(value);
        }
        attr = attr->next;
    }

    // expand constants in text content
    xmlNodePtr child = node->children;
    while (child) {
        if (child->type == XML_TEXT_NODE) {
            xmlChar *value = xmlNodeGetContent(child);
            char     cleaned_value[DC_VALUE_STRING_BUFFER_SIZE];
            _dereference_constants(context, (char *)value, cleaned_value, sizeof(cleaned_value));
            xmlNodeSetContent(child, BAD_CAST cleaned_value);
            xmlFree(value);
        }
        child = child->next;
    }
}

// must be freed!
static char *_unquote(const char *str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str);
    if (len >= 2 && ((str[0] == '"' && str[len - 1] == '"') || (str[0] == '\'' && str[len - 1] == '\''))) {
        char *result = strndup(str + 1, len - 2);
        return result;
    }
    return strdup(str);
}
