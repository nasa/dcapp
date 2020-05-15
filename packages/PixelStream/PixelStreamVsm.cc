#if defined(CURL_ENABLED) && defined(JPEG_ENABLED)

#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <string>
#include <sstream>
#include <netdb.h>
#include <arpa/inet.h>
#include <jpeglib.h>
#include "basicutils/timer.hh"
#include "basicutils/msg.hh"
#include "PixelStreamVsm.hh"
#include "curlLib.hh"

#define INITIAL_CAMERA_ASSIGN_INTERVAL 0.1
#define CAMERA_ASSIGN_INTERVAL 1.0
#define CONNECTION_ATTEMPT_INTERVAL 2.0
#define CONNECTION_TIMEOUT 2.0

PixelStreamVsm::PixelStreamVsm()
:
vsmhost(0x0),
vsmport(0),
curr_camera(0x0),
userpath(0x0),
cameraassigned(false)
{
    this->protocol = PixelStreamVsmProtocol;
    this->assigncameraattempt = new Timer;
    this->curl = curl_easy_init();
}

PixelStreamVsm::~PixelStreamVsm()
{
    if (this->vsmhost) free(this->vsmhost);
    if (this->userpath) free(this->userpath);
    delete this->assigncameraattempt;
    curl_easy_cleanup(this->curl);
}

bool PixelStreamVsm::operator == (const PixelStreamVsm &that)
{
    if (!strcmp(this->vsmhost, that.vsmhost) && this->vsmport == that.vsmport && this->curr_camera == that.curr_camera) return true;
    else return false;
}

bool PixelStreamVsm::operator != (const PixelStreamVsm &that)
{
    return !(*this == that);
}

// CURL delivers all recieved data to a callback function, which writes to the console by default.
// This overrides that default function and simply ignores the data.
static size_t write_curl_data(void * /* buffer */, size_t size, size_t nmemb, void * /* userp */)
{
    return (size * nmemb);
}

int PixelStreamVsm::readerInitialize(const char *hostspec, int portspec, const char *pathspec, Value *cameraid)
{
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

    if (!(Message::debuggingEnabled()))
    {
        // Turn off all diagnostic printouts
        result = curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        if (result != CURLE_OK) {
            error_msg("CURL error: " << curl_easy_strerror(result));
            return -1;
        }
    }

    if (hostspec)
        this->vsmhost = strdup(hostspec);
    else
        this->vsmhost = strdup("localhost");
    this->vsmport = portspec;
    this->curr_camera = cameraid;
    this->prev_camera = "";
    if (pathspec) this->userpath = strdup(pathspec);

    return 0;
}

int PixelStreamVsm::reader(void)
{
    bool first_connect_attempt = false;

    inview = true;
    this->lastinview->restart();

    // user has requested a new camera
    if (curr_camera->getString() != prev_camera)
    {
        if (cameraassigned)
        {
            curlDisconnect();
            curlLibDestroyHandle(mjpegIO);
            cameraassigned = false;
        }
        this->assigncameraattempt->restart();
        first_assign_attempt = true;
        prev_camera = curr_camera->getString();
    }

    if (!cameraassigned)
    {
        if ((first_assign_attempt && (this->assigncameraattempt->getSeconds() > INITIAL_CAMERA_ASSIGN_INTERVAL)) ||
            (!first_assign_attempt && (this->assigncameraattempt->getSeconds() > CAMERA_ASSIGN_INTERVAL)))
        {
            assignNewCamera();
            this->assigncameraattempt->restart();
            if (cameraassigned) first_connect_attempt = true;
            first_assign_attempt = false;
        }
    }

    if ((connected || connectinprogress) && this->lastread->getSeconds() > CONNECTION_TIMEOUT)
    {
        debug_msg("Data connection timeout, disconnecting...");
        curlDisconnect();
        return 0;
    }

    if (!connected && !connectinprogress && cameraassigned && (first_connect_attempt || this->lastconnectattempt->getSeconds() > CONNECTION_ATTEMPT_INTERVAL))
    {
        curlConnect();
        this->lastconnectattempt->restart();
    }

    if (updated)
    {
        updated = false;
        return 1;
    }

    return 0;
}

