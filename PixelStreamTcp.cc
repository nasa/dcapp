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
#include "PixelStreamTcp.hh"
#include "timer.hh"
#include "getbytes.hh"
#include "msg.hh"

PixelStreamTcp::PixelStreamTcp()
:
host(0x0),
port(0),
ListenSocket(-1),
ClientToServerSocket(-1),
ServerToClientSocket(-1),
datasize(0),
dataalloc(0),
sockets_connected(0),
data_requested(0),
header_sent(0),
header_received(0)
{
    this->protocol = PixelStreamTcpProtocol;
    this->lastconnectattempt = new Timer;
    this->lastread = new Timer;
}

PixelStreamTcp::~PixelStreamTcp()
{
    this->disconnect_all();
    socket_disconnect(&ListenSocket);
    if (this->host) free(this->host);
    if (this->pixels) free(this->pixels);
    delete this->lastconnectattempt;
    delete this->lastread;
}

int PixelStreamTcp::read_socket_connect(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error_msg("Unable to create socket: " << strerror(errno));
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
            if (select(sockfd+1, 0x0, &myset, 0x0, &tv) > 0)
            {
                socklen_t optionlen = sizeof(valopt);
                getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &optionlen);
                if (valopt)
                {
                    debug_msg("Socket not ready to communicate: " << valopt << " - " << strerror(valopt));
                    close(sockfd);
                    return -1;
                }
            }
            else
            {
                debug_msg("Timeout or error in select: " << valopt << " - " << strerror(valopt));
                close(sockfd);
                return -1;
            }
        }
        else
        {
            debug_msg("Socket connect error: " << errno);
            close(sockfd);
            return -1;
        }
    }

    return sockfd;
}

int PixelStreamTcp::write_socket_connect(int sockfd)
{
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    return (accept(sockfd, (struct sockaddr *) &cli_addr, &clilen));
}

int PixelStreamTcp::readerInitialize(char *hostspec, int portspec)
{
    struct hostent *server;

    if (hostspec)
        server = gethostbyname(hostspec);
    else
        server = gethostbyname("localhost");

    if (!server)
    {
        error_msg("Unable to resolve host name: " << hostspec);
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

int PixelStreamTcp::writerInitialize(int portspec)
{
    // Ignore broken pipe in case the client shuts down a socket
    signal(SIGPIPE, SIG_IGN);

    // Open up listening socket on specified port
    ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ListenSocket < 0)
    {
        error_msg("Unable to create listen socket: " << strerror(errno));
        return (-1);
    }

    int on = 1;
    setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, (socklen_t) sizeof(on));

    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portspec);
    if (bind(ListenSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        error_msg("Unable to bind listen socket: " << strerror(errno));
        return (-1);
    }

    int flags = fcntl(ListenSocket, F_GETFL, 0);
    fcntl(ListenSocket, F_SETFL, flags | O_NONBLOCK);

    listen(ListenSocket, 2);  // listen for two connections (one for each comm port)

    return 0;
}

int PixelStreamTcp::reader(void)
{
    int updated = 0;

    if (!sockets_connected)
    {
        if (this->lastconnectattempt->getSeconds() > CONNECTION_ATTEMPT_INTERVAL)
        {
            if (ClientToServerSocket < 0) ClientToServerSocket = read_socket_connect();
            if (ClientToServerSocket >= 0 && ServerToClientSocket < 0) ServerToClientSocket = read_socket_connect();
            if (ClientToServerSocket >= 0 && ServerToClientSocket >= 0)
            {
                rfd.fd = ServerToClientSocket;
                rfd.events = POLLIN;
                rfd.revents = 0;
                wfd.fd = ClientToServerSocket;
                wfd.events = POLLOUT;
                wfd.revents = 0;

                sockets_connected = 1;
                this->lastread->restart();
            }
            this->lastconnectattempt->restart();
        }
    }
    else
    {
        if (this->lastread->getSeconds() > CONNECTION_TIMEOUT)
        {
            debug_msg("Data connection timeout, disconnecting...");
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

int PixelStreamTcp::writer(void)
{
    if (!this->pixels) return 0;

    if (!sockets_connected)
    {
        if (ClientToServerSocket < 0) ClientToServerSocket = write_socket_connect(ListenSocket);
        else if (ServerToClientSocket < 0) ServerToClientSocket = write_socket_connect(ListenSocket);
        else
        {
            rfd.fd = ClientToServerSocket;
            rfd.events = POLLIN;
            rfd.revents = 0;
            wfd.fd = ServerToClientSocket;
            wfd.events = POLLOUT;
            wfd.revents = 0;

            sockets_connected = 1;
            this->lastread->restart();
        }
    }
    else
    {
        if (this->lastread->getSeconds() > CONNECTION_TIMEOUT)
        {
            error_msg("PixelStream data connection read timeout, disconnecting...");
            disconnect_all();
        }
        if (!data_requested) data_requested = RecvDataRequest();
        if (data_requested && !header_sent) header_sent = SendHeader(this->width, this->height);
        if (header_sent)
        {
            SendData((char *)(this->pixels), this->width * this->height * 3);
            data_requested = 0;
            header_sent = 0;
        }
    }

    return 1;
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
    debug_msg("Disconnecting from server...");
    socket_disconnect(&ClientToServerSocket);
    socket_disconnect(&ServerToClientSocket);

    sockets_connected = 0;
    data_requested = 0;
    header_sent = 0;
    header_received = 0;
}

int PixelStreamTcp::SendDataRequest(void)
{
    uint8_t command = REQUEST_BUFFER;

    if (write(ClientToServerSocket, &command, sizeof(command)) == sizeof(command)) return 1;
    else
    {
        debug_msg("Unable to write data: " << strerror(errno));
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
        this->lastread->restart();
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
                debug_msg("Unable to read data: " << strerror(errno));
                disconnect_all();
                return 0;
            }
            else totalBytesRead += n;
        }
    }

    this->lastread->restart();
    return totalBytesRead;
}

int PixelStreamTcp::RecvDataRequest(void)
{
    uint8_t command;

    if (read(ClientToServerSocket, &command, sizeof(command)) == sizeof(command) && command == REQUEST_BUFFER)
    {
        this->lastread->restart();
        return 1;
    }
    else return 0;
}

int PixelStreamTcp::SendHeader(uint32_t width, uint32_t height)
{
    char sendbuf[10];

    uint16_t *sendendianflag = (uint16_t *)(&(sendbuf[0]));
    *sendendianflag = 1;
    bcopy((void *)&width, (void *)(&(sendbuf[2])), sizeof(width));
    bcopy((void *)&height, (void *)(&(sendbuf[6])), sizeof(height));

    if (write(ServerToClientSocket, &sendbuf, sizeof(sendbuf)) == sizeof(sendbuf)) return 1;
    else
    {
        error_msg("Unable to write data: " << strerror(errno));
        disconnect_all();
        return 0;
    }
}

int PixelStreamTcp::SendData(char *buffer, uint32_t datasize)
{
    ssize_t n, totalBytesWritten = 0;

    while (totalBytesWritten < datasize)
    {
        if (poll(&wfd, 1, 0) == 1)
        {
            n = write(ServerToClientSocket, buffer + totalBytesWritten, datasize - totalBytesWritten);
            if (n < 0)
            {
                error_msg("Unable to write data: " << strerror(errno));
                disconnect_all();
                return 0;
            }
            else totalBytesWritten += n;
        }
    }
    return 1;
}

char *PixelStreamTcp::getHost(void)
{
    return this->host;
}

int PixelStreamTcp::getPort(void)
{
    return this->port;
}
