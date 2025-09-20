
// dcapp includes
#include "value.hpp"
#include <app.hpp>
#include <utils/file.hpp>
#include <utils/string.hpp>
#include <utils/xml.hpp>

// library includes

// c++ standard includes
#include <map>
#include <stdexcept>
#include <vector>

// static utils
static void _dc_app_clean_xml_node(xmlNodePtr node, std::string directory);

std::string dc_app_elem_type_to_string(DcAppElemType type)
{
    switch (type)
    {
    case DC_APP_ELEM_TYPE_CONSTANT:
        return "Constant";
    case DC_APP_ELEM_TYPE_CONTAINER:
        return "Container";
    case DC_APP_ELEM_TYPE_DCAPP:
        return "DCAPP";
    case DC_APP_ELEM_TYPE_DEM:
        return "DEM";
    case DC_APP_ELEM_TYPE_DUMMY:
        return "Dummy";
    case DC_APP_ELEM_TYPE_INCLUDE:
        return "Include";
    case DC_APP_ELEM_TYPE_LOGIC:
        return "Logic";
    case DC_APP_ELEM_TYPE_MAP:
        return "Map";
    case DC_APP_ELEM_TYPE_NONELEM:
        return "<Non-Element>";
    case DC_APP_ELEM_TYPE_PANEL:
        return "Panel";
    case DC_APP_ELEM_TYPE_POLYGON:
        return "Polygon";
    case DC_APP_ELEM_TYPE_VARIABLE:
        return "Variable";
    case DC_APP_ELEM_TYPE_VERTEX:
        return "Vertex";
    case DC_APP_ELEM_TYPE_WINDOW:
        return "Window";
    case DC_APP_ELEM_TYPE_UNDEFINED:
    default:
        return "Undefined";
    }
}

DcAppElemType dc_app_string_to_elem_type(std::string name)
{
    if (name == "Constant")
        return DC_APP_ELEM_TYPE_CONSTANT;
    if (name == "Container")
        return DC_APP_ELEM_TYPE_CONTAINER;
    if (name == "DCAPP")
        return DC_APP_ELEM_TYPE_DCAPP;
    if (name == "DEM")
        return DC_APP_ELEM_TYPE_DEM;
    if (name == "Dummy")
        return DC_APP_ELEM_TYPE_DUMMY;
    if (name == "Include")
        return DC_APP_ELEM_TYPE_INCLUDE;
    if (name == "Logic")
        return DC_APP_ELEM_TYPE_LOGIC;
    if (name == "Map")
        return DC_APP_ELEM_TYPE_MAP;
    if (name == "Panel")
        return DC_APP_ELEM_TYPE_PANEL;
    if (name == "Polygon")
        return DC_APP_ELEM_TYPE_POLYGON;
    if (name == "Variable")
        return DC_APP_ELEM_TYPE_VARIABLE;
    if (name == "Vertex")
        return DC_APP_ELEM_TYPE_VERTEX;
    if (name == "Window")
        return DC_APP_ELEM_TYPE_WINDOW;
    return DC_APP_ELEM_TYPE_UNDEFINED;
}

DcAppElemType dc_app_xml_node_to_elem_type(xmlNodePtr node)
{
    if (node && node->type == XML_ELEMENT_NODE)
    {
        std::string name = (char *)(node->name);
        DcAppElemType type = dc_app_string_to_elem_type(name);
        if (type == DC_APP_ELEM_TYPE_UNDEFINED)
        {
            throw std::runtime_error("Undefined element name: " + name);
        }
        return type;
    }
    return DC_APP_ELEM_TYPE_NONELEM;
}

void dc_app_set_constant(const std::string &name, const std::string &text)
{
    dc_app_data.constants[name] = text;
}

std::string dc_app_get_constant(const std::string &name)
{
    if (dc_app_data.constants.count(name))
    {
        return dc_app_data.constants[name];
    }
    return "";
}

std::string dc_app_dereference_constants(const char *text)
{
    return dc_app_dereference_constants(std::string(text));
}