int PixelStreamVsm::assignNewCamera(void)
{
    CURLcode result;

    if (this->curr_camera->getString().empty())
    {
        debug_msg("No camera specified");
        return 0;
    }

    // Build the URL
    std::ostringstream url;
    url << this->vsmhost << ":" << this->vsmport << "/streams/" << this->curr_camera->getString();

    // Tell CURL what URL to GET
    result = curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    if (result != CURLE_OK)
    {
        error_msg("CURL error: " << curl_easy_strerror(result));
        return -1;
    }

    // Perform the HTTP request - debug_msg here or else this will scroll if VSM server isn't active
    result = curl_easy_perform(curl);
    if (result != CURLE_OK)
    {
        debug_msg("CURL error: " << curl_easy_strerror(result));
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
            debug_msg("Redirected to: " << location);
            resolveURL(location);
            cameraassigned = true;
            break;

        case 404:
            // This camera does not exist in any EDGE instance.
            debug_msg("No such camera: " << this->curr_camera->getString());
            break;

        case 503:
            // This camera exists, but all EDGE instances capable of rendering it are
            // busy rendering other cameras for other clients. (We're out of resources.)
            debug_msg("Camera unavailable: " << this->curr_camera->getString());
            break;

        default:
            warning_msg("Unhandled reponse code. VSM may have changed!");
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
        char *tmp_host = (char *)malloc(mylen);
        char *tmp_port = (char *)malloc(mylen);
        char *tmp_path = (char *)malloc(mylen);
        char *mypath;

        for (ii=addr_start, jj=0; ii<=addr_end; ii++, jj++) tmp_host[jj] = instr[ii];
        tmp_host[addr_end + 1 - addr_start] = '\0';

        for (ii=port_start, jj=0; ii<=port_end; ii++, jj++) tmp_port[jj] = instr[ii];
        tmp_port[port_end + 1 - port_start] = '\0';

        if (userpath) mypath = userpath;
        else
        {
            for (ii=path_start, jj=0; ii<=path_end; ii++, jj++) tmp_path[jj] = instr[ii];
            tmp_path[path_end + 1 - path_start] = '\0';
            mypath = tmp_path;
        }

        int portnum = strtol(tmp_port, 0x0, 10);

        struct hostent *server = gethostbyname(tmp_host);

        if (!server)
        {
            error_msg("Unable to resolve host name: " << tmp_host);
            return -1;
        }

        mjpegIO = curlLibCreateHandle(inet_ntoa(*(in_addr * )server->h_addr), portnum, mypath, 0x0, 0x0, this);
        if (!mjpegIO) return -1;

        free(tmp_path);
        free(tmp_port);
        free(tmp_host);
    }

    return 0;
}

#else

#include <cstring>
#include "basicutils/msg.hh"
#include "PixelStreamVsm.hh"

PixelStreamVsm::PixelStreamVsm()
:
vsmhost(0x0),
vsmport(0),
curr_camera(0x0)
{
    this->protocol = PixelStreamVsmProtocol;
}

PixelStreamVsm::~PixelStreamVsm()
{
    if (this->vsmhost) free(this->vsmhost);
}

bool PixelStreamVsm::operator == (const PixelStreamVsm &that)
{
    if (!strcmp(this->vsmhost, that.vsmhost) && this->vsmport == that.vsmport && this->curr_camera == that.curr_camera) return true;
    else return false;
}

bool PixelStreamVsm::operator != (const PixelStreamVsm &that)
{
    return !(*this == that);
}

int PixelStreamVsm::readerInitialize(const char *hostspec, int portspec, const char *pathspec, Value *cameraid)
{
#ifndef CURL_ENABLED
    warning_msg("VSM requested, but unable to locate libcurl");
#endif
#ifndef JPEG_ENABLED
    warning_msg("VSM requested, but unable to locate libjpeg or libjpeg-turbo");
#endif

    if (hostspec)
        this->vsmhost = strdup(hostspec);
    else
        this->vsmhost = strdup("localhost");
    this->vsmport = portspec;
    this->curr_camera = cameraid;

    return 0;
}

int PixelStreamVsm::reader(void)
{
    return 0;
}

#endif
