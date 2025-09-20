// dcapp includes
#include <utils/xml.hpp>

// library includes

// c++ standard includes

char *dc_utils_get_attribute_string(xmlNodePtr node, std::string attr) {
    return (char *)xmlGetProp(node, (xmlChar *)(attr.c_str()));
}

char *dc_utils_get_node_content_string(xmlNodePtr node) {
    return (char *)xmlNodeGetContent(node);
}
