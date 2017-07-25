#include "loadUtils.hh"
#include "window.hh"

extern int *getIntegerPointer(const char *); // TODO: put in header file

dcWindow::dcWindow() : currentPanel(0x0)
{
    displayID = dcLoadConstant(0);
}

void dcWindow::setActiveDisplay(const char *inval)
{
    if (inval) displayID = getIntegerPointer(inval);
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

void dcWindow::draw(void)
{
    if (currentPanel) currentPanel->draw();
}

void dcWindow::handleKeyPress(char key)
{
    if (currentPanel) currentPanel->handleKeyPress(key);
}

void dcWindow::handleKeyRelease(char key)
{
    if (currentPanel) currentPanel->handleKeyRelease(key);
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

void dcWindow::updateStreams(unsigned passcount)
{
    if (currentPanel) currentPanel->updateStreams(passcount);
}
