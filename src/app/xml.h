#ifndef DC_APP_XML_H
#define DC_APP_XML_H

#include "app/elem_types.h"
#include "app/node_types.h"

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct DcAppNode DcAppNode;
typedef struct DcAppXmlContext DcAppXmlContext;

struct DcAppContext;
struct DcAppFontContext;
struct DcAppLogicContext;
struct DcAppPixelstreamContext;
struct DcAppSceneContext;
struct DcAppTextureContext;
struct DcAppDataLinkContext;
struct _xmlNode;

// Called after the Window has been parsed and registered, before its children
// are parsed. The app initializes the display runtime here and injects
// GPU-dependent font, texture, and pixelstream contexts through the setters
// below.
typedef void (*DcAppXmlBootstrapFn)(struct DcAppContext *app_context, DcAppXmlContext *xml_ctx, DcAppNode *window);

void dc_app_xml_init(plApiRegistryI *api_registry);

DcAppXmlContext *dc_app_xml_context_create(
    struct DcAppContext *app_context,
    struct DcAppSceneContext *scene,
    struct DcAppLogicContext *logic,
    struct DcAppDataLinkContext *data_link,
    const char *dcapp_root,
    DcAppXmlBootstrapFn bootstrap);
void dc_app_xml_context_destroy(DcAppXmlContext *xml_ctx);

void dc_app_xml_set_fonts(DcAppXmlContext *xml_ctx, struct DcAppFontContext *fonts);
void dc_app_xml_set_textures(DcAppXmlContext *xml_ctx, struct DcAppTextureContext *textures);
void dc_app_xml_set_pixelstreams(DcAppXmlContext *xml_ctx, struct DcAppPixelstreamContext *pixelstreams);

DcAppNodeIndex dc_app_process_xml_node(DcAppXmlContext *xml_ctx, struct _xmlNode *xml_node, DcAppNodeIndex parent_node_index, DcAppElemType parent_elem_type, const char *directory);

#endif
