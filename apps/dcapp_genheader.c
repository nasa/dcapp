#include "../src/app/config.h"
#include "../src/app/elem.h"
#include "../src/app/enums.h"
#include "../src/app/lookup.h"
#include "../src/utils/env.h"
#include "../src/utils/file.h"
#include "../src/utils/log.h"
#include "../src/utils/string.h"

#include <libxml/parser.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void _process_node_children(xmlNodePtr xml_node, DcAppLookup *lookup);
static void _process_node(xmlNodePtr xml_node, DcAppLookup *lookup);
static void _write_draw_api(FILE *file);

int main(int argc, char **argv) {

    if (argc < 2) {
        DC_LOG_ERROR("GenHeader", "Usage: dcapp-genheader <config.xml> [--preprocessed <output.xml>] [CONSTANT=value ...]");
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

    // create lookup
    DcAppLookup *lookup = dc_app_lookup_create();

    // set environment (used for dcapp XMLs)
    dc_utils_set_env("dcappDisplayHome", config->config_dir_path, 1);
    dc_utils_set_env("dcappHome", config->dcapp_dir_path, 1);

    // preprocess XML file
    dc_app_config_preprocess_xml(config, lookup);

    // dump preprocessed XML for debugging
    dc_app_config_save_preprocessed(config, preprocessed_output);

    // process XML
    xmlNodePtr root_node = xmlDocGetRootElement(config->xml_doc);
    _process_node(root_node, lookup);

    // create directory
    char logic_dir[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_join_paths(config->config_dir_path, "logic", logic_dir, sizeof(logic_dir));
    dc_utils_create_directory(logic_dir);

    // open/create file
    char logic_filepath[DC_UTILS_FILEPATH_BUFFER_SIZE];
    dc_utils_join_paths(logic_dir, "dcapp.h", logic_filepath, sizeof(logic_filepath));
    FILE *file = fopen(logic_filepath, "w");
    if (!file) {
        perror("DCAPP main(): Unable to open file");
        return 1;
    }

    int var_count           = dc_app_lookup_get_var_count(lookup);

    // file header
    fprintf(file, "%s\n", "// ********************************************* //");
    fprintf(file, "%s\n", "// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //");
    fprintf(file, "%s\n", "// ********************************************* //");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "#include <stdbool.h>");
    fprintf(file, "%s\n", "#include <stddef.h>");
    fprintf(file, "%s\n", "#include <stdint.h>");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "#ifndef DCAPP_H");
    fprintf(file, "%s\n", "#define DCAPP_H");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "#ifndef DCAPP_DEPRECATED");
    fprintf(file, "%s\n", "#if defined(__GNUC__) || defined(__clang__)");
    fprintf(file, "%s\n", "#define DCAPP_DEPRECATED(message) __attribute__((deprecated(message)))");
    fprintf(file, "%s\n", "#elif defined(_MSC_VER)");
    fprintf(file, "%s\n", "#define DCAPP_DEPRECATED(message) __declspec(deprecated(message))");
    fprintf(file, "%s\n", "#else");
    fprintf(file, "%s\n", "#define DCAPP_DEPRECATED(message)");
    fprintf(file, "%s\n", "#endif");
    fprintf(file, "%s\n", "#endif");
    fprintf(file, "%s\n", "");

    _write_draw_api(file);
    fprintf(file, "%s\n", "#ifndef _DCAPP_LOGIC_EXTERN_");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "#ifdef __cplusplus");
    fprintf(file, "%s\n", "extern \"C\" {");
    fprintf(file, "%s\n", "#endif");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// api tables are filled by dcapp before display_init().");
    fprintf(file, "%s\n", "DcAppContext *dc_app_ctx;");
    fprintf(file, "%s\n", "const DcAppApi *dc_app;");
    fprintf(file, "%s\n", "const DcDrawApi *dc_draw;");
    fprintf(file, "%s\n", "const DcMouseApi *dc_mouse;");
    fprintf(file, "%s\n", "const DcTextureApi *dc_texture;");
    fprintf(file, "%s\n", "const DcPlanetApi *dc_planet;");
    fprintf(file, "%s\n", "");

    // file variable definitions
    if (var_count > DC_APP_LOOKUP_FIRST_INDEX) {
        fprintf(file, "%s\n", "// XML variable pointers resolved during display_pre_init().");
    }
    for (DcAppVarIndex var_index = DC_APP_LOOKUP_FIRST_INDEX; var_index < var_count; var_index++) {
        DcAppLookupVar *var      = dc_app_lookup_get_var(lookup, var_index);
        const char     *var_name = dc_app_lookup_get_var_name(lookup, var_index);
        DcValue        *value    = dc_app_lookup_get_value(lookup, var->value_index);
        switch (value->type) {
            case DC_VALUE_TYPE_STRING:
                fprintf(file, "char   (*%s)[%d];\n", var_name, DC_VALUE_STRING_BUFFER_SIZE);
                break;
            case DC_VALUE_TYPE_DOUBLE:
                fprintf(file, "double *%s;\n", var_name);
                break;
            case DC_VALUE_TYPE_INTEGER:
                fprintf(file, "int    *%s;\n", var_name);
                break;
            case DC_VALUE_TYPE_BOOLEAN:
                fprintf(file, "bool   *%s;\n", var_name);
                break;
            default:
                DC_LOG_ERROR("GenHeader", "Invalid variable value type %d", value->type);
                break;
        }
    }
    fprintf(file, "%s\n", "");

    // file function declarations
    fprintf(file, "%s\n", "// lifecycle callbacks get app_ctx for app-owned resources and optional user_data.");
    fprintf(file, "%s\n", "void display_init(DcAppContext *app_ctx, void **user_data);");
    fprintf(file, "%s\n", "void display_draw(DcAppContext *app_ctx, void *user_data);");
    fprintf(file, "%s\n", "void display_close(DcAppContext *app_ctx, void *user_data);");
    fprintf(file, "%s\n", "");

    // define dc_get_variable() and display_pre_init()
    fprintf(file, "%s\n", "// Legacy lookup helper for variables declared in XML.");
    fprintf(file, "%s\n", "// Deprecated: use generated variable pointers instead.");
    fprintf(file, "%s\n", "DCAPP_DEPRECATED(\"use generated variable pointers instead of dc_get_variable()\")");
    fprintf(file, "%s\n", "void *dc_get_variable(const char *name) {");
    fprintf(file, "%s\n", "    if (!dc_app || !dc_app->get_variable) return NULL;");
    fprintf(file, "%s\n", "    return dc_app->get_variable(dc_app_ctx, name);");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// Internal setup called before display_init().");
    fprintf(file, "%s\n", "void display_pre_init(const DcInit *init) {");
    fprintf(file, "%s\n", "    if (init && init->version >= 1 && init->size >= sizeof(DcInit)) {");
    fprintf(file, "%s\n", "        dc_app_ctx = init->app_ctx;");
    fprintf(file, "%s\n", "        dc_app = init->app;");
    fprintf(file, "%s\n", "        dc_draw = init->draw;");
    fprintf(file, "%s\n", "        dc_mouse = init->mouse;");
    fprintf(file, "%s\n", "        dc_texture = init->texture;");
    fprintf(file, "%s\n", "        dc_planet = init->planet;");
    for (DcAppVarIndex var_index = DC_APP_LOOKUP_FIRST_INDEX; var_index < var_count; var_index++) {
        DcAppLookupVar *var      = dc_app_lookup_get_var(lookup, var_index);
        const char     *var_name = dc_app_lookup_get_var_name(lookup, var_index);
        DcValue        *value    = dc_app_lookup_get_value(lookup, var->value_index);
        switch (value->type) {
            case DC_VALUE_TYPE_STRING:
                fprintf(file, "        %s = (char (*)[%d])(dc_app && dc_app->get_variable ? dc_app->get_variable(dc_app_ctx, \"%s\") : NULL);\n", var_name, DC_VALUE_STRING_BUFFER_SIZE, var_name);
                break;
            case DC_VALUE_TYPE_DOUBLE:
                fprintf(file, "        %s = (double *)(dc_app && dc_app->get_variable ? dc_app->get_variable(dc_app_ctx, \"%s\") : NULL);\n", var_name, var_name);
                break;
            case DC_VALUE_TYPE_INTEGER:
                fprintf(file, "        %s = (int *)(dc_app && dc_app->get_variable ? dc_app->get_variable(dc_app_ctx, \"%s\") : NULL);\n", var_name, var_name);
                break;
            case DC_VALUE_TYPE_BOOLEAN:
                fprintf(file, "        %s = (bool *)(dc_app && dc_app->get_variable ? dc_app->get_variable(dc_app_ctx, \"%s\") : NULL);\n", var_name, var_name);
                break;
            default:
                DC_LOG_ERROR("GenHeader", "Invalid variable value type %d", value->type);
                break;
        }
    }
    fprintf(file, "%s\n", "    }");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");

    // C/C++ compatibility guard closer
    fprintf(file, "%s\n", "#ifdef __cplusplus");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "#endif");
    fprintf(file, "%s\n", "");

    // extern definitions for logic externs
    fprintf(file, "%s\n", "#else");
    fprintf(file, "%s\n", "");

    // lookup function for externs
    fprintf(file, "%s\n", "#ifdef __cplusplus");
    fprintf(file, "%s\n", "extern \"C\" {");
    fprintf(file, "%s\n", "#endif");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "DCAPP_DEPRECATED(\"use generated variable pointers instead of dc_get_variable()\")");
    fprintf(file, "%s\n", "void *dc_get_variable(const char *name);");
    fprintf(file, "%s\n", "extern DcAppContext *dc_app_ctx;");
    fprintf(file, "%s\n", "extern const DcAppApi *dc_app;");
    fprintf(file, "%s\n", "extern const DcDrawApi *dc_draw;");
    fprintf(file, "%s\n", "extern const DcMouseApi *dc_mouse;");
    fprintf(file, "%s\n", "extern const DcTextureApi *dc_texture;");
    fprintf(file, "%s\n", "extern const DcPlanetApi *dc_planet;");
    fprintf(file, "%s\n", "");

    for (DcAppVarIndex var_index = DC_APP_LOOKUP_FIRST_INDEX; var_index < var_count; var_index++) {
        if (var_index == DC_APP_LOOKUP_FIRST_INDEX) {
            fprintf(file, "%s\n", "// XML variable pointers shared from the main logic translation unit.");
        }
        DcAppLookupVar *var      = dc_app_lookup_get_var(lookup, var_index);
        const char     *var_name = dc_app_lookup_get_var_name(lookup, var_index);
        DcValue        *value    = dc_app_lookup_get_value(lookup, var->value_index);
        switch (value->type) {
            case DC_VALUE_TYPE_STRING:
                fprintf(file, "extern char   (*%s)[%d];\n", var_name, DC_VALUE_STRING_BUFFER_SIZE);
                break;
            case DC_VALUE_TYPE_DOUBLE:
                fprintf(file, "extern double *%s;\n", var_name);
                break;
            case DC_VALUE_TYPE_INTEGER:
                fprintf(file, "extern int    *%s;\n", var_name);
                break;
            case DC_VALUE_TYPE_BOOLEAN:
                fprintf(file, "extern bool   *%s;\n", var_name);
                break;
            default:
                DC_LOG_ERROR("GenHeader", "Invalid variable value type %d", value->type);
                break;
        }
    }
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "#ifdef __cplusplus");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "#endif");
    fprintf(file, "%s\n", "");

    // file closer
    fprintf(file, "%s\n", "#endif");
    fprintf(file, "%s\n", "#endif");

    // exit
    fclose(file);
    return 0;
}

