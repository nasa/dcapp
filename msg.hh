#ifndef _MSG_HH_
#define _MSG_HH_

#include <stdio.h>

#ifdef DEBUG
#define _debugactive 1
#else
#define _debugactive 0
#endif

extern void set_msg_label(const char *);
extern const char *get_msg_label(void);

#define user_msg(...) { printf("%s: ", get_msg_label()); printf(__VA_ARGS__); printf("\n"); }

#define debug_msg(...) { if (_debugactive) { printf("%s: function = %s, file = %s, line = %d: ", get_msg_label(), __FUNCTION__, __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); }}
#define error_msg(...) { printf("%s ERROR: function = %s, file = %s, line = %d: ", get_msg_label(), __FUNCTION__, __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); }

#define debug_perror(...) { if (_debugactive) { printf("%s: function = %s, file = %s, line = %d: ", get_msg_label(), __FUNCTION__, __FILE__, __LINE__); printf(__VA_ARGS__);  printf(": "); fflush(0); perror(0x0); }}
#define error_perror(...) { printf("%s ERROR: function = %s, file = %s, line = %d: ", get_msg_label(), __FUNCTION__, __FILE__, __LINE__); printf(__VA_ARGS__); printf(": "); fflush(0); perror(0x0); }

#endif
