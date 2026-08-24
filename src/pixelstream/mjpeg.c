#include "mjpeg.h"
#include "../utils/stb_sb.h"
#include "../utils/log.h"

#include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
    _CONNECTION_STATE_UNKNOWN = 0,
    _CONNECTION_STATE_DISCONNECTED,
    _CONNECTION_STATE_CONNECTING,
    _CONNECTION_STATE_CONNECTED
} _ConnectionState;

#define _MAX_CONNECTIONS 10
#define _MAX_URL_LENGTH 2048

struct DcPsMjpegServer {
    DcPsMjpegContext *context;

    // user settings
    char url[_MAX_URL_LENGTH];
    int timeout_s;

    // connection state
    _ConnectionState state;
    bool has_new_data;

    // internal timeout
    time_t timeout_begin;

    // curl handle
    CURL *easy_handle;

    // internal stretchy buffer
    unsigned char *sb_buffer;

    // latest data
    unsigned char *sb_latest_frame;
    size_t latest_frame_size;
};

struct DcPsMjpegContext {
    CURLM *multi_handle;
    // Servers are separate allocations because callers and libcurl need stable addresses.
    DcPsMjpegServer **sb_servers;
};

// static functions
static void _mjpeg_connect(DcPsMjpegServer *server);
static _ConnectionState _get_connection_state(CURL *easy_handle);
static void _mjpeg_server_cleanup(DcPsMjpegServer *server);
static size_t _mjpeg_write_callback(char *ptr, size_t size, size_t nmemb, void *void_context);
static void *_memmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len);
static void *_memrmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len);

DcPsMjpegContext *dc_ps_mjpeg_context_create(void) {
    // init curl
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        DC_LOG_ERROR("MJPEG", "Failed to initialize libcurl");
        return NULL;
    }

    DcPsMjpegContext *context = calloc(1, sizeof(*context));
    if (!context) {
        curl_global_cleanup();
        return NULL;
    }

    context->multi_handle = curl_multi_init();
    if (!context->multi_handle) {
        free(context);
        curl_global_cleanup();
        return NULL;
    }

    // allocate stretchy buffers
    sbgrow(context->sb_servers, _MAX_CONNECTIONS, sizeof(*context->sb_servers));

    return context;
}

