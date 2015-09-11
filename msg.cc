#include <string.h>

static char *msg_top_label = 0x0;

void set_msg_label(const char *label)
{
    msg_top_label = strdup(label);
}

const char *get_msg_label(void)
{
    return msg_top_label;
}
