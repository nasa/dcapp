#ifndef DC_APP_DISPLAY_BUILDER_H
#define DC_APP_DISPLAY_BUILDER_H

#include "app/xml_element_types.h"
#include "app/node_types.h"

//~ types

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct DcAppNode DcAppNode;
typedef struct DcAppDisplayBuilderContext DcAppDisplayBuilderContext;

struct DcAppContext;
struct DcAppFontContext;
struct DcAppDisplayLogicContext;
struct DcAppPixelstreamContext;
struct DcAppDisplayModelContext;
struct DcAppTextureContext;
struct DcAppDataLinkContext;
struct _xmlNode;

//~ bootstrap

// bootstrap after window registration and before child parsing to inject runtime contexts
typedef void (*DcAppDisplayBuilderBootstrapFn)(struct DcAppContext *app_context, DcAppDisplayBuilderContext *builder, DcAppNode *window);

//~ lifecycle

void dc_app_display_builder_init(plApiRegistryI *api_registry);

DcAppDisplayBuilderContext *dc_app_display_builder_context_create(
    struct DcAppContext *app_context,
    struct DcAppDisplayModelContext *model,
    struct DcAppDisplayLogicContext *display_logic,
    struct DcAppDataLinkContext *data_link,
    const char *dcapp_root,
    DcAppDisplayBuilderBootstrapFn bootstrap);
void dc_app_display_builder_context_destroy(DcAppDisplayBuilderContext *builder);

//~ runtime dependencies

void dc_app_display_builder_set_fonts(DcAppDisplayBuilderContext *builder, struct DcAppFontContext *fonts);
void dc_app_display_builder_set_textures(DcAppDisplayBuilderContext *builder, struct DcAppTextureContext *textures);
void dc_app_display_builder_set_pixelstreams(DcAppDisplayBuilderContext *builder, struct DcAppPixelstreamContext *pixelstreams);

//~ xml processing

DcAppNodeIndex dc_app_display_builder_process_xml_node(DcAppDisplayBuilderContext *builder, struct _xmlNode *xml_node, DcAppNodeIndex parent_node_index, DcAppXmlElementType parent_element_type, const char *directory);

#endif
