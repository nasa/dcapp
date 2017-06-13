#ifndef _UTIL_COMM_HH_
#define _UTIL_COMM_HH_

#include <csignal>
#include <cctype>
#include "net_util.hh"

struct sock_param
{
    ENT_PARAM_S epar;
    int  socket_id;
    int  uflag;
};

extern int *mycomm_init(int type, const char *hostname, int port, int insize, int outsize, int swap, int timeout);
extern void mycomm_write(int *ptr, void *w_buffer);
extern int  mycomm_read(int *ptr, void *r_buffer);
extern void mycomm_close(int *ptr);
extern void mycomm_set_flag(int *ptr, int flag);
extern int  mycomm_get_flag(int *ptr);

#endif
