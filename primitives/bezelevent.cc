#include <string>
#include "basicutils/stringutils.hh"
#include "bezelevent.hh"

dcBezelEvent::dcBezelEvent(dcParent *myparent) : mykey(0), selected(false)
{
    coreConstructor(myparent);
}

dcBezelEvent::dcBezelEvent(dcParent *myparent, const std::string &key) : selected(false)
{
    coreConstructor(myparent);
    if (!key.empty()) mykey = StringToInteger(key);
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

void dcBezelEvent::setKey(const std::string &key)
{
    mykey = StringToInteger(key);
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

void dcBezelEvent::processPreCalculations(void) 
{
    PressList->processPreCalculations();
    ReleaseList->processPreCalculations();
}

void dcBezelEvent::processPostCalculations(void) 
{
    PressList->processPostCalculations();
    ReleaseList->processPostCalculations();
}
