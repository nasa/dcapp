#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cmath>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "basicutils/msg.hh"
#include "basicutils/tidy.hh"
#include "udp_comm.hh"


/*******************************************************************
* Open socket, set options, and synch as necessary with other agent
*
* Parameters:
*     sinfo - socket information structure
*******************************************************************/
static void create_udp_socket(SocketInfo *sinfo)
{
    struct protoent *pent;
    int sd;
    int optval = 1;
    int noblockflag = 1;
    int i;

    sinfo->socket = -1;

    if ((pent = getprotobyname("udp")))
    {
        debug_msg("protocol ID: " << pent->p_proto);
    }
    else
    {
        error_msg("getprotobyname() failed: " << strerror(errno));
        return;
    }

    /* CREATE A UDP SOCKET */
    if ((sd = socket(PF_INET, SOCK_DGRAM, pent->p_proto)) == -1)
    {
        error_msg("socket() failed: " << strerror(errno));
        return;
    }

    /* ADDRESS REUSABLE */
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) == -1)
    {
        error_msg("setsocketopt(SO_REUSEADDR) failed: " << strerror(errno));
        close(sd);
        return;
    }

    /* SET INPUT & OUTPUT BUFFER SIZE, OTHERWISE LET IT TO DEFAULT */
    if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char *)&(sinfo->insize_netbuf), sizeof(sinfo->insize_netbuf)) == -1)
    {
        error_msg("setsocketopt(SO_RCVBUF) failed: " << strerror(errno));
        close(sd);
        return;
    }

    /* SET SOCKET TO NON-BLOCKING */
    if (ioctl(sd, FIONBIO, (char *)&noblockflag) == -1)
    {
        error_msg("ioctl(FIONBIO) failed: " << strerror(errno));
        close(sd);
        return;
    }

    /* ZERO OUT AND INIT OWN/CLIENT NAME */
    struct sockaddr_in my_sckaddr;
    memset((char *)&my_sckaddr, 0, sizeof(struct sockaddr_in));
    my_sckaddr.sin_family = PF_INET;
    my_sckaddr.sin_addr.s_addr = INADDR_ANY;
    my_sckaddr.sin_port = htons(sinfo->server_port);
    for (i=0; i<8; i++) my_sckaddr.sin_zero[i] = 0;

    /* ASSOCIATE A NAME WITH A SOCKKET */
    if (bind(sd, (struct sockaddr *)&my_sckaddr, sizeof(struct sockaddr_in)) == -1)
    {
        error_msg("bind() failed: " << strerror(errno));
        close(sd);
        return;
    }

    sinfo->socket = sd;
}


/*******************************************************************
* Read from the UDP socket if data is ready
*
* Return: Length of bytes read, -1=Error or -2=WouldBlock
*
* Parameters:
*     sinfo - socket information structure
*     dbuf  - memory into which data from socket will be written
*******************************************************************/
static int peek_read_udp(SocketInfo *sinfo, char *dbuf)
{
    static int peek_counter = 0;
    int statlen = 0;
    fd_set readsfd;

    /* POLL SOCKET BEFORE READING */
    FD_ZERO(&readsfd);
    FD_SET(sinfo->socket, &readsfd);

    if (select(sinfo->socket + 1, &readsfd, 0x0, 0x0, &(sinfo->timeout)) > 0)
    {
#if 0
        static socklen_t len_sckaddr = sizeof(struct sockaddr_in);
        struct sockaddr_in yo_sckaddr;
        statlen = recvfrom(sinfo->socket, dbuf, sinfo->in_pack_len, 0, (struct sockaddr*)&yo_sckaddr, &len_sckaddr);
#else
        statlen = recv(sinfo->socket, dbuf, sinfo->in_pack_len, 0);
#endif
        peek_counter = 0;
    }
    else
    {
        if (peek_counter++ > 250)
        {
            peek_counter = 0;
            warning_msg("No input data to read");
        }
    }

    if (statlen == -1)
    {
        if (errno == EWOULDBLOCK) return (-2);
        else error_msg("RECV error: " << strerror(errno));
    }

    return statlen;
}


