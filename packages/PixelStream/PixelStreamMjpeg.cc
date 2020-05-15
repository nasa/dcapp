#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <netdb.h>
#include <arpa/inet.h>
#ifdef JPEG_ENABLED
#include <setjmp.h>
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
mjpegIO(0x0),
connectinprogress(false),
inview(false),
updated(false),
host(0x0),
port(0),
path(0x0),
username(0x0),
password(0x0),
readbuf(0x0),
readbufalloc(0),
pixelsalloc(0),
imagebytes(0),
totalbytes(0),
masteroffset(0),
readinprogress(false)
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
#ifndef JPEG_ENABLED
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

    size_t newtotal = totalbytes + memsize;

    if (newtotal > readbufalloc)
    {
        readbufalloc = newtotal + 1000; // cushion by 1000 bytes to reduce the number of reallocs
        readbuf = (char *)realloc(readbuf, readbufalloc);
    }

    memcpy(&readbuf[totalbytes], memptr, memsize);
    totalbytes = newtotal;

    while (masteroffset < totalbytes)
    {
        if (!readinprogress)
        {
            if (totalbytes - masteroffset > 200) // don't bother starting new image unless there are a healthy number of bytes available
            {
                size_t ii;
                int boundarystart = -1;
                for (ii = 0; ii < totalbytes && boundarystart < 0; ii++)
                {
                    if (readbuf[ii] == '-' && readbuf[ii + 1] == '-')
                    {
                        if (!strncmp(&readbuf[ii], BOUNDARYTAG, BOUNDARYLENGTH)) boundarystart = ii + BOUNDARYLENGTH;
                    }
                }
                if (boundarystart > 0)
                {
                    if (sscanf(&readbuf[boundarystart], "%ld", &imagebytes) != 1)
                    {
                        warning_msg("ERROR, malformed myboundary");
                        return;
                    }
                    size_t sizelen = 0;
                    for (ii = boundarystart; ii < totalbytes && sizelen == 0; ii++)
                    {
                        if (isspace(readbuf[ii])) sizelen = ii - boundarystart;
                    }

                    masteroffset = boundarystart + sizelen + 4; // accommodate the \r\n\r\n at the end of the boundary tag
                    readinprogress = true;
                }
            }
        }

        size_t imageend = masteroffset + imagebytes;

        if (readinprogress && (totalbytes >= imageend))
        {
            // only loadPixels if we haven't already loaded pixels this pass
            if (!updated)
            {
                loadPixels(&readbuf[masteroffset], imagebytes);
                updated = true;
            }

            // shift unread bytes to the start of readbuf
            char *store_buf = &(readbuf[imageend]);
            for (size_t i=0; i < totalbytes - imageend; i++) readbuf[i] = store_buf[i];

            totalbytes -= imageend;
            masteroffset = 0;
            readinprogress = false;
        }
        else break;
    }
}

#if defined(JPEG_ENABLED) && JPEG_LIB_VERSION < 80 && !defined(MEM_SRCDST_SUPPORTED)
// jpeg_mem_src isn't provided by older versions of libjpeg, so these routines provide a workaround
static void init_source(j_decompress_ptr) {}

static boolean fill_input_buffer(j_decompress_ptr)
{
    error_msg("Input buffer empty");
    return true;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr *src = (struct jpeg_source_mgr *)(cinfo->src);

    if (num_bytes > 0)
    {
        src->next_input_byte += (size_t)num_bytes;
        src->bytes_in_buffer -= (size_t)num_bytes;
    }
}

static void term_source(j_decompress_ptr) {}

static void jpeg_mem_src(j_decompress_ptr cinfo, void *buffer, long nbytes)
{
    struct jpeg_source_mgr *src;

    if (cinfo->src == 0x0) /* first time for this JPEG object? */
    {
        cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr));
    }

    src = (struct jpeg_source_mgr *)(cinfo->src);
    src->init_source = init_source;
    src->fill_input_buffer = fill_input_buffer;
    src->skip_input_data = skip_input_data;
    src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->term_source = term_source;
    src->bytes_in_buffer = nbytes;
    src->next_input_byte = (JOCTET *)buffer;
}
#endif

#ifdef JPEG_ENABLED
struct jpegErrorManager
{
    struct jpeg_error_mgr error_mgr;
    jmp_buf setjmp_buffer;
};

static char jpegLastErrorMsg[JMSG_LENGTH_MAX];

static void jpegErrorExit(j_common_ptr cinfo)
{
    // form an error message and jump to the setjmp point
    (*(cinfo->err->format_message))(cinfo, jpegLastErrorMsg);
    jpegErrorManager *myerr = (jpegErrorManager *)(cinfo->err);
    longjmp(myerr->setjmp_buffer, 1);
}

void PixelStreamMjpeg::loadPixels(const char *memptr, size_t memsize)
{
    struct jpeg_decompress_struct jinfo;
    jpegErrorManager jerr;
    unsigned char *line;
    size_t num_pixels;

    jinfo.err = jpeg_std_error(&jerr.error_mgr);
    jerr.error_mgr.error_exit = jpegErrorExit;

    if (setjmp(jerr.setjmp_buffer))
    {
        // hande the JPEG error, clean up, and return to the calling routine
        warning_msg("MJPEG PixelStream: " << jpegLastErrorMsg);
        jpeg_destroy_decompress(&jinfo);
        return;
    }

    jpeg_create_decompress(&jinfo);
    jpeg_mem_src(&jinfo, (unsigned char *)memptr, memsize);
    jpeg_read_header(&jinfo, TRUE);

    width = jinfo.image_width;
    height = jinfo.image_height;
    num_pixels = width * height * 3;
    if (num_pixels > pixelsalloc)
    {
        pixelsalloc = num_pixels + 1000; // cushion by 1000 bytes to reduce the number of reallocs
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
}
#else
void PixelStreamMjpeg::loadPixels(const char * /* memptr */, size_t /* memsize */)
{
    pixelsalloc = 0;
}
#endif

void PixelStreamMjpeg::updateStatus(void)
{
    if ((connected || connectinprogress) && this->lastinview->getSeconds() > INVIEW_TIMEOUT) curlDisconnect();
}
