/*****************************************************************************
PURPOSE:
    (Utility functions for a UDP/IP communication.)
REFERENCE:
    ((None))
ASSUMPTIONS AND LIMITATIONS:
    ((None))
CLASS:
    (N/A)
LIBRARY DEPENDENCY:
    ((net_util.o))
PROGRAMMERS:
    (((Hadi Tjandrasa)   (LMSO)   (03/03)  (ECO 8678 - Replace SCRAMNet with UDP/IP))
     ((Hadi Tjandrasa)   (LMSO)   (05/03)  (Fwd SCI Upgrade))
     ((Hadi Tjandrasa)   (LMSO)   (09/03)  (ECO 8785 - Baseline for DST I/F))
     ((Hadi Tjandrasa)   (LMSO)   (02/04)  (SES Port to Linux)))
*******************************************************************************/

#include "net_util.hh"

#define SYNCH_BYTE_LEN 4

static socklen_t len_sckaddr = sizeof(struct sockaddr_in);   /*constant*/
#ifdef VxWorks
static unsigned long inet_addr(char *inchar);
#endif


/***************************************************************************
 * To initialize default config for a UDP/IP socket
 *
 * Parameters:
 *    which - ENT_CLIENT, ENT_SERVER, ENT_NOSYNCH.
 *    epar  - socket and IP parameter data structure.
 *    ilen  - Length of input packet in bytes.
 *    olen  - Length of output packet in bytes.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
 * Modified By: Hadi Tjandrasa   LMSO  05/03  ECO 8724 (Fwd SCI Upgrade)
 *
***************************************************************************/
int init_ent_default_param(int which, ENT_PARAM_S *epar, int ilen, int olen)
{
    char *senv;
    int i;
    struct hostent *phst;

    epar->tv_sec = epar->timeout_ms/1000; /* Peek-socket timeout */
    epar->tv_usec = (epar->timeout_ms - (epar->tv_sec*1000)) * 1000;

    strcpy(epar->proto, "udp");           /* UDP only for now */
    epar->domain  = PF_INET;              /* NETWORK domain only for now */
    epar->base_port = 15000;

    epar->in_pack_len  = ilen;            /* input packet length */
    epar->out_pack_len = olen;            /* output packet length */

    if (epar->insize_netbuf < epar->in_pack_len+48) epar->insize_netbuf = ENT_BUFLEN_BYTES;
    if (epar->outsize_netbuf < epar->out_pack_len+48) epar->outsize_netbuf = ENT_BUFLEN_BYTES;

    /* Hard coded server_port prevails if it set to >= ENT_SRV_MIN_PORT */
    if (epar->server_port < ENT_SRV_MIN_PORT)
    {
        senv = getenv("ENT_DELTA_PORT");
        if (senv && ((int)strlen(senv) > 0)) epar->delta_port = atoi(senv);
        else if ((epar->delta_port < 0) || (epar->delta_port > 9999))
        {
            epar->delta_port = 0;
            fprintf(stderr, "WARNING/init_ent_default_param: delta_port default to 0\n");
        }
        epar->server_port = create_server_port(epar);
    }

    if (which == ENT_SERVER)
    {
        epar->myport = epar->server_port;
        /* MUST explicitly initialize 'server_ip' for VxWorks */
#ifndef VxWorks
        if (epar->server_ip[0] == 0)
        {
            senv = getenv("ENT_SERVER_IP_ADDR");
            if (senv && ((int)strlen(senv) > 2))
            {
                strcpy(epar->server_ip, senv);
                fprintf(stderr, "init_ent_default_param: ENT_SERVER_IP_ADDR env.\n");
            }
            else
            {
                gethostname(epar->server_ip, sizeof(epar->server_ip));
                fprintf(stderr,"init_ent_default_param: SERVER_IP= %s\n", epar->server_ip);
            }
        }
#endif
    }
    else if (which == ENT_CLIENT)
    {
        epar->myport = 0;

        /* INIT SERVER NAME */
        memset((char *)&(epar->yo_sckaddr), 0, len_sckaddr);
        epar->yo_sckaddr.sin_family = epar->domain;
        epar->yo_sckaddr.sin_port = htons(epar->server_port);
        for (i=0; i<8; i++) epar->yo_sckaddr.sin_zero[i] = 0;

#ifndef VxWorks
        if (epar->server_ip[0] == 0)
        {
            senv = getenv("ENT_SERVER_IP_ADDR");
            if (senv && ((int)strlen(senv) > 2)) strcpy(epar->server_ip, senv);
            else
            {
                fprintf(stderr, "ERROR/init_ent_default_param: invalid server name %s\n", epar->server_ip);
                return (-1);
            }
        }
        if ((phst = gethostbyname( epar->server_ip)) == 0)
        {
            fprintf(stderr, "ERROR/init_ent_default_param: gethostbyname - invalid server name %s\n", epar->server_ip);
            return (-1);
        }
        epar->yo_sckaddr.sin_addr.s_addr = *((in_addr_t*)phst->h_addr);
#else
        /* VxWorks does not support gethostbyname*/
        epar->yo_sckaddr.sin_addr.s_addr = htonl(inet_addr(epar->server_ip));
#endif
    }
    else fprintf(stderr, "ERROR/init_ent_default_param: invalid agent type %d\n", which);

    return 0;
}