/*******************************************************************
 * Swap bytes within a specified word of a buffer
 *
 * Parameters:
 *     in    - input buffer
 *     out   - outpur buffer
 *     blen  - length in bytes
 *     wsize - size of word type (2, 4, or 8)
*******************************************************************/
static void swap_byte(char *in, char *out, int blen, int wsize)
{
    int k;
    int nword;
    unsigned short *i_2, *o_2;
    unsigned int *i_4, *o_4;
    unsigned long long *i_8, *o_8;
    unsigned char *ibuf, *obuf;

    switch (wsize)
    {
        case 1:
            for (k=0; k<blen; k++) out[k] = in[k];
            break;
        case 2:
            i_2 = (unsigned short *)in;
            o_2 = (unsigned short *)out;
            nword = blen / wsize;
            for (k=0; k<nword; k++)
            {
                ibuf = (unsigned char *)&i_2[k];
                obuf = (unsigned char *)&o_2[k];
                obuf[0] = ibuf[1];
                obuf[1] = ibuf[0];
            }
            break;
        case 4:
            i_4 = (unsigned int *)in;
            o_4 = (unsigned int *)out;
            nword = blen / wsize;
            for (k=0; k<nword; k++)
            {
                ibuf = (unsigned char *)&i_4[k];
                obuf = (unsigned char *)&o_4[k];
                obuf[0] = ibuf[3];
                obuf[1] = ibuf[2];
                obuf[2] = ibuf[1];
                obuf[3] = ibuf[0];
            }
            break;
        case 8:
            i_8 = (unsigned long long *)in;
            o_8 = (unsigned long long *)out;
            nword = blen / wsize;
            for (k=0; k<nword; k++)
            {
                ibuf = (unsigned char *)&i_8[k];
                obuf = (unsigned char *)&o_8[k];
                obuf[0] = ibuf[7];
                obuf[1] = ibuf[6];
                obuf[2] = ibuf[5];
                obuf[3] = ibuf[4];
                obuf[4] = ibuf[3];
                obuf[5] = ibuf[2];
                obuf[6] = ibuf[1];
                obuf[7] = ibuf[0];
            }
            break;
    }
}


/*******************************************************************
* Initialize a server UDP socket
*
* Return: Pointer to the socket information
*
* Parameters:
*     port    - udp port offset (udp port = 30000 + port)
*     insize  - size of input buffer (read buffer)
*     swap    - swap byte flag
*     timeout - read timeout interval (in seconds)
*******************************************************************/
SocketInfo *mycomm_init(int port, int insize, bool swap, double timeout)
{
    SocketInfo *mysock = (SocketInfo *)calloc(sizeof(SocketInfo), 1);
    if (!mysock)
    {
        error_msg("Unable to allocate memory for mysock");
        return 0x0;
    }

    if (swap)
    {
        mysock->swap_space = (char *)calloc(insize, 1);
        if (!mysock->swap_space)
        {
            error_msg("Unable to allocate memory for swap_space");
            free(mysock);
            return 0x0;
        }
    }

    mysock->swap_flag = swap;
    mysock->server_port = 30000 + port;

    debug_msg("Byte Swapping is " << (swap ? "ON" : "OFF"));
    debug_msg("UEI port: " << mysock->server_port);

    double integer, fractional;
    fractional = modf(timeout, &integer);
    mysock->timeout.tv_sec = (time_t)integer;
    mysock->timeout.tv_usec = (suseconds_t)((fractional * 1000000.0) + 0.5);

    mysock->in_pack_len = insize;  /* input packet length */

    if (mysock->insize_netbuf < mysock->in_pack_len + 48) mysock->insize_netbuf = 61440; /* Max UDP buffer size - SGI default */

    create_udp_socket(mysock);

    if (mysock->socket == -1)
    {
        error_msg("create_udp_socket() failed: " << strerror(errno));
        TIDY(mysock->swap_space);
        free(mysock);
        return 0x0;
    }

    return mysock;
}


/*******************************************************************
* Read from the specified UDP port
*
* Return: Size of the buffer received (0=no data, negative number=invalid)
*
* Parameters:
*     mysock   - pointer to the socket information
*     r_buffer - pointer to the input buffer (read buffer)
*******************************************************************/
int mycomm_read(SocketInfo *mysock, char *r_buffer)
{
    int statlen;

    if (mysock->swap_flag)
    {
        swap_byte(r_buffer, mysock->swap_space, mysock->in_pack_len, sizeof(int));
        statlen = peek_read_udp(mysock, mysock->swap_space);
    }
    else
    {
        statlen = peek_read_udp(mysock, r_buffer);
    }

    /* if an error occurred reading the socket, close the socket */
    if (statlen == -1)
    {
        if (mysock->socket > 2)
        {
            shutdown(mysock->socket, 2);
            close(mysock->socket);
        }
        else error_msg("shutdown/close socket: invalid socket: " << mysock->socket);
    }

    return statlen;
}
