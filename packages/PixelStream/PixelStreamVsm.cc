#include <sstream>
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
#include "PixelStreamVsm.hh"

PixelStreamVsm::PixelStreamVsm()
:
curlhost(0x0),
curlport(0),
curlcamera(0x0),
prevcamera(0x0),
data_request(0x0),
CommSocket(-1),
cameraassigned(false),
readbuf(0x0),
readbufalloc(0),
dataalloc(0),
data_requested(false),
header_received(false),
totalbytes(0),
masteroffset(0)
{
    this->protocol = PixelStreamVsmProtocol;
    this->assigncameraattempt = new Timer;
    this->lastconnectattempt = new Timer;
    this->lastread = new Timer;
    this->curl = curl_easy_init();
}

PixelStreamVsm::~PixelStreamVsm()
{
    this->socket_disconnect();
    if (this->curlhost) free(this->curlhost);
    if (this->data_request) free(this->data_request);
    if (this->pixels) free(this->pixels);
    delete this->assigncameraattempt;
    delete this->lastconnectattempt;
    delete this->lastread;
    curl_easy_cleanup(this->curl);
}

bool PixelStreamVsm::operator == (const PixelStreamVsm &that)
{
    if (!strcmp(this->curlhost, that.curlhost) && this->curlport == that.curlport && this->curlcamera == that.curlcamera) return true;
    else return false;
}

bool PixelStreamVsm::operator != (const PixelStreamVsm &that)
{
    return !(*this == that);
}

// CURL delivers all recieved data to a callback function, which writes to the console by default.
// This overrides that default function and simply ignores the data.
static size_t write_curl_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    return (size * nmemb);
}

int PixelStreamVsm::readerInitialize(const char *hostspec, int portspec, char *cameraid)
{
#ifdef JPEG_ENABLED
#if JPEG_LIB_VERSION < 80 && !defined(MEM_SRCDST_SUPPORTED)
warning_msg("Version 80 or higher of libjpeg or libjpeg-turbo is required (current installed version is " << JPEG_LIB_VERSION);
#endif
#else
warning_msg("Could not find libjpeg or libjpeg-turbo");
#endif
    CURLcode result;

    if (!curl)
    {
        error_msg("Failed to initialize CURL");
        return -1;
    }

    result = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_curl_data);
    if (result != CURLE_OK)
    {
        error_msg("CURL error: " << curl_easy_strerror(result));
        return -1;
    }

#ifndef DEBUG
    // Turn off all diagnostic printouts
    result = curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    if (result != CURLE_OK) {
        error_msg("CURL error: " << curl_easy_strerror(result));
        return -1;
    }
#endif

    if (hostspec)
        this->curlhost = strdup(hostspec);
    else
        this->curlhost = strdup("localhost");
    this->curlport = portspec;
    this->curlcamera = cameraid;
    this->prevcamera = strdup("");

    return 0;
}

int PixelStreamVsm::reader(void)
{
#if defined(JPEG_ENABLED) && (JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED))
    bool updated = false, first_assign_attempt = false, first_connect_attempt = false;
    int updatepixels = 0, bytes_to_read, newbytes;

    // user has requested a new camera
    if (cameraassigned && strcmp(curlcamera, prevcamera))
    {
        socket_disconnect();
        first_assign_attempt = true;
    }

    if (!cameraassigned)
    {
        if ((first_assign_attempt && (this->assigncameraattempt->getSeconds() > INITIAL_CAMERA_ASSIGN_INTERVAL)) ||
            (!first_assign_attempt && (this->assigncameraattempt->getSeconds() > CAMERA_ASSIGN_INTERVAL)))
        {
            assignNewCamera();
            this->assigncameraattempt->restart();
            if (cameraassigned) first_connect_attempt = true;
        }
    }

    if (!connected)
    {
        if (cameraassigned && (first_connect_attempt || (this->lastconnectattempt->getSeconds() > CONNECTION_ATTEMPT_INTERVAL)))
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
#else
    readbufalloc = 0;
    return 0;
#endif
}

int PixelStreamVsm::assignNewCamera()
{
    CURLcode result;

    // Build the URL
    std::ostringstream url;
    url << this->curlhost << ":" << this->curlport << "/streams/" << this->curlcamera;

    // Tell CURL what URL to GET
    result = curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    if (result != CURLE_OK)
    {
        error_msg("CURL error: " << curl_easy_strerror(result));
        return -1;
    }

    // Perform the HTTP request
    result = curl_easy_perform(curl);
    if (result != CURLE_OK)
    {
        error_msg("CURL error: " << curl_easy_strerror(result));
        return -1;
    }

    // Get the HTTP response code
    long response_code;
    result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (result != CURLE_OK)
    {
        error_msg("CURL error: " << curl_easy_strerror(result));
        return -1;
    }

    switch (response_code)
    {
        case 307:
            // 307 is a redirect, get the location
            const char *location;
            result = curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &location);
            if (result != CURLE_OK)
            {
                error_msg("CURL error: " << curl_easy_strerror(result));
                return -1;
            }
            user_msg("Redirected to: " << location);
            resolveURL(location);
            if (prevcamera) free(prevcamera);
            prevcamera = strdup(curlcamera);
            cameraassigned = true;
            break;

        case 404:
            // This camera does not exist in any EDGE instance.
            user_msg("No such camera: " << curlcamera);
            break;

        case 503:
            // This camera exists, but all EDGE instances capable of rendering it are
            // busy rendering other cameras for other clients. (We're out of resources.)
            user_msg("Camera unavailable: " << curlcamera);
            break;

        default:
            error_msg("Unhandled reponse code. VSM may have changed!");
            break;
    }

    return 0;
}

