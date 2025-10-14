#ifndef _DC_SOCK_
#define _DC_SOCK_

#include <stddef.h>
#include <stdbool.h>
#ifdef _WIN32
    #include <stdint.h>
    typedef uintptr_t DcSockFd;
#else
    typedef int DcSockFd;
#endif

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

typedef struct _DcSock {
    DcSockFd    sock_fd;
    DcSockFlags flags;
} DcSock;

#ifdef __cplusplus
extern "C" {
#endif

DcSock       dc_sock_create(DcSockFlags flags);
DcSockResult dc_sock_host_to_ip(const char *host, char *out);
DcSockResult dc_sock_set_non_nagle(DcSock *sock);
DcSockResult dc_sock_set_non_blocking(DcSock *sock);
DcSockResult dc_sock_connect(DcSock *sock, const char *ip, int port);
void         dc_sock_close(DcSock *sock);
DcSockState  dc_sock_connection_status(DcSock *sock);
DcSockResult dc_sock_send(DcSock *sock, const char *in, size_t in_size, int *sent_size);
DcSockResult dc_sock_receive(DcSock *sock, char *out, size_t out_size, int *receive_size);

#ifdef __cplusplus
}
#endif

#endif