static void _write_draw_api(FILE *file) {
    fprintf(file, "%s\n", "// Alignment values used by DcPlacement.");
    fprintf(file, "%s\n", "typedef enum _DcAlign {");
    fprintf(file, "    DC_ALIGN_UNDEFINED = %d,\n", DC_APP_ALIGN_TYPE_UNDEFINED);
    fprintf(file, "    DC_ALIGN_LEFT = %d,\n", DC_APP_ALIGN_TYPE_LEFT);
    fprintf(file, "    DC_ALIGN_CENTER = %d,\n", DC_APP_ALIGN_TYPE_CENTER);
    fprintf(file, "    DC_ALIGN_RIGHT = %d,\n", DC_APP_ALIGN_TYPE_RIGHT);
    fprintf(file, "    DC_ALIGN_BOTTOM = %d,\n", DC_APP_ALIGN_TYPE_BOTTOM);
    fprintf(file, "    DC_ALIGN_MIDDLE = %d,\n", DC_APP_ALIGN_TYPE_MIDDLE);
    fprintf(file, "    DC_ALIGN_TOP = %d,\n", DC_APP_ALIGN_TYPE_TOP);
    fprintf(file, "%s\n", "} DcAlign;");
    fprintf(file, "%s\n", "");

    fprintf(file, "%s\n", "// Type values used by XML <Arg> entries.");
    fprintf(file, "%s\n", "typedef enum _DcValueType {");
    fprintf(file, "    DC_VALUE_TYPE_UNDEFINED = %d,\n", DC_VALUE_TYPE_UNDEFINED);
    fprintf(file, "    DC_VALUE_TYPE_STRING = %d,\n", DC_VALUE_TYPE_STRING);
    fprintf(file, "    DC_VALUE_TYPE_INTEGER = %d,\n", DC_VALUE_TYPE_INTEGER);
    fprintf(file, "    DC_VALUE_TYPE_DOUBLE = %d,\n", DC_VALUE_TYPE_DOUBLE);
    fprintf(file, "    DC_VALUE_TYPE_BOOLEAN = %d,\n", DC_VALUE_TYPE_BOOLEAN);
    fprintf(file, "%s\n", "} DcValueType;");
    fprintf(file, "%s\n", "");

    fprintf(file, "%s\n", "// Coordinate reference systems for planet positions.");
    fprintf(file, "%s\n", "typedef enum _DcPlanetCrs {");
    fprintf(file, "    DC_PLANET_CRS_UNDEFINED = %d,\n", DC_APP_PLANET_CRS_UNDEFINED);
    fprintf(file, "    DC_PLANET_CRS_GEODETIC = %d,\n", DC_APP_PLANET_CRS_GEODETIC);
    fprintf(file, "    DC_PLANET_CRS_CARTESIAN = %d,\n", DC_APP_PLANET_CRS_CARTESIAN);
    fprintf(file, "%s\n", "} DcPlanetCrs;");
    fprintf(file, "%s\n", "");

    fprintf(file, "%s\n", "// Basic vector types used by the draw API.");
    fprintf(file, "%s\n", "typedef union _DcVec2 {");
    fprintf(file, "%s\n", "    struct { float x, y; };");
    fprintf(file, "%s\n", "    struct { float r, g; };");
    fprintf(file, "%s\n", "    struct { float u, v; };");
    fprintf(file, "%s\n", "    float d[2];");
    fprintf(file, "%s\n", "} DcVec2;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "typedef union _DcVec3 {");
    fprintf(file, "%s\n", "    struct { float x, y, z; };");
    fprintf(file, "%s\n", "    struct { float r, g, b; };");
    fprintf(file, "%s\n", "    struct { float u, v, __; };");
    fprintf(file, "%s\n", "    struct { float roll, pitch, yaw; };");
    fprintf(file, "%s\n", "    struct { DcVec2 xy; float ignore0_; };");
    fprintf(file, "%s\n", "    struct { DcVec2 rg; float ignore1_; };");
    fprintf(file, "%s\n", "    struct { DcVec2 uv; float ignore2_; };");
    fprintf(file, "%s\n", "    struct { float ignore3_; DcVec2 yz; };");
    fprintf(file, "%s\n", "    struct { float ignore4_; DcVec2 gb; };");
    fprintf(file, "%s\n", "    struct { float ignore5_; DcVec2 v__; };");
    fprintf(file, "%s\n", "    float d[3];");
    fprintf(file, "%s\n", "} DcVec3;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "typedef union _DcVec4 {");
    fprintf(file, "%s\n", "    struct {");
    fprintf(file, "%s\n", "        union {");
    fprintf(file, "%s\n", "            DcVec3 xyz;");
    fprintf(file, "%s\n", "            struct { float x, y, z; };");
    fprintf(file, "%s\n", "        };");
    fprintf(file, "%s\n", "        float w;");
    fprintf(file, "%s\n", "    };");
    fprintf(file, "%s\n", "    struct {");
    fprintf(file, "%s\n", "        union {");
    fprintf(file, "%s\n", "            DcVec3 rgb;");
    fprintf(file, "%s\n", "            struct { float r, g, b; };");
    fprintf(file, "%s\n", "        };");
    fprintf(file, "%s\n", "        float a;");
    fprintf(file, "%s\n", "    };");
    fprintf(file, "%s\n", "    struct {");
    fprintf(file, "%s\n", "        DcVec2 xy;");
    fprintf(file, "%s\n", "        float ignored0_, ignored1_;");
    fprintf(file, "%s\n", "    };");
    fprintf(file, "%s\n", "    struct {");
    fprintf(file, "%s\n", "        float ignored2_;");
    fprintf(file, "%s\n", "        DcVec2 yz;");
    fprintf(file, "%s\n", "        float ignored3_;");
    fprintf(file, "%s\n", "    };");
    fprintf(file, "%s\n", "    struct {");
    fprintf(file, "%s\n", "        float ignored4_, ignored5_;");
    fprintf(file, "%s\n", "        DcVec2 zw;");
    fprintf(file, "%s\n", "    };");
    fprintf(file, "%s\n", "    float d[4];");
    fprintf(file, "%s\n", "} DcVec4;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// Logic-side equivalent of XML #_stencil_color_.");
    fprintf(file, "%s\n", "static inline DcVec4 dc_stencil_color(void) {");
    fprintf(file, "%s\n", "    DcVec4 color = {0};");
    fprintf(file, "%s\n", "    color.a = 1.0f;");
    fprintf(file, "%s\n", "    return color;");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// Stroke settings for outline-style drawing.");
    fprintf(file, "%s\n", "typedef struct _DcStroke {");
    fprintf(file, "%s\n", "    DcVec4 color;");
    fprintf(file, "%s\n", "    float width;");
    fprintf(file, "%s\n", "    uint8_t pattern;");
    fprintf(file, "%s\n", "} DcStroke;");
    fprintf(file, "%s\n", "");

    fprintf(file, "%s\n", "// Text settings for DrawFunction text rendering.");
    fprintf(file, "%s\n", "typedef struct _DcTextStyle {");
    fprintf(file, "%s\n", "    DcVec4 color;");
    fprintf(file, "%s\n", "    float size;");
    fprintf(file, "%s\n", "    float wrap;");
    fprintf(file, "%s\n", "} DcTextStyle;");
    fprintf(file, "%s\n", "");

    fprintf(file, "%s\n", "// Placement controls alignment, rotation, and pivot behavior.");
    fprintf(file, "%s\n", "typedef struct _DcPlacement {");
    fprintf(file, "%s\n", "    float rotation;");
    fprintf(file, "%s\n", "    DcAlign parent_align_x;");
    fprintf(file, "%s\n", "    DcAlign parent_align_y;");
    fprintf(file, "%s\n", "    DcAlign local_align_x;");
    fprintf(file, "%s\n", "    DcAlign local_align_y;");
    fprintf(file, "%s\n", "    DcAlign pivot_align_x;");
    fprintf(file, "%s\n", "    DcAlign pivot_align_y;");
    fprintf(file, "%s\n", "    float pivot_x;");
    fprintf(file, "%s\n", "    float pivot_y;");
    fprintf(file, "%s\n", "} DcPlacement;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// Convenience constructors for common placement patterns.");
    fprintf(file, "%s\n", "static inline DcPlacement dc_place_center(void) {");
    fprintf(file, "%s\n", "    DcPlacement placement = {0};");
    fprintf(file, "%s\n", "    placement.parent_align_x = DC_ALIGN_CENTER;");
    fprintf(file, "%s\n", "    placement.parent_align_y = DC_ALIGN_MIDDLE;");
    fprintf(file, "%s\n", "    placement.local_align_x  = DC_ALIGN_CENTER;");
    fprintf(file, "%s\n", "    placement.local_align_y  = DC_ALIGN_MIDDLE;");
    fprintf(file, "%s\n", "    return placement;");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "static inline DcPlacement dc_place_left(void) {");
    fprintf(file, "%s\n", "    DcPlacement placement = {0};");
    fprintf(file, "%s\n", "    placement.parent_align_x = DC_ALIGN_LEFT;");
    fprintf(file, "%s\n", "    placement.parent_align_y = DC_ALIGN_MIDDLE;");
    fprintf(file, "%s\n", "    placement.local_align_x  = DC_ALIGN_LEFT;");
    fprintf(file, "%s\n", "    placement.local_align_y  = DC_ALIGN_MIDDLE;");
    fprintf(file, "%s\n", "    return placement;");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "static inline DcPlacement dc_place_right(void) {");
    fprintf(file, "%s\n", "    DcPlacement placement = {0};");
    fprintf(file, "%s\n", "    placement.parent_align_x = DC_ALIGN_RIGHT;");
    fprintf(file, "%s\n", "    placement.parent_align_y = DC_ALIGN_MIDDLE;");
    fprintf(file, "%s\n", "    placement.local_align_x  = DC_ALIGN_RIGHT;");
    fprintf(file, "%s\n", "    placement.local_align_y  = DC_ALIGN_MIDDLE;");
    fprintf(file, "%s\n", "    return placement;");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "static inline DcPlacement dc_place_top(void) {");
    fprintf(file, "%s\n", "    DcPlacement placement = {0};");
    fprintf(file, "%s\n", "    placement.parent_align_x = DC_ALIGN_CENTER;");
    fprintf(file, "%s\n", "    placement.parent_align_y = DC_ALIGN_TOP;");
    fprintf(file, "%s\n", "    placement.local_align_x  = DC_ALIGN_CENTER;");
    fprintf(file, "%s\n", "    placement.local_align_y  = DC_ALIGN_TOP;");
    fprintf(file, "%s\n", "    return placement;");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "static inline DcPlacement dc_place_bottom(void) {");
    fprintf(file, "%s\n", "    DcPlacement placement = {0};");
    fprintf(file, "%s\n", "    placement.parent_align_x = DC_ALIGN_CENTER;");
    fprintf(file, "%s\n", "    placement.parent_align_y = DC_ALIGN_BOTTOM;");
    fprintf(file, "%s\n", "    placement.local_align_x  = DC_ALIGN_CENTER;");
    fprintf(file, "%s\n", "    placement.local_align_y  = DC_ALIGN_BOTTOM;");
    fprintf(file, "%s\n", "    return placement;");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "static inline DcPlacement dc_place_top_left(void) {");
    fprintf(file, "%s\n", "    DcPlacement placement = {0};");
    fprintf(file, "%s\n", "    placement.parent_align_x = DC_ALIGN_LEFT;");
    fprintf(file, "%s\n", "    placement.parent_align_y = DC_ALIGN_TOP;");
    fprintf(file, "%s\n", "    placement.local_align_x  = DC_ALIGN_LEFT;");
    fprintf(file, "%s\n", "    placement.local_align_y  = DC_ALIGN_TOP;");
    fprintf(file, "%s\n", "    return placement;");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "static inline DcPlacement dc_place_top_right(void) {");
    fprintf(file, "%s\n", "    DcPlacement placement = {0};");
    fprintf(file, "%s\n", "    placement.parent_align_x = DC_ALIGN_RIGHT;");
    fprintf(file, "%s\n", "    placement.parent_align_y = DC_ALIGN_TOP;");
    fprintf(file, "%s\n", "    placement.local_align_x  = DC_ALIGN_RIGHT;");
    fprintf(file, "%s\n", "    placement.local_align_y  = DC_ALIGN_TOP;");
    fprintf(file, "%s\n", "    return placement;");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "static inline DcPlacement dc_place_bottom_left(void) {");
    fprintf(file, "%s\n", "    DcPlacement placement = {0};");
    fprintf(file, "%s\n", "    placement.parent_align_x = DC_ALIGN_LEFT;");
    fprintf(file, "%s\n", "    placement.parent_align_y = DC_ALIGN_BOTTOM;");
    fprintf(file, "%s\n", "    placement.local_align_x  = DC_ALIGN_LEFT;");
    fprintf(file, "%s\n", "    placement.local_align_y  = DC_ALIGN_BOTTOM;");
    fprintf(file, "%s\n", "    return placement;");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "static inline DcPlacement dc_place_bottom_right(void) {");
    fprintf(file, "%s\n", "    DcPlacement placement = {0};");
    fprintf(file, "%s\n", "    placement.parent_align_x = DC_ALIGN_RIGHT;");
    fprintf(file, "%s\n", "    placement.parent_align_y = DC_ALIGN_BOTTOM;");
    fprintf(file, "%s\n", "    placement.local_align_x  = DC_ALIGN_RIGHT;");
    fprintf(file, "%s\n", "    placement.local_align_y  = DC_ALIGN_BOTTOM;");
    fprintf(file, "%s\n", "    return placement;");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "");

    fprintf(file, "%s\n", "// Mouse state in the current DrawFunction coordinate system.");
    fprintf(file, "%s\n", "typedef struct _DcMouse {");
    fprintf(file, "%s\n", "    float x;");
    fprintf(file, "%s\n", "    float y;");
    fprintf(file, "%s\n", "    bool position_valid;");
    fprintf(file, "%s\n", "    bool pressed;");
    fprintf(file, "%s\n", "    bool released;");
    fprintf(file, "%s\n", "    bool down;");
    fprintf(file, "%s\n", "} DcMouse;");
    fprintf(file, "%s\n", "");

    fprintf(file, "%s\n", "// A resolved drawing area that can be reused as another shape's parent.");
    fprintf(file, "%s\n", "typedef struct _DcDrawArea {");
    fprintf(file, "%s\n", "    float position[2];");
    fprintf(file, "%s\n", "    float dimensions[2];");
    fprintf(file, "%s\n", "    float transform[16];");
    fprintf(file, "%s\n", "} DcDrawArea;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// Extended draw output. More fields can be added without changing every draw call.");
    fprintf(file, "%s\n", "typedef struct _DcDrawResult {");
    fprintf(file, "%s\n", "    DcDrawArea area;");
    fprintf(file, "%s\n", "} DcDrawResult;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// app_ctx is passed to lifecycle callbacks and app-owned APIs.");
    fprintf(file, "%s\n", "typedef struct _DcAppContext DcAppContext;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// draw context passed only to DrawFunction callbacks.");
    fprintf(file, "%s\n", "typedef struct _DcDrawContext DcDrawContext;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// Texture handle returned by dc_texture->load_image() and consumed by dc_draw->image().");
    fprintf(file, "%s\n", "typedef uint32_t DcTextureId;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// opaque planet handles.");
    fprintf(file, "%s\n", "typedef struct DcPlanet *DcPlanetHandle;");
    fprintf(file, "%s\n", "typedef struct DcPlanetView *DcPlanetViewHandle;");
    fprintf(file, "%s\n", "typedef struct DcDrawPlanetView *DcDrawPlanetViewHandle;");
    fprintf(file, "%s\n", "typedef struct DcPlanetBreadcrumbs *DcPlanetBreadcrumbsHandle;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "typedef struct _DcPlanetCreateInfo {");
    fprintf(file, "%s\n", "    const char *data_path;");
    fprintf(file, "%s\n", "    uint32_t mesh_cache_size; // bytes, 0 uses renderer default.");
    fprintf(file, "%s\n", "} DcPlanetCreateInfo;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "typedef struct _DcPlanetBreadcrumbsPoints {");
    fprintf(file, "%s\n", "    const DcVec3 *points;");
    fprintf(file, "%s\n", "    uint32_t count;");
    fprintf(file, "%s\n", "    DcPlanetCrs crs;");
    fprintf(file, "%s\n", "} DcPlanetBreadcrumbsPoints;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// One XML <Arg> value passed into a DrawFunction.");
    fprintf(file, "%s\n", "typedef struct _DcDrawFuncArg {");
    fprintf(file, "%s\n", "    DcValueType type;");
    fprintf(file, "%s\n", "    const char *value_string;");
    fprintf(file, "%s\n", "    int value_integer;");
    fprintf(file, "%s\n", "    double value_double;");
    fprintf(file, "%s\n", "    bool value_boolean;");
    fprintf(file, "%s\n", "} DcDrawFuncArg;");
    fprintf(file, "%s\n", "");

    fprintf(file, "%s\n", "// Full XML argument list passed into a DrawFunction.");
    fprintf(file, "%s\n", "typedef struct _DcDrawFuncArgs {");
    fprintf(file, "%s\n", "    uint32_t count;");
    fprintf(file, "%s\n", "    const DcDrawFuncArg *values;");
    fprintf(file, "%s\n", "} DcDrawFuncArgs;");
    fprintf(file, "%s\n", "");

    fprintf(file, "%s\n", "// draw functions are only valid inside DrawFunction callbacks.");
    fprintf(file, "%s\n", "// use DcAppContext APIs for app-owned resources outside drawing.");
    fprintf(file, "%s\n", "typedef struct _DcDrawApi {");
    fprintf(file, "%s\n", "    // Current draw area.");
    fprintf(file, "%s\n", "    const DcDrawArea *(*get_area)(DcDrawContext *draw_ctx);");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "    // Basic draw functions.");
    fprintf(file, "%s\n", "    void (*line)(DcDrawContext *draw_ctx, DcVec2 p0, DcVec2 p1, DcStroke stroke);");
    fprintf(file, "%s\n", "    void (*polyline)(DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count, DcStroke stroke);");
    fprintf(file, "%s\n", "    void (*polygon)(DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count, DcStroke stroke);");
    fprintf(file, "%s\n", "    void (*polygon_filled)(DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*rounded_polygon)(DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count, float corner_radius, DcStroke stroke);");
    fprintf(file, "%s\n", "    void (*rounded_polygon_filled)(DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count, float corner_radius, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*quad)(DcDrawContext *draw_ctx, DcVec2 p0, DcVec2 p1, DcVec2 p2, DcVec2 p3, DcStroke stroke);");
    fprintf(file, "%s\n", "    void (*quad_filled)(DcDrawContext *draw_ctx, DcVec2 p0, DcVec2 p1, DcVec2 p2, DcVec2 p3, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*rounded_quad)(DcDrawContext *draw_ctx, DcVec2 p0, DcVec2 p1, DcVec2 p2, DcVec2 p3, float corner_radius, DcStroke stroke);");
    fprintf(file, "%s\n", "    void (*rounded_quad_filled)(DcDrawContext *draw_ctx, DcVec2 p0, DcVec2 p1, DcVec2 p2, DcVec2 p3, float corner_radius, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*image)(DcDrawContext *draw_ctx, DcTextureId texture_id, DcVec2 position, DcVec2 size, DcVec4 tint);");
    fprintf(file, "%s\n", "    void (*rect)(DcDrawContext *draw_ctx, DcVec2 position, DcVec2 size, DcStroke stroke);");
    fprintf(file, "%s\n", "    void (*rect_filled)(DcDrawContext *draw_ctx, DcVec2 position, DcVec2 size, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*rounded_rect)(DcDrawContext *draw_ctx, DcVec2 position, DcVec2 size, float corner_radius, DcStroke stroke);");
    fprintf(file, "%s\n", "    void (*rounded_rect_filled)(DcDrawContext *draw_ctx, DcVec2 position, DcVec2 size, float corner_radius, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*circle)(DcDrawContext *draw_ctx, DcVec2 center, float radius, DcStroke stroke);");
    fprintf(file, "%s\n", "    void (*circle_filled)(DcDrawContext *draw_ctx, DcVec2 center, float radius, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*ellipse)(DcDrawContext *draw_ctx, DcVec2 center, DcVec2 radius, DcStroke stroke);");
    fprintf(file, "%s\n", "    void (*ellipse_filled)(DcDrawContext *draw_ctx, DcVec2 center, DcVec2 radius, DcVec4 color);");
    fprintf(file, "%s\n", "    DcVec2 (*text_size)(DcDrawContext *draw_ctx, const char *text, DcTextStyle style);");
    fprintf(file, "%s\n", "    void (*text)(DcDrawContext *draw_ctx, DcVec2 position, const char *text, DcTextStyle style);");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "    // Extended draw functions with placement and result metadata.");
    fprintf(file, "%s\n", "    void (*line_ex)(DcDrawContext *draw_ctx, DcVec2 p0, DcVec2 p1, DcStroke stroke, DcVec2 position, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*polyline_ex)(DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count, DcStroke stroke, DcVec2 position, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*polygon_ex)(DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count, DcStroke stroke, DcVec2 position, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*polygon_filled_ex)(DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count, DcVec4 color, DcVec2 position, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*rounded_polygon_ex)(DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count, float corner_radius, DcStroke stroke, DcVec2 position, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*rounded_polygon_filled_ex)(DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count, float corner_radius, DcVec4 color, DcVec2 position, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*quad_ex)(DcDrawContext *draw_ctx, DcVec2 p0, DcVec2 p1, DcVec2 p2, DcVec2 p3, DcStroke stroke, DcVec2 position, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*quad_filled_ex)(DcDrawContext *draw_ctx, DcVec2 p0, DcVec2 p1, DcVec2 p2, DcVec2 p3, DcVec4 color, DcVec2 position, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*rounded_quad_ex)(DcDrawContext *draw_ctx, DcVec2 p0, DcVec2 p1, DcVec2 p2, DcVec2 p3, float corner_radius, DcStroke stroke, DcVec2 position, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*rounded_quad_filled_ex)(DcDrawContext *draw_ctx, DcVec2 p0, DcVec2 p1, DcVec2 p2, DcVec2 p3, float corner_radius, DcVec4 color, DcVec2 position, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*image_ex)(DcDrawContext *draw_ctx, DcTextureId texture_id, DcVec2 position, DcVec2 size, DcVec4 tint, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*rect_ex)(DcDrawContext *draw_ctx, DcVec2 position, DcVec2 size, DcStroke stroke, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*rect_filled_ex)(DcDrawContext *draw_ctx, DcVec2 position, DcVec2 size, DcVec4 color, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*rounded_rect_ex)(DcDrawContext *draw_ctx, DcVec2 position, DcVec2 size, float corner_radius, DcStroke stroke, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*rounded_rect_filled_ex)(DcDrawContext *draw_ctx, DcVec2 position, DcVec2 size, float corner_radius, DcVec4 color, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*circle_ex)(DcDrawContext *draw_ctx, DcVec2 center, float radius, DcStroke stroke, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*circle_filled_ex)(DcDrawContext *draw_ctx, DcVec2 center, float radius, DcVec4 color, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*ellipse_ex)(DcDrawContext *draw_ctx, DcVec2 center, DcVec2 radius, DcStroke stroke, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*ellipse_filled_ex)(DcDrawContext *draw_ctx, DcVec2 center, DcVec2 radius, DcVec4 color, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*text_ex)(DcDrawContext *draw_ctx, DcVec2 position, const char *text, DcTextStyle style, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "    // Container helpers.");
    fprintf(file, "%s\n", "    bool (*container_push)(DcDrawContext *draw_ctx, DcVec2 position, DcVec2 size, DcVec2 virtual_size);");
    fprintf(file, "%s\n", "    bool (*container_push_ex)(DcDrawContext *draw_ctx, DcVec2 position, DcVec2 size, DcVec2 virtual_size, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    bool (*container_push_area)(DcDrawContext *draw_ctx, const DcDrawArea *area);");
    fprintf(file, "%s\n", "    void (*container_pop)(DcDrawContext *draw_ctx);");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "    // Stencil helpers.");
    fprintf(file, "%s\n", "    bool (*stencil_begin)(DcDrawContext *draw_ctx);");
    fprintf(file, "%s\n", "    void (*stencil_add)(DcDrawContext *draw_ctx);");
    fprintf(file, "%s\n", "    void (*stencil_remove)(DcDrawContext *draw_ctx);");
    fprintf(file, "%s\n", "    void (*stencil_draw)(DcDrawContext *draw_ctx);");
    fprintf(file, "%s\n", "    void (*stencil_end)(DcDrawContext *draw_ctx);");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "    // draws planet views and overlays through dcapp handles.");
    fprintf(file, "%s\n", "    DcDrawPlanetViewHandle (*planet_view_geodetic)(DcDrawContext *draw_ctx, DcPlanetViewHandle view, double lat, double lon, double elevation, DcVec3 rpy, float fov_degrees, bool orthographic, DcVec2 position, DcVec2 size, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    DcDrawPlanetViewHandle (*planet_view_cartesian)(DcDrawContext *draw_ctx, DcPlanetViewHandle view, DcVec3 camera_position, DcVec3 rpy, float fov_degrees, bool orthographic, DcVec2 position, DcVec2 size, DcPlacement placement, DcDrawResult *result);");
    fprintf(file, "%s\n", "    void (*planet_sphere_geodetic)(DcDrawContext *draw_ctx, DcDrawPlanetViewHandle view, double lat, double lon, double height, double radius, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*planet_sphere_cartesian)(DcDrawContext *draw_ctx, DcDrawPlanetViewHandle view, DcVec3 position, float radius, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*planet_line_geodetic)(DcDrawContext *draw_ctx, DcDrawPlanetViewHandle view, const DcVec3 *points, uint32_t point_count, float line_width, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*planet_line_cartesian)(DcDrawContext *draw_ctx, DcDrawPlanetViewHandle view, const DcVec3 *points, uint32_t point_count, float line_width, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*planet_polygon_geodetic)(DcDrawContext *draw_ctx, DcDrawPlanetViewHandle view, const DcVec3 *points, uint32_t point_count, float line_width, DcVec4 line_color, DcVec4 fill_color);");
    fprintf(file, "%s\n", "    void (*planet_polygon_cartesian)(DcDrawContext *draw_ctx, DcDrawPlanetViewHandle view, const DcVec3 *points, uint32_t point_count, float line_width, DcVec4 line_color, DcVec4 fill_color);");
    fprintf(file, "%s\n", "    void (*planet_text_geodetic)(DcDrawContext *draw_ctx, DcDrawPlanetViewHandle view, double lat, double lon, double height, const char *text, float size, DcVec4 color);");
    fprintf(file, "%s\n", "    void (*planet_text_cartesian)(DcDrawContext *draw_ctx, DcDrawPlanetViewHandle view, DcVec3 position, const char *text, float size, DcVec4 color);");
    fprintf(file, "%s\n", "} DcDrawApi;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// Mouse hit registration and event queries available through dc_mouse.");
    fprintf(file, "%s\n", "typedef struct _DcMouseApi {");
    fprintf(file, "%s\n", "    // Basic mouse hit registration.");
    fprintf(file, "%s\n", "    void (*rect)(DcDrawContext *draw_ctx, const char *id, DcVec2 position, DcVec2 size);");
    fprintf(file, "%s\n", "    void (*circle)(DcDrawContext *draw_ctx, const char *id, DcVec2 center, float radius);");
    fprintf(file, "%s\n", "    void (*ellipse)(DcDrawContext *draw_ctx, const char *id, DcVec2 center, DcVec2 radius);");
    fprintf(file, "%s\n", "    void (*polygon)(DcDrawContext *draw_ctx, const char *id, const DcVec2 *points, uint32_t point_count, DcVec2 position);");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "    // Extended mouse hit registration with placement.");
    fprintf(file, "%s\n", "    void (*rect_ex)(DcDrawContext *draw_ctx, const char *id, DcVec2 position, DcVec2 size, DcPlacement placement);");
    fprintf(file, "%s\n", "    void (*circle_ex)(DcDrawContext *draw_ctx, const char *id, DcVec2 center, float radius, DcPlacement placement);");
    fprintf(file, "%s\n", "    void (*ellipse_ex)(DcDrawContext *draw_ctx, const char *id, DcVec2 center, DcVec2 radius, DcPlacement placement);");
    fprintf(file, "%s\n", "    void (*polygon_ex)(DcDrawContext *draw_ctx, const char *id, const DcVec2 *points, uint32_t point_count, DcVec2 position, DcPlacement placement);");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "    // Event queries.");
    fprintf(file, "%s\n", "    bool (*hovered)(DcDrawContext *draw_ctx, const char *id);");
    fprintf(file, "%s\n", "    bool (*pressed)(DcDrawContext *draw_ctx, const char *id);");
    fprintf(file, "%s\n", "    bool (*released)(DcDrawContext *draw_ctx, const char *id);");
    fprintf(file, "%s\n", "    bool (*active)(DcDrawContext *draw_ctx, const char *id);");
    fprintf(file, "%s\n", "    bool (*clicked)(DcDrawContext *draw_ctx, const char *id);");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "    // Current mouse state in the draw context's local space.");
    fprintf(file, "%s\n", "    bool (*down)(DcDrawContext *draw_ctx);");
    fprintf(file, "%s\n", "    const DcMouse *(*get_state)(DcDrawContext *draw_ctx);");
    fprintf(file, "%s\n", "} DcMouseApi;");
    fprintf(file, "%s\n", "");

    fprintf(file, "%s\n", "// texture resources are owned by the app context.");
    fprintf(file, "%s\n", "typedef struct _DcTextureApi {");
    fprintf(file, "%s\n", "    DcTextureId (*load_image)(DcAppContext *app_ctx, const char *path, DcVec2 *out_size);");
    fprintf(file, "%s\n", "    bool (*get_size)(DcAppContext *app_ctx, DcTextureId texture_id, DcVec2 *out_size);");
    fprintf(file, "%s\n", "} DcTextureApi;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// planet resources are owned by the app context.");
    fprintf(file, "%s\n", "typedef struct _DcPlanetApi {");
    fprintf(file, "%s\n", "    // planet resources live until app shutdown.");
    fprintf(file, "%s\n", "    DcPlanetHandle (*get_planet_by_id)(DcAppContext *app_ctx, const char *id);");
    fprintf(file, "%s\n", "    DcPlanetHandle (*create_planet)(DcAppContext *app_ctx, DcPlanetCreateInfo info);");
    fprintf(file, "%s\n", "    // fails if id already exists.");
    fprintf(file, "%s\n", "    DcPlanetHandle (*create_planet_with_id)(DcAppContext *app_ctx, const char *id, DcPlanetCreateInfo info);");
    fprintf(file, "%s\n", "    bool (*set_texture_geodetic)(DcAppContext *app_ctx, DcPlanetHandle planet, const char *path, double lat, double lon, float meters_per_pixel);");
    fprintf(file, "%s\n", "    bool (*set_texture_cartesian)(DcAppContext *app_ctx, DcPlanetHandle planet, const char *path, DcVec3 position, float meters_per_pixel);");
    fprintf(file, "%s\n", "    DcPlanetViewHandle (*create_geodetic_view)(DcAppContext *app_ctx, DcPlanetHandle planet, uint32_t width, uint32_t height);");
    fprintf(file, "%s\n", "    DcPlanetViewHandle (*create_cartesian_view)(DcAppContext *app_ctx, DcPlanetHandle planet, uint32_t width, uint32_t height);");
    fprintf(file, "%s\n", "    bool (*set_view_shaders)(DcPlanetViewHandle view, const char *vertex_shader, const char *fragment_shader);");
    fprintf(file, "%s\n", "    DcPlanetBreadcrumbsHandle (*create_breadcrumbs)(DcAppContext *app_ctx, DcPlanetCrs crs, uint32_t max_points, float point_spacing);");
    fprintf(file, "%s\n", "    void (*update_breadcrumbs_geodetic)(DcPlanetBreadcrumbsHandle breadcrumbs, DcPlanetHandle planet, DcVec3 position);");
    fprintf(file, "%s\n", "    void (*update_breadcrumbs_cartesian)(DcPlanetBreadcrumbsHandle breadcrumbs, DcVec3 position);");
    fprintf(file, "%s\n", "    void (*clear_breadcrumbs)(DcPlanetBreadcrumbsHandle breadcrumbs);");
    fprintf(file, "%s\n", "    DcPlanetBreadcrumbsPoints (*get_breadcrumbs_points)(DcPlanetBreadcrumbsHandle breadcrumbs);");
    fprintf(file, "%s\n", "} DcPlanetApi;");
    fprintf(file, "%s\n", "");

    fprintf(file, "%s\n", "// app-level functions require an explicit app context.");
    fprintf(file, "%s\n", "typedef void *(*DcGetVariableFn)(DcAppContext *app_ctx, const char *name);");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "typedef struct _DcAppApi {");
    fprintf(file, "%s\n", "    void *(*get_variable)(DcAppContext *app_ctx, const char *name);");
    fprintf(file, "%s\n", "} DcAppApi;");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "// runtime hooks filled in by dcapp before user display_init().");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "typedef struct _DcInit {");
    fprintf(file, "%s\n", "    uint32_t size;");
    fprintf(file, "%s\n", "    uint32_t version;");
    fprintf(file, "%s\n", "    DcAppContext *app_ctx;");
    fprintf(file, "%s\n", "    DcGetVariableFn get_variable;");
    fprintf(file, "%s\n", "    const DcDrawApi *draw;");
    fprintf(file, "%s\n", "    const DcMouseApi *mouse;");
    fprintf(file, "%s\n", "    const DcTextureApi *texture;");
    fprintf(file, "%s\n", "    const DcPlanetApi *planet;");
    fprintf(file, "%s\n", "    const DcAppApi *app;");
    fprintf(file, "%s\n", "} DcInit;");
    fprintf(file, "%s\n", "");
}

