#include "xml.h"

char *dc_utils_get_attribute_string(xmlNodePtr node, const char *attr) {
    return (char *)xmlGetProp(node, (xmlChar *)(attr));
}

char *dc_utils_get_node_content_string(xmlNodePtr node) {
    return (char *)xmlNodeGetContent(node);
}
