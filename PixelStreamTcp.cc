#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include "PixelStream.hh"
#include "PixelStreamTcp.hh"
#include "msg.hh"
#include "timer.hh"
#include "getbytes.hh"

PixelStreamTcp::PixelStreamTcp()
:
host(0x0),
port(0),
ClientToServerSocket(-1),
ServerToClientSocket(-1),
datasize(0),
dataalloc(0),
sockets_connected(0),
data_requested(0),
header_received(0)
{
    StartTimer(&lastconnectattempt);
}

PixelStreamTcp::~PixelStreamTcp()
{
    this->disconnect_all();
    if (this->host) free(this->host);
}

int PixelStreamTcp::socket_connect(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error_msg("Unable to create socket");
        perror(0x0);
        return (-1);
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    int retval = connect(sockfd, (struct sockaddr *)(&server_address), sizeof(struct sockaddr_in));
    if (retval < 0)
    {
        if (errno == EINPROGRESS)
        {
            struct timeval tv;
            fd_set myset;
            int valopt = 0;

            tv.tv_sec = 0;
            tv.tv_usec = CONNECTION_TIMEOUT_USEC;

            FD_ZERO(&myset);
            FD_SET(sockfd, &myset);
            if (select(sockfd+1, NULL, &myset, NULL, &tv) > 0)
            {
                socklen_t optionlen = sizeof(valopt);
                getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &optionlen);
                if (valopt)
                {
                    debug_msg("Socket not ready to communicate: %d - %s\n", valopt, strerror(valopt));
                    close(sockfd);
                    return -1;
                }
            }
            else
            {
                debug_msg("Timeout or error in select: %d - %s\n", valopt, strerror(valopt));
                close(sockfd);
                return -1;
            }
        }
        else
        {
            debug_msg("Socket connect error: %d\n", errno);
            close(sockfd);
            return -1;
        }
    }

    return sockfd;
}

int PixelStreamTcp::initialize(char *hostspec, int portspec)
{
    struct hostent *server;

    if (hostspec)
        server = gethostbyname(hostspec);
    else
        server = gethostbyname("localhost");

    if (!server)
    {
        error_msg("Unable to resolve host name: %s\n", hostspec);
        return -1;
    }

    this->host = strdup(server->h_name);
    this->port = portspec;

    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(this->port);

    return 0;
}

int PixelStreamTcp::update(void)
{
    int updated = 0;

    if (!sockets_connected)
    {
        if (SecondsElapsed(lastconnectattempt) > CONNECTION_ATTEMPT_INTERVAL)
        {
            if (ClientToServerSocket < 0) ClientToServerSocket = socket_connect();
            if (ClientToServerSocket >= 0 && ServerToClientSocket < 0) ServerToClientSocket = socket_connect();
            if (ClientToServerSocket >= 0 && ServerToClientSocket >= 0)
            {
                rfd.fd = ServerToClientSocket;
                rfd.events = POLLIN;
                rfd.revents = 0;
                wfd.fd = ClientToServerSocket;
                wfd.events = POLLOUT;
                wfd.revents = 0;

                sockets_connected = 1;
                StartTimer(&lastread);
            }
            StartTimer(&lastconnectattempt);
        }
    }
    else
    {
        if (SecondsElapsed(lastread) > CONNECTION_TIMEOUT)
        {
            debug_msg("Data connection timeout, disconnecting...\n");
            disconnect_all();
        }
        if (!data_requested) data_requested = SendDataRequest();
        if (data_requested && !header_received)
        {
            header_received = RecvHeader(&width, &height);
            if (header_received)
            {
                datasize = width * height * 3;
                if (datasize > dataalloc)
                {
                    dataalloc = datasize;
                    pixels = (void *)realloc(pixels, dataalloc);
                }
            }
        }
        if (header_received)
        {
            if (RecvData((char *)pixels, datasize)) updated = 1;
            data_requested = 0;
            header_received = 0;
        }
    }

    return updated;
}

void PixelStreamTcp::socket_disconnect(int *sockfd)
{
    if (*sockfd >= 0)
    {
        shutdown(*sockfd, SHUT_RDWR);
        close(*sockfd);
        *sockfd = -1;
    }
}

void PixelStreamTcp::disconnect_all(void)
{
    debug_msg("Disconnecting from server...\n");
    socket_disconnect(&ClientToServerSocket);
    socket_disconnect(&ServerToClientSocket);

    sockets_connected = 0;
    data_requested = 0;
    header_received = 0;
}

int PixelStreamTcp::SendDataRequest(void)
{
    uint8_t command = REQUEST_BUFFER;

    if (write(ClientToServerSocket, &command, sizeof(command)) == sizeof(command)) return 1;
    else
    {
        debug_msg("SendDataRequest failed");
        debug_perror(0x0);
        disconnect_all();
        return 0;
    }
}

int PixelStreamTcp::RecvHeader(uint32_t *width, uint32_t *height)
{
    char recvbuf[10];
    int byteswap;

    if (read(ServerToClientSocket, &recvbuf, sizeof(recvbuf)) == sizeof(recvbuf))
    {
        uint16_t *recvendianflag = (uint16_t *)(&(recvbuf[0]));
        if (*recvendianflag == 1) byteswap = 0;
        else byteswap = 1;
        getbytes((char *)&(recvbuf[2]), (char *)width, sizeof(*width), byteswap);
        getbytes((char *)&(recvbuf[6]), (char *)height, sizeof(*height), byteswap);
        StartTimer(&lastread);
        return 1;
    }
    else return 0;
}

int PixelStreamTcp::RecvData(char *buffer, uint32_t datasize)
{
    ssize_t n, totalBytesRead = 0;

    while (totalBytesRead < datasize)
    {
        if (poll(&rfd, 1, 0) == 1)
        {
            n = read(ServerToClientSocket, buffer + totalBytesRead, datasize - totalBytesRead);
            if (n < 0)
            {
                debug_msg("RecvData failed");
                debug_perror(0x0);
                disconnect_all();
                return 0;
            }
            else totalBytesRead += n;
        }
    }

    StartTimer(&lastread);
    return totalBytesRead;
}

char *PixelStreamTcp::getHost(void)
{
    return this->host;
}

int PixelStreamTcp::getPort(void)
{
    return this->port;
}
