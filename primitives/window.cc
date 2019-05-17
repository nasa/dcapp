#include "loadUtils.hh"
#include "varlist.hh"
#include "window.hh"
#include "opengl_draw.hh"

dcWindow::dcWindow() : currentPanel(0x0)
{
    init_window();
    displayID = dcLoadConstant(0);
}

void dcWindow::setActiveDisplay(const char *inval)
{
    if (inval) displayID = getIntegerPointer(inval);
}

void dcWindow::setCurrentPanel(void)
{
    if (!displayID) return;

    for (const auto &myobj : children)
    {
        dcPanel *mypanel = (dcPanel *)myobj; // TODO: fix this
        if (mypanel->checkID(*displayID))
        {
            currentPanel = mypanel;
            return;
        }
    }
}

void dcWindow::reshape(double w, double h)
{
    reshape_window((int)w, (int)h);
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

void dcWindow::handleMousePress(double x, double y)
{
    if (currentPanel) currentPanel->handleMousePress(x, y);
}

#if 0
void dcWindow::handleMouseRelease(void)
{
    // check all panels for release in case the active panel changed after the press event
    for (const auto &myobj : children) myobj->handleMouseRelease();
}
#endif

void dcWindow::handleBezelPress(int key)
{
    if (currentPanel) currentPanel->handleBezelPress(key);
}

#if 0
void dcWindow::handleBezelRelease(int key)
{
    // check all panels for release in case the active panel changed after the press event
    for (const auto &myobj : children) myobj->handleBezelRelease(key);
}
#endif

void dcWindow::handleMouseMotion(double x, double y)
{
    if (currentPanel) currentPanel->handleMouseMotion(x, y);
}

void dcWindow::updateStreams(unsigned passcount)
{
    if (currentPanel) currentPanel->updateStreams(passcount);
}
