#ifndef _XML_STRINGSUB_HH_
#define _XML_STRINGSUB_HH_

#include <libxml/parser.h>
#include "xml_data.hh"

extern xmldata get_node_content(xmlNodePtr);
extern xmldata get_element_data(xmlNodePtr, const char *);
extern void processArgument(const char *, const char *);
extern void processConstantNode(xmlNodePtr);
extern void processStyleNode(xmlNodePtr);
extern void processDefaultsNode(xmlNodePtr);

#endif
