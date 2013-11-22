#ifndef _EDGE_RCS_H_
#define _EDGE_RCS_H_

extern int EDGE_rcs_init(char *, char *);
extern void EDGE_rcs_term(void);
extern int send_doug_command(const char *, char **, char **);

#endif
