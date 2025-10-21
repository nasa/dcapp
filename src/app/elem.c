#include "elem.h"

#include <string.h>

#include <libxml/parser.h>

const char *dc_app_elem_type_to_string(DcAppElemType type) {
    switch (type) {
        case DC_APP_ELEM_TYPE_CONSTANT:
            return "Constant";
        case DC_APP_ELEM_TYPE_CONTAINER:
            return "Container";
        case DC_APP_ELEM_TYPE_DCAPP:
            return "DCAPP";
        case DC_APP_ELEM_TYPE_DEFAULT:
            return "Default";
        case DC_APP_ELEM_TYPE_DUMMY:
            return "Dummy";
        case DC_APP_ELEM_TYPE_FALSE:
            return "False";
        case DC_APP_ELEM_TYPE_IF:
            return "If";
        case DC_APP_ELEM_TYPE_INCLUDE:
            return "Include";
        case DC_APP_ELEM_TYPE_LOGIC:
            return "Logic";
        case DC_APP_ELEM_TYPE_NONELEM:
            return "<Non-Element>";
        case DC_APP_ELEM_TYPE_PANEL:
            return "Panel";
        case DC_APP_ELEM_TYPE_POLYGON:
            return "Polygon";
        case DC_APP_ELEM_TYPE_SET:
            return "Set";
        case DC_APP_ELEM_TYPE_STYLE:
            return "Style";
        case DC_APP_ELEM_TYPE_TERRAIN:
            return "Terrain";
        case DC_APP_ELEM_TYPE_TERRAIN_DEM:
            return "TerrainDEM";
        case DC_APP_ELEM_TYPE_TEXT:
            return "Text";
        case DC_APP_ELEM_TYPE_TRICK_FROM:
            return "FromTrick";
        case DC_APP_ELEM_TYPE_TRICK_IO:
            return "TrickIO";
        case DC_APP_ELEM_TYPE_TRICK_TO:
            return "ToTrick";
        case DC_APP_ELEM_TYPE_TRICK_VARIABLE:
            return "TrickVariable";
        case DC_APP_ELEM_TYPE_TRUE:
            return "True";
        case DC_APP_ELEM_TYPE_VARIABLE:
            return "Variable";
        case DC_APP_ELEM_TYPE_VERTEX:
            return "Vertex";
        case DC_APP_ELEM_TYPE_WINDOW:
            return "Window";
        case DC_APP_ELEM_TYPE_UNDEFINED:
        default:
            return "Undefined";
    }
}

DcAppElemType dc_app_string_to_elem_type(const char *name) {
    if (strcmp(name, "Constant") == 0)
        return DC_APP_ELEM_TYPE_CONSTANT;
    if (strcmp(name, "Container") == 0)
        return DC_APP_ELEM_TYPE_CONTAINER;
    if (strcmp(name, "DCAPP") == 0)
        return DC_APP_ELEM_TYPE_DCAPP;
    if (strcmp(name, "Default") == 0)
        return DC_APP_ELEM_TYPE_DEFAULT;
    if (strcmp(name, "Dummy") == 0)
        return DC_APP_ELEM_TYPE_DUMMY;
    if (strcmp(name, "False") == 0)
        return DC_APP_ELEM_TYPE_FALSE;
    if (strcmp(name, "FromTrick") == 0)
        return DC_APP_ELEM_TYPE_TRICK_FROM;
    if (strcmp(name, "If") == 0)
        return DC_APP_ELEM_TYPE_IF;
    if (strcmp(name, "Include") == 0)
        return DC_APP_ELEM_TYPE_INCLUDE;
    if (strcmp(name, "Logic") == 0)
        return DC_APP_ELEM_TYPE_LOGIC;
    if (strcmp(name, "Panel") == 0)
        return DC_APP_ELEM_TYPE_PANEL;
    if (strcmp(name, "Polygon") == 0)
        return DC_APP_ELEM_TYPE_POLYGON;
    if (strcmp(name, "Set") == 0)
        return DC_APP_ELEM_TYPE_SET;
    if (strcmp(name, "Style") == 0)
        return DC_APP_ELEM_TYPE_STYLE;
    if (strcmp(name, "Terrain") == 0)
        return DC_APP_ELEM_TYPE_TERRAIN;
    if (strcmp(name, "TerrainDEM") == 0)
        return DC_APP_ELEM_TYPE_TERRAIN_DEM;
    if (strcmp(name, "Text") == 0)
        return DC_APP_ELEM_TYPE_TEXT;
    if (strcmp(name, "ToTrick") == 0)
        return DC_APP_ELEM_TYPE_TRICK_TO;
    if (strcmp(name, "TrickIO") == 0)
        return DC_APP_ELEM_TYPE_TRICK_IO;
    if (strcmp(name, "TrickVariable") == 0)
        return DC_APP_ELEM_TYPE_TRICK_VARIABLE;
    if (strcmp(name, "True") == 0)
        return DC_APP_ELEM_TYPE_TRUE;
    if (strcmp(name, "Variable") == 0)
        return DC_APP_ELEM_TYPE_VARIABLE;
    if (strcmp(name, "Vertex") == 0)
        return DC_APP_ELEM_TYPE_VERTEX;
    if (strcmp(name, "Window") == 0)
        return DC_APP_ELEM_TYPE_WINDOW;
    return DC_APP_ELEM_TYPE_UNDEFINED;
}

DcAppElemType dc_app_xml_node_to_elem_type(xmlNodePtr node) {
    if (node && node->type == XML_ELEMENT_NODE) {
        const char   *name = (const char *)(node->name);
        DcAppElemType type = dc_app_string_to_elem_type(name);
        if (type == DC_APP_ELEM_TYPE_UNDEFINED) {
            fprintf(stderr, "DCAPP dc_app_xml_node_to_elem_type: Undefined element name\n");
        }
        return type;
    }
    return DC_APP_ELEM_TYPE_NONELEM;
}
