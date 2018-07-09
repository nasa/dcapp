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
#include <arpa/inet.h>
#ifdef JPEG_ENABLED
#include <jpeglib.h>
#endif
#include "basicutils/timer.hh"
#include "basicutils/msg.hh"
#include "PixelStreamMjpeg.hh"
#include "curlLib.hh"

#define CONNECTION_ATTEMPT_INTERVAL 2.0
#define CONNECTION_TIMEOUT 2.0
#define INVIEW_TIMEOUT 2.0

#define BOUNDARYTAG "--myboundary\r\nContent-Type: image/jpeg\r\nContent-Length: "
#define BOUNDARYLENGTH strlen(BOUNDARYTAG)

PixelStreamMjpeg::PixelStreamMjpeg()
:
host(0x0),
port(0),
path(0x0),
username(0x0),
password(0x0),
mjpegIO(0x0),
imagebytes(0),
readbuf(0x0),
readbufalloc(0),
pixelsalloc(0),
totalbytes(0),
masteroffset(0),
connectinprogress(false),
readinprogress(false),
inview(false),
updated(false)
{
    this->protocol = PixelStreamMjpegProtocol;
    this->lastconnectattempt = new Timer;
    this->lastread = new Timer;
    this->lastinview = new Timer;
}

PixelStreamMjpeg::~PixelStreamMjpeg()
{
    curlDisconnect();
    curlLibDestroyHandle(mjpegIO);
    if (this->host) free(this->host);
    if (this->path) free(this->path);
    if (this->username) free(this->username);
    if (this->password) free(this->password);
    if (this->pixels) free(this->pixels);
    delete this->lastconnectattempt;
    delete this->lastread;
    delete this->lastinview;
}

bool PixelStreamMjpeg::operator == (const PixelStreamMjpeg &that)
{
    bool usermatch = false, passmatch = false;

    if (this->port == that.port && !strcmp(this->host, that.host) && !strcmp(this->path, that.path))
    {
        if (!this->username && !that.username) usermatch = true;
        else if (this->username && that.username)
        {
            if (!strcmp(this->username, that.username)) usermatch = true;
        }
        if (!this->password && !that.password) passmatch = true;
        else if (this->password && that.password)
        {
            if (!strcmp(this->password, that.password)) passmatch = true;
        }
        if (usermatch && passmatch) return true;
    }

    return false;
}

bool PixelStreamMjpeg::operator != (const PixelStreamMjpeg &that)
{
    return !(*this == that);
}

int PixelStreamMjpeg::readerInitialize(const char *hostspec, int portspec, const char *pathspec, const char *usrspec, const char *pwdspec)
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

    this->host = strdup(inet_ntoa(*(in_addr * )server->h_addr));
    this->port = portspec;

    if (pathspec)
        this->path = strdup(pathspec);
    else
        this->path = strdup("video");

    if (usrspec) this->username = strdup(usrspec);
    if (pwdspec) this->password = strdup(pwdspec);

    mjpegIO = curlLibCreateHandle(this->host, this->port, this->path, this->username, this->password, this);
    if (!mjpegIO) return -1;

    return 0;
}

int PixelStreamMjpeg::reader(void)
{
#if defined(JPEG_ENABLED) && (JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED))
    inview = true;
    this->lastinview->restart();

    if ((connected || connectinprogress) && this->lastread->getSeconds() > CONNECTION_TIMEOUT)
    {
        debug_msg("Data connection timeout, disconnecting...");
        curlDisconnect();
        return 0;
    }

    if (!connected && !connectinprogress && this->lastconnectattempt->getSeconds() > CONNECTION_ATTEMPT_INTERVAL)
    {
        curlConnect();
    }

    if (updated)
    {
        updated = false;
        return 1;
    }
#endif
    return 0;
}

