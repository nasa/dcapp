#ifndef DC_APP_DISPLAY_LOGIC_H
#define DC_APP_DISPLAY_LOGIC_H

#include <stdbool.h>

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct DcAppDisplayLogicContext DcAppDisplayLogicContext;
struct DcAppContext;
struct DcAppDisplayLogicInit;

void dc_app_display_logic_init(plApiRegistryI *api_registry);

DcAppDisplayLogicContext *dc_app_display_logic_context_create(void);
void dc_app_display_logic_context_destroy(DcAppDisplayLogicContext *display_logic, struct DcAppContext *app_context);

bool dc_app_display_logic_load(DcAppDisplayLogicContext *display_logic, const char *path, const char *base_directory);
bool dc_app_display_logic_is_loaded(const DcAppDisplayLogicContext *display_logic);
void *dc_app_display_logic_symbol(DcAppDisplayLogicContext *display_logic, const char *name);

void dc_app_display_logic_pre_init(DcAppDisplayLogicContext *display_logic, const struct DcAppDisplayLogicInit *init);
void dc_app_display_logic_initialize(DcAppDisplayLogicContext *display_logic, struct DcAppContext *app_context);
void dc_app_display_logic_update(DcAppDisplayLogicContext *display_logic, struct DcAppContext *app_context, double update_rate);
void *dc_app_display_logic_user_data(DcAppDisplayLogicContext *display_logic);

#endif
