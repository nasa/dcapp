#include <cstdio>
#include "nodes.hh"

extern void SetNeedsRedraw(void);

extern appdata AppData;

void UpdateDisplay(void)
{
    AppData.toplevel->setCurrentPanel();

    AppData.DisplayLogic();

    SetNeedsRedraw();
}
