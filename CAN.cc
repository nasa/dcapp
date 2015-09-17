#include <stdio.h>
#include <stdlib.h>
#ifdef NTCAN
#include "ntcan.h"

extern void HandleBezelButton(int, int, int);
extern void HandleBezelControl(int, int, int);

static void CAN_error(char *, NTCAN_RESULT);

static NTCAN_HANDLE ntCanHandle;
static int CAN_active = 0;
static uint32_t buttonID = 0, controlID = 0;
#endif

void CAN_init(char *networkstr, char *buttonIDstr, char *controlIDstr)
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
        CAN_error("canOpen", retval);
        return;
    }

    // Set the baud rate to 1000.
    retval = canSetBaudrate(ntCanHandle, NTCAN_BAUD_1000);
    if (retval != NTCAN_SUCCESS)
    {
        CAN_error("canSetBaudrate", retval);
        return;
    }

    // Tell the driver which message IDs we're interested in.
    retval = canIdAdd(ntCanHandle, buttonID);
    if (retval != NTCAN_SUCCESS)
    {
        CAN_error("canIdAdd", retval);
        return;
    }
    retval = canIdAdd(ntCanHandle, controlID);
    if (retval != NTCAN_SUCCESS)
    {
        CAN_error("canIdAdd", retval);
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
        else CAN_error("canTake", retval);
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
        if (retval != NTCAN_SUCCESS) CAN_error("canClose", retval);
    }
#endif
}

#ifdef NTCAN
static void CAN_error(char *command, NTCAN_RESULT retval)
{
    printf("dcapp: CAN error: %s call failed with code %d\n", command, retval);
}
#endif
