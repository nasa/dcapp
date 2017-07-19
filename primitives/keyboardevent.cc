#include "keyboardevent.hh"

// TODO: include file for this
extern void UpdateDisplay(void);

dcKeyboardEvent::dcKeyboardEvent(char key)
{
    mykey = key;
    PressList = new dcParent;
    ReleaseList = new dcParent;
}

dcKeyboardEvent::~dcKeyboardEvent()
{
    delete PressList;
    delete ReleaseList;
}

void dcKeyboardEvent::handleKeyPress(char key)
{
    if (mykey == key) this->PressList->handleEvent();
}

void dcKeyboardEvent::handleKeyRelease(char key)
{
    if (mykey == key) this->ReleaseList->handleEvent();
}