void _process_node_children(xmlNodePtr xml_node, DcAppLookup *lookup) {
    xmlNodePtr xml_child_node = xml_node->children;
    while (xml_child_node) {
        _process_node(xml_child_node, lookup);
        xml_child_node = xml_child_node->next;
    }
}

void _process_node(xmlNodePtr xml_node, DcAppLookup *lookup) {

    switch (dc_app_xml_node_to_elem_type(xml_node)) {

        // ignore non-element nodes
        case DC_APP_ELEM_TYPE_NONELEM: {
            return;
        }

        case DC_APP_ELEM_TYPE_VARIABLE: {

            // name
            xmlChar *raw_name = xmlNodeGetContent(xml_node);
            char     clean_name[DC_VALUE_STRING_BUFFER_SIZE];
            if (raw_name) {
                strncpy(clean_name, (const char *)raw_name, DC_VALUE_STRING_BUFFER_SIZE - 1);
                clean_name[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
                xmlFree(raw_name);
                dc_utils_trim_whitespace_inplace(clean_name);
                if (clean_name[0] == '\0') {
                    DC_LOG_ERROR("GenHeader", "Empty variable name in <Variable> definition");
                }
            } else {
                DC_LOG_ERROR("GenHeader", "Node content missing in <Variable> definition");
                clean_name[0] = '\0';
            }

            // type
            xmlChar *raw_type = xmlGetProp(xml_node, BAD_CAST "Type");
            char     clean_type[DC_VALUE_STRING_BUFFER_SIZE];
            if (raw_type) {
                strncpy(clean_type, (const char *)raw_type, DC_VALUE_STRING_BUFFER_SIZE - 1);
                clean_type[DC_VALUE_STRING_BUFFER_SIZE - 1] = '\0';
                xmlFree(raw_type);
            } else {
                DC_LOG_ERROR("GenHeader", "Attribute 'Type' missing in <Variable> definition");
                clean_type[0] = '\0';
            }

            // don't care about initial value here
            // xmlChar *raw_init_value = xmlGetProp(xml_node, BAD_CAST "InitialValue");

            // register variable
            DcValue value      = {};
            value.type         = dc_utils_string_to_integer((const char *)clean_type);
            DcAppLookupVar var = {};
            var.value_index    = dc_app_lookup_register_value(lookup, &value);
            dc_app_lookup_register_var(lookup, clean_name, &var);
            break;
        }

        // for anything else, just process the children inside
        default: {
            _process_node_children(xml_node, lookup);
            break;
        }
    }
}
