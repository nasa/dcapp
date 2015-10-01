#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include "util_comm.hh"
#include "msg.hh"
#include "bezel.hh"

#define UEI_BUFFER_SIZE 60
#define NUM_BEZELS 3

extern void HandleBezelButton(int, int, int);

static int UEI_active = 0;
static int bezelID;
static int first = 1;
static int *uei_socket;
static char uei_buffer[UEI_BUFFER_SIZE];

void UEI_init(char *host, char *port, char *ID)
{
    bezelID = strtol(ID, 0x0, 10);

    uei_socket = mycomm_init(ENT_SERVER, host, strtol(port, 0x0, 10), UEI_BUFFER_SIZE, 0, 0, 40);

    bzero(uei_buffer, UEI_BUFFER_SIZE);

    if (uei_socket) UEI_active = 1;
}

extern void UEI_read(void)
{
    int statlen, button[NUM_BEZELS];
    static int prev_button;

    if (UEI_active)
    {
        statlen = mycomm_read(uei_socket, uei_buffer);
        if (statlen)
        {
            if (sscanf(uei_buffer, "%d %d %d", &button[0], &button[1], &button[2]) != 3)
                error_msg("Partial buffer received");
            else
            {
                if (button[bezelID] != prev_button)
                {
                    if (!first)
                    {
                        if (button[bezelID]) HandleBezelButton(BEZEL_BUTTON, button[bezelID], BEZEL_PRESSED);
                        else HandleBezelButton(BEZEL_BUTTON, prev_button, BEZEL_RELEASED);
                    }
                    prev_button = button[bezelID];
                }
            }
        }
        if (first) first=0;
    }
}
