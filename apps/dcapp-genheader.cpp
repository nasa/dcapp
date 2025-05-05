//-----------------------------------------------------------------------------
// [SECTION] dcapp includes
//-----------------------------------------------------------------------------

#include <dcapp-data.hpp>
#include <utils/file-utils.hpp>
#include <utils/string-utils.hpp>
#include <utils/xml-utils.hpp>
#include <value.hpp>

namespace dc
{
    DcappData dcData;
}

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// [SECTION] dcapp state
//-----------------------------------------------------------------------------

namespace dc
{
    static void processNodeChildren(xmlNodePtr xmlNode);
    static void processNode(xmlNodePtr xmlNode);
} // namespace dc

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
    std::filesystem::path fsExePath = getExeFilepath();
    std::filesystem::path fsLogPath = fsExePath.parent_path() / ".." / "logs";
    std::filesystem::create_directory(fsLogPath);
    std::string logDirPath = std::filesystem::canonical(fsLogPath).string();
    std::filesystem::path fsCachePath = fsExePath.parent_path() / ".." / "cache";
    std::filesystem::create_directory(fsCachePath);
    std::string cacheDirPath = std::filesystem::canonical(fsCachePath).string();

    // init dcData object
    dc::initData();

    // begin setting up dcappData object
    dc::dcData.configFilePath = configFilePath;
    dc::dcData.configDirPath = configDirPath;
    dc::dcData.logDirPath = logDirPath;
    dc::dcData.cacheDirPath = cacheDirPath;

    // set environment (used for dcapp XMLs)
    setenv("dcappDisplayHome", dc::dcData.configDirPath.c_str(), 1);

    // load XML file
    dc::dcData.doc = xmlReadFile(configFilePath.c_str(), "UTF-8", XML_PARSE_NOBLANKS);
    if (!dc::dcData.doc)
    {
        throw std::runtime_error("Unable to read configuration file: " + configFilePath);
    }

    // clean XML file
    dc::cleanXmlData();

    // process XML
    xmlNodePtr rootNode = xmlDocGetRootElement(dc::dcData.doc);
    dc::processNode(rootNode);

    // create directory
    std::filesystem::path directory = std::filesystem::path(dc::dcData.configDirPath) / "logic";
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
    for (auto &[name, variable] : dc::dcData.variables)
    {
        switch (dc::indexToDcValue(variable.valueIndex)->type)
        {
        case DC_VALUE_TYPE_STRING:
            outFile << "std::string ";
            break;
        case DC_VALUE_TYPE_FLOAT:
            outFile << "float       ";
            break;
        case DC_VALUE_TYPE_INTEGER:
            outFile << "int         ";
            break;
        case DC_VALUE_TYPE_BOOLEAN:
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

namespace dc
{
    // returns the first child (if any)
    void processNodeChildren(xmlNodePtr xmlNode)
    {
        xmlNodePtr xmlChildNode = xmlNode->children;
        while (xmlChildNode)
        {
            processNode(xmlChildNode);
            xmlChildNode = xmlChildNode->next;
        }
    }

    void processNode(xmlNodePtr xmlNode)
    {
        // by default, the element is not a node
        DcNodeIndex nodeIndex = DC_NODE_INDEX_UNDEFINED;

        switch (xmlNodeToElementType(xmlNode))
        {

        // ignore non-element nodes
        case DC_ELEM_TYPE_NONELEM:
        {
            return;
        }

        case DC_ELEM_TYPE_CONSTANT:
        {
            // name
            char *cName = getAttributeString(xmlNode, "Name");
            if (!cName)
            {
                throw std::runtime_error("Non-existent node 'Name' in <Constant> definition");
            }
            std::string name = dereferenceConstants(cName);
            free(cName);

            // value
            char *cValue = getNodeContentString(xmlNode);
            if (!cValue)
            {
                throw std::runtime_error("Non-existent node content in <Constant> definition");
            }
            std::string value = dereferenceConstants(cValue);
            free(cValue);

            // set constant value
            setConstant(name, value);
            break;
        }

        // really just the root element, left in for legacy reasons
        case DC_ELEM_TYPE_DCAPP:
        {
            processNodeChildren(xmlNode);
            break;
        }

        // at this point, just used to set the directory path
        case DC_ELEM_TYPE_INCLUDE:
        {
            char *cDirectory = getAttributeString(xmlNode, "Directory");
            if (cDirectory)
            {
                processNodeChildren(xmlNode);
                free(cDirectory);
            }
            else
            {
                // should never get here
                throw std::runtime_error("Invalid condition; <Include> node with no directory");
            }
            break;
        }

        case DC_ELEM_TYPE_VARIABLE:
        {
            // name
            char *cName = getNodeContentString(xmlNode);
            if (!cName)
            {
                throw std::runtime_error("Non-existent node content in <Variable> definition");
            }
            std::string name = dereferenceConstants(cName);
            free(cName);

            // type
            char *cType = getAttributeString(xmlNode, "Type");
            DcValueType type = DC_VALUE_TYPE_STRING;
            if (cType)
            {
                type = valueTypeFromString(dereferenceConstants(cType));
            }

            // value
            char *cInitialValue = getAttributeString(xmlNode, "InitialValue");
            DcValue initialValue;
            if (cInitialValue)
            {
                initialValue = createValueString(dereferenceConstants(cInitialValue));
            }
            else
            {
                initialValue = createValueString("");
            }
            initialValue.type = type;
            initialValue.isDynamic = true;

            // register variable
            DcValueIndex index = registerDcValue(initialValue);
            setVariable(name, index);
            break;
        }

        default:
        {
            processNodeChildren(xmlNode);
            break;
        }
        }
    }
} // namespace dc
