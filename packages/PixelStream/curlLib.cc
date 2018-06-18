#ifdef CURL_ENABLED
#include <curl/curl.h>
#endif
#include "basicutils/msg.hh"
#include "PixelStreamMjpeg.hh"

#ifdef CURL_ENABLED
static CURLM *multiCURL;
#endif

void curlLibInit(void)
{
#ifdef CURL_ENABLED
    multiCURL = curl_multi_init();
#endif
}

int curlLibAddHandle(void *handle)
{
#ifdef CURL_ENABLED
    if (!handle) return 0;

    CURLMcode mresult = curl_multi_add_handle(multiCURL, (CURL *)handle);

    if (mresult != CURLM_OK)
    {
        error_msg("CURL error " << mresult << ": " << curl_multi_strerror(mresult));
        return -1;
    }
#endif
    return 0;
}

void curlLibRemoveHandle(void *handle)
{
#ifdef CURL_ENABLED
    if (handle) curl_multi_remove_handle(multiCURL, (CURL *)handle);
#endif
}

void curlLibRun(void)
{
#ifdef CURL_ENABLED
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
#endif
}

size_t curlLibProcessBuffer(char *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t newbytes = size * nmemb;
    PixelStreamMjpeg *mystream = (PixelStreamMjpeg *)userp;
    mystream->processData(buffer, newbytes);
    return newbytes;
}

void curlLibTerm(void)
{
#ifdef CURL_ENABLED
    curl_multi_cleanup(multiCURL);
#endif
}
