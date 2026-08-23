#include "sock.h"
#include "utils/log.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Sentinel values for sock_fd.
// _DcSockFd is 'int' on POSIX and 'uintptr_t' on Windows (unsigned), so we
// can't use raw negative literals in comparisons — define typed sentinels instead.
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef uintptr_t _DcSockFd;
#define _DC_SOCK_FD_FAILED ((_DcSockFd)INVALID_SOCKET)
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int _DcSockFd;
#define _DC_SOCK_FD_FAILED ((_DcSockFd) - 1)
#endif

// A socket can exist before it owns a native socket.
#define _DC_SOCK_FD_ALLOCATED ((_DcSockFd) - 2) // reserved but not yet connected
#define _DC_SOCK_FD_IS_INVALID(fd) ((fd) == _DC_SOCK_FD_FAILED || (fd) == _DC_SOCK_FD_ALLOCATED)

struct DcSock {
    _DcSockFd   sock_fd;
    DcSockFlags flags;
};

static void         _close_fd(DcSock *sock);
static DcSockResult _set_non_nagle(DcSock *sock);
static DcSockResult _set_non_blocking(DcSock *sock);

DcSock *dc_sock_create(DcSockFlags flags) {

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        DC_LOG_ERROR("Sock", "dc_sock_create: WSAStartup failed");
        return NULL;
    }
#endif

    DcSock *sock = (DcSock *)malloc(sizeof(DcSock));
    if (!sock) {
        DC_LOG_ERROR("Sock", "dc_sock_create: allocation failed");
#ifdef _WIN32
        WSACleanup();
#endif
        return NULL;
    }

    sock->sock_fd = _DC_SOCK_FD_ALLOCATED;
    sock->flags   = flags;
    return sock;
}

DcSockResult dc_sock_host_to_ip(const char *host, char *out) {

    // Check if already a valid IP address
    struct in_addr  addr4;
    struct in6_addr addr6;
    if (inet_pton(AF_INET, host, &addr4) == 1 || inet_pton(AF_INET6, host, &addr6) == 1) {
        strncpy(out, host, INET6_ADDRSTRLEN - 1);
        out[INET6_ADDRSTRLEN - 1] = '\0';
        return DC_SOCK_RESULT_SUCCESS;
    }

    struct addrinfo hints = {0}, *result, *entry;
    hints.ai_family       = AF_UNSPEC;
    hints.ai_socktype     = SOCK_STREAM;
    if (getaddrinfo(host, NULL, &hints, &result) != 0) {
        DC_LOG_ERROR("Sock", "Failed to resolve host: %s", host);
        return DC_SOCK_RESULT_FAIL;
    }

    char  ip_str[INET6_ADDRSTRLEN] = {0};
    void *addr_ptr                 = NULL;
    int   family                   = 0;

    // default to ipv4
    for (entry = result; entry != NULL; entry = entry->ai_next) {
        if (entry->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)entry->ai_addr;
            addr_ptr                 = &(ipv4->sin_addr);
            family                   = AF_INET;
            break;
        }
    }

    // fallback to ipv6
    if (!addr_ptr) {
        for (entry = result; entry != NULL; entry = entry->ai_next) {
            if (entry->ai_family == AF_INET6) {
                struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)entry->ai_addr;
                addr_ptr                  = &(ipv6->sin6_addr);
                family                    = AF_INET6;
                break;
            }
        }
    }

    if (addr_ptr && inet_ntop(family, addr_ptr, ip_str, sizeof(ip_str))) {
        strncpy(out, ip_str, INET6_ADDRSTRLEN - 1);
        out[INET6_ADDRSTRLEN - 1] = '\0';
    } else {
        DC_LOG_ERROR("Sock", "host_to_ip inet_ntop: %s", strerror(errno));
    }

    freeaddrinfo(result);
    return DC_SOCK_RESULT_SUCCESS;
}

