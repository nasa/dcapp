#include "xml_preprocessor.h"
#include "xml_element.h"
#include "draw_types.h"
#include "node_types.h"
#include "pixelstream_types.h"
#include "planet_types.h"
#include "libxml/tree.h"
#include "value.h"
#include "../utils/env.h"
#include "../utils/file.h"
#include "../utils/log.h"
#include "../utils/math.h"
#include "../utils/stb_sb.h"
#include "../utils/string.h"

#include <libxml/parser.h>
#include <libxml/xmlsave.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//~ indices and flags

//- style indices
typedef int _StyleIndex;
static const _StyleIndex _STYLE_INDEX_UNDEFINED = 0;
static const _StyleIndex _STYLE_INDEX_DEFAULT = 1;

//- constant indices
typedef int _ConstIndex;
static const _ConstIndex _CONST_INDEX_UNDEFINED = 0;
#define _CONST_FIRST_INDEX 1

// warning suppression from the root suppresswarnings attribute
enum {
    _SUPPRESS_NONE = 0,
    _SUPPRESS_MISSING_CONSTANT = 1 << 0,
    _SUPPRESS_MISSING_VARIABLE = 1 << 1,
    _SUPPRESS_MISSING_STYLE = 1 << 2,
    _SUPPRESS_STYLE_OVERRIDE = 1 << 3,
};

//~ internal state

typedef struct __ElemStyle {
    xmlNodePtr xml_nodes[DC_APP_XML_ELEMENT_TYPE__COUNT];
} _ElemStyle;

typedef struct __Constant {
    char *val;
    bool is_immutable;
} _Constant;

typedef struct __ConfigContext {

    //- warning suppression
    unsigned int suppress_warnings;

    //- constants
    char *sb_const_names;
    int *sb_const_name_offsets;
    _Constant *sb_consts;

    //- styles
    char *sb_style_names;
    int *sb_style_name_offsets;
    _ElemStyle *sb_styles;

} _XmlPreprocessorPassContext;

struct DcAppXmlPreprocessorContext {
    _XmlPreprocessorPassContext context;

    //- xml document
    xmlDocPtr xml_doc;
    bool xml_doc_is_cleaned;

    //- paths
    char *dcapp_dir_path;
    char *config_file_path;
    char *config_dir_path;
    char *cache_dir_path;
};

//~ forward declarations

//- constant helpers
static void _register_const_by_name(_XmlPreprocessorPassContext *context, const char *name, const char *new_value, bool is_immutable);
static const char *_get_const_by_name(_XmlPreprocessorPassContext *context, const char *name);
static void _dereference_constants(_XmlPreprocessorPassContext *context, const char *in, char *out, size_t out_size);
static _ConstIndex _get_const_index(_XmlPreprocessorPassContext *context, const char *name);
static void _set_const(_XmlPreprocessorPassContext *context, _ConstIndex index, const char *new_value);
static void _add_const(_XmlPreprocessorPassContext *context, const char *name, const char *value, bool is_immutable);
static void _add_const_int(_XmlPreprocessorPassContext *context, const char *name, int value_int, bool is_immutable);

//- style helpers
static _StyleIndex _get_style_index(_XmlPreprocessorPassContext *context, const char *name);
static void _add_style(_XmlPreprocessorPassContext *context, const char *name, DcAppXmlElementType elem_type, xmlNodePtr xml_node);
static xmlChar *_get_style_attr(_XmlPreprocessorPassContext *context, int style_index, DcAppXmlElementType elem_type, const char *name);
static xmlChar *_get_style_content(_XmlPreprocessorPassContext *context, int style_index, DcAppXmlElementType elem_type);

//- xml helpers
static void _preprocess_xml_node(_XmlPreprocessorPassContext *context, xmlNodePtr node, char *directory);
static void _dereference_node_attrs_and_content(_XmlPreprocessorPassContext *context, xmlNodePtr node);
static void _splice_children_into_parent_and_free_wrapper(xmlNodePtr node);
static void _save_to_file(DcAppXmlPreprocessorContext *config, const char *filepath);
static void _write_indent(FILE *f, int depth);
static void _write_xml_node(FILE *f, xmlNodePtr node, int depth);

//- argument helpers
static char *_unquote(const char *str);

//~ preprocessor lifecycle

