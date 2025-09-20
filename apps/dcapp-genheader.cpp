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

static void _process_node_children(xmlNodePtr xmlNode);
static void _process_node(xmlNodePtr xmlNode);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        throw std::runtime_error("Missing dcapp configuration file");
    }

    std::string configRelativePath = std::string(argv[1]);
    // TODO check that it exists

    // set paths
    std::filesystem::path fsFilePath = std::filesystem::canonical(configRelativePath);
    std::filesystem::path fsDirPath = fsFilePath.parent_path();
    std::string configFilePath = fsFilePath.string();
    std::string configDirPath = fsDirPath.string();

    // create cache and log dirs
    std::filesystem::path fsExePath = dc_utils_get_exe_filepath();
    std::filesystem::path fsLogPath = fsExePath.parent_path() / ".." / "logs";
    std::filesystem::create_directory(fsLogPath);
    std::string logDirPath = std::filesystem::canonical(fsLogPath).string();
    std::filesystem::path fsCachePath = fsExePath.parent_path() / ".." / "cache";
    std::filesystem::create_directory(fsCachePath);
    std::string cacheDirPath = std::filesystem::canonical(fsCachePath).string();

    // init dc_app_data object
    dc_app_init_data();

    // begin setting up dcappData object
    dc_app_data.configFilePath = configFilePath;
    dc_app_data.configDirPath = configDirPath;
    dc_app_data.logDirPath = logDirPath;
    dc_app_data.cacheDirPath = cacheDirPath;

    // set environment (used for dcapp XMLs)
    setenv("dcappDisplayHome", dc_app_data.configDirPath.c_str(), 1);

    // load XML file
    dc_app_data.doc = xmlReadFile(configFilePath.c_str(), "UTF-8", XML_PARSE_NOBLANKS);
    if (!dc_app_data.doc)
    {
        throw std::runtime_error("Unable to read configuration file: " + configFilePath);
    }

    // clean XML file
    dc_app_clean_xml_data();

    // process XML
    xmlNodePtr rootNode = xmlDocGetRootElement(dc_app_data.doc);
    _process_node(rootNode);

    // create directory
    std::filesystem::path directory = std::filesystem::path(dc_app_data.configDirPath) / "logic";
    std::filesystem::create_directory(directory);

    // create file
    std::filesystem::path filePath = directory / "dcapp.h";
    std::ofstream outFile;
    outFile.open(filePath.string());

    // file header
    outFile << "// ********************************************* //\n";
    outFile << "// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //\n";
    outFile << "// ********************************************* //\n";
    outFile << "\n";
    outFile << "#pragma once\n";
    outFile << "\n";
    outFile << "#include <string>\n";
    outFile << "\n";
    outFile << "#ifdef __cplusplus\n";
    outFile << "extern \"C\" {\n";
    outFile << "#endif\n";
    outFile << "\n";

    // file function declarations
    outFile << "void DisplayPreInit();\n";
    outFile << "void DisplayInit();\n";
    outFile << "void DisplayDraw();\n";
    outFile << "void DisplayClose();\n";
    outFile << "\n";

    // file variable definitions
    for (auto &[name, variable] : dc_app_data.variables)
    {
        switch (dc_app_index_to_dc_value(variable.valueIndex)->type)
        {
        case DC_APP_VALUE_TYPE_STRING:
            outFile << "std::string ";
            break;
        case DC_APP_VALUE_TYPE_FLOAT:
            outFile << "float       ";
            break;
        case DC_APP_VALUE_TYPE_INTEGER:
            outFile << "int         ";
            break;
        case DC_APP_VALUE_TYPE_BOOLEAN:
            outFile << "bool        ";
            break;
        default:
            throw std::runtime_error("Unknown value type");
            break;
        }
        outFile << name << ";\n";
    }
    outFile << "\n";

    // file closer
    outFile << "#ifdef __cplusplus\n";
    outFile << "}\n";
    outFile << "#endif\n";

    // exit
    outFile.close();
    return 0;
}

// returns the first child (if any)
void _process_node_children(xmlNodePtr xmlNode)
{
    xmlNodePtr xmlChildNode = xmlNode->children;
    while (xmlChildNode)
    {
        _process_node(xmlChildNode);
        xmlChildNode = xmlChildNode->next;
    }
}

void _process_node(xmlNodePtr xmlNode)
{
    // by default, the element is not a node
    DcAppNodeIndex nodeIndex = DC_NODE_INDEX_UNDEFINED;

    switch (dc_app_xml_node_to_elem_type(xmlNode))
    {

    // ignore non-element nodes
    case DC_APP_ELEM_TYPE_NONELEM:
    {
        return;
    }

    case DC_APP_ELEM_TYPE_CONSTANT:
    {
        // name
        char *cName = dc_utils_get_attribute_string(xmlNode, "Name");
        if (!cName)
        {
            throw std::runtime_error("Non-existent node 'Name' in <Constant> definition");
        }
        std::string name = dc_app_dereference_constants(cName);
        free(cName);

        // value
        char *cValue = dc_utils_get_node_content_string(xmlNode);
        if (!cValue)
        {
            throw std::runtime_error("Non-existent node content in <Constant> definition");
        }
        std::string value = dc_app_dereference_constants(cValue);
        free(cValue);

        // set constant value
        dc_app_set_constant(name, value);
        break;
    }

    // really just the root element, left in for legacy reasons
    case DC_APP_ELEM_TYPE_DCAPP:
    {
        _process_node_children(xmlNode);
        break;
    }

    // at this point, just used to set the directory path
    case DC_APP_ELEM_TYPE_INCLUDE:
    {
        char *cDirectory = dc_utils_get_attribute_string(xmlNode, "Directory");
        if (cDirectory)
        {
            _process_node_children(xmlNode);
            free(cDirectory);
        }
        else
        {
            // should never get here
            throw std::runtime_error("Invalid condition; <Include> node with no directory");
        }
        break;
    }

    case DC_APP_ELEM_TYPE_VARIABLE:
    {
        // name
        char *cName = dc_utils_get_node_content_string(xmlNode);
        if (!cName)
        {
            throw std::runtime_error("Non-existent node content in <Variable> definition");
        }
        std::string name = dc_app_dereference_constants(cName);
        free(cName);

        // type
        char *cType = dc_utils_get_attribute_string(xmlNode, "Type");
        DcValueType type = DC_APP_VALUE_TYPE_STRING;
        if (cType)
        {
            type = dc_value_type_from_string(dc_app_dereference_constants(cType));
        }

        // value
        char *cInitialValue = dc_utils_get_attribute_string(xmlNode, "InitialValue");
        DcValue initialValue;
        if (cInitialValue)
        {
            initialValue = dc_value_create_value_string(dc_app_dereference_constants(cInitialValue));
        }
        else
        {
            initialValue = dc_value_create_value_string("");
        }
        initialValue.type = type;
        initialValue.isDynamic = true;

        // register variable
        DcAppValueIndex index = dc_app_register_dc_value(initialValue);
        dc_app_set_variable(name, index);
        break;
    }

    default:
    {
        _process_node_children(xmlNode);
        break;
    }
    }
}
