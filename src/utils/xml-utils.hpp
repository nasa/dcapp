#pragma once

// dcapp includes

// library includes
#include <libxml/parser.h>

// c++ standard includes
#include <string>

char* getAttributeString(xmlNodePtr node, std::string attr);
char* getNodeContentString(xmlNodePtr node);
