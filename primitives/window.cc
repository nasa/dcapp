#include "window.hh"

dcWindow::dcWindow(int *dispid) : currentPanel(0x0)
{
    displayID = dispid;
}

void dcWindow::setCurrentPanel(void)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        if ((*myobj)->checkID(*displayID))
        {
            currentPanel = (dcPanel *)(*myobj);
            return;
        }
    }
}

void dcWindow::handleKeyboard(char key)
{
    if (currentPanel) currentPanel->handleKeyboard(key);
}
