#include "sock.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/tcp.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

static int sock_count = 0;

DcSock dc_sock_create(DcSockFlags flags) {

    // windows global socket context
#ifdef _WIN32
    if (!sock_count) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
#endif
    sock_count++;

    // create sock
    DcSock sock;
    sock.sock_fd = 0;
    sock.flags   = flags;
    return sock;
}

DcSockResult dc_sock_host_to_ip(const char *host, char *out) {

    // Check if already a valid IP address
    struct in_addr addr4;
    struct in6_addr addr6;
    if (inet_pton(AF_INET, host, &addr4) == 1 || inet_pton(AF_INET6, host, &addr6) == 1) {
        strcpy(out, host);
        return DC_SOCK_RESULT_SUCCESS;
    }

    struct addrinfo hints = {0}, *result, *entry;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, NULL, &hints, &result) != 0) {
        fprintf(stderr, "Failed to resolve host: %s\n", host);
        return DC_SOCK_RESULT_FAIL;
    }

    char ip_str[INET6_ADDRSTRLEN] = {0};
    void *addr_ptr = NULL;
    int family = 0;

    // default to ipv4
    for (entry = result; entry != NULL; entry = entry->ai_next) {
        if (entry->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)entry->ai_addr;
            addr_ptr = &(ipv4->sin_addr);
            family = AF_INET;
            break;
        }
    }

    // fallback to ipv6
    if (!addr_ptr) {
        for (entry = result; entry != NULL; entry = entry->ai_next) {
            if (entry->ai_family == AF_INET6) {
                struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)entry->ai_addr;
                addr_ptr = &(ipv6->sin6_addr);
                family = AF_INET6;
                break;
            }
        }
    }

    if (addr_ptr && inet_ntop(family, addr_ptr, ip_str, sizeof(ip_str))) {
        strcpy(out, ip_str);
    } else {
        perror("dc_sock_host_to_ip(): inet_ntop");
    }

    freeaddrinfo(result);
    return DC_SOCK_RESULT_SUCCESS;
}

DcSockResult dc_sock_set_non_nagle(DcSock *sock) {
    int flag = 1;
    int result = setsockopt(sock->sock_fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, (socklen_t)sizeof(flag));
    if (result < 0) {
        perror("dc_sock_set_non_nagle() error");
        return DC_SOCK_RESULT_FAIL;
    }
    return DC_SOCK_RESULT_SUCCESS;

}

DcSockResult dc_sock_set_non_blocking(DcSock *sock) {
#ifdef _WIN32
    u_long mode = 1;
    if (!ioctlsocket(sock->_sock_fd, FIONBIO, &mode)) {
#else
    int flags = fcntl(sock->sock_fd, F_GETFL, 0);
    if (fcntl(sock->sock_fd, F_SETFL, flags | O_NONBLOCK) != 0) {
#endif
        perror("dc_sock_set_non_blocking(): Unable to set socket to non-blocking\n");
        return DC_SOCK_RESULT_FAIL;
    }
    return DC_SOCK_RESULT_SUCCESS;
}

DcSockResult dc_sock_connect(DcSock *sock, const char *ip, int port) {
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;
    socklen_t socket_addr_len;
    int family;
    struct sockaddr *addr_ptr = NULL;

    // setup ipv4 vs. v6
    if (inet_pton(AF_INET, ip, &addr4.sin_addr) == 1) {
        family = AF_INET;
        addr4.sin_family = AF_INET;
        addr4.sin_port = htons(port);
        socket_addr_len = sizeof(addr4);
        addr_ptr = (struct sockaddr *)&addr4;
    } else if (inet_pton(AF_INET6, ip, &addr6.sin6_addr) == 1) {
        family = AF_INET6;
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(port);
        socket_addr_len = sizeof(addr6);
        addr_ptr = (struct sockaddr *)&addr6;
    } else {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        return DC_SOCK_RESULT_FAIL;
    }

    // create socket
    sock->sock_fd = socket(family, SOCK_STREAM, 0);
    if (sock->sock_fd < 0) {
        perror("dc_sock_connect(): Failed to create socket");
        return DC_SOCK_RESULT_FAIL;
    }

    // disable nagle's algorithm
    if (sock->flags & DC_SOCK_FLAGS_NON_NAGLE) {
        dc_sock_set_non_nagle(sock);
    }

    // make socket non-blocking
    if (sock->flags & DC_SOCK_FLAGS_NON_BLOCKING) {
        dc_sock_set_non_blocking(sock);
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
                perror("dc_sock_connect(): Failed to connect to socket");
                return DC_SOCK_RESULT_FAIL;
            }
        }
    }

    // connection now active
    return DC_SOCK_RESULT_SUCCESS;
}

