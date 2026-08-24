#ifndef DC_APP_DATA_LINK_H
#define DC_APP_DATA_LINK_H

#include "app/variable_registry_types.h"

//~ types

typedef struct _plApiRegistryI plApiRegistryI;
typedef struct DcAppDataLinkContext DcAppDataLinkContext;
struct DcAppVariableRegistryContext;
struct DcAppValue;

//~ lifecycle

void dc_app_data_link_init(plApiRegistryI *api_registry);

DcAppDataLinkContext *dc_app_data_link_context_create(void);
void dc_app_data_link_context_destroy(DcAppDataLinkContext *data_link);

//~ connection bindings

// binding calls apply to the most recently added connection of that type
void dc_app_data_link_add_edge(DcAppDataLinkContext *data_link, const char *host, int port, float data_rate, DcAppVariableRegistryVariableIndex connected_var_index);
void dc_app_data_link_add_edge_rx(DcAppDataLinkContext *data_link, const char *command, DcAppVariableRegistryVariableIndex dcapp_var_index);
void dc_app_data_link_add_edge_tx(DcAppDataLinkContext *data_link, const char *command, DcAppVariableRegistryVariableIndex dcapp_var_index, const struct DcAppValue *initial_value);

void dc_app_data_link_add_trick(DcAppDataLinkContext *data_link, const char *host, int port, float data_rate, DcAppVariableRegistryVariableIndex connected_var_index);
void dc_app_data_link_add_trick_rx(DcAppDataLinkContext *data_link, const char *path, const char *units, DcAppVariableRegistryVariableIndex dcapp_var_index);
void dc_app_data_link_add_trick_tx(DcAppDataLinkContext *data_link, const char *path, const char *units, DcAppVariableRegistryVariableIndex dcapp_var_index, const struct DcAppValue *initial_value);

//~ updates

void dc_app_data_link_update(DcAppDataLinkContext *data_link, struct DcAppVariableRegistryContext *registry);

#endif
