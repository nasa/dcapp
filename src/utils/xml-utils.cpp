// dcapp includes
#include <utils/xml-utils.hpp>

// library includes

// c++ standard includes

char *getAttributeString(xmlNodePtr node, std::string attr)
{
    return (char *)xmlGetProp(node, (xmlChar *)(attr.c_str()));
}

char *getNodeContentString(xmlNodePtr node)
{
    return (char *)xmlNodeGetContent(node);
}
