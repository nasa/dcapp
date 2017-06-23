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

void dcWindow::handleMousePress(float x, float y)
{
    if (currentPanel) currentPanel->handleMousePress(x, y);
}

void dcWindow::handleMouseRelease(void)
{
    // check all panels for release in case the active panel changed after the press event
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleMouseRelease();
    }
}

void dcWindow::handleBezelPress(int key)
{
    if (currentPanel) currentPanel->handleBezelPress(key);
}

void dcWindow::handleBezelRelease(int key)
{
    // check all panels for release in case the active panel changed after the press event
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleBezelRelease(key);
    }
}
