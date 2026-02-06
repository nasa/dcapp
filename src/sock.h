#ifndef _DC_SOCK_
#define _DC_SOCK_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DC_SOCK_RESULT_SUCCESS = 0,
    DC_SOCK_RESULT_FAIL,
    DC_SOCK_RESULT_CONN_WOULD_BLOCK,
    DC_SOCK_RESULT_CONN_INTERRUPTED,
    DC_SOCK_RESULT_CONN_CLOSED,
} DcSockResult;

typedef enum {
    DC_SOCK_FLAGS_NONE         = 0,
    DC_SOCK_FLAGS_NON_BLOCKING = 1 << 0,
    DC_SOCK_FLAGS_NON_NAGLE    = 1 << 1,
} DcSockFlags;

typedef enum {
    DC_SOCK_STATE_UNKNOWN = 0,
    DC_SOCK_STATE_DISCONNECTED,
    DC_SOCK_STATE_CONNECTING,
    DC_SOCK_STATE_CONNECTED
} DcSockState;

typedef struct _DcSockHandle {
    uint8_t index;
} DcSockHandle;

#ifdef __cplusplus
extern "C" {
#endif

DcSockHandle dc_sock_create(DcSockFlags flags);
DcSockResult dc_sock_host_to_ip(const char *host, char *out);
DcSockResult dc_sock_connect(DcSockHandle sock, const char *ip, int port);
void         dc_sock_close(DcSockHandle sock);
DcSockState  dc_sock_connection_status(DcSockHandle sock);
DcSockResult dc_sock_send(DcSockHandle sock, const char *in, size_t in_size, int *sent_size);
DcSockResult dc_sock_receive(DcSockHandle sock, char *out, size_t out_size, int *receive_size);
DcSockResult dc_sock_set_blocking(DcSockHandle sock);
DcSockResult dc_sock_set_recv_timeout(DcSockHandle sock, int timeout_ms);
DcSockResult dc_sock_shutdown_write(DcSockHandle sock);

#ifdef __cplusplus
}
#endif

#endif
