#ifndef DC_APP_LOGIC_RUNTIME_H
#define DC_APP_LOGIC_RUNTIME_H

#include <stdbool.h>

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct DcAppLogicContext DcAppLogicContext;
struct DcAppContext;
struct DcAppInit;

void dc_app_logic_runtime_init(plApiRegistryI *api_registry);

DcAppLogicContext *dc_app_logic_context_create(void);
void dc_app_logic_context_destroy(DcAppLogicContext *logic, struct DcAppContext *app_context);

bool  dc_app_logic_load(DcAppLogicContext *logic, const char *path, const char *base_directory);
bool  dc_app_logic_is_loaded(const DcAppLogicContext *logic);
void *dc_app_logic_symbol(DcAppLogicContext *logic, const char *name);

void dc_app_logic_pre_init(DcAppLogicContext *logic, const struct DcAppInit *init);
void dc_app_logic_initialize(DcAppLogicContext *logic, struct DcAppContext *app_context);
void dc_app_logic_update(DcAppLogicContext *logic, struct DcAppContext *app_context, double update_rate);
void *dc_app_logic_user_data(DcAppLogicContext *logic);

#endif
