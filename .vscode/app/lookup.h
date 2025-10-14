#ifndef _DC_APP_LOOKUP_
#define _DC_APP_LOOKUP_

#include <stddef.h>
#include <stdint.h>

typedef uint32_t DcAppLookupIndex;

typedef struct _DcAppLookup {
    DcAppLookupIndex index;
} DcAppLookup;

#ifdef __cplusplus
extern "C" {
#endif

void dc_app_dereference_constants(DcAppLookup *lookup, const char *in, char *out, size_t out_size);

DcValueType dc_value_type_from_string(const std::string &type);
DcValue     dc_value_create_typed_value_from_string(DcValueType type, const std::string &value);

#ifdef __cplusplus
}
#endif

#endif
