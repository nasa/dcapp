
// dcapp includes
#include "value.hpp"
#include <dcapp-data.hpp>
#include <utils/file-utils.hpp>
#include <utils/string-utils.hpp>
#include <utils/xml-utils.hpp>

// library includes

// c++ standard includes
#include <map>
#include <stdexcept>
#include <vector>

// dcapp utils
namespace dc
{
    // static utils
    static void cleanXmlNode(xmlNodePtr node, std::string directory);

    std::string elemToString(DcElemType type)
    {
        switch (type)
        {
        case DC_ELEM_TYPE_CONSTANT:
            return "Constant";
        case DC_ELEM_TYPE_CONTAINER:
            return "Container";
        case DC_ELEM_TYPE_DCAPP:
            return "DCAPP";
        case DC_ELEM_TYPE_DEM:
            return "DEM";
        case DC_ELEM_TYPE_DUMMY:
            return "Dummy";
        case DC_ELEM_TYPE_INCLUDE:
            return "Include";
        case DC_ELEM_TYPE_LOGIC:
            return "Logic";
        case DC_ELEM_TYPE_MAP:
            return "Map";
        case DC_ELEM_TYPE_NONELEM:
            return "<Non-Element>";
        case DC_ELEM_TYPE_PANEL:
            return "Panel";
        case DC_ELEM_TYPE_POLYGON:
            return "Polygon";
        case DC_ELEM_TYPE_VARIABLE:
            return "Variable";
        case DC_ELEM_TYPE_VERTEX:
            return "Vertex";
        case DC_ELEM_TYPE_WINDOW:
            return "Window";
        case DC_ELEM_TYPE_UNDEFINED:
        default:
            return "Undefined";
        }
    }

    DcElemType stringToElem(std::string name)
    {
        if (name == "Constant")
            return DC_ELEM_TYPE_CONSTANT;
        if (name == "Container")
            return DC_ELEM_TYPE_CONTAINER;
        if (name == "DCAPP")
            return DC_ELEM_TYPE_DCAPP;
        if (name == "DEM")
            return DC_ELEM_TYPE_DEM;
        if (name == "Dummy")
            return DC_ELEM_TYPE_DUMMY;
        if (name == "Include")
            return DC_ELEM_TYPE_INCLUDE;
        if (name == "Logic")
            return DC_ELEM_TYPE_LOGIC;
        if (name == "Map")
            return DC_ELEM_TYPE_MAP;
        if (name == "Panel")
            return DC_ELEM_TYPE_PANEL;
        if (name == "Polygon")
            return DC_ELEM_TYPE_POLYGON;
        if (name == "Variable")
            return DC_ELEM_TYPE_VARIABLE;
        if (name == "Vertex")
            return DC_ELEM_TYPE_VERTEX;
        if (name == "Window")
            return DC_ELEM_TYPE_WINDOW;
        return DC_ELEM_TYPE_UNDEFINED;
    }

    DcElemType xmlNodeToElementType(xmlNodePtr node)
    {
        if (node && node->type == XML_ELEMENT_NODE)
        {
            std::string name = (char *)(node->name);
            DcElemType type = stringToElem(name);
            if (type == DC_ELEM_TYPE_UNDEFINED)
            {
                throw std::runtime_error("Undefined element name: " + name);
            }
            return type;
        }
        return DC_ELEM_TYPE_NONELEM;
    }

    void setConstant(const std::string &name, const std::string &text)
    {
        dcData.constants[name] = text;
    }

    std::string getConstant(const std::string &name)
    {
        if (dcData.constants.count(name))
        {
            return dcData.constants[name];
        }
        return "";
    }

    std::string dereferenceConstants(const char *text)
    {
        return dereferenceConstants(std::string(text));
    }

    std::string dereferenceConstants(std::string text)
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
                subtext = dereferenceConstants(subtext);