int PixelStreamVsm::resolveURL(const char *instr)
{
    size_t ii, jj, kk, mylen = strlen(instr);
    bool found_addr = false, found_port = false, found_path = false;
    size_t addr_start, addr_end, port_start, port_end, path_start, path_end;

    for (ii = 0; !found_addr && ii < mylen-4; ii++)
    {
        if (instr[ii] == ':' && instr[ii+1] == '/' && instr[ii+2] == '/')
        {
            addr_start = ii+3;
            found_addr = true;
            for (jj = addr_start; !found_port && jj < mylen-2; jj++)
            {
                if (instr[jj] == ':')
                {
                    addr_end = jj-1;
                    port_start = jj+1;
                    found_port = true;
                    for (kk = port_start; !found_path && kk < mylen-2; kk++)
                    {
                        if (instr[kk] == '/')
                        {
                            port_end = kk-1;
                            path_start = kk+1;
                            path_end = mylen-1;
                            found_path = true;
                        }
                    }
                }
            }
        }
    }

    if (found_addr && found_port && found_path)
    {
        char *host = (char *)malloc(mylen);
        char *port = (char *)malloc(mylen);
        char *path = (char *)malloc(mylen);
        int portnum;

        for (ii=addr_start, jj=0; ii<=addr_end; ii++, jj++) host[jj] = instr[ii];
        host[addr_end + 1 - addr_start] = '\0';

        for (ii=port_start, jj=0; ii<=port_end; ii++, jj++) port[jj] = instr[ii];
        port[port_end + 1 - port_start] = '\0';

        for (ii=path_start, jj=0; ii<=path_end; ii++, jj++) path[jj] = instr[ii];
        path[path_end + 1 - path_start] = '\0';

        portnum = strtol(port, 0x0, 10);

        struct hostent *server = gethostbyname(host);

        if (!server)
        {
            error_msg("Unable to resolve host name: " << host);
            return -1;
        }

        bzero((char *) &server_address, sizeof(server_address));
        server_address.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
        server_address.sin_port = htons(portnum);

        if (this->data_request) free(this->data_request);
        asprintf(&(this->data_request), "GET /%s HTTP/1.0\r\n\r\n", path);

        free(path);
        free(port);
        free(host);
    }

    return 0;
}

int PixelStreamVsm::socket_connect(void)
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

void PixelStreamVsm::socket_disconnect(void)
{
    debug_msg("Disconnecting from server...");

    if (CommSocket >= 0)
    {
        shutdown(CommSocket, SHUT_RDWR);
        close(CommSocket);
        CommSocket = -1;
    }

    cameraassigned = false;
    connected = false;
    data_requested = false;
    header_received = false;
    totalbytes = 0;
    masteroffset = 0;
}

bool PixelStreamVsm::SendDataRequest(const char *command)
{
    if (write(CommSocket, command, strlen(command)) == (ssize_t)strlen(command)) return true;
    else
    {
        debug_msg("Unable to write data: " << strerror(errno));
        socket_disconnect();
        return false;
    }
}

int PixelStreamVsm::findCrlf(char *bufptr, unsigned buflen)
{
    for (unsigned i=1; i<buflen; i++)
    {
        if (bufptr[i-1] == '\r' && bufptr[i] == '\n') return i;
    }
    return 0;
}

int PixelStreamVsm::findCrlfCrlf(char *bufptr, unsigned buflen)
{
    for (unsigned i=3; i<buflen; i++)
    {
        if (bufptr[i-3] == '\r' && bufptr[i-2] == '\n' && bufptr[i-1] == '\r' && bufptr[i] == '\n') return i;
    }
    return 0;
}

bool PixelStreamVsm::RecvHeader(void)
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

bool PixelStreamVsm::RecvData(void)
{
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
}

void PixelStreamVsm::loadPixels(const char *memptr, size_t memsize)
{
#if defined(JPEG_ENABLED) && (JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED))
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
#else
    dataalloc = 0;
#endif
}
