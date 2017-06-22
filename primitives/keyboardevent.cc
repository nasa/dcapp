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
        UpdateDisplay(); // TODO: UpdateDisplay should only be called once AFTER all keypress events are processed
        this->ReleaseList->updateData();
        UpdateDisplay(); // TODO: UpdateDisplay should only be called once AFTER all keyrelease events are processed
    }
}
