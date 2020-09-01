#ifndef _UDP_COMM_HH_
#define _UDP_COMM_HH_

#include <string>
#include <sys/time.h>
#include <netinet/in.h>

typedef struct
{
    bool swap_flag;                /* Swap byte order */
    char *swap_space;              /* Space reserved for byte swapping */
    int server_port;               /* Server port # */
    int insize_netbuf;             /* Network input size */
    int in_pack_len;               /* Inout packet size */
    int socket;                    /* Socket descriptor */
    struct timeval timeout;        /* Timeout interval */
} SocketInfo;

extern SocketInfo *mycomm_init(int, int, bool, double);
extern int mycomm_read(SocketInfo *, char *);

#endif