void PixelStreamMjpeg::curlConnect(void)
{
    this->lastconnectattempt->restart();
    
    if (!curlLibAddHandle(mjpegIO))
    {
        connectinprogress = true;
        this->lastread->restart();
    }
}

void PixelStreamMjpeg::curlDisconnect(void)
{
    curlLibRemoveHandle(mjpegIO);
    connected = false;
    connectinprogress = false;
    readinprogress = false;
    masteroffset = 0;
    totalbytes = 0;
    imagebytes = 0;
}

void PixelStreamMjpeg::processData(const char *memptr, size_t memsize)
{
    connectinprogress = false;
    connected = true;
    this->lastread->restart();

    if (readinprogress)
    {
        totalbytes += memsize;
        if (totalbytes > imagebytes) // make sure we don't write into unallocated space
            memcpy(&readbuf[masteroffset], memptr, memsize + imagebytes - totalbytes);
        else
            memcpy(&readbuf[masteroffset], memptr, memsize);
    }
    else
    {
        size_t ii;
        int boundarystart = -1;
        for (ii = 0; ii < strlen(memptr) && boundarystart < 0; ii++)
        {
            if (memptr[ii] == '-' && memptr[ii+1] == '-')
            {
                if (!strncmp(&memptr[ii], BOUNDARYTAG, BOUNDARYLENGTH)) boundarystart = ii + BOUNDARYLENGTH;
            }
        }
        if (boundarystart > 0)
        {
            if (sscanf(&memptr[boundarystart], "%ld", &imagebytes) != 1)
            {
                warning_msg("ERROR, malformed myboundary");
                return;
            }
            if (imagebytes > readbufalloc)
            {
                readbufalloc = imagebytes + 1000; // cushion by 1000 bytes to reduce the number of reallocs
                readbuf = (char *)realloc(readbuf, readbufalloc);
            }
            size_t sizelen = 0, offset;
            for (ii = boundarystart; ii < memsize && sizelen == 0; ii++)
            {
                if (isspace(memptr[ii])) sizelen = ii - boundarystart;
            }
            offset = boundarystart + sizelen + 4; // accommodate the \r\n\r\n at the end of the boundary tag
            memcpy(&readbuf[masteroffset], &memptr[offset], memsize - offset);
            totalbytes += memsize - offset;
            readinprogress = true;
        }
    }

    if (readinprogress && totalbytes >= imagebytes)
    {
        // only loadPixels if we haven't already loaded pixels this pass
        if (!updated)
        {
            loadPixels(readbuf, imagebytes);
            updated = true;
        }

        totalbytes = 0;
        imagebytes = 0;
        readinprogress = false;
    }

    masteroffset = totalbytes;
}

void PixelStreamMjpeg::loadPixels(const char *memptr, size_t memsize)
{
#if defined(JPEG_ENABLED) && (JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED))
    struct jpeg_decompress_struct jinfo;
    struct jpeg_error_mgr jerr;
    unsigned char *line;
    size_t num_pixels;

    jinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&jinfo);
    jpeg_mem_src(&jinfo, (unsigned char *)memptr, memsize);
    jpeg_read_header(&jinfo, TRUE);

    width = jinfo.image_width;
    height = jinfo.image_height;
    num_pixels = width * height * 3;
    if (num_pixels > pixelsalloc)
    {
        pixelsalloc = num_pixels;
        pixels = (void *)realloc(pixels, pixelsalloc);
    }

    jpeg_start_decompress(&jinfo);
    while (jinfo.output_scanline < height)
    {
        line = (unsigned char *)pixels + (3 * width * (height - jinfo.output_scanline - 1));
        jpeg_read_scanlines(&jinfo, &line, 1);
    }
    jpeg_finish_decompress(&jinfo);
    jpeg_destroy_decompress(&jinfo);
#endif
}

void PixelStreamMjpeg::updateStatus(void)
{
    if ((connected || connectinprogress) && this->lastinview->getSeconds() > INVIEW_TIMEOUT) curlDisconnect();
}

