//-----------------------------------------------------------------------------
// [SECTION] dcapp includes
//-----------------------------------------------------------------------------

#include <app.hpp>
#include <utils/file.hpp>
#include <utils/string.hpp>
#include <utils/xml.hpp>
#include <value.hpp>

DcAppData dc_app_data;

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// [SECTION] dcapp state
//-----------------------------------------------------------------------------

static void _process_node_children(xmlNodePtr xml_node);
static void _process_node(xmlNodePtr xml_node);

int main(int argc, char **argv) {
    if (argc < 2) {
        throw std::runtime_error("Missing dcapp configuration file");
    }

    std::string configRelativePath = std::string(argv[1]);
    // TODO check that it exists

    // set paths
    std::filesystem::path fsFilePath      = std::filesystem::canonical(configRelativePath);
    std::filesystem::path fs_dir_path     = fsFilePath.parent_path();
    std::string           configFilePath  = fsFilePath.string();
    std::string           config_dir_path = fs_dir_path.string();

    // create cache and log dirs
    std::filesystem::path fsExePath   = dc_utils_get_exe_filepath();
    std::filesystem::path fs_log_path = fsExePath.parent_path() / ".." / "logs";
    std::filesystem::create_directory(fs_log_path);
    std::string           log_dir_path  = std::filesystem::canonical(fs_log_path).string();
    std::filesystem::path fs_cache_path = fsExePath.parent_path() / ".." / "cache";
    std::filesystem::create_directory(fs_cache_path);
    std::string cache_dir_path = std::filesystem::canonical(fs_cache_path).string();

    // init dc_app_data object
    dc_app_init_data();

    // begin setting up dcappData object
    dc_app_data.configFilePath  = configFilePath;
    dc_app_data.config_dir_path = config_dir_path;
    dc_app_data.log_dir_path    = log_dir_path;
    dc_app_data.cache_dir_path  = cache_dir_path;

    // set environment (used for dcapp XMLs)
    setenv("dcappDisplayHome", dc_app_data.config_dir_path.c_str(), 1);

    // load XML file
    dc_app_data.doc = xmlReadFile(configFilePath.c_str(), "UTF-8", XML_PARSE_NOBLANKS);
    if (!dc_app_data.doc) {
        throw std::runtime_error("Unable to read configuration file: " + configFilePath);
    }

    // clean XML file
    dc_app_clean_xml_data();

    // process XML
    xmlNodePtr root_node = xmlDocGetRootElement(dc_app_data.doc);
    _process_node(root_node);

    // create directory
    std::filesystem::path directory = std::filesystem::path(dc_app_data.config_dir_path) / "logic";
    std::filesystem::create_directory(directory);

    // create file
    std::filesystem::path filePath = directory / "dcapp.h";
    std::ofstream         out_file;
    out_file.open(filePath.string());

    // file header
    out_file << "// ********************************************* //\n";
    out_file << "// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //\n";
    out_file << "// ********************************************* //\n";
    out_file << "\n";
    out_file << "#pragma once\n";
    out_file << "\n";
    out_file << "#include <string>\n";
    out_file << "\n";
    out_file << "#ifdef __cplusplus\n";
    out_file << "extern \"C\" {\n";
    out_file << "#endif\n";
    out_file << "\n";

    // file function declarations
    out_file << "void DisplayPreInit();\n";
    out_file << "void DisplayInit();\n";
    out_file << "void DisplayDraw();\n";
    out_file << "void DisplayClose();\n";
    out_file << "\n";

    // file variable definitions
    for (auto &[name, variable] : dc_app_data.variables) {
        switch (dc_app_index_to_dc_value(variable.value_index)->type) {
            case DC_APP_VALUE_TYPE_STRING:
                out_file << "std::string ";
                break;
            case DC_APP_VALUE_TYPE_FLOAT:
                out_file << "float       ";
                break;
            case DC_APP_VALUE_TYPE_INTEGER:
                out_file << "int         ";
                break;
            case DC_APP_VALUE_TYPE_BOOLEAN:
                out_file << "bool        ";
                break;
            default:
                throw std::runtime_error("Unknown value type");
                break;
        }
        out_file << name << ";\n";
    }
    out_file << "\n";

    // file closer
    out_file << "#ifdef __cplusplus\n";
    out_file << "}\n";
    out_file << "#endif\n";

    // exit
    out_file.close();
    return 0;
}

// returns the first child (if any)
void _process_node_children(xmlNodePtr xml_node) {
    xmlNodePtr xml_child_node = xml_node->children;
    while (xml_child_node) {
        _process_node(xml_child_node);
        xml_child_node = xml_child_node->next;
    }
}

void _process_node(xmlNodePtr xml_node) {
    // by default, the element is not a node
    DcAppNodeIndex node_index = DC_APP_NODE_INDEX_UNDEFINED;

    switch (dc_app_xml_node_to_elem_type(xml_node)) {

        // ignore non-element nodes
        case DC_APP_ELEM_TYPE_NONELEM: {
            return;
        }

        case DC_APP_ELEM_TYPE_CONSTANT: {
            // name
            char *c_name = dc_utils_get_attribute_string(xml_node, "Name");
            if (!c_name) {
                throw std::runtime_error("Non-existent node 'Name' in <Constant> definition");
            }
            std::string name = dc_app_dereference_constants(c_name);
            free(c_name);

            // value
            char *c_value = dc_utils_get_node_content_string(xml_node);
            if (!c_value) {
                throw std::runtime_error("Non-existent node content in <Constant> definition");
            }
            std::string value = dc_app_dereference_constants(c_value);
            free(c_value);

            // set constant value
            dc_app_set_constant(name, value);
            break;
        }

        // really just the root element, left in for legacy reasons
        case DC_APP_ELEM_TYPE_DCAPP: {
            _process_node_children(xml_node);
            break;
        }

        // at this point, just used to set the directory path
        case DC_APP_ELEM_TYPE_INCLUDE: {
            char *c_directory = dc_utils_get_attribute_string(xml_node, "Directory");
            if (c_directory) {
                _process_node_children(xml_node);
                free(c_directory);
            } else {
                // should never get here
                throw std::runtime_error("Invalid condition; <Include> node with no directory");
            }
            break;
        }

        case DC_APP_ELEM_TYPE_VARIABLE: {
            // name
            char *c_name = dc_utils_get_node_content_string(xml_node);
            if (!c_name) {
                throw std::runtime_error("Non-existent node content in <Variable> definition");
            }
            std::string name = dc_app_dereference_constants(c_name);
            free(c_name);

            // type
            char       *c_type = dc_utils_get_attribute_string(xml_node, "Type");
            DcValueType type   = DC_APP_VALUE_TYPE_STRING;
            if (c_type) {
                type = dc_value_type_from_string(dc_app_dereference_constants(c_type));
            }

            // value
            char   *c_initial_value = dc_utils_get_attribute_string(xml_node, "InitialValue");
            DcValue initial_value;
            if (c_initial_value) {
                initial_value = dc_value_create_value_string(dc_app_dereference_constants(c_initial_value));
            } else {
                initial_value = dc_value_create_value_string("");
            }
            initial_value.type       = type;
            initial_value.is_dynamic = true;

            // register variable
            DcAppValueIndex index = dc_app_register_dc_value(initial_value);
            dc_app_set_variable(name, index);
            break;
        }

        default: {
            _process_node_children(xml_node);
            break;
        }
    }
}
