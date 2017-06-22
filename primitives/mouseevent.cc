#include "mouseevent.hh"

// TODO: include file for this
extern void UpdateDisplay(void);

dcMouseEvent::dcMouseEvent(int key) : selected(false)
{
    mykey = key;
    PressList = new dcParent;
    ReleaseList = new dcParent;
}

dcMouseEvent::~dcMouseEvent()
{
    delete PressList;
    delete ReleaseList;
}

void dcMouseEvent::handleMousePress(int key)
{
    if (mykey == key)
    {
        this->selected = true;
        this->PressList->updateData();
        UpdateDisplay(); // TODO: UpdateDisplay should only be called once AFTER all events are processed
    }
    else this->selected = false; // is this really necessary?
}

void dcMouseEvent::handleMouseRelease(int key)
{
    if (this->selected && mykey == key)
    {
        this->selected = false;
        this->ReleaseList->updateData();
        UpdateDisplay(); // TODO: UpdateDisplay should only be called once AFTER all events are processed
    }
}
