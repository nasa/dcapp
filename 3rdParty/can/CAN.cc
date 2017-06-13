#include <cstdio>
#include <cstdlib>
#include "basicutils/msg.hh"
#ifdef NTCAN
#include "ntcan.h"

extern void HandleBezelButton(int, int, int);
extern void HandleBezelControl(int, int, int);

static NTCAN_HANDLE ntCanHandle;
static int CAN_active = 0;
static uint32_t buttonID = 0, controlID = 0;
#endif

void CAN_init(const char *networkstr, const char *buttonIDstr, const char *controlIDstr)
{
#ifdef NTCAN
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

    CAN_active = 1;
#endif
}

extern void CAN_read(void)
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
                if (message.id == buttonID) HandleBezelButton(message.data[0], message.data[1], message.data[2]);
                else if (message.id == controlID) HandleBezelControl(message.data[0], message.data[1], message.data[2]);
            }
        }
        else error_msg("canTake failed with code " << retval);
    }
#endif
}

extern void CAN_term(void)
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
