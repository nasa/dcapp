#ifndef _XML_STRINGSUB_HH_
#define _XML_STRINGSUB_HH_

#include <libxml/parser.h>

extern const char *get_node_content(xmlNodePtr);
extern const char *get_element_data(xmlNodePtr, const char *);
extern void processArgument(const char *, const char *);
extern void processConstantNode(xmlNodePtr);
extern void processStyleNode(xmlNodePtr);
extern void processDefaultsNode(xmlNodePtr);

#endif
