#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>
#include "msg.hh"

bool NodeCheck(xmlNodePtr, const char *);
static bool xmlStrEq(const xmlChar *, const char *);

int XMLFileOpen(xmlDocPtr *mydoc, xmlNodePtr *root_element, const char *filename, const char *req_name)
{
    *mydoc = xmlReadFile(filename, 0x0, 0);
    if (!(*mydoc))
    {
        error_msg("Couldn't process XML file: " << filename);
        return (-1);
    }

    if (xmlXIncludeProcess(*mydoc) < 0)
    {
        error_msg("XInclude processing failed in XML file: " << filename);
    }

    *root_element = xmlDocGetRootElement(*mydoc);
    if (!(*root_element))
    {
        error_msg("Couldn't find root element in XML file: " << filename);
        return (-1);
    }

    if (req_name)
    {
        if (!NodeCheck(*root_element, req_name))
        {
            error_msg("Bad root element in XML file: \"" << (*root_element)->name << "\" (should be \"" << req_name << "\")");
            return (-1);
        }
    }

    return 0;
}

void XMLFileClose(xmlDocPtr mydoc)
{
    xmlFreeDoc(mydoc);
    xmlCleanupParser();
}

char **get_XML_attribute_address(xmlNodePtr node, const char *key)
{
    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
    {
        if (xmlStrEq(attr->name, key))
        {
            if (attr->children) return (char **)&(attr->children->content);
        }
    }
    return 0x0;
}

char *get_XML_attribute(xmlNodePtr node, const char *key)
{
    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
    {
        if (xmlStrEq(attr->name, key))
        {
            if (attr->children) return ((char *)attr->children->content);
        }
    }
    return 0x0;
}

char **get_XML_content_address(xmlNodePtr node)
{
    if (node->xmlChildrenNode) return (char **)&(node->xmlChildrenNode->content);
    else return 0x0;
}

char *get_XML_content(xmlNodePtr node)
{
    if (node->xmlChildrenNode) return ((char *)node->xmlChildrenNode->content);
    else return 0x0;
}

char *get_node_type(xmlNodePtr node)
{
    if (node->type == XML_ELEMENT_NODE) return (char *)(node->name);
    return 0x0;
}

bool NodeValid(xmlNodePtr node)
{
    if (node->type == XML_ELEMENT_NODE) return true;
    else return false;
}

bool NodeCheck(xmlNodePtr node, const char *str)
{
    if (node->type == XML_ELEMENT_NODE && xmlStrEq(node->name, str)) return true;
    else return false;
}

xmlNodePtr NodeFind(xmlNodePtr startnode, const char *key)
{
    xmlNodePtr node, child;

    for (node = startnode; node; node = node->next)
    {
        if (NodeCheck(node, key)) return node;
        else
        {
            child = NodeFind(node->children, key);
            if (child) return child;
        }
    }

    return 0x0;
}

static bool xmlStrEq(const xmlChar *xmlstr, const char *charstr)
{
    if (!strcmp((const char *)xmlstr, charstr)) return true;
    else return false;
}
