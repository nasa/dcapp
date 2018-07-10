#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cerrno>
#include <stdint.h>
#include <strings.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include "basicutils/timer.hh"
#include "basicutils/msg.hh"
#include "PixelStreamTcp.hh"

#define CONNECTION_ATTEMPT_INTERVAL 2.0
#define CONNECTION_TIMEOUT 2.0
#define CONNECTION_TIMEOUT_USEC 10000
#define REQUEST_BUFFER 1

PixelStreamTcp::PixelStreamTcp()
:
host(0x0),
port(0),
ListenSocket(-1),
ClientToServerSocket(-1),
ServerToClientSocket(-1),
datasize(0),
dataalloc(0),
data_requested(false),
header_sent(false),
header_received(false)
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
    delete this->lastconnectattempt;
    delete this->lastread;
}

bool PixelStreamTcp::operator == (const PixelStreamTcp &that)
{
    if (!strcmp(this->host, that.host) && this->port == that.port) return true;
    else return false;
}

bool PixelStreamTcp::operator != (const PixelStreamTcp &that)
{
    return !(*this == that);
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

void PixelStreamTcp::connect_write_sockets(void)
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

        connected = true;
        this->lastread->restart();
    }
}

int PixelStreamTcp::readerInitialize(const char *hostspec, int portspec)
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

    if (!connected)
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

                connected = true;
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
            data_requested = false;
            header_received = false;
        }
    }

    return updated;
}

bool PixelStreamTcp::writeRequested(void)
{
    if (!connected) connect_write_sockets();

    return connected;
}

int PixelStreamTcp::writer(void)
{
    if (!this->pixels) return 0;

    if (!connected) connect_write_sockets();
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
            data_requested = false;
            header_sent = false;
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

    connected = false;
    data_requested = false;
    header_sent = false;
    header_received = false;
}

bool PixelStreamTcp::SendDataRequest(void)
{
    uint8_t command = REQUEST_BUFFER;

    if (write(ClientToServerSocket, &command, sizeof(command)) == sizeof(command)) return true;
    else
    {
        debug_msg("Unable to write data: " << strerror(errno));
        disconnect_all();
        return false;
    }
}

bool PixelStreamTcp::RecvHeader(uint32_t *width, uint32_t *height)
{
    char recvbuf[10];

    if (read(ServerToClientSocket, &recvbuf, sizeof(recvbuf)) == sizeof(recvbuf))
    {
        uint16_t *recvendianflag = (uint16_t *)(&(recvbuf[0]));
        char *wbytes = (char *)width;
        char *hbytes = (char *)height;

        if (*recvendianflag == 1)
        {
            wbytes[0] = recvbuf[2]; wbytes[1] = recvbuf[3]; wbytes[2] = recvbuf[4]; wbytes[3] = recvbuf[5];
            hbytes[0] = recvbuf[6]; hbytes[1] = recvbuf[7]; hbytes[2] = recvbuf[8]; hbytes[3] = recvbuf[9];
        }
        else
        {
            wbytes[3] = recvbuf[2]; wbytes[2] = recvbuf[3]; wbytes[1] = recvbuf[4]; wbytes[0] = recvbuf[5];
            hbytes[3] = recvbuf[6]; hbytes[2] = recvbuf[7]; hbytes[1] = recvbuf[8]; hbytes[0] = recvbuf[9];
        }
        this->lastread->restart();
        return true;
    }
    else return false;
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

bool PixelStreamTcp::RecvDataRequest(void)
{
    uint8_t command;

    if (read(ClientToServerSocket, &command, sizeof(command)) == sizeof(command) && command == REQUEST_BUFFER)
    {
        this->lastread->restart();
        return true;
    }
    else return false;
}

bool PixelStreamTcp::SendHeader(uint32_t width, uint32_t height)
{
    char sendbuf[10];

    uint16_t *sendendianflag = (uint16_t *)(&(sendbuf[0]));
    *sendendianflag = 1;
    bcopy((void *)&width, (void *)(&(sendbuf[2])), sizeof(width));
    bcopy((void *)&height, (void *)(&(sendbuf[6])), sizeof(height));

    if (write(ServerToClientSocket, &sendbuf, sizeof(sendbuf)) == sizeof(sendbuf)) return true;
    else
    {
        error_msg("Unable to write data: " << strerror(errno));
        disconnect_all();
        return false;
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
