#include <list>
#include <string>
#include <map>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <libxml/parser.h>
#include "xml_data.hh"
#include "xml_utils.hh"

struct xmlStyle
{
    std::string name;
    xmlNodePtr node;
};

static std::map<std::string, std::string> arglist;
static std::map<std::string, std::string> ppclist;
static std::list<xmlNodePtr> xmldefaults;
static std::list<struct xmlStyle> xmlstyles;

static std::string replace_string(const std::string &instr)
{
    if (instr.find_first_of("#$") == std::string::npos) return instr;

    std::string outstr;
    
    for (unsigned i=0; i<instr.length(); i++)
    {
        if (instr[i] == '\\' && i+1 < instr.length() && (instr[i+1] == '#' || instr[i+1] == '$'))
        {
            outstr += instr[i+1];
            i++;
        }
        else if (instr[i] == '#' || instr[i] == '$')
        {
            unsigned brackets = 0; // keep track of how many characters are brackets
            std::string evalstr, tmpstr;

            if (i+1 < instr.length() && instr[i+1] == '{')
            {
                brackets = 2;
                for (unsigned j=i+2, count=1; count>0 && j<instr.length(); j++)
                {
                    if (instr[j] == '{') count++;
                    else if (instr[j] == '}') count--;
                    if (count) evalstr += instr[j];
                }
            }
            else
            {
                for (unsigned j=i+1; j<instr.length() && (isalnum(instr[j]) || instr[j] == '_'); j++) evalstr += instr[j];
            }

            tmpstr = replace_string(evalstr);

            if (instr[i] == '#')
            {
                // First, check arguments list...
                std::map<std::string, std::string>::iterator it = arglist.find(tmpstr);
                if (it != arglist.end()) outstr += it->second;
                else
                {
                    // If not in arguments list, check if the constant has been set in the XML file hierarchy...
                    it = ppclist.find(tmpstr);
                    if (it != ppclist.end()) outstr += it->second;
                }
            }
            else
            {
                char *myenv = getenv(tmpstr.c_str());
                if (myenv) outstr += myenv;
            }

            i += evalstr.length() + brackets;
        }
        else outstr += instr[i];
    }

    return outstr;
}

xmldata get_node_content(xmlNodePtr node)
{
    char *mycontent = get_XML_content(node);
    if (mycontent) return replace_string(mycontent);
    else return 0x0;
}

xmldata get_element_data(xmlNodePtr innode, const char *key)
{
    char *myattr;

    myattr = get_XML_attribute(innode, key);
    if (myattr) return replace_string(myattr);

    char *type = get_node_type(innode);

    // If not explicitly defined, check to see if a Style has been defined...
    char *style = get_XML_attribute(innode, "Style");
    if (style)
    {
        std::string mystyle = replace_string(style);
        std::list<struct xmlStyle>::iterator xmls;
        for (xmls = xmlstyles.begin(); xmls != xmlstyles.end(); xmls++)
        {
            if (NodeCheck(xmls->node, type) && mystyle == xmls->name)
            {
                myattr = get_XML_attribute(xmls->node, key);
                if (myattr) return replace_string(myattr);
            }
        }
    }

    // ...if not, check to see if a Default has been defined...
    std::list<xmlNodePtr>::iterator xmld;
    for (xmld = xmldefaults.begin(); xmld != xmldefaults.end(); xmld++)
    {
        if (NodeCheck(*xmld, type))
        {
            myattr = get_XML_attribute(*xmld, key);
            if (myattr) return replace_string(myattr);
        }
    }

    // ...if not, return undefined xmlstring
    return 0x0;
}

void processArgument(const char *key, const char *value)
{
    arglist[std::string(key)] = std::string(value);
}

void processConstantNode(xmlNodePtr node)
{
    ppclist[get_element_data(node, "Name")] = get_node_content(node);
}

void processStyleNode(xmlNodePtr node)
{
    xmlNodePtr node1;

    for (node1 = node->children; node1; node1 = node1->next)
    {
        if (NodeValid(node1))
        {
            struct xmlStyle mystruct;
            mystruct.name = get_element_data(node, "Name");
            mystruct.node = xmlCopyNode(node1, 1);
            xmlstyles.push_front(mystruct);
        }
    }
}

void processDefaultsNode(xmlNodePtr node)
{
    xmlNodePtr node1;

    for (node1 = node->children; node1; node1 = node1->next)
    {
        if (NodeValid(node1)) xmldefaults.push_front(xmlCopyNode(node1, 1));
    }
}
