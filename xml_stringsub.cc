#include <list>
#include <string>
#include <map>
#include <libxml/parser.h>
#include "xml_utils.hh"

struct xmlStyle
{
    char *name;
    xmlNodePtr node;
};

static std::map<std::string, std::string> arglist;
static std::map<std::string, std::string> ppclist;
static std::list<xmlNodePtr> xmldefaults;
static std::list<struct xmlStyle> xmlstyles;

static const char *get_constval(char *instr)
{
    // First, check arguments list...
    if (arglist.find(instr) != arglist.end()) return arglist[instr].c_str();

    // Then, check to see if the constant has been set in the XML file hierarchy...
    if (ppclist.find(instr) != ppclist.end()) return ppclist[instr].c_str();

    // If no match found in either list, return NULL
    return 0x0;
}

static void replace_string(char **instr)
{
    int count = 0, i, j, len, outlen, repl_len, in_start, in_end, start, end, outptr = 0;
    char *in_val, *outstr = 0x0;
    const char *repl_val;

    if (!instr) return;
    if (!(*instr)) return;

    len = strlen(*instr);
    for (i=0; i<len; i++)
    {
        if ((*instr)[i] == '#' || (*instr)[i] == '$') count++;
    }
    if (!count) return;

    outlen = len;
    outstr = (char *)calloc(outlen+1, sizeof(char));

    for (i=0; i<len; i++)
    {
        if ((*instr)[i] == '#' || (*instr)[i] == '$')
        {
            start = -1;
            end = -1;
            in_start = i;
            if ((*instr)[i+1] == '{')
            {
                start = i+2;
                for (j=start; j<len; j++)
                {
                    if ((*instr)[j] == '}')
                    {
                        end = j-1;
                        in_end = j;
                        break;
                    }
                }
            }
            else
            {
                start = i+1;
                for (j=start; j<len; j++)
                {
                    if (!isalnum((*instr)[j]) && (*instr)[j] != '_')
                    {
                        end = j-1;
                        in_end = j-1;
                        break;
                    }
                }
                if (end < 0)
                {
                    end = len;
                    in_end = len;
                }
            }

            if (start >= 0 && end >= start)
            {
                in_val = (char *)calloc(2+end-start, sizeof(char));
                strncpy(in_val, &(*instr)[start], 1+end-start);

                if ((*instr)[i] == '#') repl_val = get_constval(in_val);
                else repl_val = getenv(in_val);

                if (repl_val) repl_len = strlen(repl_val);
                else repl_len = 0;

                outlen += repl_len - (1+end-start);
                outstr = (char *)realloc(outstr, outlen+1);

                for (j=0; j<repl_len; j++) outstr[outptr++] = repl_val[j];

                repl_val = 0x0;
                free(in_val);
                i += in_end - in_start;
            }
        }
        else if ((*instr)[i] == '\\' && ((*instr)[i+1] == '#' || (*instr)[i+1] == '$')) outstr[outptr++] = (*instr)[++i];
        else outstr[outptr++] = (*instr)[i];
    }

    if (outstr)
    {
        outstr[outptr] = '\0';
// This memory should be freed, but I commented this out to avoid "pointer being freed was not allocated" errors
//        free(*instr);
        *instr = outstr;
    }
}

char *get_node_content(xmlNodePtr node)
{
    char **retval = get_XML_content_address(node);
    if (retval)
    {
        if (*retval)
        {
            replace_string(retval);
            return *retval;
        }
    }
    return 0x0;
}

char *get_element_data(xmlNodePtr innode, const char *key)
{
    char **retval = get_XML_attribute_address(innode, key);
    if (retval)
    {
        if (*retval)
        {
            replace_string(retval);
            return *retval;
        }
    }

    char *type = get_node_type(innode);

    // If not explicitly defined, check to see if a Style has been defined...
    char *style = get_XML_attribute(innode, "Style");
    if (style)
    {
        std::list<struct xmlStyle>::iterator xmls;
        for (xmls = xmlstyles.begin(); xmls != xmlstyles.end(); xmls++)
        {
            if (NodeCheck(xmls->node, type) && !strcmp(xmls->name, style))
            {
                retval = get_XML_attribute_address(xmls->node, key);
                if (retval)
                {
                    if (*retval)
                    {
                        replace_string(retval);
                        return *retval;
                    }
                }
            }
        }
    }

    // ...if not, check to see if a Default has been defined...
    std::list<xmlNodePtr>::iterator xmld;
    for (xmld = xmldefaults.begin(); xmld != xmldefaults.end(); xmld++)
    {
        if (NodeCheck(*xmld, type))
        {
            retval = get_XML_attribute_address(*xmld, key);
            if (retval)
            {
                if (*retval)
                {
                    replace_string(retval);
                    return *retval;
                }
            }
        }
    }

    // ...if not, return NULL
    return 0x0;
}

void processArgument(const char *key, const char *value)
{
    arglist[std::string(key)] = std::string(value);
}

void processConstantNode(xmlNodePtr node)
{
    ppclist[std::string(get_element_data(node, "Name"))] = std::string(get_node_content(node));
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
            mystruct.node = node1;
            xmlstyles.push_front(mystruct);
        }
    }
}

void processDefaultsNode(xmlNodePtr node)
{
    xmlNodePtr node1;

    for (node1 = node->children; node1; node1 = node1->next)
    {
        if (NodeValid(node1)) xmldefaults.push_front(node1);
    }
}