/***************************************************************************
 * To clear/zero out a user UDP comm data structure - ENT_PARAM_S.
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
void clear_param(ENT_PARAM_S *epar)
{
    char *par = (char*)epar;
    int ii = sizeof(ENT_PARAM_S);
    while (ii) par[--ii] = 0;
}


/***************************************************************************
 * To generate a port number for a server agent.
 * A same user need to specify a different delta port value for each server agent.
 *
 * Return: port #.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
int create_server_port(ENT_PARAM_S *epar)
{
#ifndef VxWorks
    return(epar->base_port + epar->delta_port + (int)getuid());
#else
    /* VxWorks does not support getuid */
    return(ENT_SRV_MIN_PORT);
#endif
}


/***************************************************************************
 * To enable NON-BLOCKING socket.
 *
 * Return: -1=Error, 0=OK.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
int set_socket_nonblocking(ENT_PARAM_S *epar)
{
    epar->nonblock_opt_flag = 1;
    if (ioctl(epar->socket, FIONBIO, (char*)&(epar->nonblock_opt_flag)) == -1)
    {
        fprintf(stderr, "ERROR/set_socket_nonblocking: failed %d\n", errno);
        return (-1);
    }
    return 0;
}


/***************************************************************************
 * To disable NON-BLOCKING socket - set to BLOCKING.
 *
 * Return: -1=Error, 0=OK.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
int set_socket_blocking(ENT_PARAM_S *epar)
{
    epar->nonblock_opt_flag = 0;
    if (ioctl(epar->socket, FIONBIO, (char*)&(epar->nonblock_opt_flag)) == -1)
    {
        fprintf(stderr, "ERROR/set_socket_blocking: failed %d\n", errno);
        return (-1);
    }
    return 0;
}


/***************************************************************************
 * To check if the socket is somewhat valid.
 *
 * Return: 1=Valid, 0=Not-Valid
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
int comm_is_open(ENT_PARAM_S *epar)
{
    if ((epar->socket > 2) && (epar->socket < 99)) return 1;
    else return 0;
}


/***************************************************************************
 * Open socket, set options, and synch as necessary with other agent.
 *
 * Parameters:
 *    which - ENT_CLIENT, ENT_SERVER, ENT_NOSYNCH.
 *    epar  - socket and IP parameter data structure.
 *
 * Return:  Socket descriptor or -1=Error.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
 * Modified By: Hadi Tjandrasa   LMSO  05/03  ECO 8724 (Fwd SCI Upgrade)
 *
***************************************************************************/
int create_udp_socket(int which, ENT_PARAM_S *epar)
{
    struct  protoent *pent;
#ifdef VxWorks
    struct  protoent spent;
#endif
    int  type=0, sd;
    int  optval=1;
    int  i;

#ifndef VxWorks
    if ((pent = getprotobyname(epar->proto)))
    {
        fprintf(stderr, "create_udp_socket(): %s protocol ID: %d \n", epar->proto, pent->p_proto);
    }
    else
    {
        fprintf(stderr, "ERROR/create_udp_socket: getprotobyname() failed %d\n", errno);
        return (-1);
    }

    if (!strcasecmp(epar->proto, "udp"))
        type = SOCK_DGRAM;
    else
        fprintf(stderr, "ERROR/create_udp_socket: unknown protocol %s.\n", epar->proto);
#else
    /* VxWorks does not support getprotobyname */
    type = SOCK_DGRAM;
    pent = &spent;
    pent->p_proto = IPPROTO_UDP;
#endif

    /* CREATE A SOCKET */
    if ((sd = socket(epar->domain, type, pent->p_proto)) == -1)
    {
        fprintf(stderr, "ERROR/create_udp_socket: socket() failed %d\n", errno);
        return (-1);
    }
    else epar->socket = sd;

    /* ADDRESS REUSABLE */
    if ( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) == -1)
    {
        fprintf(stderr, "ERROR/create_udp_socket: setsocketopt(SO_REUSEADDR) failed %d\n", errno);
        close(sd);
        return (-1);
    }

    /*SET INPUT&OUTPUT BUFFER SIZE, OTHERWISE LET IT TO DEFAULT */
    if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char *)&(epar->insize_netbuf), sizeof(epar->insize_netbuf)) == -1)
    {
        fprintf(stderr, "ERROR/create_udp_socket: setsocketopt(SO_RCVBUF) failed %d\n", errno);
        close(sd);
        return (-1);
    }

    if (setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char *)&(epar->outsize_netbuf), sizeof(epar->outsize_netbuf)) == -1)
    {
        fprintf(stderr, "ERROR/create_udp_socket: setsocketopt(SO_SNDBUF) failed %d\n", errno);
        close(sd);
        return (-1);
    }

    if (ioctl(sd, FIONBIO, (char*)&(epar->nonblock_opt_flag)) == -1)
    {
        fprintf(stderr, "ERROR/create_udp_socket: ioctl(FIONBIO) failed %d\n", errno);
        close(sd);
        return (-1);
    }

    /* ZERO OUT AND INIT OWN/CLIENT NAME */
    memset((char *)&(epar->my_sckaddr), 0, len_sckaddr);
    epar->my_sckaddr.sin_family = epar->domain;
    epar->my_sckaddr.sin_addr.s_addr = INADDR_ANY;
    epar->my_sckaddr.sin_port = htons(epar->myport);
    for (i=0; i<8; i++) epar->my_sckaddr.sin_zero[i] = 0;

    /* ASSOCIATE A NAME WITH A SOCKKET*/
    if (bind(sd, (struct sockaddr *)&(epar->my_sckaddr), len_sckaddr) == -1)
    {
        fprintf(stderr, "ERROR/create_udp_socket: bind() failed %d\n", errno);
        close(sd);
        exit(-1);
    }

    /* SYNCH WITH THE OTHER AGENT */
    if (which == ENT_SERVER)
        wait_client_synch(epar);
    else if (which == ENT_CLIENT)
        probe_server_synch(epar);
    else if (which == ENT_NOSYNCH)
        fprintf( stderr, "create_udp_socket: no synch in opening socket\n");
    else
        fprintf(stderr, "ERROR/create_udp_socket: invalid agent type %d\n", which);

    return sd;
}


