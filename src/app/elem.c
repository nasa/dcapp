#include "elem.h"

#include "../utils/log.h"

#include <string.h>

#include <libxml/parser.h>

const char *dc_app_elem_type_to_string(DcAppElemType type) {
    switch (type) {
        case DC_APP_ELEM_TYPE_ARC:
            return "Arc";
        case DC_APP_ELEM_TYPE_ARG:
            return "Arg";
        case DC_APP_ELEM_TYPE_BLINK:
            return "Blink";
        case DC_APP_ELEM_TYPE_BUTTON:
            return "Button";
        case DC_APP_ELEM_TYPE_BUTTON_DISABLED:
            return "ButtonDisabled";
        case DC_APP_ELEM_TYPE_BUTTON_ENABLED:
            return "ButtonEnabled";
        case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_OFF:
            return "ButtonIndicatorOff";
        case DC_APP_ELEM_TYPE_BUTTON_INDICATOR_ON:
            return "ButtonIndicatorOn";
        case DC_APP_ELEM_TYPE_BUTTON_PRESSED:
            return "ButtonPressed";
        case DC_APP_ELEM_TYPE_BUTTON_RELEASED:
            return "ButtonReleased";
        case DC_APP_ELEM_TYPE_BUTTON_TRANSITION:
            return "ButtonTransition";
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
        case DC_APP_ELEM_TYPE_EDGE_FROM:
            return "EdgeFrom";
        case DC_APP_ELEM_TYPE_EDGE_IO:
            return "EdgeIO";
        case DC_APP_ELEM_TYPE_EDGE_TO:
            return "EdgeTo";
        case DC_APP_ELEM_TYPE_EDGE_VARIABLE:
            return "EdgeVariable";
        case DC_APP_ELEM_TYPE_ELLIPSE:
            return "Ellipse";
        case DC_APP_ELEM_TYPE_FALSE:
            return "False";
        case DC_APP_ELEM_TYPE_DRAW_FUNCTION:
            return "DrawFunction";
        case DC_APP_ELEM_TYPE_FUNCTION:
            return "Function";
        case DC_APP_ELEM_TYPE_IF:
            return "If";
        case DC_APP_ELEM_TYPE_IMAGE:
            return "Image";
        case DC_APP_ELEM_TYPE_INCLUDE:
            return "Include";
        case DC_APP_ELEM_TYPE_LINE:
            return "Line";
        case DC_APP_ELEM_TYPE_LOGIC:
            return "Logic";
        case DC_APP_ELEM_TYPE_MOUSE_ACTIVE:
            return "MouseActive";
        case DC_APP_ELEM_TYPE_MOUSE_HOVERED:
            return "MouseHovered";
        case DC_APP_ELEM_TYPE_MOUSE_INACTIVE:
            return "MouseInactive";
        case DC_APP_ELEM_TYPE_MOUSE_MOTION:
            return "MouseMotion";
        case DC_APP_ELEM_TYPE_MOUSE_PRESSED:
            return "MousePressed";
        case DC_APP_ELEM_TYPE_MOUSE_RELEASED:
            return "MouseReleased";
        case DC_APP_ELEM_TYPE_NONELEM:
            return "<Non-Element>";
        case DC_APP_ELEM_TYPE_PANEL:
            return "Panel";
        case DC_APP_ELEM_TYPE_PIXELSTREAM:
            return "PixelStream";
        case DC_APP_ELEM_TYPE_POLYGON:
            return "Polygon";
        case DC_APP_ELEM_TYPE_RECTANGLE:
            return "Rectangle";
        case DC_APP_ELEM_TYPE_SET:
            return "Set";
        case DC_APP_ELEM_TYPE_SPHERE:
            return "Sphere";
        case DC_APP_ELEM_TYPE_STENCIL:
            return "Stencil";
        case DC_APP_ELEM_TYPE_STENCIL_ADD:
            return "StencilAdd";
        case DC_APP_ELEM_TYPE_STENCIL_DRAW:
            return "StencilDraw";
        case DC_APP_ELEM_TYPE_STENCIL_REMOVE:
            return "StencilRemove";
        case DC_APP_ELEM_TYPE_STYLE:
            return "Style";
        case DC_APP_ELEM_TYPE_PLANET:
            return "Planet";
        case DC_APP_ELEM_TYPE_PLANET_DATA:
            return "PlanetData";
        case DC_APP_ELEM_TYPE_PLANET_ELLIPSE:
            return "PlanetEllipse";
        case DC_APP_ELEM_TYPE_PLANET_GEO_JSON:
            return "PlanetGeoJSON";
        case DC_APP_ELEM_TYPE_PLANET_LINE:
            return "PlanetLine";
        case DC_APP_ELEM_TYPE_PLANET_POLYGON:
            return "PlanetPolygon";
        case DC_APP_ELEM_TYPE_PLANET_SHADER:
            return "PlanetShader";
        case DC_APP_ELEM_TYPE_PLANET_SPHERE:
            return "PlanetSphere";
        case DC_APP_ELEM_TYPE_PLANET_TEXT:
            return "PlanetText";
        case DC_APP_ELEM_TYPE_PLANET_TEXTURE:
            return "PlanetTexture";
        case DC_APP_ELEM_TYPE_PLANET_VIEW:
            return "PlanetView";
        case DC_APP_ELEM_TYPE_TEXT:
            return "Text";
        case DC_APP_ELEM_TYPE_TRICK_FROM:
            return "TrickFrom";
        case DC_APP_ELEM_TYPE_TRICK_IO:
            return "TrickIO";
        case DC_APP_ELEM_TYPE_TRICK_TO:
            return "TrickTo";
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
    if (strcmp(name, "Arc") == 0)
        return DC_APP_ELEM_TYPE_ARC;
    if (strcmp(name, "Arg") == 0)
        return DC_APP_ELEM_TYPE_ARG;
    if (strcmp(name, "Blink") == 0)
        return DC_APP_ELEM_TYPE_BLINK;
    if (strcmp(name, "Button") == 0)
        return DC_APP_ELEM_TYPE_BUTTON;
    if (strcmp(name, "ButtonDisabled") == 0)
        return DC_APP_ELEM_TYPE_BUTTON_DISABLED;
    if (strcmp(name, "ButtonEnabled") == 0)
        return DC_APP_ELEM_TYPE_BUTTON_ENABLED;
    if (strcmp(name, "ButtonIndicatorOff") == 0)
        return DC_APP_ELEM_TYPE_BUTTON_INDICATOR_OFF;
    if (strcmp(name, "ButtonIndicatorOn") == 0)
        return DC_APP_ELEM_TYPE_BUTTON_INDICATOR_ON;
    if (strcmp(name, "ButtonPressed") == 0)
        return DC_APP_ELEM_TYPE_BUTTON_PRESSED;
    if (strcmp(name, "ButtonReleased") == 0)
        return DC_APP_ELEM_TYPE_BUTTON_RELEASED;
    if (strcmp(name, "ButtonTransition") == 0)
        return DC_APP_ELEM_TYPE_BUTTON_TRANSITION;
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
    if (strcmp(name, "EdgeFrom") == 0)
        return DC_APP_ELEM_TYPE_EDGE_FROM;
    if (strcmp(name, "EdgeIO") == 0)
        return DC_APP_ELEM_TYPE_EDGE_IO;
    if (strcmp(name, "EdgeTo") == 0)
        return DC_APP_ELEM_TYPE_EDGE_TO;
    if (strcmp(name, "EdgeVariable") == 0)
        return DC_APP_ELEM_TYPE_EDGE_VARIABLE;
    if (strcmp(name, "Ellipse") == 0)
        return DC_APP_ELEM_TYPE_ELLIPSE;
    if (strcmp(name, "False") == 0)
        return DC_APP_ELEM_TYPE_FALSE;
    if (strcmp(name, "DrawFunction") == 0)
        return DC_APP_ELEM_TYPE_DRAW_FUNCTION;
    if (strcmp(name, "Function") == 0)
        return DC_APP_ELEM_TYPE_FUNCTION;
    if (strcmp(name, "If") == 0)
        return DC_APP_ELEM_TYPE_IF;
    if (strcmp(name, "Image") == 0)
        return DC_APP_ELEM_TYPE_IMAGE;
    if (strcmp(name, "Include") == 0)
        return DC_APP_ELEM_TYPE_INCLUDE;
    if (strcmp(name, "Line") == 0)
        return DC_APP_ELEM_TYPE_LINE;
    if (strcmp(name, "Logic") == 0)
        return DC_APP_ELEM_TYPE_LOGIC;
    if (strcmp(name, "MouseActive") == 0)
        return DC_APP_ELEM_TYPE_MOUSE_ACTIVE;
    if (strcmp(name, "MouseHovered") == 0)
        return DC_APP_ELEM_TYPE_MOUSE_HOVERED;
    if (strcmp(name, "MouseInactive") == 0)
        return DC_APP_ELEM_TYPE_MOUSE_INACTIVE;
    if (strcmp(name, "MouseMotion") == 0)
        return DC_APP_ELEM_TYPE_MOUSE_MOTION;
    if (strcmp(name, "MousePressed") == 0)
        return DC_APP_ELEM_TYPE_MOUSE_PRESSED;
    if (strcmp(name, "MouseReleased") == 0)
        return DC_APP_ELEM_TYPE_MOUSE_RELEASED;
    if (strcmp(name, "Panel") == 0)
        return DC_APP_ELEM_TYPE_PANEL;
    if (strcmp(name, "PixelStream") == 0)
        return DC_APP_ELEM_TYPE_PIXELSTREAM;
    if (strcmp(name, "Polygon") == 0)
        return DC_APP_ELEM_TYPE_POLYGON;
    if (strcmp(name, "Rectangle") == 0)
        return DC_APP_ELEM_TYPE_RECTANGLE;
    if (strcmp(name, "Set") == 0)
        return DC_APP_ELEM_TYPE_SET;
    if (strcmp(name, "Sphere") == 0)
        return DC_APP_ELEM_TYPE_SPHERE;
    if (strcmp(name, "Stencil") == 0)
        return DC_APP_ELEM_TYPE_STENCIL;
    if (strcmp(name, "StencilAdd") == 0)
        return DC_APP_ELEM_TYPE_STENCIL_ADD;
    if (strcmp(name, "StencilDraw") == 0)
        return DC_APP_ELEM_TYPE_STENCIL_DRAW;
    if (strcmp(name, "StencilRemove") == 0)
        return DC_APP_ELEM_TYPE_STENCIL_REMOVE;
    if (strcmp(name, "Style") == 0)
        return DC_APP_ELEM_TYPE_STYLE;
    if (strcmp(name, "Planet") == 0)
        return DC_APP_ELEM_TYPE_PLANET;
    if (strcmp(name, "PlanetData") == 0)
        return DC_APP_ELEM_TYPE_PLANET_DATA;
    if (strcmp(name, "PlanetEllipse") == 0)
        return DC_APP_ELEM_TYPE_PLANET_ELLIPSE;
    if (strcmp(name, "PlanetGeoJSON") == 0)
        return DC_APP_ELEM_TYPE_PLANET_GEO_JSON;
    if (strcmp(name, "PlanetLine") == 0)
        return DC_APP_ELEM_TYPE_PLANET_LINE;
    if (strcmp(name, "PlanetPolygon") == 0)
        return DC_APP_ELEM_TYPE_PLANET_POLYGON;
    if (strcmp(name, "PlanetShader") == 0)
        return DC_APP_ELEM_TYPE_PLANET_SHADER;
    if (strcmp(name, "PlanetSphere") == 0)
        return DC_APP_ELEM_TYPE_PLANET_SPHERE;
    if (strcmp(name, "PlanetText") == 0)
        return DC_APP_ELEM_TYPE_PLANET_TEXT;
    if (strcmp(name, "PlanetTexture") == 0)
        return DC_APP_ELEM_TYPE_PLANET_TEXTURE;
    if (strcmp(name, "PlanetView") == 0)
        return DC_APP_ELEM_TYPE_PLANET_VIEW;
    if (strcmp(name, "Text") == 0)
        return DC_APP_ELEM_TYPE_TEXT;
    if (strcmp(name, "TrickFrom") == 0)
        return DC_APP_ELEM_TYPE_TRICK_FROM;
    if (strcmp(name, "TrickIO") == 0)
        return DC_APP_ELEM_TYPE_TRICK_IO;
    if (strcmp(name, "TrickTo") == 0)
        return DC_APP_ELEM_TYPE_TRICK_TO;
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
            DC_LOG_WARN("Elem", "dc_app_xml_node_to_elem_type: Undefined element name: %s", name);
        }
        return type;
    }
    return DC_APP_ELEM_TYPE_NONELEM;
}
