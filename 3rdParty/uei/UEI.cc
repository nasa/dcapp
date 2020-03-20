#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <strings.h>
#include "basicutils/msg.hh"
#include "udp_comm.hh"

#define UEI_BUFFER_SIZE 60
#define NUM_BEZELS 3

extern void HandleBezelPress(int);
extern void HandleBezelRelease(int);

static bool UEI_active = false;
static int bezelID;
static bool first = true;
static SocketInfo *uei_socket = 0x0;
static char uei_buffer[UEI_BUFFER_SIZE];

void UEI_init(const char *host, const char *port, const char *ID)
{
    bezelID = strtol(ID, 0x0, 10);

    uei_socket = mycomm_init(host, strtol(port, 0x0, 10), UEI_BUFFER_SIZE, 0, false, 40);

    bzero(uei_buffer, UEI_BUFFER_SIZE);

    if (uei_socket) UEI_active = true;
}

void UEI_read(void)
{
    int statlen, button[NUM_BEZELS];
    static int prev_button;

    if (UEI_active)
    {
        statlen = mycomm_read(uei_socket, uei_buffer);
        if (statlen > 0)
        {
            if (sscanf(uei_buffer, "%d %d %d", &button[0], &button[1], &button[2]) != 3)
                error_msg("Partial buffer received");
            else
            {
                if (button[bezelID] != prev_button)
                {
                    if (!first)
                    {
                        if (button[bezelID]) HandleBezelPress(button[bezelID]);
                        else HandleBezelRelease(prev_button);
                    }
                    prev_button = button[bezelID];
                }
            }
        }
        if (first) first = false;
    }
}

void UEI_term(void)
{
}
