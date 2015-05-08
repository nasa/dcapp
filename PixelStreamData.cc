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
#include "PixelStream.hh"
#include "PixelStreamData.hh"
#include "msg.hh"
#include "timer.hh"
#include "getbytes.hh"

PixelStreamData::PixelStreamData()
:
pixels(0x0),
width(0),
height(0),
host(0x0),
port(0),
ClientToServerSocket(-1),
ServerToClientSocket(-1),
chunksize(0),
datasize(0),
dataalloc(0),
sockets_connected(0),
handshake_sent(0),
handshake_received(0),
handshake_complete(0),
data_requested(0),
header_received(0)
{
    StartTimer(&lastconnectattempt);
}

PixelStreamData::~PixelStreamData()
{
    this->disconnect_all();
    if (this->host) free(this->host);
}

int PixelStreamData::socket_connect(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error_msg("Unable to create socket");
        perror(0x0);
        return (-1);
    }

    char noblock_flag = 1;  // 0 = blocking, 1 = non-blocking
    if (ioctl(sockfd, FIONBIO, &noblock_flag) == -1)
    {
        error_msg("Unable to make socket non-blocking");
        perror(0x0);
        close(sockfd);
        return (-1);
    }

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

int PixelStreamData::initialize(char *hostspec, int portspec)
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

int PixelStreamData::update(void)
{
    Timer looptimer;
    int chunk_requested, chunk_received, bytes_read, updated=0;

    if (!handshake_complete)
    {
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
                }
                StartTimer(&lastconnectattempt);
            }
        }
        else if (!handshake_sent) handshake_sent = SendHandshake();
        else if (!handshake_received)
        {
            chunksize = RecvHandshake();
            if (chunksize) handshake_received = 1;
        }
        else
        {
            handshake_complete = 1;
            StartTimer(&lastread);
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
            chunk_requested = 0;
            chunk_received = 0;
            bytes_read = 0;
            StartTimer(&looptimer);
            while (sockets_connected && bytes_read < datasize)
            {
                if (SecondsElapsed(looptimer) > CONNECTION_TIMEOUT)
                {
                    debug_msg("Main loop timeout, disconnecting...\n");
                    disconnect_all();
                }
                if (!chunk_requested) chunk_requested = SendChunkRequest();
                if (chunk_requested && !chunk_received) chunk_received = RecvChunk(((char *)pixels)+bytes_read, chunksize, datasize-bytes_read);
                if (chunk_received)
                {
                    bytes_read += chunk_received;
                    chunk_requested = 0;
                    chunk_received = 0;
                }
            }
            if (sockets_connected)
            {
                updated = 1;
                data_requested = 0;
                header_received = 0;
            }
        }
    }

    return updated;
}

void PixelStreamData::socket_disconnect(int *sockfd)
{
    if (*sockfd >= 0)
    {
        shutdown(*sockfd, SHUT_RDWR);
        close(*sockfd);
        *sockfd = -1;
    }
}

void PixelStreamData::disconnect_all(void)
{
    debug_msg("Disconnecting from server...\n");
    socket_disconnect(&ClientToServerSocket);
    socket_disconnect(&ServerToClientSocket);

    handshake_complete = 0;
    sockets_connected = 0;
    handshake_sent = 0;
    handshake_received = 0;
    data_requested = 0;
    header_received = 0;
}

int PixelStreamData::SendHandshake(void)
{
    char sendbuf[6];

    uint32_t maxrecvbuf;
    socklen_t optionlen = sizeof(maxrecvbuf);
    if (getsockopt(ClientToServerSocket, SOL_SOCKET, SO_RCVBUF, (void *) &maxrecvbuf, &optionlen)) maxrecvbuf = DEFAULT_MAX_BUFFER;

    uint16_t *sendendianflag = (uint16_t *)(&(sendbuf[0]));
    *sendendianflag = 1;
    bcopy((void *)&maxrecvbuf, (void *)(&(sendbuf[2])), sizeof(maxrecvbuf));
    if (write(ClientToServerSocket, &sendbuf, sizeof(sendbuf)) == sizeof(sendbuf)) return 1;
    else
    {
        debug_msg("SendHandshake failed");
        debug_perror(0x0);
        disconnect_all();
        return 0;
    }
}

uint32_t PixelStreamData::RecvHandshake(void)
{
    char recvbuf[6];
    uint32_t chunksize;

    if (read(ServerToClientSocket, &recvbuf, sizeof(recvbuf)) == sizeof(recvbuf))
    {
        uint16_t *recvendianflag = (uint16_t *)(&(recvbuf[0]));

        if (*recvendianflag == 1) byteswap = 0;
        else byteswap = 1;

        getbytes((char *)&(recvbuf[2]), (char *)&chunksize, sizeof(chunksize), byteswap);
        return chunksize;
    }
    else return 0;
}

int PixelStreamData::SendDataRequest(void)
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

int PixelStreamData::RecvHeader(uint32_t *width, uint32_t *height)
{
    char recvbuf[8];

    if (read(ServerToClientSocket, &recvbuf, sizeof(recvbuf)) == sizeof(recvbuf))
    {
        getbytes((char *)&(recvbuf[0]), (char *)width, sizeof(*width), byteswap);
        getbytes((char *)&(recvbuf[4]), (char *)height, sizeof(*height), byteswap);
        StartTimer(&lastread);
        return 1;
    }
    else return 0;
}

int PixelStreamData::SendChunkRequest(void)
{
    uint8_t command = REQUEST_CHUNK;

    if (write(ClientToServerSocket, &command, sizeof(command)) == sizeof(command))
    {
        return 1;
    }
    else
    {
        debug_msg("SendChunkRequest failed");
        debug_perror(0x0);
        disconnect_all();
        return 0;
    }
}

int PixelStreamData::RecvChunk(char *chunk, uint32_t chunksize, uint32_t bytesremaining)
{
    ssize_t n, bytesread = 0;
    Timer looptimer;

    uint32_t bytestoread = MIN(chunksize, bytesremaining);

    StartTimer(&looptimer);
    while (bytestoread > 0)
    {
        if (SecondsElapsed(looptimer) > CONNECTION_TIMEOUT)
        {
            debug_msg("Data connection timeout, disconnecting...\n");
            disconnect_all();
            return 0;
        }
        if (poll(&rfd, 1, 0) == 1)
        {
            n = read(ServerToClientSocket, chunk+bytesread, bytestoread);
            if (n < 0)
            {
                debug_msg("RecvChunk failed");
                debug_perror(0x0);
                return 0;
            }
            bytesread += n;
            bytestoread -= n;
        }
    }

    StartTimer(&lastread);
    return bytesread;
}

char *PixelStreamData::getHost(void)
{
    return this->host;
}

int PixelStreamData::getPort(void)
{
    return this->port;
}

