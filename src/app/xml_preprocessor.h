#ifndef DC_APP_XML_PREPROCESSOR_H
#define DC_APP_XML_PREPROCESSOR_H

#include <stdbool.h>

//~ forward declarations

struct _xmlNode;

typedef struct DcAppXmlPreprocessorContext DcAppXmlPreprocessorContext;

#ifdef __cplusplus
extern "C" {
#endif

//~ preprocessor lifecycle

DcAppXmlPreprocessorContext *dc_app_xml_preprocessor_context_create(const char *config_path, char **args, int arg_count);
void dc_app_xml_preprocessor_context_destroy(DcAppXmlPreprocessorContext *preprocessor);
// expand the configuration in place while retaining ownership of the xml tree
void dc_app_xml_preprocessor_preprocess(DcAppXmlPreprocessorContext *preprocessor);
void dc_app_xml_preprocessor_save_preprocessed(DcAppXmlPreprocessorContext *preprocessor, const char *output_path);
void dc_app_xml_preprocessor_export_environment(const DcAppXmlPreprocessorContext *preprocessor);

//~ preprocessor results

const char *dc_app_xml_preprocessor_directory(const DcAppXmlPreprocessorContext *preprocessor);
const char *dc_app_xml_preprocessor_root_directory(const DcAppXmlPreprocessorContext *preprocessor);
struct _xmlNode *dc_app_xml_preprocessor_root(const DcAppXmlPreprocessorContext *preprocessor);
bool dc_app_xml_preprocessor_suppresses_missing_variable(const DcAppXmlPreprocessorContext *preprocessor);

#ifdef __cplusplus
}
#endif

#endif