DcSockResult dc_sock_connect(DcSock *sock, const char *ip, int port) {
    if (!sock) return DC_SOCK_RESULT_FAIL;

    struct sockaddr_in  addr4;
    struct sockaddr_in6 addr6;
    socklen_t           socket_addr_len;
    int                 family;
    struct sockaddr    *addr_ptr = NULL;

    // setup ipv4 vs. v6
    if (inet_pton(AF_INET, ip, &addr4.sin_addr) == 1) {
        family           = AF_INET;
        addr4.sin_family = AF_INET;
        addr4.sin_port   = htons((u_short)port);
        socket_addr_len  = sizeof(addr4);
        addr_ptr         = (struct sockaddr *)&addr4;
    } else if (inet_pton(AF_INET6, ip, &addr6.sin6_addr) == 1) {
        family            = AF_INET6;
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port   = htons((u_short)port);
        socket_addr_len   = sizeof(addr6);
        addr_ptr          = (struct sockaddr *)&addr6;
    } else {
        DC_LOG_ERROR("Sock", "Invalid IP address: %s", ip);
        return DC_SOCK_RESULT_FAIL;
    }

    // create socket
    _close_fd(sock);
    sock->sock_fd = socket(family, SOCK_STREAM, 0);
#ifdef _WIN32
    if (sock->sock_fd == INVALID_SOCKET) {
#else
    if (sock->sock_fd < 0) {
#endif
        DC_LOG_ERROR("Sock", "Failed to create socket: %s", strerror(errno));
        return DC_SOCK_RESULT_FAIL;
    }

    // disable nagle's algorithm
    if (sock->flags & DC_SOCK_FLAGS_NON_NAGLE) {
        _set_non_nagle(sock);
    }

    // make socket non-blocking
    if (sock->flags & DC_SOCK_FLAGS_NON_BLOCKING) {
        _set_non_blocking(sock);
    }

    // connect
    if (connect(sock->sock_fd, addr_ptr, socket_addr_len) < 0) {
        if (sock->flags & DC_SOCK_FLAGS_NON_BLOCKING) {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
#else
            if (errno != EINPROGRESS) {
#endif
                DC_LOG_ERROR("Sock", "Failed to connect: %s", strerror(errno));
                return DC_SOCK_RESULT_FAIL;
            }
        }
    }

    return DC_SOCK_RESULT_SUCCESS;
}

void dc_sock_close(DcSock *sock) {
    if (!sock) return;

#ifdef _WIN32
    _close_fd(sock);
    WSACleanup();
#else
    _close_fd(sock);
#endif

    free(sock);
}

DcSockState dc_sock_connection_status(DcSock *sock) {
    if (!sock) return DC_SOCK_STATE_DISCONNECTED;

    if (_DC_SOCK_FD_IS_INVALID(sock->sock_fd))
        return DC_SOCK_STATE_DISCONNECTED;

    if (sock->flags & DC_SOCK_FLAGS_NON_BLOCKING) {
        fd_set wfds, efds;
        FD_ZERO(&wfds);
        FD_ZERO(&efds);
        FD_SET(sock->sock_fd, &wfds);
        FD_SET(sock->sock_fd, &efds);
        struct timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = 0;
        int ret    = select((int)(sock->sock_fd + 1), NULL, &wfds, &efds, &tv);
        if (ret < 0)
            return DC_SOCK_STATE_DISCONNECTED;
        if (ret == 0)
            return DC_SOCK_STATE_CONNECTING;
        if (!FD_ISSET(sock->sock_fd, &wfds) && !FD_ISSET(sock->sock_fd, &efds))
            return DC_SOCK_STATE_CONNECTING;
    }

    int       err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(sock->sock_fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len) < 0 || err != 0)
        return DC_SOCK_STATE_DISCONNECTED;

    struct sockaddr_storage peer;
    socklen_t               plen = sizeof(peer);
    if (getpeername(sock->sock_fd, (struct sockaddr *)&peer, &plen) == 0)
        return DC_SOCK_STATE_CONNECTED;

    return (errno == ENOTCONN) ? DC_SOCK_STATE_CONNECTING : DC_SOCK_STATE_DISCONNECTED;
}

DcSockResult dc_sock_send(DcSock *sock, const char *in, size_t in_size, int *sent_size) {
    if (!sock) return DC_SOCK_RESULT_FAIL;

#if defined(__linux__)
    int sent = send(sock->sock_fd, in, (int)in_size, MSG_NOSIGNAL);
#else
    int sent = send(sock->sock_fd, in, (int)in_size, 0);
#endif
    if (sent_size) {
        *sent_size = sent;
    }

    if (sent == 0 && in_size != 0) {
        return DC_SOCK_RESULT_CONN_CLOSED;
    } else if (sent == -1) {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return DC_SOCK_RESULT_CONN_WOULD_BLOCK;
        } else if (err == WSAEINTR) {
            return DC_SOCK_RESULT_CONN_INTERRUPTED;
        } else if (err == WSAECONNRESET || err == WSAENOTCONN || err == WSAECONNABORTED) {
            return DC_SOCK_RESULT_CONN_CLOSED;
        } else {
            return DC_SOCK_RESULT_FAIL;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return DC_SOCK_RESULT_CONN_WOULD_BLOCK;
        } else if (errno == EINTR) {
            return DC_SOCK_RESULT_CONN_INTERRUPTED;
        } else if (errno == ECONNRESET || errno == EPIPE) {
            return DC_SOCK_RESULT_CONN_CLOSED;
        } else {
            DC_LOG_ERROR("Sock", "send failed: %s", strerror(errno));
            return DC_SOCK_RESULT_FAIL;
        }
#endif
    }
    return DC_SOCK_RESULT_SUCCESS;
}

