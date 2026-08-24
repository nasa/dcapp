#ifndef DC_APP_XML_ELEMENT_H
#define DC_APP_XML_ELEMENT_H

#include "xml_element_types.h"

struct _xmlNode;

#ifdef __cplusplus
extern "C" {
#endif

//~ element name conversion

const char *dc_app_xml_element_type_to_string(DcAppXmlElementType type);
DcAppXmlElementType dc_app_xml_element_type_from_string(const char *name);
DcAppXmlElementType dc_app_xml_element_type_from_xml_node(struct _xmlNode *node);

#ifdef __cplusplus
}
#endif

#endif
