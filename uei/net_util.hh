#ifndef _NET_UTIL_HH_
#define _NET_UTIL_HH_

/***********************************************************************
* PURPOSE:
*   (UDP Interface Data Structure)
* REFERENCES:
*   ((None))
* ASSUMPTIONS AND LIMITATIONS:
*   ((C Source))
* PROGRAMMERS:
*   (((Hadi Tjandrasa)  (LMSO)    (07/02)  (SCI Replacement))
*    ((Hadi Tjandrasa)  (LMSO)    (03/03)  (ECO 8678 Replace SCRAMNet with UDP/IP))
*    ((Hadi Tjandrasa)  (LMSO)    (05/03)  (ECO 8724 Fwd SCI Upgrade))
*    ((Hadi Tjandrasa)  (LMSO)    (02/04)  (SES Port to Linux)))
*
***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#ifndef VxWorks
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
//#include <stropts.h>
#else  /* VxWorks */
#include "target_header.h"
#endif


#define ENT_NOSYNCH            0
#define ENT_CLIENT             1
#define ENT_SERVER             2
#define ENT_CLI_REQ            0x01000000
#define ENT_SRV_ACK            0x02000000
#define ENT_CLI_ACK            0x03000000
#define ENT_FATAL_ERROR        -99

#define ENT_SRV_MIN_PORT       29999  /* Smallest # for hard-coded server port */
#define ENT_BUFLEN_BYTES       61440  /* Max UDP buffer size - SGI default */


/* Network socket parameters */
typedef struct
{
  int     swap_byte_flag;          /* -- Swap byte order */
  int     nonblock_opt_flag;       /* -- Nonblocking I/O flag */
  int     domain;                  /* -- Domain type */
  int     server_port;             /* -- Server port # */
  int     myport;                  /* -- This agent port # */
  int     base_port;               /* -- Base port # */
  int     delta_port;              /* -- Delta/offset port # */
  int     insize_netbuf;           /* -- Network input size */
  int     outsize_netbuf;          /* -- Network output size */
  int     in_pack_len;             /* -- Inout packet size */
  int     out_pack_len;            /* -- Output packet size */
  int     socket;                  /* -- socket descriptor */
  char    server_ip[16];           /* -- Server IP address */
  char    proto[4];                /* -- Protocol */
  int     timeout_ms;              /* -- Timeout read in millisec */
  int     tv_sec;                  /* -- Second timeout */
  int     tv_usec;                 /* -- Micro second timeout */
  struct sockaddr_in  yo_sckaddr;  /* -- Other agent socket address */
  struct sockaddr_in  my_sckaddr;  /* -- My socket address */
} ENT_PARAM_S;


/* IP address conversion */
typedef  union
{
  unsigned char   byte_addr[4];
  unsigned long   int_addr;
} ENT_IP_ADDR_S;


/* PROTOTYPE */
extern int  clear_param( ENT_PARAM_S *);
extern int  init_ent_default_param( int, ENT_PARAM_S*, int, int );
extern int  create_server_port( ENT_PARAM_S*);
extern int  comm_is_open( ENT_PARAM_S*);
extern int  set_socket_nonblocking( ENT_PARAM_S*);
extern int  set_socket_blocking( ENT_PARAM_S*);
extern int  create_udp_socket( int, ENT_PARAM_S* );
extern int  peek_socket_ready( int, int, int );
extern int  data_ready( int );
extern int  probe_server_synch( ENT_PARAM_S*);
extern int  wait_client_synch( ENT_PARAM_S*);
extern int  peek_read_udp( ENT_PARAM_S*, void*);
extern int  read_udp( ENT_PARAM_S*, void*);
extern int  write_udp( ENT_PARAM_S*, void*);
extern int  close_udp_socket( int );
extern int  swap_byte( char*, char*, int, int);
extern void nap_delay( int );

#endif
