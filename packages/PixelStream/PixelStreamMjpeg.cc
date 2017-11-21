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
path(0x0),
data_request(0x0),
CommSocket(-1),
readbuf(0x0),
readbufalloc(0),
dataalloc(0),
data_requested(false),
header_received(false),
totalbytes(0),
masteroffset(0)
{
    this->protocol = PixelStreamMjpegProtocol;
    this->lastconnectattempt = new Timer;
    this->lastread = new Timer;
}

PixelStreamMjpeg::~PixelStreamMjpeg()
{
    this->socket_disconnect();
    if (this->host) free(this->host);
    if (this->path) free(this->path);
    if (this->data_request) free(this->data_request);
    if (this->pixels) free(this->pixels);
    delete this->lastconnectattempt;
    delete this->lastread;
}

bool PixelStreamMjpeg::operator == (const PixelStreamMjpeg &that)
{
    if (!strcmp(this->host, that.host) && this->port == that.port && !strcmp(this->path, that.path)) return true;
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

int PixelStreamMjpeg::readerInitialize(const char *hostspec, int portspec, const char *pathspec)
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

    if (pathspec)
        this->path = strdup(pathspec);
    else
        this->path = strdup("video");
    asprintf(&(this->data_request), "GET /%s HTTP/1.0\n\n", this->path);

    return 0;
}

int PixelStreamMjpeg::reader(void)
{
    bool updated = false;
    int updatepixels = 0, bytes_to_read, newbytes;

    if (!connected)
    {
        if (this->lastconnectattempt->getSeconds() > CONNECTION_ATTEMPT_INTERVAL)
        {
            if (CommSocket < 0) CommSocket = socket_connect();
            if (CommSocket >= 0)
            {
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
            socket_disconnect();
            return 0;
        }
        if (!data_requested) data_requested = SendDataRequest(this->data_request);
        if (data_requested)
        {
            do
            {
                ioctl(CommSocket, FIONREAD, &bytes_to_read);
                if (bytes_to_read > 0)
                {
                    if ((totalbytes + bytes_to_read) >= (int)readbufalloc)
                    {
                        readbufalloc = totalbytes + bytes_to_read;
                        readbuf = (char *)realloc(readbuf, readbufalloc);
                    }
                    newbytes = read(CommSocket, readbuf + totalbytes, bytes_to_read);
                    if (newbytes > 0) totalbytes += newbytes;
                }
            }
            while (bytes_to_read > 0);

            flushonly = false;
            if (totalbytes > 0) do
            {
                if (!header_received) header_received = RecvHeader();
                if (header_received) updated = RecvData();
                if (updated) updatepixels = 1;
                totalbytes -= masteroffset;
                for (int i=0; i<totalbytes; i++) readbuf[i] = readbuf[masteroffset+i];
                masteroffset = 0;
            }
            while ((totalbytes > 0) && updated);
        }
    }

    return updatepixels;
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

    connected = false;
    data_requested = false;
    header_received = false;
    totalbytes = 0;
    masteroffset = 0;
}

bool PixelStreamMjpeg::SendDataRequest(const char *command)
{
    if (write(CommSocket, command, strlen(command)) == (ssize_t)strlen(command)) return true;
    else
    {
        debug_msg("Unable to write data: " << strerror(errno));
        socket_disconnect();
        return false;
    }
}

int PixelStreamMjpeg::findCrlf(char *bufptr, unsigned buflen)
{
    for (unsigned i=1; i<buflen; i++)
    {
        if (bufptr[i-1] == '\r' && bufptr[i] == '\n') return i;
    }
    return 0;
}

int PixelStreamMjpeg::findCrlfCrlf(char *bufptr, unsigned buflen)
{
    for (unsigned i=3; i<buflen; i++)
    {
        if (bufptr[i-3] == '\r' && bufptr[i-2] == '\n' && bufptr[i-1] == '\r' && bufptr[i] == '\n') return i;
    }
    return 0;
}

bool PixelStreamMjpeg::RecvHeader(void)
{
    int i, j;
    char *boundarytag = 0x0;

    int buflen = findCrlfCrlf(readbuf+masteroffset, totalbytes-masteroffset) - 3;

    // this verifies that the header is "HTTP/1.x 200 OK", where x can be any digit
    if (buflen >= 15 && !strncmp(readbuf+masteroffset, "HTTP/1.", 7) && !strncmp(readbuf+masteroffset+8, " 200 OK", 7))
    {
        for (i=0; !boundarytag && i<buflen-9; i++)
        {
            if (!strncmp(&readbuf[masteroffset+i], "boundary=", 9))
            {
                readbuf[buflen+masteroffset] = '\0';
                boundarytag=&readbuf[masteroffset+i+9];
                for (j=0; j<buflen-i-9; j++)
                {
                    if (isspace(boundarytag[j])) boundarytag[j] = '\0';
                }
            }
        }
        if (!boundarytag) return false;

        masteroffset += buflen+4;
        buflen = findCrlf(readbuf+masteroffset, totalbytes-masteroffset);
        if (buflen >= (int)strlen(boundarytag) && (!strncmp(readbuf+masteroffset, boundarytag, strlen(boundarytag)) || !strncmp(readbuf+masteroffset+2, boundarytag, strlen(boundarytag))))
        {
            masteroffset += buflen+2;
            return true;
        }
    }

    return false;
}

bool PixelStreamMjpeg::RecvData(void)
{
#if defined(JPEG_ENABLED) && (JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED))
    int i, tmpoffset;
    size_t rawsize = 0;

    int buflen = findCrlfCrlf(readbuf+masteroffset, totalbytes-masteroffset);
    for (i=0; !rawsize && i<buflen-15; i++)
    {
        if (!strncmp(&readbuf[masteroffset+i], "Content-Length:", 15))
        {
            if (sscanf(&readbuf[masteroffset+i+15], "%ld", &rawsize) != 1) return false;
        }
    }
    if (!rawsize) return false;

    tmpoffset = masteroffset + buflen + 1;
    if ((size_t)(totalbytes - tmpoffset) >= rawsize)
    {
        if (!flushonly)
        {
            loadPixels(readbuf + tmpoffset, rawsize);
            flushonly = true;
        }
        masteroffset = tmpoffset + rawsize;
        this->lastread->restart();
        return true;
    }

    return false;
#else
    dataalloc = 0;
    this->lastread->restart();
    return false;
#endif
}

void PixelStreamMjpeg::loadPixels(const char *memptr, size_t memsize)
{
    struct jpeg_decompress_struct jinfo;
    struct jpeg_error_mgr jerr;
    unsigned char *line;
    size_t datasize;

    jinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&jinfo);
    jpeg_mem_src(&jinfo, (unsigned char *)memptr, memsize);
    jpeg_read_header(&jinfo, TRUE);

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
}
