#ifndef _TIDY_HH_
#define _TIDY_HH_

#define TIDY(a) do { if (a) { free(a); a=0x0; } } while(0)

#endif