DcSockResult dc_sock_receive(DcSock *sock, char *out, size_t out_size, int *receive_size) {
    if (!sock) return DC_SOCK_RESULT_FAIL;

    int received = recv(sock->sock_fd, out, (int)out_size, 0);
    if (receive_size) {
        *receive_size = received;
    }

    if (received == 0 && out_size != 0) {
        return DC_SOCK_RESULT_CONN_CLOSED;
    } else if (received == -1) {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return DC_SOCK_RESULT_CONN_WOULD_BLOCK;
        } else if (err == WSAEINTR) {
            return DC_SOCK_RESULT_CONN_INTERRUPTED;
        } else if (err == WSAECONNRESET || err == WSAENOTCONN || err == WSAECONNABORTED) {
            return DC_SOCK_RESULT_CONN_CLOSED;
        } else {
            return DC_SOCK_RESULT_FAIL;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return DC_SOCK_RESULT_CONN_WOULD_BLOCK;
        } else if (errno == EINTR) {
            return DC_SOCK_RESULT_CONN_INTERRUPTED;
        } else if (errno == ECONNRESET || errno == EPIPE) {
            return DC_SOCK_RESULT_CONN_CLOSED;
        } else {
            DC_LOG_ERROR("Sock", "receive failed: %s", strerror(errno));
            return DC_SOCK_RESULT_FAIL;
        }
#endif
    }
    return DC_SOCK_RESULT_SUCCESS;
}

DcSockResult dc_sock_set_blocking(DcSock *sock) {
    if (!sock) return DC_SOCK_RESULT_FAIL;
#ifdef _WIN32
    u_long mode = 0;
    if (ioctlsocket(sock->sock_fd, FIONBIO, &mode) != 0) {
#else
    int flags = fcntl(sock->sock_fd, F_GETFL, 0);
    if (fcntl(sock->sock_fd, F_SETFL, flags & ~O_NONBLOCK) != 0) {
#endif
        DC_LOG_ERROR("Sock", "set_blocking: %s", strerror(errno));
        return DC_SOCK_RESULT_FAIL;
    }
    return DC_SOCK_RESULT_SUCCESS;
}

DcSockResult dc_sock_set_recv_timeout(DcSock *sock, int timeout_ms) {
    if (!sock) return DC_SOCK_RESULT_FAIL;
#ifdef _WIN32
    DWORD tv = (DWORD)timeout_ms;
    if (setsockopt(sock->sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
#else
    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (setsockopt(sock->sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
#endif
        DC_LOG_ERROR("Sock", "set_recv_timeout: %s", strerror(errno));
        return DC_SOCK_RESULT_FAIL;
    }
    return DC_SOCK_RESULT_SUCCESS;
}

DcSockResult dc_sock_shutdown_write(DcSock *sock) {
    if (!sock) return DC_SOCK_RESULT_FAIL;
#ifdef _WIN32
    if (shutdown(sock->sock_fd, SD_SEND) != 0) {
#else
    if (shutdown(sock->sock_fd, SHUT_WR) != 0) {
#endif
        DC_LOG_ERROR("Sock", "shutdown_write: %s", strerror(errno));
        return DC_SOCK_RESULT_FAIL;
    }
    return DC_SOCK_RESULT_SUCCESS;
}

static void _close_fd(DcSock *sock) {
    if (_DC_SOCK_FD_IS_INVALID(sock->sock_fd)) return;
#ifdef _WIN32
    closesocket(sock->sock_fd);
#else
    close(sock->sock_fd);
#endif
    sock->sock_fd = _DC_SOCK_FD_ALLOCATED;
}

static DcSockResult _set_non_nagle(DcSock *sock) {
    int flag   = 1;
    int result = setsockopt(sock->sock_fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, (socklen_t)sizeof(flag));
    if (result < 0) {
        DC_LOG_ERROR("Sock", "set_non_nagle: %s", strerror(errno));
        return DC_SOCK_RESULT_FAIL;
    }
    return DC_SOCK_RESULT_SUCCESS;
}

static DcSockResult _set_non_blocking(DcSock *sock) {
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(sock->sock_fd, FIONBIO, &mode) != 0) {
#else
    int flags = fcntl(sock->sock_fd, F_GETFL, 0);
    if (fcntl(sock->sock_fd, F_SETFL, flags | O_NONBLOCK) != 0) {
#endif
        DC_LOG_ERROR("Sock", "set_non_blocking: %s", strerror(errno));
        return DC_SOCK_RESULT_FAIL;
    }
    return DC_SOCK_RESULT_SUCCESS;
}
