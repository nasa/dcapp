#ifdef NTCAN

#include <string>
#include <cstdio>
#include <cstdlib>
#include "ntcan.h"
#include "basicutils/stringutils.hh"
#include "basicutils/msg.hh"
#include "CAN.hh"

#define BEZEL_CONTROL_TYPE 0xaa
#define BEZEL_CONTROL_ID 1
#define BEZEL_BUTTON 5
#define BEZEL_PRESSED 1
#define BEZEL_RELEASED 0

extern void HandleBezelPress(int);
extern void HandleBezelRelease(int);

CanDevice::CanDevice() : CAN_active(false), canbus_inhibited(0x0), buttonID(0), controlID(0) { }

CanDevice::~CanDevice()
{
    NTCAN_RESULT retval;

    if (CAN_active)
    {
        retval = canClose(this->ntCanHandle);
        if (retval != NTCAN_SUCCESS) error_msg("canClose failed with code " << retval);
    }
}

void CanDevice::initialize(const std::string &networkstr, const std::string &buttonIDstr, const std::string &controlIDstr, const std::string &inhibitstr)
{
    int network;
    NTCAN_RESULT retval;

    network = StringToInteger(networkstr);
    this->buttonID = StringToInteger(buttonIDstr);
    this->controlID = StringToInteger(controlIDstr);

    // Tell the driver to open a connection and give us a handle.
    retval = canOpen(network, 0, 1024, 1024, 1000, 0, &(this->ntCanHandle));
    if (retval != NTCAN_SUCCESS)
    {
        error_msg("canOpen failed with code " << retval);
        return;
    }

    // Set the baud rate to 1000.
    retval = canSetBaudrate(this->ntCanHandle, NTCAN_BAUD_1000);
    if (retval != NTCAN_SUCCESS)
    {
        error_msg("canSetBaudrate failed with code " << retval);
        return;
    }

    // Tell the driver which message IDs we're interested in.
    retval = canIdAdd(this->ntCanHandle, this->buttonID);
    if (retval != NTCAN_SUCCESS)
    {
        error_msg("canIdAdd failed with code " << retval);
        return;
    }
    retval = canIdAdd(this->ntCanHandle, this->controlID);
    if (retval != NTCAN_SUCCESS)
    {
        error_msg("canIdAdd failed with code " << retval);
        return;
    }

    this->canbus_inhibited = getVariable(inhibitstr);

    this->CAN_active = true;
}

void CanDevice::read(void)
{
    CMSG message;
    int32_t length = 1;
    int i;
    NTCAN_RESULT retval;

    if (this->CAN_active)
    {
        retval = canTake(this->ntCanHandle, &message, &length);
        if (retval == NTCAN_SUCCESS)
        {
            for (i=0; i<length; i++)
            {
                if (message.id == this->buttonID && message.data[0] == BEZEL_BUTTON)
                {
                    if (message.data[2] == BEZEL_PRESSED) HandleBezelPress(message.data[1]);
                    else if (message.data[2] == BEZEL_RELEASED) HandleBezelRelease(message.data[1]);
                }
                else if (message.id == this->controlID && message.data[0] == BEZEL_CONTROL_TYPE && message.data[1] == BEZEL_CONTROL_ID && this->canbus_inhibited)
                {
                    if (message.data[2]) this->canbus_inhibited->setToBoolean(false);
                    else this->canbus_inhibited->setToBoolean(true);
                }
            }
        }
        else error_msg("canTake failed with code " << retval);
    }
}

#else

#include "basicutils/msg.hh"
#include "CAN.hh"

CanDevice::CanDevice()
{
    warning_msg("CAN device requested, but CANBUS_HOME not identified...");
}

#endif
