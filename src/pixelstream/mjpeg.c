#include "mjpeg.h"
#include "../utils/stb_sb.h"

#include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    _CONNECTION_STATE_UNKNOWN = 0,
    _CONNECTION_STATE_DISCONNECTED,
    _CONNECTION_STATE_CONNECTING,
    _CONNECTION_STATE_CONNECTED
} _ConnectionState;

#define _MAX_CONNECTIONS 10
#define _MAX_URL_LENGTH 256

typedef struct __Context {

    // user settings
    char url[_MAX_URL_LENGTH];
    int  timeout_s;

    // connection state
    _ConnectionState state;
    bool             has_new_data;

    // internal timeout
    time_t timeout_begin;

    // curl handle
    CURL *easy_handle;

    // internal stretchy buffer
    unsigned char *sb_buffer;

    // latest data
    unsigned char *sb_latest_frame;
    size_t         latest_frame_size;

} _Context;

// static vars
static CURLM    *_multi_handle = NULL;
static _Context *_sb_contexts  = NULL;

// static functions
static void             _mjpeg_connect(_Context *context);
static _ConnectionState _get_connection_state(CURL *easy_handle);
static size_t           _mjpeg_write_callback(char *ptr, size_t size, size_t nmemb, void *void_context);
static void            *_memmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len);
static void            *_memrmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len);

// global init
void dc_ps_mjpeg_init() {

    // init curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    _multi_handle = curl_multi_init();

    // allocate stretchy buffers
    // * used because libcurl expects fixed pointers
    sbgrow(_sb_contexts, _MAX_CONNECTIONS, sizeof(*_sb_contexts));
}