std::string dc_app_dereference_constants(std::string text)
{
    // return string if no '$'/'#'
    if (text.find_first_of("#$") == std::string::npos)
    {
        return text;
    }

    // iterate through each character
    std::string output = "";
    for (int ii = 0; ii < text.length(); ii++)
    {
        // skip backslash-escaped # and $
        if (text[ii] == '\\' && ii + 1 < text.length() && (text[ii + 1] == '#' || text[ii + 1] == '$'))
        {
            output += text[++ii];
            continue;
        }

        // if a character is a constant '#' or env variable '$', recursively
        // expand the values within.
        if (text[ii] == '#' || text[ii] == '$')
        {
            // error if ending on a #/$
            if (ii + 1 >= text.length())
            {
                throw std::runtime_error("Invalid string format: cannot have variable ending on a #/$. " + text);
            }

            // find ending index for constant/env variable reference (account for both
            // non-squigglied and squigglied references)
            std::string subtext;
            size_t subtextStartIndex;
            size_t subtextLength;
            size_t subtextLengthWithSymbols;
            if (text[ii + 1] == '{')
            {
                int numOpenBrackets = 1;
                int jj;
                for (jj = ii + 2; jj < text.length(); jj++)
                {
                    if (text[jj] == '{')
                    {
                        numOpenBrackets++;
                    }
                    else if (text[jj] == '}')
                    {
                        numOpenBrackets--;
                    }

                    if (numOpenBrackets == 0)
                    {
                        break;
                    }
                }

                if (numOpenBrackets > 0)
                {
                    throw std::runtime_error("Invalid string format: mismatch with squiggly braces. " + text);
                }

                subtextStartIndex = ii + 2;
                subtextLength = jj - subtextStartIndex;
                subtextLengthWithSymbols = subtextLength + 2;
            }
            else
            {
                static const std::string validChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#$";
                size_t subtextEndIndex = text.find_first_not_of(validChars, ii + 1);
                if (subtextEndIndex == std::string::npos)
                {
                    subtextEndIndex = text.length();
                }

                subtextStartIndex = ii + 1;
                subtextLength = subtextEndIndex - subtextStartIndex;
                subtextLengthWithSymbols = subtextLength;
            }

            // get substring, ensure no strange values within
            subtext = text.substr(subtextStartIndex, subtextLength);
            if (subtext.find('@') != std::string::npos)
            {
                throw std::runtime_error("Invalid string format: cannot have variable nested inside variable/constant expansion. " + text);
            }

            // recursion to clean the inner text
            subtext = dc_app_dereference_constants(subtext);

            // if constant, pull value from list of constants
            if (text[ii] == '#')
            {
                output += dc_app_get_constant(subtext);
            }
            // otherwise use the environment
            else if (text[ii] == '$')
            {
                char *envValue = getenv(subtext.c_str());
                if (envValue)
                {
                    output += std::string(envValue);
                }
            }

            // increment ii by the cleaned amount
            ii += subtextLengthWithSymbols;
        }
        else
        {
            output += text[ii];
        }
    }
    return output;
}

DcValue *dc_app_index_to_dc_value(DcAppValueIndex index)
{
    return &(dc_app_data.values[index]);
}

DcAppValueIndex dc_app_register_dc_value(DcValue value)
{
    dc_app_data.values.push_back(value);
    return dc_app_data.values.size() - 1;
}

DcAppValueIndex dc_app_create_and_register_typed_value_from_string(DcValueType type, const char *text)
{
    return dc_app_create_and_register_typed_value_from_string(type, std::string(text));
}

DcAppValueIndex dc_app_create_and_register_typed_value_from_string(DcValueType type, std::string text)
{
    // check for variable
    std::string cleanedValue = dc_utils_trim_whitespace(text);
    if (cleanedValue.length() > 1 && cleanedValue[0] == '@')
    {
        std::string variableName = cleanedValue.substr(1);
        if (dc_app_data.variables.count(variableName))
        {
            return dc_app_data.variables[variableName].valueIndex;
        }
        throw std::runtime_error("Non-existant variable @" + variableName);
    }

    // otherwise create new DcValue and return its index
    return dc_app_register_dc_value(dc_value_create_typed_value_from_string(type, text));
}

void dc_app_set_variable(const std::string &name, DcAppValueIndex valueIndex)
{
    if (dc_app_data.variables.count(name))
    {
        throw std::runtime_error("Duplicate variable for name " + name);
    }
    dc_app_data.variables[name] = (DcAppVariable){
        nullptr,
        valueIndex,
    };
}

std::string dc_app_node_type_to_string(DcAppNodeType type)
{
    switch (type)
    {
    case DC_APP_NODE_TYPE_CONTAINER:
        return "Container";
    case DC_APP_NODE_TYPE_CONDITIONAL:
        return "Conditional";
    case DC_APP_NODE_TYPE_MAP:
        return "Map";
    case DC_APP_NODE_TYPE_PANEL:
        return "Panel";
    case DC_APP_NODE_TYPE_POLYGON:
        return "Polygon";
    case DC_APP_NODE_TYPE_SET:
        return "Set";
    case DC_APP_NODE_TYPE_WINDOW:
        return "Window";
    default:
        throw std::runtime_error("Invalid DcAppNode type");
    }
}

DcAppNode *dc_app_index_to_node(DcAppNodeIndex index)
{
    if (index == DC_NODE_INDEX_UNDEFINED)
    {
        return nullptr;
    }
    return &(dc_app_data.nodes[index]);
}

DcAppNodeIndex dc_app_register_node(DcAppNode node)
{
    dc_app_data.nodes.push_back(node);
    return dc_app_data.nodes.size() - 1;
}

void dc_app_clean_xml_data()
{
    // get root element
    xmlNodePtr node = xmlDocGetRootElement(dc_app_data.doc);
    if (node == NULL)
    {
        throw std::runtime_error("Unable to get root element of configuration file: " + dc_app_data.configFilePath);
    }

    // verify root node is valid
    if (dc_app_xml_node_to_elem_type(node) != DC_APP_ELEM_TYPE_DCAPP)
    {
        throw std::runtime_error("Configuration root element is not DCAPP: " + dc_app_data.configFilePath);
    }

    // clean XML file
    _dc_app_clean_xml_node(node, dc_app_data.configFilePath);
}

void _dc_app_clean_xml_node(xmlNodePtr node, std::string directory)
{
    // remove if not an element
    if (node->type != XML_ELEMENT_NODE && node->type != XML_TEXT_NODE)
    {
        xmlUnlinkNode(node);
        xmlFreeNode(node);
    }
    else
    {
        // processing before targeting children
        switch (dc_app_xml_node_to_elem_type(node))
        {

        case DC_APP_ELEM_TYPE_CONSTANT:
        {
            char *cName = dc_utils_get_attribute_string(node, "Name");
            if (!cName)
            {
                throw std::runtime_error("Non-existent node 'Name' in <Constant> definition");
            }
            std::string name = dc_app_dereference_constants(cName);
            free(cName);

            char *cValue = dc_utils_get_node_content_string(node);
            if (!cValue)
            {
                throw std::runtime_error("Non-existent node content in <Constant> definition");
            }
            std::string value = dc_app_dereference_constants(cValue);
            free(cValue);

            dc_app_set_constant(name, value);
            break;
        }

        // remove "Dummy" level, keep children
        case DC_APP_ELEM_TYPE_DUMMY:
        {
            // requires some finagling if it contains children. Otherwise, unlink it normally
            if (node->children)
            {
                xmlNodePtr firstChild = node->children;
                xmlNodePtr lastChild = node->last;

                // update parent
                if (node->parent)
                {
                    if (node->parent->children == node)
                    {
                        node->parent->children = firstChild;
                    }
                    if (node->parent->last == node)
                    {
                        node->parent->last = lastChild;
                    }
                }

                // update siblings
                if (node->prev)
                {
                    node->prev->next = firstChild;
                }
                if (node->next)
                {
                    node->next->prev = lastChild;
                }

                // update children
                for (xmlNodePtr currChild = firstChild; currChild; currChild = currChild->next)
                {
                    currChild->parent = node->parent;
                }
                firstChild->prev = node->prev;
                lastChild->next = node->next;

                // unlink node
                node->children = NULL;
                node->last = NULL;
                node->parent = NULL;
                node->next = NULL;
                node->prev = NULL;
            }
            else
            {
                xmlUnlinkNode(node);
            }
            xmlFreeNode(node);
            break;
        }

        case DC_APP_ELEM_TYPE_INCLUDE:
        {

            // get include file name
            char *cIncludeFilePath = dc_utils_get_node_content_string(node);
            if (cIncludeFilePath == nullptr)
            {
                throw std::runtime_error("Missing file path for <Include> element");
            }
            std::string includeFilePath = dc_app_dereference_constants(cIncludeFilePath);
            free(cIncludeFilePath);

            // expand constants, go to canonical
            includeFilePath = dc_utils_filepath_to_canonical(includeFilePath, directory);

            // create new prop, assign to <Include> node
            xmlAttrPtr directoryProp = xmlNewProp(node, (xmlChar *)("Directory"), (xmlChar *)(directory.c_str()));

            // read XML file
            xmlDocPtr subDoc = xmlReadFile(includeFilePath.c_str(), NULL, XML_PARSE_NOBLANKS);
            if (!subDoc)
            {
                throw std::runtime_error("Unable to read configuration file: " + includeFilePath);
            }

            // get root element
            xmlNodePtr subNode = xmlDocGetRootElement(subDoc);
            if (subNode == NULL)
            {
                throw std::runtime_error("Unable to get root element of configuration file: " + includeFilePath);
            }

            // replace child (text) with subnode
            xmlNodePtr newNode = xmlDocCopyNode(subNode, subNode->doc, 1);
            xmlNodePtr oldNode = xmlReplaceNode(node->children, newNode);

            // free old node/doc
            xmlUnlinkNode(oldNode);
            xmlFreeNode(oldNode);
            xmlFreeDoc(subDoc);
            break;
        }
        default:
            break;
        }

        // clean children
        xmlNodePtr child = node->children;
        while (child)
        {
            xmlNodePtr childNext = child->next;
            _dc_app_clean_xml_node(child, directory);
            child = childNext;
        }
    }
}

