#ifndef _DCAPP_XML_H_
#define _DCAPP_XML_H_

#include "dcapp.h"

_NodeIndex dc_app_process_xml_node(_AppData *app_data, xmlNodePtr xml_node, _NodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);

#endif
