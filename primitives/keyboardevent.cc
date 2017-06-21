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

void dcKeyboardEvent::handleKeyboard(char key)
{
    if (mykey == key)
    {
        this->PressList->updateData();
        UpdateDisplay();
        this->ReleaseList->updateData();
        UpdateDisplay();
    }
}
