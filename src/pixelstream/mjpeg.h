#ifndef _DC_PIXELSTREAM_MJPEG_
#define _DC_PIXELSTREAM_MJPEG_

#include <stdbool.h>
#include <stddef.h>

typedef struct DcPsMjpegContext DcPsMjpegContext;
typedef struct DcPsMjpegServer DcPsMjpegServer;

#ifdef __cplusplus
extern "C" {
#endif

//~ context

DcPsMjpegContext *dc_ps_mjpeg_context_create(void);
void dc_ps_mjpeg_context_destroy(DcPsMjpegContext *context);
void dc_ps_mjpeg_update(DcPsMjpegContext *context);

//~ servers
DcPsMjpegServer *dc_ps_mjpeg_add_server(DcPsMjpegContext *context, const char *url, int timeout_s);
void dc_ps_mjpeg_remove_server(DcPsMjpegServer *server);
bool dc_ps_mjpeg_server_is_connected(DcPsMjpegServer *server);
bool dc_ps_mjpeg_server_has_new_data(DcPsMjpegServer *server);
void dc_ps_mjpeg_get_server_data(DcPsMjpegServer *server, unsigned char *out_data, size_t out_data_size, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif
