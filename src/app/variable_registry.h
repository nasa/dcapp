#ifndef DC_APP_VARIABLE_REGISTRY_H
#define DC_APP_VARIABLE_REGISTRY_H

#include "variable_registry_types.h"
#include "value_types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct DcAppVariableRegistryContext DcAppVariableRegistryContext;
struct DcAppValue;

#define DC_APP_VARIABLE_REGISTRY_INDEX_UNDEFINED (0)
#define DC_APP_VARIABLE_REGISTRY_FIRST_INDEX (1)

static const DcAppVariableRegistryVariableIndex DC_APP_VARIABLE_REGISTRY_VARIABLE_INDEX_UNDEFINED = DC_APP_VARIABLE_REGISTRY_INDEX_UNDEFINED;
static const DcAppVariableRegistryValueIndex DC_APP_VARIABLE_REGISTRY_VALUE_INDEX_UNDEFINED = DC_APP_VARIABLE_REGISTRY_INDEX_UNDEFINED;

#ifdef __cplusplus
extern "C" {
#endif

// Variable registry functions
DcAppVariableRegistryContext *dc_app_variable_registry_context_create(void);
void dc_app_variable_registry_context_destroy(DcAppVariableRegistryContext *registry);
// Prevents further registration so published value pointers remain stable.
void dc_app_variable_registry_seal(DcAppVariableRegistryContext *registry);

// The pointer may move during registration but remains stable after sealing.
struct DcAppValue *dc_app_variable_registry_get_value(DcAppVariableRegistryContext *registry, DcAppVariableRegistryValueIndex index);
DcAppVariableRegistryValueIndex dc_app_variable_registry_register_value(DcAppVariableRegistryContext *registry, struct DcAppValue *value);
DcAppVariableRegistryValueIndex dc_app_variable_registry_register_value_from_string(DcAppVariableRegistryContext *registry, DcAppValueType type, const char *text);

int dc_app_variable_registry_get_variable_count(DcAppVariableRegistryContext *registry);
DcAppVariableRegistryVariableIndex dc_app_variable_registry_get_variable_index(DcAppVariableRegistryContext *registry, const char *name);
DcAppVariableRegistryValueIndex dc_app_variable_registry_get_variable_value_index(DcAppVariableRegistryContext *registry, DcAppVariableRegistryVariableIndex index);
DcAppVariableRegistryValueIndex dc_app_variable_registry_get_variable_value_index_by_name(DcAppVariableRegistryContext *registry, const char *name);
DcAppVariableRegistryVariableIndex dc_app_variable_registry_register_variable(DcAppVariableRegistryContext *registry, const char *name, DcAppVariableRegistryValueIndex value_index);
const char *dc_app_variable_registry_get_variable_name(DcAppVariableRegistryContext *registry, DcAppVariableRegistryVariableIndex index);
uint64_t dc_app_variable_registry_get_variable_write_sequence(DcAppVariableRegistryContext *registry, DcAppVariableRegistryVariableIndex index);
void dc_app_variable_registry_mark_variable_written(DcAppVariableRegistryContext *registry, DcAppVariableRegistryVariableIndex index);

void dc_app_variable_registry_set_variable_to_string(DcAppVariableRegistryContext *registry, DcAppVariableRegistryVariableIndex variable_index, const char *value);

// push/pop operations for per-variable stacks
void dc_app_variable_registry_variable_push(DcAppVariableRegistryContext *registry, DcAppVariableRegistryVariableIndex variable_index);
void dc_app_variable_registry_variable_pop(DcAppVariableRegistryContext *registry, DcAppVariableRegistryVariableIndex variable_index);
void dc_app_variable_registry_reset_variable_stacks(DcAppVariableRegistryContext *registry);

void dc_app_variable_registry_set_suppress_missing_variable(DcAppVariableRegistryContext *registry, bool suppress);

#ifdef __cplusplus
}
#endif

#endif
