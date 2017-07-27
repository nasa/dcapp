#include "string_utils.hh"
#include "bezelevent.hh"

dcBezelEvent::dcBezelEvent(dcParent *myparent) : mykey(0), selected(false)
{
    coreConstructor(myparent);
}

dcBezelEvent::dcBezelEvent(dcParent *myparent, const char *key) : selected(false)
{
    coreConstructor(myparent);
    setKey(key);
}

dcBezelEvent::~dcBezelEvent()
{
    delete PressList;
    delete ReleaseList;
}

void dcBezelEvent::coreConstructor(dcParent *myparent)
{
    myparent->addChild(this);
    PressList = new dcParent;
    ReleaseList = new dcParent;
    PressList->setParent(this);
    ReleaseList->setParent(this);
}

void dcBezelEvent::setKey(const char *key)
{
    mykey = StrToInt(key, 0);
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
