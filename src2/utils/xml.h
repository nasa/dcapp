#ifndef _DC_UTILS_XML_
#define _DC_UTILS_XML_

// TODO want to eventually make this a full blown simplified version of libxml2
// to remove the dependency and bloat. But keep using it for now
#include <libxml/parser.h>

#ifdef __cplusplus
extern "C" {
#endif

char *dc_utils_get_attribute_string(xmlNodePtr node, const char *attr);
char *dc_utils_get_node_content_string(xmlNodePtr node);

#ifdef __cplusplus
}
#endif

#endif
