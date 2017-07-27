#include "bezelevent.hh"

dcBezelEvent::dcBezelEvent(int key) : selected(false)
{
    mykey = key;
    PressList = new dcParent;
    ReleaseList = new dcParent;
    PressList->setParent(this);
    ReleaseList->setParent(this);
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
        this->PressList->handleEvent();
    }
    else this->selected = false; // is this really necessary?
}

void dcBezelEvent::handleBezelRelease(int key)
{
    if (this->selected && mykey == key)
    {
        this->selected = false;
        this->ReleaseList->handleEvent();
    }
}
