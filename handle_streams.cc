#include "nodes.hh"

extern appdata AppData;

void UpdateStreams(void)
{
    static unsigned passnum = 0;
    passnum++;
    AppData.toplevel->updateStreams(passnum);
}