void dc_sock_close(DcSock *sock) {

    sock_count--;
#ifdef _WIN32
    closesocket(sock->sock_fd);
    if (!sock_count) {
        WSACleanup();
    }
#else
    close(sock->sock_fd);
#endif
}

DcSockState dc_sock_connection_status(DcSock *s) {
    if (!s || s->sock_fd < 0) return DC_SOCK_STATE_DISCONNECTED;

    if (s->flags & DC_SOCK_FLAGS_NON_BLOCKING) {
        fd_set wfds, efds;
        FD_ZERO(&wfds); FD_ZERO(&efds);
        FD_SET(s->sock_fd, &wfds);
        FD_SET(s->sock_fd, &efds);
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 0;
        int ret = select(s->sock_fd + 1, NULL, &wfds, &efds, &tv);
        if (ret < 0) return DC_SOCK_STATE_DISCONNECTED;
        if (ret == 0) return DC_SOCK_STATE_CONNECTING;
        if (!FD_ISSET(s->sock_fd, &wfds) && !FD_ISSET(s->sock_fd, &efds))
            return DC_SOCK_STATE_CONNECTING;
    }

    int err = 0; socklen_t len = sizeof(err);
    if (getsockopt(s->sock_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0)
        return DC_SOCK_STATE_DISCONNECTED;

    struct sockaddr_storage peer; socklen_t plen = sizeof(peer);
    if (getpeername(s->sock_fd, (struct sockaddr*)&peer, &plen) == 0)
        return DC_SOCK_STATE_CONNECTED;

    return (errno == ENOTCONN) ? DC_SOCK_STATE_CONNECTING : DC_SOCK_STATE_DISCONNECTED;
}


DcSockResult dc_sock_send(DcSock *sock, const char *in, size_t in_size, int *sent_size) {
    int sent = send(sock->sock_fd, in, in_size, 0);
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
            return DC_SOCK_RESULT_CONN_FAIL;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return DC_SOCK_RESULT_CONN_WOULD_BLOCK;
        } else if (errno == EINTR) {
            return DC_SOCK_RESULT_CONN_INTERRUPTED;
        } else if (errno == ECONNRESET || errno == EPIPE) {
            return DC_SOCK_RESULT_CONN_CLOSED;
        } else {
            perror("dc_sock_send failed");
            return DC_SOCK_RESULT_FAIL;
        }
#endif
    }
    return DC_SOCK_RESULT_SUCCESS;
}

DcSockResult dc_sock_receive(DcSock *sock, char *out, size_t out_size, int *receive_size) {
    int received = recv(sock->sock_fd, out, out_size, 0);
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
            return DC_SOCK_RESULT_CONN_FAIL;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return DC_SOCK_RESULT_CONN_WOULD_BLOCK;
        } else if (errno == EINTR) {
            return DC_SOCK_RESULT_CONN_INTERRUPTED;
        } else if (errno == ECONNRESET || errno == EPIPE) {
            return DC_SOCK_RESULT_CONN_CLOSED;
        } else {
            perror("dc_sock_receive failed");
            return DC_SOCK_RESULT_FAIL;
        }
#endif
    }
    return DC_SOCK_RESULT_SUCCESS;
}
