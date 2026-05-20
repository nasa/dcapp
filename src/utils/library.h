#ifndef _DC_UTILS_LIBRARY_
#define _DC_UTILS_LIBRARY_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DcLibrary DcLibrary;

DcLibrary  *dc_utils_library_load      (const char *path);
void       *dc_utils_library_symbol    (DcLibrary *lib, const char *name);
void        dc_utils_library_close     (DcLibrary *lib);
const char *dc_utils_library_last_error(void);

#ifdef __cplusplus
}
#endif

#endif // _DC_UTILS_LIBRARY_
