#ifndef _DC_APP_ELEM_
#define _DC_APP_ELEM_

typedef struct _xmlNode *xmlNodePtr;

// element types
typedef enum _DcAppElemType {
    DC_APP_ELEM_TYPE_UNDEFINED,
    DC_APP_ELEM_TYPE_CONSTANT,
    DC_APP_ELEM_TYPE_CONTAINER,
    DC_APP_ELEM_TYPE_DCAPP,
    DC_APP_ELEM_TYPE_DEFAULT,
    DC_APP_ELEM_TYPE_DUMMY,
    DC_APP_ELEM_TYPE_FALSE,
    DC_APP_ELEM_TYPE_IF,
    DC_APP_ELEM_TYPE_INCLUDE,
    DC_APP_ELEM_TYPE_LOGIC,
    DC_APP_ELEM_TYPE_MOUSE_ACTIVE,
    DC_APP_ELEM_TYPE_MOUSE_HOVERED,
    DC_APP_ELEM_TYPE_MOUSE_INACTIVE,
    DC_APP_ELEM_TYPE_MOUSE_PRESSED,
    DC_APP_ELEM_TYPE_MOUSE_RELEASED,
    DC_APP_ELEM_TYPE_NONELEM,
    DC_APP_ELEM_TYPE_PANEL,
    DC_APP_ELEM_TYPE_POLYGON,
    DC_APP_ELEM_TYPE_SET,
    DC_APP_ELEM_TYPE_STYLE,
    DC_APP_ELEM_TYPE_TERRAIN,
    DC_APP_ELEM_TYPE_TERRAIN_DEM,
    DC_APP_ELEM_TYPE_TEXT,
    DC_APP_ELEM_TYPE_TRICK_FROM,
    DC_APP_ELEM_TYPE_TRICK_IO,
    DC_APP_ELEM_TYPE_TRICK_TO,
    DC_APP_ELEM_TYPE_TRICK_VARIABLE,
    DC_APP_ELEM_TYPE_TRUE,
    DC_APP_ELEM_TYPE_VARIABLE,
    DC_APP_ELEM_TYPE_VERTEX,
    DC_APP_ELEM_TYPE_WINDOW,
    DC_APP_ELEM_TYPE__COUNT,
    DC_APP_ELEM_TYPE__MAX = DC_APP_ELEM_TYPE__COUNT - 1
} DcAppElemType;

#ifdef __cplusplus
extern "C" {
#endif

// element functions
const char   *dc_app_elem_type_to_string(DcAppElemType type);
DcAppElemType dc_app_string_to_elem_type(const char *name);
DcAppElemType dc_app_xml_node_to_elem_type(xmlNodePtr node);

#ifdef __cplusplus
}
#endif

#endif
