#include <cstdio>
#include <cstdlib>
#include "basicutils/msg.hh"
#ifdef NTCAN
#include "ntcan.h"

#define BEZEL_CONTROL_TYPE 0xaa
#define BEZEL_CONTROL_ID 1
#define BEZEL_BUTTON 5
#define BEZEL_PRESSED 1
#define BEZEL_RELEASED 0

extern void HandleBezelPress(int);
extern void HandleBezelRelease(int);

static NTCAN_HANDLE ntCanHandle;
static bool CAN_active = false;
static int *canbus_inhibited = 0x0;
static uint32_t buttonID = 0, controlID = 0;
#endif

#ifdef NTCAN
void CAN_init(const char *networkstr, const char *buttonIDstr, const char *controlIDstr, int *inhibit_ptr)
{
    int network = 0;
    NTCAN_RESULT retval;

    if (networkstr) network = strtol(networkstr, 0x0, 10);
    if (buttonIDstr) buttonID = strtol(buttonIDstr, 0x0, 10);
    if (controlIDstr) controlID = strtol(controlIDstr, 0x0, 10);

    // Tell the driver to open a connection and give us a handle.
    retval = canOpen(network, 0, 1024, 1024, 1000, 0, &ntCanHandle);
    if (retval != NTCAN_SUCCESS)
    {
        error_msg("canOpen failed with code " << retval);
        return;
    }

    // Set the baud rate to 1000.
    retval = canSetBaudrate(ntCanHandle, NTCAN_BAUD_1000);
    if (retval != NTCAN_SUCCESS)
    {
        error_msg("canSetBaudrate failed with code " << retval);
        return;
    }

    // Tell the driver which message IDs we're interested in.
    retval = canIdAdd(ntCanHandle, buttonID);
    if (retval != NTCAN_SUCCESS)
    {
        error_msg("canIdAdd failed with code " << retval);
        return;
    }
    retval = canIdAdd(ntCanHandle, controlID);
    if (retval != NTCAN_SUCCESS)
    {
        error_msg("canIdAdd failed with code " << retval);
        return;
    }

    canbus_inhibited = inhibit_ptr;

    CAN_active = true;
}
#else
void CAN_init(const char *, const char *, const char *, int *)
{
}
#endif

void CAN_read(void)
{
#ifdef NTCAN
    CMSG message;
    int32_t length = 1;
    int i;
    NTCAN_RESULT retval;

    if (CAN_active)
    {
        retval = canTake(ntCanHandle, &message, &length);
        if (retval == NTCAN_SUCCESS)
        {
            for (i=0; i<length; i++)
            {
                if (message.id == buttonID && message.data[0] == BEZEL_BUTTON)
                {
                    if (message.data[2] == BEZEL_PRESSED) HandleBezelPress(message.data[1]);
                    else if (message.data[2] == BEZEL_RELEASED) HandleBezelRelease(message.data[1]);
                }
                else if (message.id == controlID && message.data[0] == BEZEL_CONTROL_TYPE && message.data[1] == BEZEL_CONTROL_ID && canbus_inhibited)
                {
                    if (message.data[2]) *canbus_inhibited = 0;
                    else *canbus_inhibited = 1;
                }
            }
        }
        else error_msg("canTake failed with code " << retval);
    }
#endif
}

void CAN_term(void)
{
#ifdef NTCAN
    NTCAN_RESULT retval;

    if (CAN_active)
    {
        retval = canClose(ntCanHandle);
        if (retval != NTCAN_SUCCESS) error_msg("canClose failed with code " << retval);
    }
#endif
}
