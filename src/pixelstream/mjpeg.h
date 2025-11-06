#ifndef _DC_PIXELSTREAM_MJPEG_
#define _DC_PIXELSTREAM_MJPEG_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct _DcPsMjpegHandle {
    uint8_t _index;
} DcPsMjpegHandle;

#ifdef __cplusplus
extern "C" {
#endif

// global funcs
void dc_ps_mjpeg_init();
void dc_ps_mjpeg_update();
void dc_ps_mjpeg_cleanup();

// for specific handles
DcPsMjpegHandle dc_ps_mjpeg_add_server(const char *url, int timeout_s);
void            dc_ps_mjpeg_remove_server(DcPsMjpegHandle handle);
bool            dc_ps_mjpeg_server_is_connected(DcPsMjpegHandle handle);
bool            dc_ps_mjpeg_server_has_new_data(DcPsMjpegHandle handle);
void            dc_ps_mjpeg_get_server_data(DcPsMjpegHandle handle, unsigned char *out_data, size_t out_data_size, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif
