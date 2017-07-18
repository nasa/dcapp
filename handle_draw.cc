#include "nodes.hh"

extern void SwapBuffers(void);

extern appdata AppData;

void Draw(void)
{
    AppData.toplevel->draw();
    SwapBuffers();
    AppData.last_update->restart();
}