/***************************************************************************
 * To peek if data is available for reading.
 *
 * Return: # of ready descriptors or -1 (Error).
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
int peek_socket_ready(int psock, int sec, int usec)
{
    int nb;
    fd_set readsfd;
    struct timeval timeout;

    /* SET THE SELECTED SOCKET FOR POLLING*/
    FD_ZERO(&readsfd);
    FD_SET(psock, &readsfd);
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;
    nb = select(psock+1, &readsfd, 0x0, 0x0, &timeout);

    return nb;
}


/***************************************************************************
 * To probe if data is available for reading.
 *
 * Return: # of bytes available for read.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
int data_ready(int psock)
{
    int nb;
    ioctl(psock, FIONREAD, (char*)&nb);
    return nb;
}


/***************************************************************************
 * To initiate&handshake with the other agent (server).
 *
 * Return: -1=Error,  0=OK.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
int probe_server_synch(ENT_PARAM_S *epar)
{
    int clen = 0;
    int synch_word, cbuf, swapbuf;
    int iloop = 0;

    synch_word = epar->server_port + ENT_CLI_REQ;
    if (epar->swap_byte_flag)
        swap_byte((char*)&synch_word, (char*)&swapbuf, SYNCH_BYTE_LEN, sizeof(int));
    else
        swapbuf = synch_word;

    while (!(clen == SYNCH_BYTE_LEN))
    {
        if (sendto(epar->socket, (char*)&swapbuf, SYNCH_BYTE_LEN, 0, (struct sockaddr*)&epar->yo_sckaddr,len_sckaddr) == -1)
        {
            if (errno != ECONNREFUSED) fprintf(stderr,"ERROR/probe_server_synch: sendto errno %d\n", errno);
        }

        if (!(++iloop % (5*200)))
        {
            iloop = 0;
            fprintf(stderr,">>>ENT: Request to connect to IP %s, Port %d\n",
            epar->server_ip,epar->server_port);
        }

        nap_delay(5);

        if (data_ready(epar->socket))
        {
            synch_word = epar->server_port + ENT_SRV_ACK;

            /* Get ACK/response */
            if ((clen = recvfrom(epar->socket, (char*)&cbuf, SYNCH_BYTE_LEN, 0, (struct sockaddr*)&epar->yo_sckaddr, &len_sckaddr)) == SYNCH_BYTE_LEN)
            {
                if (synch_word == cbuf)
                {
                    /* Send ACK */
                    synch_word = epar->server_port + ENT_CLI_ACK;
                    if (epar->swap_byte_flag)
                        swap_byte((char*)&synch_word, (char*)&swapbuf, SYNCH_BYTE_LEN, sizeof(int));
                    else
                        swapbuf = synch_word;

                    if (sendto(epar->socket,(char*)&swapbuf, SYNCH_BYTE_LEN, 0, (struct sockaddr*)&epar->yo_sckaddr,len_sckaddr) == -1)
                    {
                        if (errno != ECONNREFUSED) fprintf(stderr,"ERROR/probe_server_synch: sendto errno %d\n", errno);
                    }
                }
                else
                {
                    fprintf(stderr,"ERROR/:probe_server_synch invalid synch word %d\n", cbuf);
                    return(-1);
                }
            }
            else fprintf(stderr,"ERROR/probe_server_synch: recvfrom errno %d\n", errno);
        }

        nap_delay(10);
    }

    return 0;
}


