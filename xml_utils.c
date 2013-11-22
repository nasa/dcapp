#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

int NodeCheck(xmlNodePtr, const char *);
static int xmlStrEq(const xmlChar *, const char *);

int XMLFileOpen(xmlDocPtr *mydoc, xmlNodePtr *root_element, const char *filename, const char *req_name)
{
    *mydoc = xmlReadFile(filename, NULL, 0);
    if (*mydoc == NULL)
    {
        printf("dcapp: Couldn't process XML file: %s\n", filename);
        return (-1);
    }

    if (xmlXIncludeProcess(*mydoc) < 0)
    {
        printf("dcapp: XInclude processing failed in XML file: %s\n", filename);
    }

    *root_element = xmlDocGetRootElement(*mydoc);
    if (*root_element == NULL)
    {
        printf("dcapp: Couldn't find root element in XML file: %s\n", filename);
        return (-1);
    }

    if (req_name)
    {
        if (!NodeCheck(*root_element, req_name))
        {
            printf("dcapp: Bad root element in XML file: \"%s\" (should be \"%s\")", (*root_element)->name, req_name);
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
    xmlAttrPtr attr;

    for (attr = node->properties; attr != NULL; attr = attr->next)
    {
        if (xmlStrEq(attr->name, key))
        {
            if (attr->children != NULL) return (char **)&(attr->children->content);
        }
    }
    return NULL;
}

char *get_XML_attribute(xmlNodePtr node, const char *key)
{
    xmlAttrPtr attr;

    for (attr = node->properties; attr != NULL; attr = attr->next)
    {
        if (xmlStrEq(attr->name, key))
        {
            if (attr->children != NULL) return ((char *)attr->children->content);
        }
    }
    return NULL;
}

char **get_XML_content_address(xmlNodePtr node)
{
    if (node->xmlChildrenNode != NULL) return (char **)&(node->xmlChildrenNode->content);
    return NULL;
}

char *get_XML_content(xmlNodePtr node)
{
    if (node->xmlChildrenNode != NULL) return ((char *)node->xmlChildrenNode->content);
    return NULL;
}

char *get_node_type(xmlNodePtr node)
{
    if (node->type == XML_ELEMENT_NODE) return (char *)(node->name);
    return NULL;
}

int NodeValid(xmlNodePtr node)
{
    if (node->type == XML_ELEMENT_NODE) return 1;
    else return 0;
}

int NodeCheck(xmlNodePtr node, const char *str)
{
    if (node->type == XML_ELEMENT_NODE && xmlStrEq(node->name, str)) return 1;
    else return 0;
}

xmlNodePtr NodeFind(xmlNodePtr startnode, const char *key)
{
    xmlNodePtr node, child;

    for (node = startnode; node != NULL; node = node->next)
    {
        if (NodeCheck(node, key)) return node;
        else
        {
            child = NodeFind(node->children, key);
            if (child) return child;
        }
    }

    return NULL;
}

static int xmlStrEq(const xmlChar *xmlstr, const char *charstr)
{
    if (strcmp((const char *)xmlstr, charstr)) return 0;
    else return 1;
}
