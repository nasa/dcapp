#include "basicutils/msg.hh"
#include "PixelStreamMjpeg.hh"

#ifdef CURL_ENABLED

#include <string>
#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>

static CURLM *multiCURL;

static size_t curlLibProcessBuffer(char *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t newbytes = size * nmemb;
    PixelStreamMjpeg *mystream = (PixelStreamMjpeg *)userp;
    mystream->processData(buffer, newbytes);
    return newbytes;
}

void curlLibInit(void)
{
    multiCURL = curl_multi_init();
}

void *curlLibCreateHandle(const std::string &host, int port, const std::string &path, const std::string &username, const std::string &password, PixelStreamMjpeg *ptr)
{
    CURL *handle;
    CURLcode result;
    std::string myurl, mycredentials;

    myurl = host + ':' + std::to_string(port) + '/' + path;

    if (!username.empty())
    {
        mycredentials = username;
        if (!password.empty()) mycredentials += ':' + password;
    }

    handle = curl_easy_init();
    if (!handle)
    {
        error_msg("Failed to initialize CURL");
        return 0x0;
    }

    result = curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curlLibProcessBuffer);
    if (result != CURLE_OK)
    {
        error_msg("CURL error: " << curl_easy_strerror(result));
        return 0x0;
    }

    result = curl_easy_setopt(handle, CURLOPT_WRITEDATA, ptr);
    if (result != CURLE_OK)
    {
        error_msg("CURL error: " << curl_easy_strerror(result));
        return 0x0;
    }

    result = curl_easy_setopt(handle, CURLOPT_URL, myurl.c_str());
    if (result != CURLE_OK)
    {
        error_msg("CURL error: " << curl_easy_strerror(result));
        return 0x0;
    }

    if (!mycredentials.empty())
    {
        result = curl_easy_setopt(handle, CURLOPT_USERPWD, mycredentials.c_str());
        if (result != CURLE_OK)
        {
            error_msg("CURL error: " << curl_easy_strerror(result));
            return 0x0;
        }

        result = curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
        if (result != CURLE_OK)
        {
            error_msg("CURL error: " << curl_easy_strerror(result));
            return 0x0;
        }
    }

    return (void *)handle;
}

void curlLibDestroyHandle(void *handle)
{
    if (handle) curl_easy_cleanup((CURL *)handle);
}

int curlLibAddHandle(void *handle)
{
    if (!handle) return 0;

    CURLMcode mresult = curl_multi_add_handle(multiCURL, (CURL *)handle);

    if (mresult != CURLM_OK)
    {
        error_msg("CURL error " << mresult << ": " << curl_multi_strerror(mresult));
        return -1;
    }

    return 0;
}

void curlLibRemoveHandle(void *handle)
{
    if (handle) curl_multi_remove_handle(multiCURL, (CURL *)handle);
}

void curlLibRun(void)
{
    int running_handles, numfds;
    struct CURLMsg *msg;

    do
    {
        CURLMcode mresult = curl_multi_perform(multiCURL, &running_handles);
        if (mresult != CURLM_OK) warning_msg("CURL error " << mresult << ": " << curl_multi_strerror(mresult));
        curl_multi_wait(multiCURL, 0x0, 0, 1, &numfds);
    } while (numfds);

    do
    {
        int msgq = 0;
        msg = curl_multi_info_read(multiCURL, &msgq);
        if (msg)
        {
            if (msg->data.result != CURLE_OK)
                debug_msg("CURL error " << msg->data.result << ": " << curl_easy_strerror(msg->data.result));
            if (msg->msg == CURLMSG_DONE)
                curlLibRemoveHandle(msg->easy_handle);
            else
                warning_msg("CURL returned an unknown error");
        }
    } while (msg);
}

void curlLibTerm(void)
{
    curl_multi_cleanup(multiCURL);
}

#else

void curlLibInit(void)
{
    warning_msg("libcurl is required for MJPEG PixelStream");
}

void *curlLibCreateHandle(const std::string &, int, const std::string &, const std::string &, const std::string &, PixelStreamMjpeg *)
{
    return 0x0;
}

void curlLibDestroyHandle(void *) { }

int curlLibAddHandle(void *)
{
    return 0;
}

void curlLibRemoveHandle(void *) { }
void curlLibRun(void) { }
void curlLibTerm(void) { }

#endif
