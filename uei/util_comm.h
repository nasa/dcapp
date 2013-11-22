#ifndef _UTIL_COMM_H_
#define _UTIL_COMM_H_

#include <signal.h>
#include <ctype.h>

#include "net_util.h"


struct sock_param
{
  ENT_PARAM_S epar ;
  int  socket_id ;
  int  uflag ;
} ;


#ifdef __cplusplus
extern "C" {
#endif
extern int *mycomm_init( int type, char *hostname, int port, int insize, int outsize, int swap, int timeout ) ;
extern void mycomm_write( int *ptr, void *w_buffer ) ;
extern int  mycomm_read( int *ptr, void *r_buffer ) ;
extern void mycomm_close( int *ptr ) ;

extern int  mycomm_is_open( char *uid ) ;
extern void mycomm_set_flag( int *ptr, int flag ) ;
extern int  mycomm_get_flag( int *ptr ) ;
#ifdef __cplusplus
}
#endif

#endif