// global update
void dc_ps_mjpeg_update() {

    // for each server
    for (int ii = 0; ii < sbcount(_sb_contexts); ii++) {
        _Context *context = &(_sb_contexts[ii]);

        // clear new data indicator
        context->has_new_data      = false;
        context->latest_frame_size = 0;

        // check for reconnections
        if (context->state == _CONNECTION_STATE_DISCONNECTED || context->state == _CONNECTION_STATE_CONNECTING) {
            if (time(NULL) - context->timeout_begin >= context->timeout_s) {
                _mjpeg_connect(context);
            }
        }
    }

    // perform reads
    int still_running;
    curl_multi_perform(_multi_handle, &still_running);

    // Check for completed transfers
    CURLMsg *msg;
    int      msgs_left;
    while ((msg = curl_multi_info_read(_multi_handle, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {

            // get handles
            CURL     *handle = msg->easy_handle;
            CURLcode  result = msg->data.result;
            _Context *context;
            curl_easy_getinfo(handle, CURLINFO_PRIVATE, &context);

            switch (result) {

                case (CURLE_OK): {
                    break;
                }

                case CURLE_OPERATION_TIMEDOUT: {
                    // reconnect immediately
                    _mjpeg_connect(context);
                    break;
                }

                case CURLE_COULDNT_CONNECT:
                case CURLE_COULDNT_RESOLVE_HOST:
                case CURLE_COULDNT_RESOLVE_PROXY:
                case CURLE_RECV_ERROR:
                case CURLE_SEND_ERROR:
                case CURLE_GOT_NOTHING:
                case CURLE_PARTIAL_FILE:
                case CURLE_SSL_CONNECT_ERROR: {
                    fprintf(stderr, "DCApp dc_ps_mjpeg_update(): Disconnected or failed to connect: %s\n", curl_easy_strerror(result));
                    context->state = _CONNECTION_STATE_DISCONNECTED;
                    break;
                }

                case CURLE_PEER_FAILED_VERIFICATION:
                case CURLE_USE_SSL_FAILED: {
                    fprintf(stderr, "DCApp dc_ps_mjpeg_update(): SSL verification or setup failed: %s\n", curl_easy_strerror(result));
                    context->state = _CONNECTION_STATE_DISCONNECTED;
                    break;
                }

                default:
                    fprintf(stderr, "DCApp dc_ps_mjpeg_update(): Unknown code from curl_multi_info_read()\n");
                    break;
            }
        }
    }
}

// global cleanup
void dc_ps_mjpeg_cleanup() {
    curl_multi_cleanup(_multi_handle);
}

DcPsMjpegHandle dc_ps_mjpeg_add_server(const char *url, int timeout_s) {

    // check url
    if (strlen(url) > _MAX_URL_LENGTH - 1) {
        fprintf(stderr, "DCApp dc_pixelstream_mjpeg_create(): Length of URL exceeds maximum length\n");
    }

    // create context
    _Context context = {};
    strncpy(context.url, url, _MAX_URL_LENGTH);
    context.url[_MAX_URL_LENGTH - 1] = '\0';
    context.timeout_s                = timeout_s;
    context.state                    = _CONNECTION_STATE_DISCONNECTED;
    context.has_new_data             = false;
    context.sb_buffer                = NULL;
    context.sb_latest_frame          = NULL;
    context.latest_frame_size        = 0;
    context.easy_handle              = NULL;
    sbpush(_sb_contexts, context);

    // create handle
    DcPsMjpegHandle handle = {};
    handle._index          = sbcount(_sb_contexts) - 1;
    return handle;
}

void dc_ps_mjpeg_remove_server(DcPsMjpegHandle handle) {
    _Context *context = &(_sb_contexts[handle._index]);

    // remove old handle
    if (context->easy_handle) {
        curl_multi_remove_handle(_multi_handle, context->easy_handle);
        curl_easy_cleanup(context->easy_handle);
        context->easy_handle = NULL;
    }

    // free memory
    sbfree(context->sb_latest_frame);
    sbfree(context->sb_buffer);

    // set states
    context->state        = _CONNECTION_STATE_DISCONNECTED;
    context->has_new_data = false;
}

bool dc_ps_mjpeg_server_is_connected(DcPsMjpegHandle handle) {
    _Context *context = &(_sb_contexts[handle._index]);
    return context->state == _CONNECTION_STATE_CONNECTED;
}

bool dc_ps_mjpeg_server_has_new_data(DcPsMjpegHandle handle) {
    _Context *context = &(_sb_contexts[handle._index]);
    return context->has_new_data;
}

void dc_ps_mjpeg_get_server_data(DcPsMjpegHandle handle, unsigned char *out_data, size_t out_data_size, size_t *out_size) {
    _Context *context = &(_sb_contexts[handle._index]);

    if (out_data_size < context->latest_frame_size) {
        fprintf(stderr, "DCApp dc_ps_mjpeg_get_server_data(): output buffer too small\n");
        return;
    }
    memcpy(out_data, context->sb_latest_frame, context->latest_frame_size);
    *out_size = context->latest_frame_size;
}

static size_t _mjpeg_write_callback(char *ptr, size_t size, size_t nmemb, void *void_context) {

    _Context *context = (_Context *)void_context;

    size_t total_size = size * nmemb;
    if (total_size > 0) {

        // copy data
        sbpushn(context->sb_buffer, ptr, total_size);

        // set states
        context->state = _CONNECTION_STATE_CONNECTED;

        // Try to find JPEG frame boundaries
        unsigned char *end_addr = _memrmem(context->sb_buffer, sbcount(context->sb_buffer), "\xFF\xD9", 2);
        if (end_addr) {

            // size of buffer up to end_addr
            size_t upto_size = end_addr - context->sb_buffer;

            // get start address, process
            unsigned char *start_addr = _memmem(context->sb_buffer, upto_size, "\xFF\xD8", 2);
            if (start_addr) {

                // get size of jpeg
                size_t jpeg_size = end_addr - start_addr + 2;

                // save to output
                sbclear(context->sb_latest_frame);
                sbpushn(context->sb_latest_frame, start_addr, jpeg_size);

                // clear internal buffer up to end_addr + 2
                sbshiftn(context->sb_buffer, upto_size + 2);

                // raise flag
                context->has_new_data      = true;
                context->latest_frame_size = jpeg_size;
            }
        }
    }

    return total_size;
}

static void _mjpeg_connect(_Context *context) {

    // remove old handle
    if (context->easy_handle) {
        curl_multi_remove_handle(_multi_handle, context->easy_handle);
        curl_easy_cleanup(context->easy_handle);
    }

    // create new handle
    context->easy_handle = curl_easy_init();

    // self-explanatory
    curl_easy_setopt(context->easy_handle, CURLOPT_URL, context->url);

    // callback + struct used in parameter
    curl_easy_setopt(context->easy_handle, CURLOPT_WRITEFUNCTION, _mjpeg_write_callback);
    curl_easy_setopt(context->easy_handle, CURLOPT_WRITEDATA, context);

    // non blocking
    curl_easy_setopt(context->easy_handle, CURLOPT_TIMEOUT, 0L);

    // boilerplate, more or less
    curl_easy_setopt(context->easy_handle, CURLOPT_BUFFERSIZE, 512000L);
    curl_easy_setopt(context->easy_handle, CURLOPT_NOPROGRESS, 1L);

    // kernel level cleanups (really just boilerplate)
    curl_easy_setopt(context->easy_handle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(context->easy_handle, CURLOPT_TCP_KEEPIDLE, 30L);
    curl_easy_setopt(context->easy_handle, CURLOPT_TCP_KEEPINTVL, 15L);

    // timeout in n seconds if under 10 bytes
    curl_easy_setopt(context->easy_handle, CURLOPT_LOW_SPEED_LIMIT, 10L);
    curl_easy_setopt(context->easy_handle, CURLOPT_LOW_SPEED_TIME, (long)context->timeout_s);

    // hand our handle back on DONE messages without searching
    curl_easy_setopt(context->easy_handle, CURLOPT_PRIVATE, context);

    // add to multi
    curl_multi_add_handle(_multi_handle, context->easy_handle);

    // set new states
    context->state         = _CONNECTION_STATE_CONNECTING;
    context->timeout_begin = time(NULL);
    printf("DCApp _mjpeg_connect(): MJPEG Attempting to connect...\n");
}

// does not account for timeouts
static _ConnectionState _get_connection_state(CURL *easy_handle) {
    curl_socket_t curl_socket_state = CURL_SOCKET_BAD;
    if (curl_easy_getinfo(easy_handle, CURLINFO_ACTIVESOCKET, &curl_socket_state) != CURLE_OK) {
        return _CONNECTION_STATE_DISCONNECTED;
    }
    if (curl_socket_state == CURL_SOCKET_BAD) {
        return _CONNECTION_STATE_CONNECTING;
    }
    return _CONNECTION_STATE_CONNECTED;
}

// memmem() not part of the C standard
static void *_memmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len) {
    if (needle_len == 0 || haystack_len < needle_len)
        return NULL;

    const unsigned char *hh = haystack;
    for (size_t ii = 0; ii <= haystack_len - needle_len; ++ii) {
        if (memcmp(hh + ii, needle, needle_len) == 0) {
            return (void *)(hh + ii);
        }
    }
    return NULL;
}

static void *_memrmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len) {
    if (needle_len == 0 || haystack_len < needle_len) {
        return NULL;
    }

    const unsigned char *hh = (const unsigned char *)haystack;
    const unsigned char *nn = (const unsigned char *)needle;

    for (size_t ii = haystack_len - needle_len + 1; ii-- > 0;) {
        if (memcmp(hh + ii, nn, needle_len) == 0) {
            return (void *)(hh + ii);
        }
    }

    return NULL;
}