DcAppXmlPreprocessorContext *dc_app_xml_preprocessor_context_create(const char *config_path, char **args, int arg_count) {
    DcAppXmlPreprocessorContext *config = (DcAppXmlPreprocessorContext *)malloc(sizeof(DcAppXmlPreprocessorContext));

    //- resolve configuration paths
    char cwd[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_get_cwd(cwd, DC_UTILS_FILEPATH_BUFFER_SIZE);

    config->config_file_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    if (dc_utils_is_relative_path(config_path)) {
        char abs_config_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
        dc_utils_join_paths(cwd, config_path, abs_config_path, sizeof(abs_config_path));
        dc_utils_canonicalize_path(abs_config_path, config->config_file_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    } else {
        dc_utils_canonicalize_path(config_path, config->config_file_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    }

    config->config_dir_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_get_directory(config->config_file_path, config->config_dir_path, DC_UTILS_FILEPATH_BUFFER_SIZE);

    //- resolve application directories
    char exe_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_get_exe_path(exe_path, sizeof(exe_path));
    char exe_dir[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_get_directory(exe_path, exe_dir, sizeof(exe_dir));

    char dcapp_dir_path_abs[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_join_paths(exe_dir, "../..", dcapp_dir_path_abs, DC_UTILS_FILEPATH_BUFFER_SIZE);
    config->dcapp_dir_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_canonicalize_path(dcapp_dir_path_abs, config->dcapp_dir_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_create_directory(config->dcapp_dir_path);

    config->cache_dir_path = (char *)malloc(DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_join_paths(config->dcapp_dir_path, "cache", config->cache_dir_path, DC_UTILS_FILEPATH_BUFFER_SIZE);
    dc_utils_create_directory(config->cache_dir_path);

    //- load the source xml document
    config->xml_doc = xmlReadFile(config->config_file_path, "UTF-8", XML_PARSE_NOBLANKS);
    if (!config->xml_doc) {
        DC_LOG_ERROR("Config", "dc_app_xml_preprocessor_context_create: unable to read config file '%s'", config->config_file_path);
    }
    config->xml_doc_is_cleaned = false;

    //- initialize style and constant registries
    _XmlPreprocessorPassContext context = {};

    // reserve style index zero as undefined
    sbresize(context.sb_styles, 1);
    sbresize(context.sb_style_name_offsets, 1);
    sbresize(context.sb_style_names, 1);

    // reserve style index one for defaults
    sbpush(context.sb_style_name_offsets, sbcount(context.sb_style_names));
    sbpushn(context.sb_style_names, "default", (int)(strlen("default") + 1));
    _ElemStyle default_style = {};
    sbpush(context.sb_styles, default_style);

    // reserve constant index zero as undefined
    sbresize(context.sb_consts, 1);
    sbresize(context.sb_const_name_offsets, 1);
    sbresize(context.sb_const_names, 1);

    //- register built-in constants
    _add_const_int(&context, "_align_left_", DC_APP_DRAW_ALIGNMENT_TYPE_LEFT, true);
    _add_const_int(&context, "_align_center_", DC_APP_DRAW_ALIGNMENT_TYPE_CENTER, true);
    _add_const_int(&context, "_align_right_", DC_APP_DRAW_ALIGNMENT_TYPE_RIGHT, true);
    _add_const_int(&context, "_align_bottom_", DC_APP_DRAW_ALIGNMENT_TYPE_BOTTOM, true);
    _add_const_int(&context, "_align_middle_", DC_APP_DRAW_ALIGNMENT_TYPE_MIDDLE, true);
    _add_const_int(&context, "_align_top_", DC_APP_DRAW_ALIGNMENT_TYPE_TOP, true);
    _add_const_int(&context, "_button_standard_", DC_APP_BUTTON_TYPE_STANDARD, true);
    _add_const_int(&context, "_button_momentary_", DC_APP_BUTTON_TYPE_MOMENTARY, true);
    _add_const_int(&context, "_button_toggle_", DC_APP_BUTTON_TYPE_TOGGLE, true);
    _add_const_int(&context, "_if_true_", DC_APP_CONDITIONAL_TYPE_TRUE, true);
    _add_const_int(&context, "_if_false_", DC_APP_CONDITIONAL_TYPE_FALSE, true);
    _add_const_int(&context, "_if_eq_", DC_APP_CONDITIONAL_TYPE_EQ, true);
    _add_const_int(&context, "_if_ne_", DC_APP_CONDITIONAL_TYPE_NE, true);
    _add_const_int(&context, "_if_lt_", DC_APP_CONDITIONAL_TYPE_LT, true);
    _add_const_int(&context, "_if_gt_", DC_APP_CONDITIONAL_TYPE_GT, true);
    _add_const_int(&context, "_if_lte_", DC_APP_CONDITIONAL_TYPE_LTE, true);
    _add_const_int(&context, "_if_gte_", DC_APP_CONDITIONAL_TYPE_GTE, true);
    _add_const_int(&context, "_pixelstream_shmem_", DC_APP_PIXELSTREAM_TYPE_SHMEM, true);
    _add_const_int(&context, "_pixelstream_mjpeg_", DC_APP_PIXELSTREAM_TYPE_MJPEG, true);
    _add_const_int(&context, "_planet_crs_geodetic_", DC_APP_PLANET_CRS_GEODETIC, true);
    _add_const_int(&context, "_planet_crs_cartesian_", DC_APP_PLANET_CRS_CARTESIAN, true);
    _add_const_int(&context, "_planet_attitude_frame_local_ned_", DC_APP_PLANET_ATTITUDE_FRAME_LOCAL_NED, true);
    _add_const_int(&context, "_planet_attitude_frame_cartesian_rpy_", DC_APP_PLANET_ATTITUDE_FRAME_CARTESIAN_RPY, true);
    _add_const_int(&context, "_set_equal_", DC_APP_SET_TYPE_EQUAL, true);
    _add_const_int(&context, "_set_add_", DC_APP_SET_TYPE_ADD, true);
    _add_const_int(&context, "_set_subtract_", DC_APP_SET_TYPE_SUBTRACT, true);
    _add_const_int(&context, "_set_multiply_", DC_APP_SET_TYPE_MULTIPLY, true);
    _add_const_int(&context, "_set_divide_", DC_APP_SET_TYPE_DIVIDE, true);
    _add_const_int(&context, "_set_min_", DC_APP_SET_TYPE_MIN, true);
    _add_const_int(&context, "_set_max_", DC_APP_SET_TYPE_MAX, true);
    _add_const_int(&context, "_set_push_", DC_APP_SET_TYPE_PUSH, true);
    _add_const_int(&context, "_set_pop_", DC_APP_SET_TYPE_POP, true);
    _add_const_int(&context, "_set_negate_", DC_APP_SET_TYPE_NEGATE, true);
    _add_const_int(&context, "_set_reciprocal_", DC_APP_SET_TYPE_RECIPROCAL, true);
    _add_const_int(&context, "_set_absolute_", DC_APP_SET_TYPE_ABSOLUTE, true);
    _add_const_int(&context, "_set_square_", DC_APP_SET_TYPE_SQUARE, true);
    _add_const_int(&context, "_set_sqrt_", DC_APP_SET_TYPE_SQRT, true);
    _add_const_int(&context, "_set_modulo_", DC_APP_SET_TYPE_MODULO, true);
    _add_const_int(&context, "_set_power_", DC_APP_SET_TYPE_POWER, true);
    _add_const_int(&context, "_set_log_", DC_APP_SET_TYPE_LOG, true);
    _add_const_int(&context, "_set_exp_", DC_APP_SET_TYPE_EXP, true);
    _add_const_int(&context, "_set_round_", DC_APP_SET_TYPE_ROUND, true);
    _add_const_int(&context, "_set_sign_", DC_APP_SET_TYPE_SIGN, true);
    _add_const_int(&context, "_variable_string_", DC_APP_VALUE_TYPE_STRING, true);
    _add_const_int(&context, "_variable_integer_", DC_APP_VALUE_TYPE_INTEGER, true);
    _add_const_int(&context, "_variable_double_", DC_APP_VALUE_TYPE_DOUBLE, true);
    _add_const_int(&context, "_variable_boolean_", DC_APP_VALUE_TYPE_BOOLEAN, true);
    //- red and pink constants
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
    _add_const(&context, "_color_scarlet_", "1.0 0.14 0.0", false);
    _add_const(&context, "_color_tomato_", "1.0 0.39 0.28", false);
    _add_const(&context, "_color_wine_", "0.45 0.18 0.22", false);
    _add_const(&context, "_color_raspberry_", "0.89 0.04 0.36", false);
    _add_const(&context, "_color_dark_red_", "0.55 0.0 0.0", false);
    //- orange constants
    _add_const(&context, "_color_orange_", "1.0 0.5 0.0", false);
    _add_const(&context, "_color_tangerine_", "1.0 0.6 0.0", false);
    _add_const(&context, "_color_pumpkin_", "1.0 0.46 0.1", false);
    _add_const(&context, "_color_apricot_", "0.98 0.81 0.69", false);
    _add_const(&context, "_color_cantaloupe_", "1.0 0.71 0.55", false);
    _add_const(&context, "_color_amber_", "1.0 0.75 0.0", false);
    _add_const(&context, "_color_burnt_orange_", "0.8 0.33 0.0", false);
    _add_const(&context, "_color_rust_", "0.72 0.25 0.05", false);
    _add_const(&context, "_color_terracotta_", "0.89 0.45 0.36", false);
    _add_const(&context, "_color_dark_orange_", "1.0 0.55 0.0", false);
    _add_const(&context, "_color_mango_", "1.0 0.51 0.26", false);
    _add_const(&context, "_color_persimmon_", "0.93 0.35 0.0", false);
    //- yellow constants
    _add_const(&context, "_color_yellow_", "1.0 1.0 0.0", false);
    _add_const(&context, "_color_lemon_", "1.0 1.0 0.31", false);
    _add_const(&context, "_color_mustard_", "1.0 0.86 0.35", false);
    _add_const(&context, "_color_gold_", "1.0 0.84 0.0", false);
    _add_const(&context, "_color_butter_", "1.0 0.94 0.75", false);
    _add_const(&context, "_color_champagne_", "0.97 0.91 0.81", false);
    _add_const(&context, "_color_sunflower_", "1.0 0.8 0.0", false);
    _add_const(&context, "_color_flax_", "0.93 0.87 0.51", false);
    _add_const(&context, "_color_cream_", "1.0 0.99 0.82", false);
    _add_const(&context, "_color_ivory_", "1.0 1.0 0.94", false);
    _add_const(&context, "_color_saffron_", "0.96 0.77 0.19", false);
    _add_const(&context, "_color_golden_rod_", "0.85 0.65 0.13", false);
    _add_const(&context, "_color_canary_", "1.0 0.94 0.0", false);
    //- green constants
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
    _add_const(&context, "_color_dark_green_", "0.0 0.39 0.0", false);
    _add_const(&context, "_color_sage_", "0.72 0.72 0.59", false);
    _add_const(&context, "_color_spring_green_", "0.0 1.0 0.5", false);
    _add_const(&context, "_color_hunter_green_", "0.21 0.37 0.23", false);
    _add_const(&context, "_color_kelly_green_", "0.3 0.73 0.09", false);
    _add_const(&context, "_color_pine_", "0.06 0.32 0.21", false);
    _add_const(&context, "_color_fern_", "0.44 0.64 0.26", false);
    _add_const(&context, "_color_neon_green_", "0.22 1.0 0.08", false);
    //- blue constants
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
    _add_const(&context, "_color_royal_blue_", "0.25 0.41 0.88", false);
    _add_const(&context, "_color_midnight_blue_", "0.1 0.1 0.44", false);
    _add_const(&context, "_color_cobalt_", "0.0 0.28 0.67", false);
    _add_const(&context, "_color_cornflower_blue_", "0.39 0.58 0.93", false);
    _add_const(&context, "_color_turquoise_", "0.25 0.88 0.82", false);
    _add_const(&context, "_color_cyan_", "0.0 1.0 1.0", false);
    _add_const(&context, "_color_aquamarine_", "0.5 1.0 0.83", false);
    _add_const(&context, "_color_electric_blue_", "0.49 0.98 1.0", false);
    _add_const(&context, "_color_periwinkle_", "0.8 0.8 1.0", false);
    //- purple and violet constants
    _add_const(&context, "_color_purple_", "0.5 0.0 0.5", false);
    _add_const(&context, "_color_indigo_", "0.29 0.0 0.51", false);
    _add_const(&context, "_color_lavender_", "0.9 0.9 0.98", false);
    _add_const(&context, "_color_plum_", "0.56 0.27 0.52", false);
    _add_const(&context, "_color_violet_", "0.93 0.51 0.93", false);
    _add_const(&context, "_color_amethyst_", "0.6 0.4 0.8", false);
    _add_const(&context, "_color_orchid_", "0.85 0.44 0.84", false);
    _add_const(&context, "_color_thistle_", "0.85 0.75 0.85", false);
    _add_const(&context, "_color_eggplant_", "0.38 0.25 0.32", false);
    _add_const(&context, "_color_magenta_", "0.8 0.0 0.8", false);
    _add_const(&context, "_color_mauve_", "0.88 0.69 1.0", false);
    _add_const(&context, "_color_lilac_", "0.78 0.64 0.78", false);
    _add_const(&context, "_color_grape_", "0.44 0.18 0.66", false);
    _add_const(&context, "_color_royal_purple_", "0.47 0.32 0.66", false);
    //- brown constants
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
    _add_const(&context, "_color_sienna_", "0.63 0.32 0.18", false);
    _add_const(&context, "_color_cinnamon_", "0.69 0.4 0.24", false);
    _add_const(&context, "_color_sandy_brown_", "0.96 0.64 0.38", false);
    //- neutral and gray constants
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
    _add_const(&context, "_color_snow_", "1.0 0.98 0.98", false);
    _add_const(&context, "_color_pearl_", "0.94 0.92 0.88", false);
    _add_const(&context, "_color_smoke_", "0.96 0.96 0.96", false);
    _add_const(&context, "_color_bone_", "0.89 0.85 0.79", false);
    _add_const(&context, "_color_graphite_", "0.29 0.29 0.29", false);
    _add_const(&context, "_color_iron_", "0.32 0.34 0.36", false);
    _add_const(&context, "_color_steel_", "0.5 0.5 0.55", false);
    //- stencil constants
    _add_const(&context, "_stencil_color_", "0 0 0 1", false);

    // todo avoid temporary heap allocations for command line constants
    for (int ii = 0; ii < arg_count; ii++) {

        // split each name and value at the first equals sign
        const char *eq_addr = strchr(args[ii], '=');
        if (!eq_addr) {
            DC_LOG_WARN("Config", "dc_app_xml_preprocessor_context_create(): input argument '%s' has no value set; ignoring", args[ii]);
            continue;
        }

        size_t name_len = eq_addr - args[ii];
        char *name_part = strndup(args[ii], name_len);

        const char *value_part = eq_addr + 1;

        // remove matching outer quotes
        char *arg_name = _unquote(name_part);
        char *arg_value = _unquote(value_part);

        // expand constants before registration
        char arg_value_dereferenced[DC_APP_VALUE_STRING_BUFFER_SIZE];
        _dereference_constants(&context, arg_value, arg_value_dereferenced, sizeof(arg_value_dereferenced));

        _register_const_by_name(&context, arg_name, arg_value_dereferenced, true);

        free(name_part);
        free(arg_name);
        free(arg_value);
    }

    config->context = context;

    return config;
}

void dc_app_xml_preprocessor_context_destroy(DcAppXmlPreprocessorContext *config) {
    free(config->config_file_path);
    free(config->config_dir_path);
    free(config->dcapp_dir_path);
    free(config->cache_dir_path);
    xmlFreeDoc(config->xml_doc);

    _XmlPreprocessorPassContext *context = &config->context;
    sbfree(context->sb_style_name_offsets);
    sbfree(context->sb_style_names);

    // release cloned style nodes
    for (int ii = _STYLE_INDEX_DEFAULT; ii < sbcount(context->sb_styles); ii++) {
        for (int jj = 0; jj < DC_APP_XML_ELEMENT_TYPE__COUNT; jj++) {
            if (context->sb_styles[ii].xml_nodes[jj]) {
                xmlFreeNode(context->sb_styles[ii].xml_nodes[jj]);
            }
        }
    }
    sbfree(context->sb_styles);

    sbfree(context->sb_const_names);
    sbfree(context->sb_const_name_offsets);
    for (int ii = _CONST_FIRST_INDEX; ii < sbcount(context->sb_consts); ii++) {
        sbfree(context->sb_consts[ii].val);
    }
    sbfree(context->sb_consts);
    free(config);
}

//~ preprocessing

void dc_app_xml_preprocessor_preprocess(DcAppXmlPreprocessorContext *config) {

    _XmlPreprocessorPassContext *context = &config->context;

    //- validate the root element
    xmlNodePtr node = xmlDocGetRootElement(config->xml_doc);
    if (node == NULL) {
        DC_LOG_ERROR("Config", "dc_app_xml_preprocessor_preprocess(): unable to get root element of config file");
    }

    if (dc_app_xml_element_type_from_xml_node(node) != DC_APP_XML_ELEMENT_TYPE_DCAPP) {
        DC_LOG_ERROR("Config", "dc_app_xml_preprocessor_preprocess(): configuration root element is not DCAPP");
    }

    //- read warning suppression options
    xmlChar *suppress_warnings_attr = xmlGetProp(node, BAD_CAST "SuppressWarnings");
    if (suppress_warnings_attr) {
        unsigned int suppress_flags = 0;
        char *token = strtok((char *)suppress_warnings_attr, ",");
        while (token) {
            // trim surrounding spaces from each option
            while (*token == ' ')
                token++;
            char *end = token + strlen(token) - 1;
            while (end > token && *end == ' ')
                *end-- = '\0';

            if (strcmp(token, "all") == 0) {
                suppress_flags = ~0u; // suppress every warning class
            } else if (strcmp(token, "missing-constants") == 0) {
                suppress_flags |= _SUPPRESS_MISSING_CONSTANT;
            } else if (strcmp(token, "missing-variables") == 0) {
                suppress_flags |= _SUPPRESS_MISSING_VARIABLE;
            } else if (strcmp(token, "missing-styles") == 0) {
                suppress_flags |= _SUPPRESS_MISSING_STYLE;
            } else if (strcmp(token, "style-overrides") == 0) {
                suppress_flags |= _SUPPRESS_STYLE_OVERRIDE;
            }
            token = strtok(NULL, ",");
        }
        xmlFree(suppress_warnings_attr);

        context->suppress_warnings = suppress_flags;
    }

    //- preprocess the tree in place
    _preprocess_xml_node(context, node, config->config_dir_path);
    config->xml_doc_is_cleaned = true;
}

void dc_app_xml_preprocessor_save_preprocessed(DcAppXmlPreprocessorContext *config, const char *output_name) {
    char preprocessed_name[256];
    if (output_name) {
        const char *base = strrchr(output_name, '/');
        base = base ? base + 1 : output_name;
        snprintf(preprocessed_name, sizeof(preprocessed_name), "%s", base);
    } else {
        const char *name = strrchr(config->config_file_path, '/');
        name = name ? name + 1 : config->config_file_path;
        const char *dot = strrchr(name, '.');
        if (dot) {
            snprintf(preprocessed_name, sizeof(preprocessed_name), "%.*s.preprocessed.xml", (int)(dot - name), name);
        } else {
            snprintf(preprocessed_name, sizeof(preprocessed_name), "%s.preprocessed.xml", name);
        }
    }
    char filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_join_paths(config->cache_dir_path, preprocessed_name, filepath, sizeof(filepath));
    _save_to_file(config, filepath);
}

void dc_app_xml_preprocessor_export_environment(const DcAppXmlPreprocessorContext *config) {
    if (!config) return;

    const char *dcapp_home = dc_app_xml_preprocessor_root_directory(config);
    const char *display_home = dc_app_xml_preprocessor_directory(config);

    dc_utils_set_env("DCAPP_HOME", dcapp_home, 1);
    dc_utils_set_env("DCAPP_DISPLAY_HOME", display_home, 1);

    // keep legacy aliases for existing displays and logic modules
    dc_utils_set_env("dcappHome", dcapp_home, 1);
    dc_utils_set_env("dcappDisplayHome", display_home, 1);
}

//~ preprocessor results

const char *dc_app_xml_preprocessor_directory(const DcAppXmlPreprocessorContext *config) {
    return config->config_dir_path;
}

const char *dc_app_xml_preprocessor_root_directory(const DcAppXmlPreprocessorContext *config) {
    return config->dcapp_dir_path;
}

xmlNodePtr dc_app_xml_preprocessor_root(const DcAppXmlPreprocessorContext *config) {
    return xmlDocGetRootElement(config->xml_doc);
}

bool dc_app_xml_preprocessor_suppresses_missing_variable(const DcAppXmlPreprocessorContext *config) {
    return (config->context.suppress_warnings & _SUPPRESS_MISSING_VARIABLE) != 0;
}

//~ tree preprocessing

// splice processed children into the wrapper's parent before freeing it
static void _splice_children_into_parent_and_free_wrapper(xmlNodePtr node) {
    if (node->children) {
        xmlNodePtr first_child = node->children;
        xmlNodePtr last_child = node->last;

        //- reconnect the parent and siblings
        if (node->parent) {
            if (node->parent->children == node) {
                node->parent->children = first_child;
            }
            if (node->parent->last == node) {
                node->parent->last = last_child;
            }
        }

        if (node->prev) {
            node->prev->next = first_child;
        }
        if (node->next) {
            node->next->prev = last_child;
        }

        // propagate inherited directories to newly exposed children
        xmlChar *dir_attr = xmlGetProp(node, BAD_CAST "_Directory");
        if (dir_attr) {
            for (xmlNodePtr curr_child = first_child; curr_child; curr_child = curr_child->next) {
                if (curr_child->type == XML_ELEMENT_NODE && !xmlHasProp(curr_child, BAD_CAST "_Directory")) {
                    xmlSetProp(curr_child, BAD_CAST "_Directory", dir_attr);
                }
            }
            xmlFree(dir_attr);
        }

        // point every child at its new parent
        for (xmlNodePtr curr_child = first_child; curr_child; curr_child = curr_child->next) {
            curr_child->parent = node->parent;
        }
        first_child->prev = node->prev;
        last_child->next = node->next;

        // detach the empty wrapper
        node->children = NULL;
        node->last = NULL;
        node->parent = NULL;
        node->next = NULL;
        node->prev = NULL;
    } else {
        xmlUnlinkNode(node);
    }

    // release the detached wrapper
    xmlFreeNode(node);
}

static void _preprocess_xml_node(_XmlPreprocessorPassContext *context, xmlNodePtr node, char *directory) {

    // discard unsupported node kinds
    if (node->type != XML_ELEMENT_NODE && node->type != XML_TEXT_NODE && node->type != XML_ATTRIBUTE_NODE) {
        xmlUnlinkNode(node);
        xmlFreeNode(node);
        return;
    }

    // leave text and attributes to their owning element
    if (node->type != XML_ELEMENT_NODE) {
        return;
    }

    // inherit an include's source directory
    char directory_buffer[DC_UTILS_FILEPATH_BUFFER_SIZE];
    xmlChar *dir_attr = xmlGetProp(node, BAD_CAST "_Directory");
    if (dir_attr) {
        strncpy(directory_buffer, (const char *)dir_attr, sizeof(directory_buffer) - 1);
        directory_buffer[sizeof(directory_buffer) - 1] = '\0';
        directory = directory_buffer;
        xmlFree(dir_attr);
    }

    // identify the element before applying defaults
    DcAppXmlElementType elem_type = dc_app_xml_element_type_from_xml_node(node);

    // expand constants in attributes and content
    _dereference_node_attrs_and_content(context, node);

    //- merge explicitly selected style attributes
    {
        xmlChar *style_name = xmlGetProp(node, BAD_CAST "Style");
        if (style_name) {

            _StyleIndex style_index = _get_style_index(context, (char *)style_name);

            if (style_index != _STYLE_INDEX_UNDEFINED) {
                xmlNodePtr style_xml_node = context->sb_styles[style_index].xml_nodes[elem_type];

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
            } else {
                if (!(context->suppress_warnings & _SUPPRESS_MISSING_STYLE)) {
                    DC_LOG_WARN("Config", "_preprocess_xml_node(): style %s is undefined", (char *)style_name);
                }
            }

            xmlFree(style_name);
        }
    }

    //- fill missing default attributes
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

    //- expand alignment shorthand without replacing explicit attributes
    {
        xmlChar *align_x = xmlGetProp(node, BAD_CAST "AlignX");
        if (align_x) {
            if (!xmlHasProp(node, BAD_CAST "LocalAlignX")) {
                xmlSetProp(node, BAD_CAST "LocalAlignX", align_x);
            }
            if (!xmlHasProp(node, BAD_CAST "ParentAlignX")) {
                xmlSetProp(node, BAD_CAST "ParentAlignX", align_x);
            }
            xmlUnsetProp(node, BAD_CAST "AlignX");
            xmlFree(align_x);
        }

        xmlChar *align_y = xmlGetProp(node, BAD_CAST "AlignY");
        if (align_y) {
            if (!xmlHasProp(node, BAD_CAST "LocalAlignY")) {
                xmlSetProp(node, BAD_CAST "LocalAlignY", align_y);
            }
            if (!xmlHasProp(node, BAD_CAST "ParentAlignY")) {
                xmlSetProp(node, BAD_CAST "ParentAlignY", align_y);
            }
            xmlUnsetProp(node, BAD_CAST "AlignY");
            xmlFree(align_y);
        }
    }

    //- process structural elements before descending
    switch (elem_type) {

        case DC_APP_XML_ELEMENT_TYPE_CONSTANT: {
            xmlChar *name = xmlGetProp(node, BAD_CAST "Name");
            if (!name) {
                DC_LOG_ERROR("Config", "_preprocess_xml_node(): 'Name' attribute missing in <Constant> definition");
            }
            char cleaned_name[DC_APP_VALUE_STRING_BUFFER_SIZE];
            strncpy(cleaned_name, (const char *)name, DC_APP_VALUE_STRING_BUFFER_SIZE - 1);
            cleaned_name[DC_APP_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
            xmlFree(name);

            xmlChar *value = xmlNodeGetContent(node);
            char cleaned_value[DC_APP_VALUE_STRING_BUFFER_SIZE];
            if (value) {
                strncpy(cleaned_value, (const char *)value, DC_APP_VALUE_STRING_BUFFER_SIZE - 1);
                cleaned_value[DC_APP_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
                xmlFree(value);
                if (cleaned_value[0] == '\0') {
                    // preserve empty constant values
                }
            } else {
                DC_LOG_ERROR("Config", "_preprocess_xml_node(): Node content missing in <Constant> definition");
                cleaned_value[0] = '\0';
            }

            bool is_immutable = false;
            xmlChar *raw_is_immutable = xmlGetProp(node, BAD_CAST "Immutable");
            if (raw_is_immutable) {
                is_immutable = dc_utils_string_to_boolean((const char *)raw_is_immutable);
                xmlFree(raw_is_immutable);
            }

            _register_const_by_name(context, cleaned_name, cleaned_value, is_immutable);

            // remove the registered constant declaration
            xmlUnlinkNode(node);
            xmlFreeNode(node);
            return;
        }

        //- unwrap dummy elements
        case DC_APP_XML_ELEMENT_TYPE_DUMMY: {
            // preprocess children before exposing them to the parent
            xmlNodePtr child = node->children;
            while (child) {
                xmlNodePtr child_next = child->next;
                _preprocess_xml_node(context, child, directory);
                child = child_next;
            }

            _splice_children_into_parent_and_free_wrapper(node);
            return;
        }

        case DC_APP_XML_ELEMENT_TYPE_INCLUDE: {

            //- resolve the include path
            xmlChar *optional_str = xmlGetProp(node, BAD_CAST "Optional");
            int optional = optional_str ? dc_utils_string_to_boolean((const char *)optional_str) : 0;
            if (optional_str) {
                xmlFree(optional_str);
            }

            // prefer text content for the include filename
            xmlChar *filepath = xmlNodeGetContent(node);
            char cleaned_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
            bool has_filepath = false;
            if (filepath) {
                strncpy(cleaned_filepath, (const char *)filepath, sizeof(cleaned_filepath) - 1);
                cleaned_filepath[sizeof(cleaned_filepath) - 1] = '\0';
                xmlFree(filepath);
                dc_utils_trim_whitespace_inplace(cleaned_filepath);
                has_filepath = (cleaned_filepath[0] != '\0');
            }
            if (!has_filepath) {
                // fall back to the file attribute
                xmlChar *file_attr = xmlGetProp(node, BAD_CAST "File");
                if (file_attr) {
                    strncpy(cleaned_filepath, (const char *)file_attr, sizeof(cleaned_filepath) - 1);
                    cleaned_filepath[sizeof(cleaned_filepath) - 1] = '\0';
                    xmlFree(file_attr);
                    dc_utils_trim_whitespace_inplace(cleaned_filepath);
                    has_filepath = (cleaned_filepath[0] != '\0');
                }
            }
            if (!has_filepath) {
                DC_LOG_ERROR("Config", "_preprocess_xml_node(): File path missing in <Include> definition");
                cleaned_filepath[0] = '\0';
            }

            char absolute_path[DC_UTILS_FILEPATH_BUFFER_SIZE];
            if (dc_utils_is_relative_path(cleaned_filepath)) {
                dc_utils_join_paths(directory, cleaned_filepath, absolute_path, sizeof(absolute_path));
            } else {
                strncpy(absolute_path, cleaned_filepath, DC_UTILS_FILEPATH_BUFFER_SIZE);
                absolute_path[DC_UTILS_FILEPATH_BUFFER_SIZE - 1] = '\0';
            }

            // stop early when the source file is unavailable
            if (!dc_utils_file_exists(absolute_path)) {
                if (optional) {
                    // log and omit optional includes
                    DC_LOG_INFO("Config", "_preprocess_xml_node(): Optional include file '%s' not found, skipping", absolute_path);
                } else {
                    DC_LOG_ERROR("Config", "_preprocess_xml_node(): mandatory include file '%s' not found", absolute_path);
                }
                xmlUnlinkNode(node);
                xmlFreeNode(node);
                return;
            }

            //- load and adopt the included tree
            char canon_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
            dc_utils_canonicalize_path(absolute_path, canon_filepath, sizeof(canon_filepath));

            char include_directory[DC_UTILS_FILEPATH_BUFFER_SIZE];
            dc_utils_get_directory(canon_filepath, include_directory, sizeof(include_directory));

            xmlDocPtr sub_doc = xmlReadFile(canon_filepath, NULL, XML_PARSE_NOBLANKS);
            if (!sub_doc) {
                DC_LOG_ERROR("Config", "_preprocess_xml_node(): Unable to read config file %s", canon_filepath);
            }

            xmlNodePtr sub_node = xmlDocGetRootElement(sub_doc);
            if (!sub_node) {
                DC_LOG_ERROR("Config", "_preprocess_xml_node(): Unable to get root element of config file %s", canon_filepath);
            }

            // copy the included root into the owning document
            xmlNodePtr new_node = xmlDocCopyNode(sub_node, node->doc, 1);

            // preserve the include directory for nested relative paths
            xmlSetProp(new_node, BAD_CAST "_Directory", BAD_CAST include_directory);

            // replace the filename text with the included root
            xmlNodePtr old_child = node->children;
            if (old_child) {
                xmlUnlinkNode(old_child);
                xmlFreeNode(old_child);
            }

            node->children = new_node;
            node->last = new_node;
            new_node->parent = node;
            new_node->next = NULL;
            new_node->prev = NULL;

            xmlFreeDoc(sub_doc);

            // preprocess the adopted subtree before unwrapping it
            _preprocess_xml_node(context, new_node, include_directory);

            _splice_children_into_parent_and_free_wrapper(node);
            return;
        }

        case DC_APP_XML_ELEMENT_TYPE_DEFAULT:
        case DC_APP_XML_ELEMENT_TYPE_STYLE: {

            //- register style templates
            char style_name[DC_APP_VALUE_STRING_BUFFER_SIZE];
            if (elem_type == DC_APP_XML_ELEMENT_TYPE_DEFAULT) {
                strcpy(style_name, "default");
            } else {
                xmlChar *raw_style_name = xmlGetProp(node, BAD_CAST "Name");
                if (!raw_style_name) {
                    DC_LOG_ERROR("Config", "_preprocess_xml_node(): Style name missing in <Style> definition");
                }
                strncpy(style_name, (const char *)raw_style_name, DC_APP_VALUE_STRING_BUFFER_SIZE - 1);
                style_name[DC_APP_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
                xmlFree(raw_style_name);
            }

            // clone each child as a type-specific style template
            xmlNodePtr child = node->children;
            while (child) {

                xmlNodePtr child_next = child->next;

                xmlNodePtr orphan_child = xmlCopyNode(child, 1); // recursive template copy

                _dereference_node_attrs_and_content(context, orphan_child);

                DcAppXmlElementType child_type = dc_app_xml_element_type_from_xml_node(orphan_child);
                _add_style(context, style_name, child_type, orphan_child);

                child = child_next;
            }

            // remove the source declaration after registration
            xmlUnlinkNode(node);
            xmlFreeNode(node);

            return;
        }

        case DC_APP_XML_ELEMENT_TYPE_IF: {
            //- normalize implicit true branches
            xmlNodePtr wrapper = NULL;
            xmlNodePtr child = node->children;
            while (child) {
                xmlNodePtr next = child->next;
                if (child->type == XML_ELEMENT_NODE) {
                    DcAppXmlElementType child_type = dc_app_xml_element_type_from_xml_node(child);
                    if (child_type == DC_APP_XML_ELEMENT_TYPE_TRUE || child_type == DC_APP_XML_ELEMENT_TYPE_FALSE) {
                        wrapper = NULL;
                    } else {
                        if (!wrapper) {
                            wrapper = xmlNewNode(NULL, BAD_CAST "True");
                            xmlAddPrevSibling(child, wrapper);
                        }
                        xmlUnlinkNode(child);
                        xmlAddChild(wrapper, child);
                    }
                }
                child = next;
            }

            // detect compile-time conditions from attributes and values
            bool is_static = false;
            xmlChar *static_attr = xmlGetProp(node, BAD_CAST "Static");
            if (static_attr) {
                is_static = dc_utils_string_to_boolean((const char *)static_attr);
                xmlFree(static_attr);
            } else {
                // infer static evaluation when no runtime variables are referenced
                xmlChar *v1 = xmlGetProp(node, BAD_CAST "Value");
                if (!v1) v1 = xmlGetProp(node, BAD_CAST "Value1");
                xmlChar *v2 = xmlGetProp(node, BAD_CAST "Value2");

                bool has_runtime = false;
                if (v1 && v1[0] == '@') has_runtime = true;
                if (v2 && v2[0] == '@') has_runtime = true;

                bool has_value = (v1 != NULL);
                if (v1) xmlFree(v1);
                if (v2) xmlFree(v2);

                if (!has_runtime && has_value) is_static = true;
            }

            if (is_static) {
                //- evaluate the already expanded static condition
                xmlChar *raw_operation = xmlGetProp(node, BAD_CAST "Operator");
                int cond_type = DC_APP_CONDITIONAL_TYPE_TRUE;
                if (raw_operation) {
                    cond_type = dc_utils_string_to_integer((const char *)raw_operation);
                    xmlFree(raw_operation);
                }

                // read the required first value
                xmlChar *raw_value1 = xmlGetProp(node, BAD_CAST "Value");
                if (!raw_value1) {
                    raw_value1 = xmlGetProp(node, BAD_CAST "Value1");
                }
                if (!raw_value1) {
                    DC_LOG_ERROR("Config", "_preprocess_xml_node(): If Static: no value specified");
                    xmlUnlinkNode(node);
                    xmlFreeNode(node);
                    return;
                }

                // read the optional second value
                xmlChar *raw_value2 = xmlGetProp(node, BAD_CAST "Value2");

                // copy values before releasing xml storage
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

                // reject runtime variables in static conditions
                if (value1[0] == '@') {
                    DC_LOG_ERROR("Config", "_preprocess_xml_node(): If Static: Value1 '%s' uses runtime variable (@), only constants (#) are allowed", value1);
                    xmlUnlinkNode(node);
                    xmlFreeNode(node);
                    return;
                }
                if (value2 && value2[0] == '@') {
                    DC_LOG_ERROR("Config", "_preprocess_xml_node(): If Static: Value2 '%s' uses runtime variable (@), only constants (#) are allowed", value2);
                    xmlUnlinkNode(node);
                    xmlFreeNode(node);
                    return;
                }

                // evaluate according to the inferred value type
                bool result = false;
                if (value2) {
                    // infer comparison type from the first value
                    bool is_double = dc_utils_string_is_double(value1);
                    bool is_bool = !is_double && dc_utils_string_is_boolean(value1);

                    if (is_double) {
                        // compare numeric values as doubles
                        if (!dc_utils_string_is_double(value2)) {
                            DC_LOG_ERROR("Config", "_preprocess_xml_node(): If Static: Value1 is numeric but Value2 '%s' is not", value2);
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
                                DC_LOG_ERROR("Config", "_preprocess_xml_node(): If Static: invalid Operation %d", cond_type);
                                break;
                        }
                    } else if (is_bool) {
                        // compare boolean values
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
                                DC_LOG_ERROR("Config", "_preprocess_xml_node(): If Static: operator %d not supported for boolean values", cond_type);
                                xmlUnlinkNode(node);
                                xmlFreeNode(node);
                                return;
                            default:
                                DC_LOG_ERROR("Config", "_preprocess_xml_node(): If Static: invalid Operation %d for boolean comparison", cond_type);
                                break;
                        }
                    } else {
                        // compare string values
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
                                DC_LOG_ERROR("Config", "_preprocess_xml_node(): If Static: operator %d not supported for string values", cond_type);
                                xmlUnlinkNode(node);
                                xmlFreeNode(node);
                                return;
                            default:
                                DC_LOG_ERROR("Config", "_preprocess_xml_node(): If Static: invalid Operation %d for string comparison", cond_type);
                                break;
                        }
                    }
                } else {
                    // evaluate a lone value as boolean
                    bool bool_val = dc_utils_string_to_boolean(value1);
                    switch (cond_type) {
                        case DC_APP_CONDITIONAL_TYPE_TRUE:
                            result = bool_val;
                            break;
                        case DC_APP_CONDITIONAL_TYPE_FALSE:
                            result = !bool_val;
                            break;
                        default:
                            DC_LOG_ERROR("Config", "_preprocess_xml_node(): If Static: Operation %d requires Value2 attribute", cond_type);
                            break;
                    }
                }

                // discard the unmatched branch without processing it
                DcAppXmlElementType keep_type = result ? DC_APP_XML_ELEMENT_TYPE_TRUE : DC_APP_XML_ELEMENT_TYPE_FALSE;
                child = node->children;
                while (child) {
                    xmlNodePtr next = child->next;
                    if (child->type == XML_ELEMENT_NODE) {
                        DcAppXmlElementType child_type = dc_app_xml_element_type_from_xml_node(child);
                        if ((child_type == DC_APP_XML_ELEMENT_TYPE_TRUE || child_type == DC_APP_XML_ELEMENT_TYPE_FALSE) && child_type != keep_type) {
                            xmlUnlinkNode(child);
                            xmlFreeNode(child);
                        }
                    }
                    child = next;
                }

                // process the surviving branch so its constants are registered
                child = node->children;
                while (child) {
                    xmlNodePtr child_next = child->next;
                    _preprocess_xml_node(context, child, directory);
                    child = child_next;
                }

                // unwrap the surviving branch
                child = node->children;
                while (child) {
                    xmlNodePtr next = child->next;
                    if (child->type == XML_ELEMENT_NODE) {
                        DcAppXmlElementType child_type = dc_app_xml_element_type_from_xml_node(child);
                        if (child_type == DC_APP_XML_ELEMENT_TYPE_TRUE || child_type == DC_APP_XML_ELEMENT_TYPE_FALSE) {
                            _splice_children_into_parent_and_free_wrapper(child);
                        }
                    }
                    child = next;
                }

                // replace the if node with its selected children
                _splice_children_into_parent_and_free_wrapper(node);
                return;
            }

            // leave runtime conditions wrapped for node parsing
            break;
        }

        default:
            break;
    }

    // descend through ordinary element children
    xmlNodePtr child = node->children;
    while (child) {
        xmlNodePtr child_next = child->next;
        _preprocess_xml_node(context, child, directory);
        child = child_next;
    }
}

//~ xml output

// custom writer avoids libxml2's depth limit
static void _write_indent(FILE *f, int depth) {
    for (int i = 0; i < depth; i++) {
        fprintf(f, "  ");
    }
}

static void _write_xml_node(FILE *f, xmlNodePtr node, int depth) {
    if (!node) return;

    switch (node->type) {
        case XML_ELEMENT_NODE: {
            _write_indent(f, depth);
            fprintf(f, "<%s", node->name);

            // write attributes in source order
            xmlAttrPtr attr = node->properties;
            while (attr) {
                xmlChar *value = xmlGetProp(node, attr->name);
                if (value) {
                    fprintf(f, " %s=\"%s\"", attr->name, value);
                    xmlFree(value);
                }
                attr = attr->next;
            }

            if (node->children) {
                // detect text-only elements for inline output
                bool has_element_children = false;
                xmlNodePtr child = node->children;
                while (child) {
                    if (child->type == XML_ELEMENT_NODE) {
                        has_element_children = true;
                        break;
                    }
                    child = child->next;
                }

                if (has_element_children) {
                    fprintf(f, ">\n");
                    child = node->children;
                    while (child) {
                        _write_xml_node(f, child, depth + 1);
                        child = child->next;
                    }
                    _write_indent(f, depth);
                    fprintf(f, "</%s>\n", node->name);
                } else {
                    // keep text-only content inline
                    fprintf(f, ">");
                    child = node->children;
                    while (child) {
                        if (child->type == XML_TEXT_NODE && child->content) {
                            fprintf(f, "%s", child->content);
                        }
                        child = child->next;
                    }
                    fprintf(f, "</%s>\n", node->name);
                }
            } else {
                // emit empty elements as self-closing tags
                fprintf(f, "/>\n");
            }
            break;
        }
        case XML_TEXT_NODE:
            // text nodes are emitted with their parent
            break;
        case XML_COMMENT_NODE:
            _write_indent(f, depth);
            fprintf(f, "<!--%s-->\n", node->content ? (char *)node->content : "");
            break;
        default:
            break;
    }
}

static void _save_to_file(DcAppXmlPreprocessorContext *config, const char *filepath) {
    FILE *f = fopen(filepath, "w");
    if (!f) return;

    // write the xml declaration before the tree
    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

    xmlNodePtr root = xmlDocGetRootElement(config->xml_doc);
    _write_xml_node(f, root, 0);

    fclose(f);
}

//~ constant expansion

static _ConstIndex _get_const_index(_XmlPreprocessorPassContext *context, const char *name) {

    for (int ii = _CONST_FIRST_INDEX; ii < sbcount(context->sb_const_name_offsets); ii++) {
        const char *lookup_name = &(context->sb_const_names[context->sb_const_name_offsets[ii]]);
        if (strcmp(name, lookup_name) == 0) {
            return ii;
        }
    }
    return _CONST_INDEX_UNDEFINED;
}

static void _set_const(_XmlPreprocessorPassContext *context, _ConstIndex index, const char *new_value) {

    // replace the existing stretchy string
    char **addr = &(context->sb_consts[index].val);
    sbclear(*addr);
    sbpushn(*addr, new_value, (int)(strlen(new_value) + 1));
}

static void _add_const(_XmlPreprocessorPassContext *context, const char *name, const char *value, bool is_immutable) {

    // store the lookup name in stable offset form
    sbpush(context->sb_const_name_offsets, sbcount(context->sb_const_names));
    sbpushn(context->sb_const_names, name, (int)(strlen(name) + 1));

    // copy the value into independently owned storage
    _Constant constant;
    constant.val = NULL;
    sbpushn(constant.val, value, (int)(strlen(value) + 1));
    constant.is_immutable = is_immutable;

    sbpush(context->sb_consts, constant);
}

static void _add_const_int(_XmlPreprocessorPassContext *context, const char *name, int value_int, bool is_immutable) {
    char value_str[20];
    snprintf(value_str, 20, "%d", value_int);
    _add_const(context, name, value_str, is_immutable);
}

static void _register_const_by_name(_XmlPreprocessorPassContext *context, const char *name, const char *new_value, bool is_immutable) {
    _ConstIndex const_index = _get_const_index(context, name);
    if (const_index == _CONST_INDEX_UNDEFINED) {
        _add_const(context, name, new_value, is_immutable);
    } else {
        if (context->sb_consts[const_index].is_immutable) {
            DC_LOG_WARN("Config", "_register_const_by_name(): ignoring constant registration for '%s': immutable", name);
        } else {
            _set_const(context, const_index, new_value);
        }
    }
}

static const char *_get_const_by_name(_XmlPreprocessorPassContext *context, const char *name) {
    _ConstIndex const_index = _get_const_index(context, name);
    if (const_index == _CONST_INDEX_UNDEFINED) {
        if (context->suppress_warnings & _SUPPRESS_MISSING_CONSTANT) {
            return ""; // expand suppressed missing constants to nothing
        }
        DC_LOG_WARN("Config", "_get_const_by_name(): constant '%s' does not exist", name);
        return NULL;
    } else {
        return context->sb_consts[const_index].val;
    }
}

static void _dereference_constants(_XmlPreprocessorPassContext *context, const char *in, char *out, size_t out_size) {

    // fast path strings without references
    if (dc_utils_str_find_first_of(in, "#$") == -1) {
        strncpy(out, in, out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    // scan and expand references into the caller buffer
    size_t in_length = strlen(in);
    int out_index = 0;
    for (int in_index = 0; in_index < in_length && out_index < out_size - 1; in_index++) {

        // preserve escaped reference markers
        if (in[in_index] == '\\' && in_index + 1 < in_length && (in[in_index + 1] == '#' || in[in_index + 1] == '$')) {
            out[out_index++] = in[++in_index];
            continue;
        }

        // expand constant and environment references recursively
        if (in[in_index] == '#' || in[in_index] == '$') {
            // reject a trailing reference marker
            if (in_index + 1 >= in_length) {
                DC_LOG_ERROR("Config", "_dereference_constants(): Cannot have string ending on an unescaped #/$: '%s'", in);
                return;
            }

            // find the end of braced or bare references
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
                    DC_LOG_ERROR("Config", "_dereference_constants(): mismatch with squiggly braces: '%s'", in);
                    return;
                }

                subtext_start_index = in_index + 2;
                subtext_length = subtext_end_index - subtext_start_index;
                subtext_length_with_symbols = subtext_length + 2;
            } else {
                static const char *valid_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#$";
                int subtext_end_index = dc_utils_str_find_first_not_of(&(in[in_index]), valid_chars);
                if (subtext_end_index == -1) {
                    subtext_end_index = (int)in_length;
                } else {
                    subtext_end_index += in_index; // convert from relative to absolute position
                }

                subtext_start_index = in_index + 1;
                subtext_length = subtext_end_index - subtext_start_index;
                subtext_length_with_symbols = subtext_length;
            }

            // reject nested runtime variables inside references
            char subtext[DC_APP_VALUE_STRING_BUFFER_SIZE];
            strncpy(subtext, &in[subtext_start_index], subtext_length);
            subtext[subtext_length] = '\0';
            if (dc_utils_str_find_first(subtext, '@') != -1) {
                DC_LOG_ERROR("Config", "_dereference_constants(): cannot have variable nested inside variable/constant expansion: '%s'", in);
                return;
            }

            // recursively expand the reference name
            char subtext_cleaned[DC_APP_VALUE_STRING_BUFFER_SIZE];
            _dereference_constants(context, subtext, subtext_cleaned, sizeof(subtext_cleaned));

            // resolve constants from the registry and variables from the environment
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

            // advance past the full source reference
            in_index += (int)subtext_length_with_symbols;
        } else {
            out[out_index++] = in[in_index];
        }
    }

    if (out_index < out_size) {
        out[out_index] = '\0';
    }
}

//~ style resolution

static _StyleIndex _get_style_index(_XmlPreprocessorPassContext *context, const char *name) {
    if (name) {
        for (int ii = _STYLE_INDEX_DEFAULT; ii < sbcount(context->sb_styles); ii++) {
            const char *comp_name = &(context->sb_style_names[context->sb_style_name_offsets[ii]]);
            if (strcmp(name, comp_name) == 0) {
                return ii;
            }
        }
    }
    return _STYLE_INDEX_UNDEFINED;
}

static void _add_style(_XmlPreprocessorPassContext *context, const char *name, DcAppXmlElementType elem_type, xmlNodePtr xml_node) {
    if (name) {

        _StyleIndex style_index = _get_style_index(context, name);

        // create the named style on first use
        if (style_index == _STYLE_INDEX_UNDEFINED) {
            sbpush(context->sb_style_name_offsets, sbcount(context->sb_style_names));
            sbpushn(context->sb_style_names, name, (int)(strlen(name) + 1));
            _ElemStyle style = {};
            sbpush(context->sb_styles, style);
            style_index = sbcount(context->sb_styles) - 1;
        }

        // replace the template for this element type
        _ElemStyle *style = &(context->sb_styles[style_index]);
        if (style->xml_nodes[elem_type] != NULL) {
            if (!(context->suppress_warnings & _SUPPRESS_STYLE_OVERRIDE)) {
                DC_LOG_WARN("Config", "_add_style(): style '%s' already contains an entry for element '%s'; overwriting", name, dc_app_xml_element_type_to_string(elem_type));
            }
            xmlFree(style->xml_nodes[elem_type]);
        }
        style->xml_nodes[elem_type] = xml_node;
    } else {
        if (!(context->suppress_warnings & _SUPPRESS_MISSING_STYLE)) {
            DC_LOG_WARN("Config", "_set_style(): name %s is undefined", name);
        }
    }
}

static xmlChar *_get_style_attr(_XmlPreprocessorPassContext *context, int style_index, DcAppXmlElementType elem_type, const char *name) {
    xmlNodePtr style_xml_node = context->sb_styles[style_index].xml_nodes[elem_type];
    xmlChar *value = xmlGetProp(style_xml_node, BAD_CAST name);
    if (value) {
        return value;
    }
    return NULL;
}

static xmlChar *_get_style_content(_XmlPreprocessorPassContext *context, int style_index, DcAppXmlElementType elem_type) {
    xmlNodePtr style_xml_node = context->sb_styles[style_index].xml_nodes[elem_type];
    xmlChar *value = xmlNodeGetContent(style_xml_node);
    if (value) {
        return value;
    }
    return NULL;
}

//~ node expansion

static void _dereference_node_attrs_and_content(_XmlPreprocessorPassContext *context, xmlNodePtr node) {

    // expand every attribute value
    xmlAttrPtr attr = node->properties;
    while (attr) {
        xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
        if (value) {
            char cleaned_value[DC_APP_VALUE_STRING_BUFFER_SIZE];
            _dereference_constants(context, (char *)value, cleaned_value, sizeof(cleaned_value));
            xmlSetProp(node, attr->name, BAD_CAST cleaned_value);
            xmlFree(value);
        }
        attr = attr->next;
    }

    // expand direct text children
    xmlNodePtr child = node->children;
    while (child) {
        if (child->type == XML_TEXT_NODE) {
            xmlChar *value = xmlNodeGetContent(child);
            if (value) {
                char cleaned_value[DC_APP_VALUE_STRING_BUFFER_SIZE];
                _dereference_constants(context, (char *)value, cleaned_value, sizeof(cleaned_value));
                xmlNodeSetContent(child, BAD_CAST cleaned_value);
                xmlFree(value);
            }
        }
        child = child->next;
    }
}

//~ argument parsing

// caller owns the returned string
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