void dc_ps_mjpeg_update(DcPsMjpegContext *context) {
    if (!context || !context->multi_handle) return;

    // for each server
    for (int ii = 0; ii < sbcount(context->sb_servers); ii++) {
        DcPsMjpegServer *server = context->sb_servers[ii];
        if (!server) continue;

        // clear new data indicator
        server->has_new_data = false;
        server->latest_frame_size = 0;

        // libcurl keeps callback pointers. Refresh them after an app dylib reload
        // before libcurl can invoke code from the previous dylib.
        if (server->easy_handle) {
            curl_easy_setopt(server->easy_handle, CURLOPT_WRITEFUNCTION, _mjpeg_write_callback);
            curl_easy_setopt(server->easy_handle, CURLOPT_WRITEDATA, server);
            curl_easy_setopt(server->easy_handle, CURLOPT_PRIVATE, server);
        }

        // check for reconnections
        if (server->state == _CONNECTION_STATE_DISCONNECTED || server->state == _CONNECTION_STATE_CONNECTING) {
            if (time(NULL) - server->timeout_begin >= server->timeout_s) {
                _mjpeg_connect(server);
            }
        }
    }

    // perform reads
    int still_running;
    curl_multi_perform(context->multi_handle, &still_running);

    // Check for completed transfers
    CURLMsg *msg;
    int msgs_left;
    while ((msg = curl_multi_info_read(context->multi_handle, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {

            // get handles
            CURL *handle = msg->easy_handle;
            CURLcode result = msg->data.result;
            DcPsMjpegServer *server = NULL;
            curl_easy_getinfo(handle, CURLINFO_PRIVATE, &server);
            if (!server) continue;

            switch (result) {

                case (CURLE_OK): {
                    // for MJPEG streams, a completed transfer means the server
                    // closed the connection (the stream is supposed to be continuous)
                    DC_LOG_WARN("MJPEG", "[%s] Stream ended, will reconnect", server->url);
                    server->state = _CONNECTION_STATE_DISCONNECTED;
                    break;
                }

                case CURLE_OPERATION_TIMEDOUT: {
                    // reconnect immediately
                    _mjpeg_connect(server);
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
                    DC_LOG_ERROR("MJPEG", "[%s] Disconnected or failed to connect: %s", server->url, curl_easy_strerror(result));
                    server->state = _CONNECTION_STATE_DISCONNECTED;
                    break;
                }

                case CURLE_PEER_FAILED_VERIFICATION:
                case CURLE_USE_SSL_FAILED: {
                    DC_LOG_ERROR("MJPEG", "[%s] SSL verification or setup failed: %s", server->url, curl_easy_strerror(result));
                    server->state = _CONNECTION_STATE_DISCONNECTED;
                    break;
                }

                default:
                    DC_LOG_ERROR("MJPEG", "Unknown code from curl_multi_info_read()");
                    break;
            }
        }
    }
}

void dc_ps_mjpeg_context_destroy(DcPsMjpegContext *context) {
    if (!context) return;

    // cleanup each server
    for (int ii = 0; ii < sbcount(context->sb_servers); ii++) {
        DcPsMjpegServer *server = context->sb_servers[ii];
        if (!server) continue;
        _mjpeg_server_cleanup(server);
        free(server);
    }
    sbfree(context->sb_servers);

    if (context->multi_handle)
        curl_multi_cleanup(context->multi_handle);
    free(context);
    curl_global_cleanup();
}

DcPsMjpegServer *dc_ps_mjpeg_add_server(DcPsMjpegContext *context, const char *url, int timeout_s) {
    if (!context) return NULL;

    // check url
    if (strlen(url) > _MAX_URL_LENGTH - 1) {
        DC_LOG_ERROR("MJPEG", "dc_pixelstream_mjpeg_create(): Length of URL exceeds maximum length");
    }

    // create context
    DcPsMjpegServer *server = (DcPsMjpegServer *)malloc(sizeof(DcPsMjpegServer));
    if (!server) return NULL;
    memset(server, 0, sizeof(DcPsMjpegServer));

    strncpy(server->url, url, _MAX_URL_LENGTH);
    server->url[_MAX_URL_LENGTH - 1] = '\0';
    server->timeout_s = timeout_s;
    server->state = _CONNECTION_STATE_DISCONNECTED;
    server->has_new_data = false;
    server->sb_buffer = NULL;
    server->sb_latest_frame = NULL;
    server->latest_frame_size = 0;
    server->easy_handle = NULL;
    server->context = context;

    bool stored = false;
    for (int ii = 0; ii < sbcount(context->sb_servers); ii++) {
        if (!context->sb_servers[ii]) {
            context->sb_servers[ii] = server;
            stored = true;
            break;
        }
    }
    if (!stored)
        sbpush(context->sb_servers, server);

    return server;
}

void dc_ps_mjpeg_remove_server(DcPsMjpegServer *server) {
    if (!server) return;

    DcPsMjpegContext *context = server->context;
    if (context) {
        for (int ii = 0; ii < sbcount(context->sb_servers); ii++) {
            if (context->sb_servers[ii] == server) {
                context->sb_servers[ii] = NULL;
                break;
            }
        }
    }

    _mjpeg_server_cleanup(server);
    free(server);
}

bool dc_ps_mjpeg_server_is_connected(DcPsMjpegServer *server) {
    return server && server->state == _CONNECTION_STATE_CONNECTED;
}

bool dc_ps_mjpeg_server_has_new_data(DcPsMjpegServer *server) {
    return server && server->has_new_data;
}

void dc_ps_mjpeg_get_server_data(DcPsMjpegServer *server, unsigned char *out_data, size_t out_data_size, size_t *out_size) {
    if (!server) {
        *out_size = 0;
        return;
    }

    if (out_data_size < server->latest_frame_size) {
        DC_LOG_ERROR("MJPEG", "dc_ps_mjpeg_get_server_data(): output buffer too small");
        return;
    }
    memcpy(out_data, server->sb_latest_frame, server->latest_frame_size);
    *out_size = server->latest_frame_size;
}

static size_t _mjpeg_write_callback(char *ptr, size_t size, size_t nmemb, void *void_context) {

    DcPsMjpegServer *server = (DcPsMjpegServer *)void_context;

    size_t total_size = size * nmemb;
    if (total_size > 0) {

        // copy data
        sbpushn(server->sb_buffer, ptr, (int)total_size);

        // set states
        server->state = _CONNECTION_STATE_CONNECTED;

        // Try to find JPEG frame boundaries
        unsigned char *end_addr = _memrmem(server->sb_buffer, sbcount(server->sb_buffer), "\xFF\xD9", 2);
        if (end_addr) {

            // size of buffer up to end_addr
            size_t upto_size = end_addr - server->sb_buffer;

            // get start address, process
            unsigned char *start_addr = _memmem(server->sb_buffer, upto_size, "\xFF\xD8", 2);
            if (start_addr) {

                // get size of jpeg
                size_t jpeg_size = end_addr - start_addr + 2;

                // save to output
                sbclear(server->sb_latest_frame);
                sbpushn(server->sb_latest_frame, start_addr, (int)jpeg_size);

                // clear internal buffer up to end_addr + 2
                sbshiftn(server->sb_buffer, (int)(upto_size + 2));

                // raise flag
                server->has_new_data = true;
                server->latest_frame_size = jpeg_size;
            }
        }
    }

    return total_size;
}

static void _mjpeg_connect(DcPsMjpegServer *server) {

    DcPsMjpegContext *context = server->context;
    if (!context || !context->multi_handle) return;

    // remove old handle
    if (server->easy_handle) {
        curl_multi_remove_handle(context->multi_handle, server->easy_handle);
        curl_easy_cleanup(server->easy_handle);
    }

    // create new handle
    server->easy_handle = curl_easy_init();
    if (!server->easy_handle) {
        server->state = _CONNECTION_STATE_DISCONNECTED;
        return;
    }

    // self-explanatory
    curl_easy_setopt(server->easy_handle, CURLOPT_URL, server->url);

    // callback + struct used in parameter
    curl_easy_setopt(server->easy_handle, CURLOPT_WRITEFUNCTION, _mjpeg_write_callback);
    curl_easy_setopt(server->easy_handle, CURLOPT_WRITEDATA, server);

    // non blocking
    curl_easy_setopt(server->easy_handle, CURLOPT_TIMEOUT, 0L);

    // boilerplate, more or less
    curl_easy_setopt(server->easy_handle, CURLOPT_BUFFERSIZE, 512000L);
    curl_easy_setopt(server->easy_handle, CURLOPT_NOPROGRESS, 1L);

    // kernel level cleanups (really just boilerplate)
    curl_easy_setopt(server->easy_handle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(server->easy_handle, CURLOPT_TCP_KEEPIDLE, 30L);
    curl_easy_setopt(server->easy_handle, CURLOPT_TCP_KEEPINTVL, 15L);

    // timeout in n seconds if under 10 bytes
    curl_easy_setopt(server->easy_handle, CURLOPT_LOW_SPEED_LIMIT, 10L);
    curl_easy_setopt(server->easy_handle, CURLOPT_LOW_SPEED_TIME, (long)server->timeout_s);

    // hand our handle back on DONE messages without searching
    curl_easy_setopt(server->easy_handle, CURLOPT_PRIVATE, server);

    // add to multi
    curl_multi_add_handle(context->multi_handle, server->easy_handle);

    // set new states
    server->state = _CONNECTION_STATE_CONNECTING;
    server->timeout_begin = time(NULL);
    DC_LOG_INFO("MJPEG", "[%s] Attempting to connect...", server->url);
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

static void _mjpeg_server_cleanup(DcPsMjpegServer *server) {
    // remove old handle
    if (server->easy_handle) {
        curl_multi_remove_handle(server->context->multi_handle, server->easy_handle);
        curl_easy_cleanup(server->easy_handle);
        server->easy_handle = NULL;
    }

    // free memory
    sbfree(server->sb_buffer);
    sbfree(server->sb_latest_frame);
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
