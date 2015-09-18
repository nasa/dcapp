#include "util_comm.hh"

char *global_r_swap;
char *global_w_swap;
int global_swap_flag;
int global_r_length;
int global_w_length;

/*******************************************************************
*
* Description: This function initializes a server udp socket using
*              the net_util api.
*
* Function API:
*       returns integer containing the socket id
*       ENT_PARAM_S *epar - pointer to the net_util structure that
*                           that contains all the relevant socket
*                           information
*       int type          - flag specifying server or client udp
*                           socket (ENT_SERVER | ENT_CLIENT)
*       char *hostname    - string containing the hostname, both
*                           hostname and ip are valid strings
*       int port          - udp port offset (udp port = 30000 + port)
*       int insize        - size of input buffer (read buffer)
*       int outsize       - size of output buffer (write buffer)
*       int swap          - swap byte flag (1=swap | 0=no swap)
*
*******************************************************************/
int *mycomm_init(int type, char *hostname, int port, int insize, int outsize, int swap, int timeout)
{
    struct sock_param *mysock = (struct sock_param*)malloc(sizeof(struct sock_param));

    global_r_swap = (char*)malloc(insize);
    global_w_swap = (char*)malloc(outsize);
    global_swap_flag = swap;
    global_r_length = insize;
    global_w_length = outsize;

    if ( !((type == ENT_SERVER) || (type == ENT_CLIENT)) )
    {
        fprintf(stderr, "%s: ERROR invalid type %d, must be ENT_SERVER %d or ENT_CLIENT %d\n", __FUNCTION__, type, ENT_SERVER, ENT_CLIENT);
        exit(-1);
    }
    fprintf(stderr, "%s: Byte Swapping is %s (%d)\n", __FUNCTION__, (swap ? "ON" : "OFF"), swap);

    if (!hostname)
    {
        strcpy(hostname, "localhost");
        fprintf(stderr, "%s: WARNING - Failed to get HOSTNAME variable, using default %s\n", __FUNCTION__, hostname);
    }
    fprintf(stderr, "%s: Host name is %s \n", __FUNCTION__, hostname);

    clear_param(&mysock->epar);                              /* Zero out first */
    mysock->epar.timeout_ms = timeout;                       /* timeout for read */
    mysock->epar.nonblock_opt_flag = 1;                      /* 0= Blocking */
    mysock->epar.swap_byte_flag = swap;                      /* 0= No swap of byte */
    mysock->epar.server_port = ENT_SRV_MIN_PORT + 1 + port;  /* Hardcoded port must >= SRV_MIN_PORT */
    strcpy(mysock->epar.server_ip, hostname);
    fprintf(stderr, "%s: Server_IP %s  Host name %s \n", __FUNCTION__, mysock->epar.server_ip, hostname);
    init_ent_default_param( type , &mysock->epar, insize, outsize/*Out*/);

    fprintf(stderr, "%s: IP: %s, port= %d\n", __FUNCTION__,mysock->epar.server_ip, mysock->epar.server_port);

    if ( (mysock->socket_id = create_udp_socket( ENT_NOSYNCH, &mysock->epar )) == -1 )
    {
        fprintf(stderr, "ERROR in %s: create_udp_socket(epar) failed, errno= %d\n", __FUNCTION__, errno);
        exit(-1);
    }

    return ((int *)mysock);
}


/*******************************************************************
*
* Description: This function writes to the specified udp port using
*              the net_util api.
*
* Function API:
*       ENT_PARAM_S *epar - pointer to the net_util structure that
*                           that contains all the relevant socket
*                           information
*       int socket_id     - integer containing the socket id
*       unsigned short
*             *w_buffer   - pointer to the output buffer (write buffer)
*
*******************************************************************/
void mycomm_write(int *ptr, void *w_buffer)
{
    int statlen;
    struct sock_param *mysock = (struct sock_param *)ptr;

    if (global_swap_flag)
    {
        swap_byte((char*)w_buffer, global_w_swap, global_w_length, sizeof(int));
        statlen = write_udp(&mysock->epar, (char *)global_w_swap) ;
        //if(statlen) fprintf(stderr,"\t%s: LINE %d swapped\n", __FUNCTION__, __LINE__);
    }

    else
    {
        statlen = write_udp(&mysock->epar, (char *)w_buffer);
        //if(statlen) fprintf(stderr,"\t%s: LINE %d no swap\n", __FUNCTION__, __LINE__);
    }

    if (statlen == -1) fprintf(stderr, "ERROR in %s/: SEND error\n", __FUNCTION__);
}

/*******************************************************************
*
* Description: This function reads from the specified udp port using
*              the net_util api.
*
* Function API:
*       returns integer containing the size of the buffer received
*               (-1=invalid, 0=do nothing, anything else=got data)
*       ENT_PARAM_S *epar - pointer to the net_util structure that
*                           that contains all the relevant socket
*                           information
*       int socket_id     - integer containing the socket id
*       unsigned short
*             *r_buffer   - pointer to the input buffer (read buffer)
*
*******************************************************************/
int mycomm_read(int *ptr, void *r_buffer)
{
    int statlen;
    struct sock_param *mysock = (struct sock_param *)ptr;

    if (global_swap_flag)
    {
        swap_byte((char*)r_buffer, global_r_swap, global_r_length, sizeof(int));
        statlen = peek_read_udp(&mysock->epar, (char *)global_r_swap);
        //if(statlen) fprintf(stderr,">>\t%s: LINE %d swapped\n", __FUNCTION__, __LINE__);
    }
    else
    {
        statlen = peek_read_udp(&mysock->epar, (char *)r_buffer);
        //if(statlen) fprintf(stderr,"\t%s: LINE %d no swap\n", __FUNCTION__, __LINE__);
    }

    if (statlen == -1) close_udp_socket(mysock->socket_id);

    return statlen;
}

/*******************************************************************
*
* Description: This function sets global comm flag (the flag's use
*              is determined by the app developer).
*
* Function API:
*             flag - copied into global comm flag
*
*******************************************************************/
void mycomm_set_flag(int *ptr, int flag)
{
    struct sock_param *mysock = (struct sock_param *)ptr;
    mysock->uflag = flag;
}

/*******************************************************************
*
* Description: This function gets global comm flag (the flag's use
*              is determined by the app developer).
*
* Function API:
*             returns integer containing the global comm flag
*
*******************************************************************/
int mycomm_get_flag(int *ptr)
{
    struct sock_param *mysock = (struct sock_param *)ptr;
    int tmp = mysock->uflag;
    mycomm_set_flag(ptr, 0);
    return(tmp);
}
