#ifndef _DC_APP_CONFIG_
#define _DC_APP_CONFIG_

#include "lookup.h"

#include <stddef.h>
#include <stdint.h>

#include <libxml/parser.h>

// element
typedef enum _DcAppXmlElemType {
    DC_APP_XML_ELEM_TYPE_UNDEFINED,
    DC_APP_XML_ELEM_TYPE_CONSTANT,
    DC_APP_XML_ELEM_TYPE_CONTAINER,
    DC_APP_XML_ELEM_TYPE_DCAPP,
    DC_APP_XML_ELEM_TYPE_DUMMY,
    DC_APP_XML_ELEM_TYPE_FALSE,
    DC_APP_XML_ELEM_TYPE_IF,
    DC_APP_XML_ELEM_TYPE_INCLUDE,
    DC_APP_XML_ELEM_TYPE_LOGIC,
    DC_APP_XML_ELEM_TYPE_NONELEM,
    DC_APP_XML_ELEM_TYPE_PANEL,
    DC_APP_XML_ELEM_TYPE_POLYGON,
    DC_APP_XML_ELEM_TYPE_SET,
    DC_APP_XML_ELEM_TYPE_TERRAIN,
    DC_APP_XML_ELEM_TYPE_TERRAIN_DEM,
    DC_APP_XML_ELEM_TYPE_TEXT,
    DC_APP_XML_ELEM_TYPE_TRICK_FROM,
    DC_APP_XML_ELEM_TYPE_TRICK_IO,
    DC_APP_XML_ELEM_TYPE_TRICK_TO,
    DC_APP_XML_ELEM_TYPE_TRICK_VARIABLE,
    DC_APP_XML_ELEM_TYPE_TRUE,
    DC_APP_XML_ELEM_TYPE_VARIABLE,
    DC_APP_XML_ELEM_TYPE_VERTEX,
    DC_APP_XML_ELEM_TYPE_WINDOW,
} DcAppXmlElemType;

#ifdef __cplusplus
extern "C" {
#endif

const char      *dc_app_elem_type_to_string(DcAppXmlElemType type);
DcAppXmlElemType dc_app_string_to_elem_type(const char *name);
DcAppXmlElemType dc_app_xml_node_to_elem_type(xmlNodePtr node);

void dc_app_init_data();
void dc_app_clean_xml_data();

#ifdef __cplusplus
}
#endif

#endif
