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
#ifdef JPEG_ENABLED
#include <jpeglib.h>
#endif
#include "basicutils/timer.hh"
#include "basicutils/msg.hh"
#include "PixelStreamMjpeg.hh"

PixelStreamMjpeg::PixelStreamMjpeg()
:
host(0x0),
port(0),
CommSocket(-1),
readbuf(0x0),
rawdata(0x0),
readbufalloc(0),
rawalloc(0),
dataalloc(0),
socket_connected(0),
data_requested(0),
header_received(0)
{
    this->protocol = PixelStreamMjpegProtocol;
    this->lastconnectattempt = new Timer;
    this->lastread = new Timer;
}

PixelStreamMjpeg::~PixelStreamMjpeg()
{
    this->socket_disconnect();
    if (this->host) free(this->host);
    if (this->rawdata) free(this->rawdata);
    if (this->pixels) free(this->pixels);
    delete this->lastconnectattempt;
    delete this->lastread;
}

bool PixelStreamMjpeg::operator == (const PixelStreamMjpeg &that)
{
    if (!strcmp(this->host, that.host) && this->port == that.port) return true;
    else return false;
}

bool PixelStreamMjpeg::operator != (const PixelStreamMjpeg &that)
{
    return !(*this == that);
}

int PixelStreamMjpeg::socket_connect(void)
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

int PixelStreamMjpeg::readerInitialize(char *hostspec, int portspec)
{
#ifdef JPEG_ENABLED
#if JPEG_LIB_VERSION < 80 && !defined(MEM_SRCDST_SUPPORTED)
warning_msg("Version 80 or higher of libjpeg or libjpeg-turbo is required (current installed version is " << JPEG_LIB_VERSION);
#endif
#else
warning_msg("Could not find libjpeg or libjpeg-turbo");
#endif
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

int PixelStreamMjpeg::reader(void)
{
    int updated = 0;

    if (!socket_connected)
    {
        if (this->lastconnectattempt->getSeconds() > CONNECTION_ATTEMPT_INTERVAL)
        {
            if (CommSocket < 0) CommSocket = socket_connect();
            if (CommSocket >= 0)
            {
                socket_connected = 1;
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
            socket_disconnect();
            return 0;
        }
        if (!data_requested) data_requested = SendDataRequest("GET /video?nativeresolution=1 HTTP/1.0\n\n");
        if (data_requested && !header_received) header_received = RecvHeader();
        if (header_received) updated = RecvData();

        // Purge any extra data to prevent the socket from getting saturated
        if (updated)
        {
            int extra_bytes;
            ioctl(CommSocket, FIONREAD, &extra_bytes);
            if (extra_bytes)
            {
                char dummy;
                for (int i=0; i<extra_bytes; i++) read(CommSocket, &dummy, 1);
            }
        }
    }

    return updated;
}

void PixelStreamMjpeg::socket_disconnect(void)
{
    debug_msg("Disconnecting from server...");

    if (CommSocket >= 0)
    {
        shutdown(CommSocket, SHUT_RDWR);
        close(CommSocket);
        CommSocket = -1;
    }

    socket_connected = 0;
    data_requested = 0;
    header_received = 0;
}

int PixelStreamMjpeg::SendDataRequest(const char *command)
{
    if (write(CommSocket, command, strlen(command)) == (ssize_t)strlen(command)) return 1;
    else
    {
        debug_msg("Unable to write data: " << strerror(errno));
        socket_disconnect();
        return 0;
    }
}

int PixelStreamMjpeg::GetBuffer(unsigned sep)
{
    int i, n, buflen = 0;

    for (i=0; i<MAX_ATTEMPTS; i++)
    {
        if (buflen >= (int)readbufalloc)
        {
            readbufalloc += READBUF_ALLOC_CHUNK;
            readbuf = (char *)realloc(readbuf, readbufalloc);
        }
        n = read(CommSocket, &readbuf[buflen], 1);
        if (n == 1)
        {
            if (buflen > 0 && sep == crlf && readbuf[buflen-1] == '\r' && readbuf[buflen] == '\n')
            {
                readbuf[buflen-1] = '\0';
                return buflen-1;
            }
            else if (buflen > 2 && readbuf[buflen-3] == '\r' && readbuf[buflen-2] == '\n' && readbuf[buflen-1] == '\r' && readbuf[buflen] == '\n')
            {
                readbuf[buflen-3] = '\0';
                return buflen-3;
            }
            buflen++;
        }
    }

    return 0;
}

int PixelStreamMjpeg::RecvHeader(void)
{
    int i, j;
    char *boundarytag = 0x0;

    int buflen = GetBuffer(crlfcrlf);
    if (buflen >= 15 && !strncmp(readbuf, "HTTP/1.1 200 OK", 15))
    {
        for (i=0; !boundarytag && i<buflen-9; i++)
        {
            if (!strncmp(&readbuf[i], "boundary=", 9))
            {
                boundarytag=&readbuf[i+9];
                for (j=0; j<buflen-i-9; j++)
                {
                    if (isspace(boundarytag[j])) boundarytag[j] = '\0';
                }
            }
        }
        if (!boundarytag) return 0;
        for (j=0; j<MAX_ATTEMPTS; j++)
        {
            buflen = GetBuffer(crlf);
            if (buflen >= (int)strlen(boundarytag) && (!strncmp(readbuf, boundarytag, strlen(boundarytag)) || !strncmp(readbuf+2, boundarytag, strlen(boundarytag))))
                return 1;
        }
    }
    
    return 0;
}

int PixelStreamMjpeg::RecvData(void)
{
#if defined(JPEG_ENABLED) && (JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED))
    int i;
    struct jpeg_decompress_struct jinfo;
    struct jpeg_error_mgr jerr;
    unsigned char *line;
    size_t datasize, rawsize = 0;

    int buflen = GetBuffer(crlfcrlf);
    for (i=0; !rawsize && i<buflen-15; i++)
    {
        if (!strncmp(&readbuf[i], "Content-Length:", 15))
        {
            if (sscanf(&readbuf[i+15], "%ld", &rawsize) != 1) return 0;
        }
    }
    if (!rawsize) return 0;

    if (rawsize > rawalloc)
    {
        rawalloc = rawsize;
        rawdata = (void *)realloc(rawdata, rawalloc);
    }

    if (read(CommSocket, rawdata, rawsize) != (ssize_t)rawsize) return 0;

    jinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&jinfo);
    jpeg_mem_src(&jinfo, (unsigned char *)rawdata, rawsize);
    jpeg_read_header(&jinfo, 1);

    width = jinfo.image_width;
    height = jinfo.image_height;
    datasize = width * height * 3;
    if (datasize > dataalloc)
    {
        dataalloc = datasize;
        pixels = (void *)realloc(pixels, dataalloc);
    }

    jpeg_start_decompress(&jinfo);
    while (jinfo.output_scanline < height)
    {
        line = (unsigned char *)pixels + (3 * width * (height - jinfo.output_scanline - 1));
        jpeg_read_scanlines(&jinfo, &line, 1);
    }
    jpeg_finish_decompress(&jinfo);
    jpeg_destroy_decompress(&jinfo);

    this->lastread->restart();

    return 1;
#else
    rawalloc = 0;
    dataalloc = 0;
    this->lastread->restart();
    return 0;
#endif
}

char *PixelStreamMjpeg::getHost(void)
{
    return this->host;
}

int PixelStreamMjpeg::getPort(void)
{
    return this->port;
}
