#include "bezelevent.hh"

// TODO: include file for this
extern void UpdateDisplay(void);

dcBezelEvent::dcBezelEvent(int key) : selected(false)
{
    mykey = key;
    PressList = new dcParent;
    ReleaseList = new dcParent;
}

dcBezelEvent::~dcBezelEvent()
{
    delete PressList;
    delete ReleaseList;
}

void dcBezelEvent::handleBezelPress(int key)
{
    if (mykey == key)
    {
        this->selected = true;
        this->PressList->updateData();
        UpdateDisplay(); // TODO: UpdateDisplay should only be called once AFTER all events are processed
    }
    else this->selected = false; // is this really necessary?
}

void dcBezelEvent::handleBezelRelease(int key)
{
    if (this->selected && mykey == key)
    {
        this->selected = false;
        this->ReleaseList->updateData();
        UpdateDisplay(); // TODO: UpdateDisplay should only be called once AFTER all events are processed
    }
}
