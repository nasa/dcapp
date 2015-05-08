#ifndef _MSG_HH_
#define _MSG_HH_

#include <stdio.h>

#ifdef DEBUG
#define _debugactive 1
#else
#define _debugactive 0
#endif

#define user_msg(...) { printf("dcapp: "); printf(__VA_ARGS__); printf("\n"); }
#define debug_msg(...) { if (_debugactive) { printf("dcapp: function = %s, file = %s, line = %d: ", __FUNCTION__, __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); }}
#define error_msg(...) { printf("dcapp ERROR: function = %s, file = %s, line = %d: ", __FUNCTION__, __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); }

#define debug_perror(a) { if (_debugactive) { perror(a); }}

#endif