                // if constant, pull value from list of constants
                if (text[ii] == '#')
                {
                    output += getConstant(subtext);
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

    DcValue *indexToDcValue(DcValueIndex index)
    {
        return &(dcData.values[index]);
    }

    DcValueIndex registerDcValue(DcValue value)
    {
        dcData.values.push_back(value);
        return dcData.values.size() - 1;
    }

    DcValueIndex createAndRegisterTypedDcValueFromString(DcValueType type, const char *text)
    {
        return createAndRegisterTypedDcValueFromString(type, std::string(text));
    }

    DcValueIndex createAndRegisterTypedDcValueFromString(DcValueType type, std::string text)
    {
        // check for variable
        std::string cleanedValue = trimWhitespace(text);
        if (cleanedValue.length() > 1 && cleanedValue[0] == '@')
        {
            std::string variableName = cleanedValue.substr(1);
            if (dcData.variables.count(variableName))
            {
                return dcData.variables[variableName].valueIndex;
            }
            throw std::runtime_error("Non-existant variable @" + variableName);
        }

        // otherwise create new DcValue and return its index
        return registerDcValue(createTypedValueFromString(type, text));
    }

    void setVariable(const std::string &name, DcValueIndex valueIndex)
    {
        if (dcData.variables.count(name))
        {
            throw std::runtime_error("Duplicate variable for name " + name);
        }
        dcData.variables[name] = (DcVariable){
            nullptr,
            valueIndex,
        };
    }

    std::string dcNodeTypeToString(DcNodeType type)
    {
        switch (type)
        {
        case DC_NODE_TYPE_CONTAINER:
            return "Container";
        case DC_NODE_TYPE_CONDITIONAL:
            return "Conditional";
        case DC_NODE_TYPE_MAP:
            return "Map";
        case DC_NODE_TYPE_PANEL:
            return "Panel";
        case DC_NODE_TYPE_POLYGON:
            return "Polygon";
        case DC_NODE_TYPE_SET:
            return "Set";
        case DC_NODE_TYPE_WINDOW:
            return "Window";
        default:
            throw std::runtime_error("Invalid DcNode type");
        }
    }

    DcNode *indexToDcNode(DcNodeIndex index)
    {
        if (index == DC_NODE_INDEX_UNDEFINED)
        {
            return nullptr;
        }
        return &(dcData.nodes[index]);
    }

    DcNodeIndex registerDcNode(DcNode node)
    {
        dcData.nodes.push_back(node);
        return dcData.nodes.size() - 1;
    }

    void cleanXmlData()
    {
        // get root element
        xmlNodePtr node = xmlDocGetRootElement(dcData.doc);
        if (node == NULL)
        {
            throw std::runtime_error("Unable to get root element of configuration file: " + dcData.configFilePath);
        }

        // verify root node is valid
        if (xmlNodeToElementType(node) != DC_ELEM_TYPE_DCAPP)
        {
            throw std::runtime_error("Configuration root element is not DCAPP: " + dcData.configFilePath);
        }

        // clean XML file
        cleanXmlNode(node, dcData.configFilePath);
    }

    void cleanXmlNode(xmlNodePtr node, std::string directory)
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
            switch (xmlNodeToElementType(node))
            {

            case DC_ELEM_TYPE_CONSTANT:
            {
                char *cName = getAttributeString(node, "Name");
                if (!cName)
                {
                    throw std::runtime_error("Non-existent node 'Name' in <Constant> definition");
                }
                std::string name = dereferenceConstants(cName);
                free(cName);

                char *cValue = getNodeContentString(node);
                if (!cValue)
                {
                    throw std::runtime_error("Non-existent node content in <Constant> definition");
                }
                std::string value = dereferenceConstants(cValue);
                free(cValue);

                setConstant(name, value);
                break;
            }

            // remove "Dummy" level, keep children
            case DC_ELEM_TYPE_DUMMY:
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

            case DC_ELEM_TYPE_INCLUDE:
            {

                // get include file name
                char *cIncludeFilePath = getNodeContentString(node);
                if (cIncludeFilePath == nullptr)
                {
                    throw std::runtime_error("Missing file path for <Include> element");
                }
                std::string includeFilePath = dereferenceConstants(cIncludeFilePath);
                free(cIncludeFilePath);

                // expand constants, go to canonical
                includeFilePath = filepathToCanonical(includeFilePath, directory);

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
                cleanXmlNode(child, directory);
                child = childNext;
            }
        }
    }

    void initData()
    {
        // initialize members
        dcData.configFilePath = "";
        dcData.configDirPath = "";
        dcData.cacheDirPath = "";
        dcData.logDirPath = "";
        dcData.constants.clear();
        dcData.variables.clear();
        dcData.values.resize(1);
        dcData.nodes.resize(1);
        dcData.window = DC_NODE_INDEX_UNDEFINED;
        dcData.logic = (DcLogic){
            .library = nullptr,
            .preInit = nullptr,
            .init = nullptr,
            .draw = nullptr,
            .close = nullptr,
        };
        dcData.doc = nullptr;

        // set default constants
        setConstant("_align_left_", std::to_string(DC_ALIGN_TYPE_LEFT));
        setConstant("_align_center_", std::to_string(DC_ALIGN_TYPE_CENTER));
        setConstant("_align_right_", std::to_string(DC_ALIGN_TYPE_RIGHT));
        setConstant("_align_bottom_", std::to_string(DC_ALIGN_TYPE_BOTTOM));
        setConstant("_align_middle_", std::to_string(DC_ALIGN_TYPE_MIDDLE));
        setConstant("_align_top_", std::to_string(DC_ALIGN_TYPE_TOP));

        setConstant("_conditional_true_", std::to_string(DC_CONDITIONAL_TYPE_TRUE));
        setConstant("_conditional_false_", std::to_string(DC_CONDITIONAL_TYPE_FALSE));
        setConstant("_conditional_eq_", std::to_string(DC_CONDITIONAL_TYPE_EQ));
        setConstant("_conditional_ne_", std::to_string(DC_CONDITIONAL_TYPE_NE));
        setConstant("_conditional_lt_", std::to_string(DC_CONDITIONAL_TYPE_LT));
        setConstant("_conditional_gt_", std::to_string(DC_CONDITIONAL_TYPE_GT));
        setConstant("_conditional_lte_", std::to_string(DC_CONDITIONAL_TYPE_LTE));
        setConstant("_conditional_gte_", std::to_string(DC_CONDITIONAL_TYPE_GTE));

        setConstant("_color_black_", "0 0 0");
        setConstant("_color_blue_", "0 0 1");
        setConstant("_color_green_", "0 1 0");
        setConstant("_color_cyan_", "0 1 1");
        setConstant("_color_red_", "1 0 0");
        setConstant("_color_magenta_", "1 0 1");
        setConstant("_color_yellow_", "1 1 0");
        setConstant("_color_white_", "1 1 1");
    }
} // namespace dc
