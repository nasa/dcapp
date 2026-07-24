#ifndef DC_APP_ELEM_H
#define DC_APP_ELEM_H

#include "elem_types.h"

struct _xmlNode;

#ifdef __cplusplus
extern "C" {
#endif

// element functions
const char   *dc_app_elem_type_to_string(DcAppElemType type);
DcAppElemType dc_app_elem_type_from_string(const char *name);
DcAppElemType dc_app_elem_type_from_xml_node(struct _xmlNode *node);

#ifdef __cplusplus
}
#endif

#endif