/***************************************************************************
 * To wait&handshake with the other agent (client).
 *
 * Return: -1=Error, 0=OK.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
int wait_client_synch(ENT_PARAM_S *epar)
{
    int dataready = 0;
    int synch_word, cbuf, swapbuf;
    int iloop = 0;

    synch_word = epar->server_port + ENT_CLI_REQ;

    while ( !dataready)
    {
        /* Get REQ response and send ACK acknowledgement */
        if ((dataready= data_ready(epar->socket)) > 0)
        {
            if (recvfrom(epar->socket, (char*)&cbuf, SYNCH_BYTE_LEN, 0, (struct sockaddr*)&epar->yo_sckaddr, &len_sckaddr) == -1)
            {
                fprintf(stderr,"ERROR/wait_client_synch: recvfrom errno %d\n", errno );
            }
            if (synch_word != cbuf)
            {
                fprintf(stderr,"ERROR/wait_client_synch: invalid synch word %d\n", cbuf);
                return(-1);
            }

            synch_word = epar->server_port + ENT_SRV_ACK;
            if (epar->swap_byte_flag)
                swap_byte((char*)&synch_word, (char*)&swapbuf, SYNCH_BYTE_LEN, sizeof(int));
            else
                swapbuf = synch_word;

            if (sendto(epar->socket, (char*)&swapbuf, SYNCH_BYTE_LEN, 0, (struct sockaddr*)&epar->yo_sckaddr,len_sckaddr) == -1)
            {
                fprintf(stderr,"ERROR/wait_client_synch: sendto errno %d\n", errno );
            }
        }
        else if (dataready == -1)
        {
            fprintf(stderr,"ERROR/wait_client_synch: ioctl errno %d\n", errno);
            return (-1);
        }
        else nap_delay(1);


        if (!(++iloop % (1*500)))
        {
            iloop = 0;
            fprintf(stderr,">>>ENT: IP %s, Port %d waiting for connection\n", epar->server_ip,epar->server_port);
        }
    }

    /* Get the CLI_ACK synch word */
    synch_word = epar->server_port + ENT_CLI_ACK;
    do
    {
        if ((dataready = data_ready(epar->socket)) >= SYNCH_BYTE_LEN)
            recvfrom(epar->socket, (char*)&cbuf,SYNCH_BYTE_LEN, 0, (struct sockaddr*)&epar->yo_sckaddr, &len_sckaddr);
        else
            nap_delay(1);

        if (!(++iloop % (4*500)))
        {
            iloop = 0;
            fprintf(stderr, ">>>ENT: IP %s, Port %d waiting for connection\n", epar->server_ip, epar->server_port);
        }
    } while (!(synch_word == cbuf));

    return 0;
}


