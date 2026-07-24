#include "scene.h"

#include "pl.h"

#include "app/lookup.h"
#include "node.h"
#include "utils/stb_sb.h"

#include <stdlib.h>
#include <string.h>

struct DcAppSceneContext {
    DcAppLookup *lookup;

    // nodes
    DcAppNode      *sb_nodes;
    DcAppNodeIndex  window;

    // planet instances
    // Definitions are allocated separately so their addresses survive registry growth.
    DcAppPlanetDefinition **sb_planet_definitions;
    DcAppNodeIndex         *sb_planet_view_nodes;
};

static const plMemoryI *_ext_memory = NULL;

#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_FREE(x)  _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

void dc_app_scene_init(plApiRegistryI *api_registry) {
    _ext_memory = pl_get_api_latest(api_registry, plMemoryI);
}

DcAppSceneContext *dc_app_scene_create(void) {
    DcAppSceneContext *scene = PL_ALLOC(sizeof(*scene));
    if (!scene) return NULL;

    memset(scene, 0, sizeof(*scene));
    scene->lookup = dc_app_lookup_create();
    if (!scene->lookup) {
        PL_FREE(scene);
        return NULL;
    }

    sbresize(scene->sb_nodes, NODE_FIRST_INDEX);
    return scene;
}

void dc_app_scene_destroy(DcAppSceneContext *scene) {
    if (!scene) return;

    // cleanup per-node resources (stretchy buffers and malloc'd memory)
    for (int i = NODE_FIRST_INDEX; i < sbcount(scene->sb_nodes); i++) {
        DcAppNode *node = &scene->sb_nodes[i];
        switch (node->type) {
            case NODE_TYPE_LINE:
                sbfree(node->line.sb_vertices);
                break;
            case NODE_TYPE_PIXELSTREAM:
                break;
            case NODE_TYPE_PLANET_BREADCRUMBS:
                sbfree(node->planet_breadcrumbs.sb_points);
                break;
            case NODE_TYPE_PLANET_LINE:
                sbfree(node->planet_line.sb_points_static);
                sbfree(node->planet_line.sb_points_dynamic);
                break;
            case NODE_TYPE_PLANET_POLYGON:
                sbfree(node->planet_polygon.sb_points_static);
                sbfree(node->planet_polygon.sb_points_dynamic);
                break;
            case NODE_TYPE_PLANET_VIEW:
                break;
            case NODE_TYPE_POLYGON:
                sbfree(node->polygon.sb_vertices);
                break;
            case NODE_TYPE_DRAW_FUNCTION:
                sbfree(node->draw_function.sb_args);
                break;
            case NODE_TYPE_STENCIL:
                sbfree(node->stencil.sb_children);
                break;
            case NODE_TYPE_TEXT:
                sbfree(node->text.sb_vals);
                sbfree(node->text.sb_fillers);
                sbfree(node->text.sb_filler_indices);
                sbfree(node->text.sb_formats);
                sbfree(node->text.sb_format_indices);
                sbfree(node->text.sb_format_types);
                sbfree(node->text.sb_cached_text);
                break;
            case NODE_TYPE_PLANET_TEXT:
                sbfree(node->planet_text.sb_vals);
                sbfree(node->planet_text.sb_fillers);
                sbfree(node->planet_text.sb_filler_indices);
                sbfree(node->planet_text.sb_formats);
                sbfree(node->planet_text.sb_format_indices);
                sbfree(node->planet_text.sb_format_types);
                break;
            case NODE_TYPE_WINDOW:
                if (node->window.title) {
                    free(node->window.title);
                }
                break;
            default:
                break;
        }
    }

    sbfree(scene->sb_nodes);

    // cleanup planet definitions
    for (int i = 0; i < sbcount(scene->sb_planet_definitions); i++) {
        DcAppPlanetDefinition *def = scene->sb_planet_definitions[i];
        if (!def) continue;

        if (def->name) free(def->name);
        for (int j = 0; j < sbcount(def->sb_data_files); j++) {
            free(def->sb_data_files[j]);
        }
        sbfree(def->sb_data_files);
        for (int j = 0; j < sbcount(def->sb_shaders); j++) {
            if (def->sb_shaders[j].vertex_path) free(def->sb_shaders[j].vertex_path);
            if (def->sb_shaders[j].fragment_path) free(def->sb_shaders[j].fragment_path);
        }
        sbfree(def->sb_shaders);
        for (int j = 0; j < sbcount(def->sb_textures); j++) {
            if (def->sb_textures[j].source) free(def->sb_textures[j].source);
        }
        sbfree(def->sb_textures);
        PL_FREE(def);
    }
    sbfree(scene->sb_planet_definitions);
    sbfree(scene->sb_planet_view_nodes);
    dc_app_lookup_destroy(scene->lookup);
    PL_FREE(scene);
}

