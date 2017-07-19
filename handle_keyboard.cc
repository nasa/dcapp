#include "nodes.hh"

extern appdata AppData;

extern void ProcessEvents(void);

void HandleKeyboard(unsigned char key)
{
    AppData.toplevel->handleKeyPress(key);
    ProcessEvents();
    AppData.toplevel->handleKeyRelease(key);
    ProcessEvents();
}
