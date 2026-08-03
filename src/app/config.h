#ifndef DC_APP_CONFIG_H
#define DC_APP_CONFIG_H

#include <stdbool.h>

// forward declarations
struct _xmlNode;

// config type
typedef struct DcAppConfig DcAppConfig;

#ifdef __cplusplus
extern "C" {
#endif

// config functions
DcAppConfig *dc_app_config_create(const char *config_path, char **args, int arg_count);
void         dc_app_config_destroy(DcAppConfig *config);
// Expands the configuration in place while retaining ownership of the XML tree.
void         dc_app_config_preprocess(DcAppConfig *config);
void         dc_app_config_save_preprocessed(DcAppConfig *config, const char *output_path);
void         dc_app_config_export_environment(const DcAppConfig *config);

const char *dc_app_config_directory(const DcAppConfig *config);
const char *dc_app_config_root_directory(const DcAppConfig *config);
struct _xmlNode *dc_app_config_root(const DcAppConfig *config);
bool        dc_app_config_suppresses_missing_variable(const DcAppConfig *config);

#ifdef __cplusplus
}
#endif

#endif
