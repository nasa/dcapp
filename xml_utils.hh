#ifndef _XML_UTILS_HH_
#define _XML_UTILS_HH_

#include <libxml/parser.h>

extern int XMLFileOpen(xmlDocPtr *, xmlNodePtr *, const char *);
int XMLParseString(xmlDocPtr *mydoc, xmlNodePtr *root_element, const char *pDataA );
extern void XMLFileClose(xmlDocPtr);
extern void XMLEndParsing(void);
extern char *get_XML_attribute(xmlNodePtr, const char *);
extern char *get_XML_content(xmlNodePtr);
extern char *get_node_type(xmlNodePtr);
extern bool NodeValid(xmlNodePtr node);
extern bool NodeCheck(xmlNodePtr, const char *);
extern xmlNodePtr NodeFind(xmlNodePtr, const char *);

#endif