void dc_app_init_data()
{
    // initialize members
    dc_app_data.configFilePath = "";
    dc_app_data.configDirPath = "";
    dc_app_data.cacheDirPath = "";
    dc_app_data.logDirPath = "";
    dc_app_data.constants.clear();
    dc_app_data.variables.clear();
    dc_app_data.values.resize(1);
    dc_app_data.nodes.resize(1);
    dc_app_data.window = DC_NODE_INDEX_UNDEFINED;
    dc_app_data.logic = (DcAppLogic){
        .library = nullptr,
        .preInit = nullptr,
        .init = nullptr,
        .draw = nullptr,
        .close = nullptr,
    };
    dc_app_data.doc = nullptr;

    // set default constants
    dc_app_set_constant("_align_left_", std::to_string(DC_APP_ALIGN_TYPE_LEFT));
    dc_app_set_constant("_align_center_", std::to_string(DC_APP_ALIGN_TYPE_CENTER));
    dc_app_set_constant("_align_right_", std::to_string(DC_APP_ALIGN_TYPE_RIGHT));
    dc_app_set_constant("_align_bottom_", std::to_string(DC_APP_ALIGN_TYPE_BOTTOM));
    dc_app_set_constant("_align_middle_", std::to_string(DC_APP_ALIGN_TYPE_MIDDLE));
    dc_app_set_constant("_align_top_", std::to_string(DC_APP_ALIGN_TYPE_TOP));

    dc_app_set_constant("_conditional_true_", std::to_string(DC_APP_CONDITIONAL_TYPE_TRUE));
    dc_app_set_constant("_conditional_false_", std::to_string(DC_APP_CONDITIONAL_TYPE_FALSE));
    dc_app_set_constant("_conditional_eq_", std::to_string(DC_APP_CONDITIONAL_TYPE_EQ));
    dc_app_set_constant("_conditional_ne_", std::to_string(DC_APP_CONDITIONAL_TYPE_NE));
    dc_app_set_constant("_conditional_lt_", std::to_string(DC_APP_CONDITIONAL_TYPE_LT));
    dc_app_set_constant("_conditional_gt_", std::to_string(DC_APP_CONDITIONAL_TYPE_GT));
    dc_app_set_constant("_conditional_lte_", std::to_string(DC_APP_CONDITIONAL_TYPE_LTE));
    dc_app_set_constant("_conditional_gte_", std::to_string(DC_APP_CONDITIONAL_TYPE_GTE));

    dc_app_set_constant("_color_black_", "0 0 0");
    dc_app_set_constant("_color_blue_", "0 0 1");
    dc_app_set_constant("_color_green_", "0 1 0");
    dc_app_set_constant("_color_cyan_", "0 1 1");
    dc_app_set_constant("_color_red_", "1 0 0");
    dc_app_set_constant("_color_magenta_", "1 0 1");
    dc_app_set_constant("_color_yellow_", "1 1 0");
    dc_app_set_constant("_color_white_", "1 1 1");
}
