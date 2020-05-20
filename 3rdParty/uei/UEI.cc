#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <strings.h>
#include "basicutils/msg.hh"
#include "string_utils.hh"
#include "udp_comm.hh"
#include "UEI.hh"

#define UEI_BUFFER_SIZE 60
#define NUM_BEZELS 3

extern void HandleBezelPress(int);
extern void HandleBezelRelease(int);

UeiDevice::UeiDevice() : UEI_active(false), first(false), uei_socket(0x0), uei_buffer(0x0)
{
    this->uei_buffer = (char *)calloc(UEI_BUFFER_SIZE, sizeof(char));
}

UeiDevice::~UeiDevice()
{
    if (this->uei_buffer) free(this->uei_buffer);
}

void UeiDevice::connect(const char *host, const char *port, const char *ID)
{
    this->bezelID = StringToInteger(ID);
    this->uei_socket = mycomm_init(host, StringToInteger(port), UEI_BUFFER_SIZE, 0, false, 40);
    if (this->uei_socket) UEI_active = true;
}

void UeiDevice::read(void)
{
    int statlen, button[NUM_BEZELS];
    static int prev_button;

    if (this->UEI_active)
    {
        statlen = mycomm_read(this->uei_socket, this->uei_buffer);
        if (statlen > 0)
        {
            if (sscanf(this->uei_buffer, "%d %d %d", &button[0], &button[1], &button[2]) != 3)
                error_msg("Partial buffer received");
            else
            {
                if (button[this->bezelID] != prev_button)
                {
                    if (!(this->first))
                    {
                        if (button[this->bezelID]) HandleBezelPress(button[this->bezelID]);
                        else HandleBezelRelease(prev_button);
                    }
                    prev_button = button[this->bezelID];
                }
            }
        }
        if (this->first) this->first = false;
    }
}
