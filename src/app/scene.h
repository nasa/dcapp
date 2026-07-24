#ifndef DC_APP_SCENE_H
#define DC_APP_SCENE_H

#include "app/node_types.h"

#include <stdint.h>

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct DcAppNode DcAppNode;
typedef struct DcAppPlanetDefinition DcAppPlanetDefinition;
typedef struct DcAppSceneContext DcAppSceneContext;
struct DcAppLookup;

void dc_app_scene_init(plApiRegistryI *api_registry);

DcAppSceneContext *dc_app_scene_create(void);
void dc_app_scene_destroy(DcAppSceneContext *scene);

struct DcAppLookup *dc_app_scene_lookup(DcAppSceneContext *scene);

DcAppNodeIndex dc_app_scene_add_node(DcAppSceneContext *scene, const DcAppNode *node);
// The returned pointer remains valid only until another node is added.
DcAppNode         *dc_app_scene_get_node(DcAppSceneContext *scene, DcAppNodeIndex index);
int            dc_app_scene_get_node_count(const DcAppSceneContext *scene);

DcAppPlanetDefinition *dc_app_scene_add_planet_definition(
    DcAppSceneContext *scene,
    const DcAppPlanetDefinition *definition);
uint32_t dc_app_scene_get_planet_definition_count(const DcAppSceneContext *scene);
DcAppPlanetDefinition *dc_app_scene_get_planet_definition(
    DcAppSceneContext *scene,
    uint32_t index);

void dc_app_scene_add_planet_view_node(
    DcAppSceneContext *scene,
    DcAppNodeIndex node_index);
uint32_t dc_app_scene_get_planet_view_node_count(const DcAppSceneContext *scene);
DcAppNodeIndex dc_app_scene_get_planet_view_node(
    const DcAppSceneContext *scene,
    uint32_t index);

void           dc_app_scene_set_window(DcAppSceneContext *scene, DcAppNodeIndex index);
DcAppNodeIndex dc_app_scene_get_window(const DcAppSceneContext *scene);

#endif
