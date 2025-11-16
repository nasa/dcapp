#include "../src/app/config.h"
#include "../src/app/elem.h"
#include "../src/app/lookup.h"
#include "../src/utils/env.h"
#include "../src/utils/file.h"

#include <libxml/parser.h>

#include <stdio.h>
#include <string.h>

static void _process_node_children(xmlNodePtr xml_node, DcAppLookup *lookup);
static void _process_node(xmlNodePtr xml_node, DcAppLookup *lookup);

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "DCAPP main(): Missing config file\n");
    }

    // create config
    DcAppConfig *config;
    const char  *config_filepath = argv[1];
    printf("%d\n", argc);
    if (argc < 3) {
        config = dc_app_config_create(config_filepath, NULL, 0);
    } else {
        config = dc_app_config_create(config_filepath, &(argv[2]), argc - 2);
    }

    // create lookup
    DcAppLookup *lookup = dc_app_lookup_create();

    // set environment (used for dcapp XMLs)
    dc_utils_set_env("dcappDisplayHome", config->config_dir_path, 1);

    // clean XML file
    dc_app_config_clean_xml(config, lookup);

    // dump to file
    char log_file[256];
    dc_utils_join_paths(config->cache_dir_path, "xml.log", log_file, sizeof(log_file));
    dc_app_config_save_to_file(config, log_file);

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

    // file header
    fprintf(file, "%s\n", "// ********************************************* //");
    fprintf(file, "%s\n", "// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //");
    fprintf(file, "%s\n", "// ********************************************* //");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "#ifndef _DCAPP_LOGIC_HEADER_");
    fprintf(file, "%s\n", "#define _DCAPP_LOGIC_HEADER_");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "#include <stdbool.h>");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "#ifdef __cplusplus");
    fprintf(file, "%s\n", "extern \"C\" {");
    fprintf(file, "%s\n", "#endif");
    fprintf(file, "%s\n", "");

    // file function declarations
    fprintf(file, "%s\n", "void DisplayPreInit();");
    fprintf(file, "%s\n", "void DisplayInit();");
    fprintf(file, "%s\n", "void DisplayDraw();");
    fprintf(file, "%s\n", "void DisplayClose();");
    fprintf(file, "%s\n", "");

    // file variable definitions
    int var_count = dc_app_lookup_get_var_count(lookup);
    for (DcAppVarIndex var_index = 0; var_index < var_count; var_index++) {

        DcAppLookupVar *var      = dc_app_lookup_get_var(lookup, var_index);
        const char     *var_name = dc_app_lookup_get_var_name(lookup, var_index);
        DcValue        *value    = dc_app_lookup_get_value(lookup, var->value_index);
        switch (value->type) {
            case DC_VALUE_TYPE_STRING:
                fprintf(file, "char   %s[%d];\n", var_name, DC_VALUE_STRING_BUFFER_SIZE);
                break;
            case DC_VALUE_TYPE_DOUBLE:
                fprintf(file, "double %s;\n", var_name);
                break;
            case DC_VALUE_TYPE_INTEGER:
                fprintf(file, "int    %s;\n", var_name);
                break;
            case DC_VALUE_TYPE_BOOLEAN:
                fprintf(file, "bool   %s;\n", var_name);
                break;
            default:
                fprintf(stderr, "DCAPP main(): Invalid variable value type %d\n", value->type);
                break;
        }
    }
    fprintf(file, "%s\n", "");

    // file closer
    fprintf(file, "%s\n", "#ifdef __cplusplus");
    fprintf(file, "%s\n", "}");
    fprintf(file, "%s\n", "#endif");
    fprintf(file, "%s\n", "");
    fprintf(file, "%s\n", "#endif");

    // exit
    fclose(file);
    return 0;
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
                xmlFree(raw_name);
            } else {
                fprintf(stderr, "DCAPP _process_node(): Node content missing in <Variable> definition\n");
            }

            // type
            xmlChar *raw_type = xmlGetProp(xml_node, BAD_CAST "Type");
            char     clean_type[DC_VALUE_STRING_BUFFER_SIZE];
            if (raw_type) {
                strncpy(clean_type, (const char *)raw_type, DC_VALUE_STRING_BUFFER_SIZE - 1);
                xmlFree(raw_type);
            } else {
                fprintf(stderr, "DCAPP _process_node(): Attribute Type missing in <Variable> definition\n");
            }

            // don't care about initial value here
            // xmlChar *raw_init_value = xmlGetProp(xml_node, BAD_CAST "InitialValue");

            // register variable
            DcValue value      = {};
            value.type         = dc_app_value_type_from_string(clean_type);
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
