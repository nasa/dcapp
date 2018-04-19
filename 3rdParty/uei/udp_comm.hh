#ifndef _UDP_COMM_HH_
#define _UDP_COMM_HH_

#include <netinet/in.h>

typedef struct
{
    bool swap_flag;                /* Swap byte order */
    char *swap_space;              /* Space reserved for byte swapping */
    int server_port;               /* Server port # */
    int myport;                    /* This agent port # */
    int base_port;                 /* Base port # */
    int delta_port;                /* Delta/offset port # */
    int insize_netbuf;             /* Network input size */
    int outsize_netbuf;            /* Network output size */
    int in_pack_len;               /* Inout packet size */
    int out_pack_len;              /* Output packet size */
    int socket;                    /* Socket descriptor */
    char server_ip[16];            /* Server IP address */
    int tv_sec;                    /* Second timeout */
    int tv_usec;                   /* Micro second timeout */
    struct sockaddr_in yo_sckaddr; /* Other agent socket address */
    struct sockaddr_in my_sckaddr; /* My socket address */
} SocketInfo;

extern SocketInfo *mycomm_init(const char *, int, int, int, bool, int);
extern int mycomm_read(SocketInfo *, char *);

#endif