DcAppLookup *dc_app_scene_lookup(DcAppSceneContext *scene) {
    return scene ? scene->lookup : NULL;
}

DcAppNodeIndex dc_app_scene_add_node(DcAppSceneContext *scene, const DcAppNode *node) {
    if (!scene || !node) return NODE_INDEX_UNDEFINED;
    sbpush(scene->sb_nodes, *node);
    return sbcount(scene->sb_nodes) - 1;
}

DcAppNode *dc_app_scene_get_node(DcAppSceneContext *scene, DcAppNodeIndex index) {
    if (!scene || index <= NODE_INDEX_UNDEFINED || index >= sbcount(scene->sb_nodes)) return NULL;
    return &scene->sb_nodes[index];
}

int dc_app_scene_get_node_count(const DcAppSceneContext *scene) {
    return scene ? sbcount(scene->sb_nodes) : 0;
}

DcAppPlanetDefinition *dc_app_scene_add_planet_definition(
    DcAppSceneContext *scene,
    const DcAppPlanetDefinition *definition) {
    if (!scene || !definition) return NULL;

    DcAppPlanetDefinition *stored = PL_ALLOC(sizeof(*stored));
    if (!stored) return NULL;

    // The scene takes ownership of the copied definition and its nested allocations.
    *stored = *definition;
    sbpush(scene->sb_planet_definitions, stored);
    return stored;
}

uint32_t dc_app_scene_get_planet_definition_count(const DcAppSceneContext *scene) {
    return scene ? (uint32_t)sbcount(scene->sb_planet_definitions) : 0;
}

DcAppPlanetDefinition *dc_app_scene_get_planet_definition(
    DcAppSceneContext *scene,
    uint32_t index) {
    if (!scene || index >= (uint32_t)sbcount(scene->sb_planet_definitions)) return NULL;
    return scene->sb_planet_definitions[index];
}

void dc_app_scene_add_planet_view_node(
    DcAppSceneContext *scene,
    DcAppNodeIndex node_index) {
    if (!scene || node_index == NODE_INDEX_UNDEFINED) return;
    sbpush(scene->sb_planet_view_nodes, node_index);
}

uint32_t dc_app_scene_get_planet_view_node_count(const DcAppSceneContext *scene) {
    return scene ? (uint32_t)sbcount(scene->sb_planet_view_nodes) : 0;
}

DcAppNodeIndex dc_app_scene_get_planet_view_node(
    const DcAppSceneContext *scene,
    uint32_t index) {
    if (!scene || index >= (uint32_t)sbcount(scene->sb_planet_view_nodes)) {
        return NODE_INDEX_UNDEFINED;
    }
    return scene->sb_planet_view_nodes[index];
}

void dc_app_scene_set_window(DcAppSceneContext *scene, DcAppNodeIndex index) {
    if (scene) scene->window = index;
}

DcAppNodeIndex dc_app_scene_get_window(const DcAppSceneContext *scene) {
    return scene ? scene->window : NODE_INDEX_UNDEFINED;
}
