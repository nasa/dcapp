#pragma once

// dcapp includes

// library includes
#include <libxml/parser.h>

// c++ standard includes
#include <string>

char *dc_utils_get_attribute_string(xmlNodePtr node, std::string attr);
char *dc_utils_get_node_content_string(xmlNodePtr node);
