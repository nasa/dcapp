#ifndef DC_APP_DISPLAY_MODEL_H
#define DC_APP_DISPLAY_MODEL_H

#include "app/node_types.h"

#include <stdint.h>

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct DcAppNode DcAppNode;
typedef struct DcAppPlanetDefinition DcAppPlanetDefinition;
typedef struct DcAppDisplayModelContext DcAppDisplayModelContext;
struct DcAppVariableRegistryContext;

void dc_app_display_model_init(plApiRegistryI *api_registry);

DcAppDisplayModelContext *dc_app_display_model_context_create(void);
void dc_app_display_model_context_destroy(DcAppDisplayModelContext *model);

struct DcAppVariableRegistryContext *dc_app_display_model_get_variable_registry(DcAppDisplayModelContext *model);

DcAppNodeIndex dc_app_display_model_add_node(DcAppDisplayModelContext *model, const DcAppNode *node);
// The returned pointer remains valid only until another node is added.
DcAppNode *dc_app_display_model_get_node(DcAppDisplayModelContext *model, DcAppNodeIndex index);
int dc_app_display_model_get_node_count(const DcAppDisplayModelContext *model);

DcAppPlanetDefinition *dc_app_display_model_add_planet_definition(
    DcAppDisplayModelContext *model,
    const DcAppPlanetDefinition *definition);
uint32_t dc_app_display_model_get_planet_definition_count(const DcAppDisplayModelContext *model);
DcAppPlanetDefinition *dc_app_display_model_get_planet_definition(
    DcAppDisplayModelContext *model,
    uint32_t index);

void dc_app_display_model_add_planet_view_node(
    DcAppDisplayModelContext *model,
    DcAppNodeIndex node_index);
uint32_t dc_app_display_model_get_planet_view_node_count(const DcAppDisplayModelContext *model);
DcAppNodeIndex dc_app_display_model_get_planet_view_node(
    const DcAppDisplayModelContext *model,
    uint32_t index);

void dc_app_display_model_set_window(DcAppDisplayModelContext *model, DcAppNodeIndex index);
DcAppNodeIndex dc_app_display_model_get_window(const DcAppDisplayModelContext *model);

#endif
