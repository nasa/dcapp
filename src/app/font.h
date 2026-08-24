#ifndef DC_APP_FONT_H
#define DC_APP_FONT_H

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct _dcFont dcFont;
typedef struct DcAppFontContext DcAppFontContext;

//~ font lifecycle

void dc_app_font_init(plApiRegistryI *api_registry);

DcAppFontContext *dc_app_font_context_create(void);
void dc_app_font_context_destroy(DcAppFontContext *fonts);

//~ font registry

// register paths during parsing and build their atlas tiers afterward
int dc_app_font_register(DcAppFontContext *fonts, const char *path);
void dc_app_font_build(DcAppFontContext *fonts);

dcFont *dc_app_font_default(DcAppFontContext *fonts);
dcFont *dc_app_font_resolve(DcAppFontContext *fonts, int index, float rendered_size);

#endif