/***************************************************************************
 * To read from one UDP socket.
 *
 * Return: Length of bytes read, -1=Error, or ENT_FATAL_ERROR.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
int peek_read_udp(ENT_PARAM_S *epar, void *dbuf)
{
    static int peek_counter = 0;
    int statlen = 0;

    if (epar->nonblock_opt_flag)
    {
        /*CHECK BEFORE READING */
        if (peek_socket_ready(epar->socket,epar->tv_sec,epar->tv_usec) > 0)
        {
            statlen = recvfrom( epar->socket, (char*)dbuf, epar->in_pack_len, 0, (struct sockaddr*)&epar->yo_sckaddr, &len_sckaddr);
            peek_counter = 0;
        }
        else
        {
            if (peek_counter++ > 250)
            {
                peek_counter = 0;
                fprintf(stderr,"WARNING/peek_read_udp: No input data to read\n");
            }
        }
    }
    else
    {
        statlen = recvfrom( epar->socket, (char*)dbuf, epar->in_pack_len, 0, (struct sockaddr*)&epar->yo_sckaddr, &len_sckaddr);
    }

    if (statlen == -1)
    {
        if (errno == EWOULDBLOCK)
        {
            fprintf(stderr,"ERROR/peek_read_udp: RECV, Fatal errno=%d\n", errno );
            statlen = ENT_FATAL_ERROR;
        }
        else
        {
            fprintf(stderr,"ERROR/peek_read_udp: RECV, errno=%d\n", errno );
        }
    }

    return(statlen);
}


/***************************************************************************
 * To read from a UDP port.
 *
 * Return: Length of data read in bytes or ENT_FATAL_ERROR .
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
int read_udp(ENT_PARAM_S *epar, void *dbuf)
{
    int  statlen = 0;

    if (epar->nonblock_opt_flag)
    {
        /*CHECK BEFORE READING */
        if (data_ready(epar->socket) >= epar->in_pack_len)
        {
            statlen = recvfrom(epar->socket, (char*)dbuf, epar->in_pack_len, 0, (struct sockaddr*)&(epar->yo_sckaddr), &len_sckaddr);
        }
    }
    else
    {
        statlen = recvfrom(epar->socket, (char*)dbuf, epar->in_pack_len, 0, (struct sockaddr*)&(epar->yo_sckaddr), &len_sckaddr);
    }

    if (statlen == -1)
    {
        if (errno == EWOULDBLOCK)
        {
            fprintf(stderr,"ERROR/read_udp: RECV, Fatal errno=%d\n", errno );
            statlen = ENT_FATAL_ERROR;
        }
        else
        {
            fprintf(stderr,"ERROR/read_udp: RECV, errno=%d\n", errno );
        }
    }

    return statlen;
}


