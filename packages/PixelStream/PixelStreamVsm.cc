#include <string>
#include <cstdlib>
#include <cstring>
#include "basicutils/msg.hh"
#include "values.hh"
#include "PixelStreamVsm.hh"

#if defined(CURL_ENABLED) && defined(JPEG_ENABLED)

#include <cstddef>
#include <sstream>
#include <netdb.h>
#include <arpa/inet.h>
#include <jpeglib.h>
#include "basicutils/timer.hh"
#include "basicutils/stringutils.hh"
#include "curlLib.hh"

#define INITIAL_CAMERA_ASSIGN_INTERVAL 0.1
#define CAMERA_ASSIGN_INTERVAL 1.0
#define CONNECTION_ATTEMPT_INTERVAL 2.0
#define CONNECTION_TIMEOUT 2.0

PixelStreamVsm::PixelStreamVsm() : vsmport(0), curr_camera(0x0), cameraassigned(false)
{
    this->protocol = PixelStreamVsmProtocol;
    this->assigncameraattempt = new Timer;
    this->curl = curl_easy_init();
}

PixelStreamVsm::~PixelStreamVsm()
{
    delete this->assigncameraattempt;
    curl_easy_cleanup(this->curl);
}

bool PixelStreamVsm::operator == (const PixelStreamVsm &that)
{
    if (this->vsmhost == that.vsmhost && this->vsmport == that.vsmport && this->curr_camera == that.curr_camera) return true;
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

int PixelStreamVsm::readerInitialize(const std::string &hostspec, int portspec, const std::string &pathspec, Value *cameraid)
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

    if (!hostspec.empty())
        this->vsmhost = hostspec;
    else
        this->vsmhost = "localhost";
    this->vsmport = portspec;
    this->curr_camera = cameraid;
    this->prev_camera = "";
    this->userpath = pathspec;

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

int PixelStreamVsm::resolveURL(std::string instr)
{
    std::string myhost, myport, mypath;
    size_t delim1, delim2, delim3;

    delim1 = instr.find("://");
    delim2 = instr.find(':', delim1+3);
    delim3 = instr.find('/', delim2+1);

    if (delim1 != std::string::npos)
    {
        myhost = instr.substr(delim1+3, delim2-delim1-3);
        if (delim2 != std::string::npos)
        {
            myport = instr.substr(delim2+1, delim3-delim2-1);
            if (delim3 != std::string::npos) mypath = instr.substr(delim3+1);
        }
    }

    if (!myhost.empty() && !myport.empty() && !mypath.empty())
    {
        if (!userpath.empty()) mypath = userpath;

        struct hostent *server = gethostbyname(myhost.c_str());
        if (!server)
        {
            error_msg("Unable to resolve host name: " << myhost);
            return -1;
        }

        mjpegIO = curlLibCreateHandle(inet_ntoa(*(in_addr * )server->h_addr), StringToInteger(myport), mypath, "", "", this);
        if (!mjpegIO) return -1;
    }

    return 0;
}

#else

PixelStreamVsm::PixelStreamVsm() : vsmport(0), curr_camera(0x0)
{
    this->protocol = PixelStreamVsmProtocol;
}

bool PixelStreamVsm::operator == (const PixelStreamVsm &that)
{
    if (this->vsmhost == that.vsmhost && this->vsmport == that.vsmport && this->curr_camera == that.curr_camera) return true;
    else return false;
}

bool PixelStreamVsm::operator != (const PixelStreamVsm &that)
{
    return !(*this == that);
}

int PixelStreamVsm::readerInitialize(const std::string &hostspec, int portspec, const std::strint &pathspec, Value *cameraid)
{
#ifndef CURL_ENABLED
    warning_msg("VSM requested, but unable to locate libcurl");
#endif
#ifndef JPEG_ENABLED
    warning_msg("VSM requested, but unable to locate libjpeg or libjpeg-turbo");
#endif

    if (!hostspec.empty())
        this->vsmhost = hostspec;
    else
        this->vsmhost = "localhost";
    this->vsmport = portspec;
    this->curr_camera = cameraid;

    return 0;
}

int PixelStreamVsm::reader(void)
{
    return 0;
}

#endif
