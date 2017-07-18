#include "nodes.hh"

extern appdata AppData;

void HandleKeyboard(unsigned char key)
{
    AppData.toplevel->handleKeyboard(key);
}