/***************************************************************************
 * To write to a UDP port.
 * Return: Length of data written in bytes.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
int write_udp(ENT_PARAM_S *epar, void *dbuf)
{
    int  statlen;

    if ((statlen = sendto(epar->socket,(char *)dbuf,epar->out_pack_len, 0, (struct sockaddr*)&epar->yo_sckaddr, len_sckaddr)) == -1)
        fprintf( stderr,"ERROR/write_udp: errno=%d, server_ip=%s, server_port=%d, buf %08x\n", errno, epar->server_ip, epar->server_port, *((int*)dbuf));

    return statlen;
}


/***************************************************************************
 * To close the socket.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
void close_udp_socket(int psock)
{
    if (psock > 2)
    {
        shutdown(psock, 2);
        close(psock);
    }
    else
        fprintf(stderr,"ERROR/close_udp_socket: invalid socket %d\n", psock);
}


/***************************************************************************
 * Convert IP address in dot format to an array of characters.
 *
 * Originator: Hadi Tjandrasa      LMSO               03/02
***************************************************************************/
#ifdef VxWorks
static unsigned long inet_addr(char *inchar)
{
    int c0, c1, c2, c3;
    int status;
    ENT_IP_ADDR_S inet_adrs;

    /* Convert from string to binary */
    if ((status = sscanf( inchar, "%d.%d.%d.%d", &c0, &c1, &c2, &c3 ) != 4))
        printf("ERROR: number of item = %d\n", status );

    inet_adrs.byte_addr[0] = c0;
    inet_adrs.byte_addr[1] = c1;
    inet_adrs.byte_addr[2] = c2;
    inet_adrs.byte_addr[3] = c3;

    return inet_adrs.int_addr;
}
#endif


/***************************************************************************
 * To swap bytes within a specified word of a buffer.
 * Parameters:
 *   in    - input buffer
 *   out   - outpur buffer
 *   len   - length in bytes
 *   wsize - size of word type (2, 4, or 8)
 *
 * Originator: Hadi Tjandrasa      LMSO       (04/02)
***************************************************************************/
void swap_byte(char *in, char *out, int blen, int wsize)
{
    int k;
    int nword;
    unsigned short *i_2, *o_2;
    unsigned int *i_4, *o_4;
    unsigned long long *i_8, *o_8;
    unsigned char *ibuf, *obuf;

    switch (wsize)
    {
        case 2:
            i_2 = (unsigned short*)in;
            o_2 = (unsigned short*)out;
            nword = blen / wsize;
            for (k=0; k<nword; k++)
            {
                ibuf = (unsigned char*)&i_2[k];
                obuf = (unsigned char*)&o_2[k];
                obuf[0] = ibuf[1];
                obuf[1] = ibuf[0];
            }
            break;
        case 4:
            i_4 = (unsigned int*)in;
            o_4 = (unsigned int*)out;
            nword = blen / wsize;
            for (k=0; k<nword; k++)
            {
                ibuf = (unsigned char*)&i_4[k];
                obuf = (unsigned char*)&o_4[k];
                obuf[0] = ibuf[3];
                obuf[1] = ibuf[2];
                obuf[2] = ibuf[1];
                obuf[3] = ibuf[0];
            }
            break;
        case 8:
            i_8 = (unsigned long long*)in;
            o_8 = (unsigned long long*)out;
            nword = blen / wsize;
            for (k=0; k<nword; k++)
            {
                ibuf = (unsigned char*)&i_8[k];
                obuf = (unsigned char*)&o_8[k];
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
        case 1:    /* This is only copying - for convenience in testing/development */
            for (k=0; k<blen; k++) out[k] = in[k];
            break;
    }
}


/***************************************************************************
 * To sleep/delay either for Unix or VxWorks.
 * Parameters:
 *   value -  Either in millisecond (Unix) or tick (VxWorks0.
 *
 * Originator: Hadi Tjandrasa      LMSO       (04/02)
***************************************************************************/
void nap_delay(int value)
{
#ifndef VxWorks
    struct timespec rqtp, rmtp;

    rqtp.tv_sec = 0;
    rqtp.tv_nsec = value*1000000;
    nanosleep(&rqtp, &rmtp);
#else
    taskDelay(value);
#endif
}
