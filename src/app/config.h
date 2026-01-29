#ifndef _DC_APP_CONFIG_
#define _DC_APP_CONFIG_

#include <stdbool.h>

// forward declarations
typedef struct _DcAppLookup DcAppLookup;
typedef struct _xmlDoc     *xmlDocPtr;
typedef struct _xmlNode    *xmlNodePtr;

// config type
typedef struct _DcAppConfig {

    // index
    int _index;

    // xml pointer
    xmlDocPtr xml_doc;
    bool      xml_doc_is_cleaned;

    // filepaths
    char *dcapp_dir_path;
    char *config_file_path;
    char *config_dir_path;
    char *cache_dir_path;
    char *log_dir_path;

} DcAppConfig;

#ifdef __cplusplus
extern "C" {
#endif

// config functions
DcAppConfig *dc_app_config_create(const char *config_path, char **args, int arg_count);
void         dc_app_config_cleanup(DcAppConfig *config);
void         dc_app_config_preprocess_xml(DcAppConfig *config, DcAppLookup *lookup);
void         dc_app_config_save_to_file(DcAppConfig *config, const char *filepath);

// const
void dc_app_config_register_const_by_name(DcAppConfig *config, const char *name, const char *new_value, bool is_immutable);

#ifdef __cplusplus
}
#endif

#endif
