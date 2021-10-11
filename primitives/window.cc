#include <string>
#include "constants.hh"
#include "values.hh"
#include "window.hh"
#include "RenderLib/RenderLib.hh"

dcWindow::dcWindow() : currentPanel(0x0)
{
    init_window();
    displayID = getConstantFromInteger(0);
}

void dcWindow::setActiveDisplay(const std::string &inval)
{
    if (!inval.empty()) displayID = getValue(inval);
}

void dcWindow::setCurrentPanel(void)
{
    if (!displayID) return;

    for (const auto &myobj : children)
    {
        dcPanel *mypanel = (dcPanel *)myobj; // TODO: fix this
        if (mypanel->checkID(displayID->getInteger()))
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
    processPreCalculations();
    if (currentPanel) currentPanel->draw();
    processPostCalculations();
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
