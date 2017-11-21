#include "string_utils.hh"
#include "keyboardevent.hh"

dcKeyboardEvent::dcKeyboardEvent(dcParent *myparent) : mykey(0)
{
    coreConstructor(myparent);
}

dcKeyboardEvent::dcKeyboardEvent(dcParent *myparent, const char *key)
{
    coreConstructor(myparent);
    setKey(key);
}

dcKeyboardEvent::dcKeyboardEvent(dcParent *myparent, const char *key, const char *keyascii)
{
    coreConstructor(myparent);
    if (key) setKey(key);
    else setKeyAscii(keyascii);
}

dcKeyboardEvent::~dcKeyboardEvent()
{
    delete PressList;
    delete ReleaseList;
}

void dcKeyboardEvent::coreConstructor(dcParent *myparent)
{
    myparent->addChild(this);
    PressList = new dcParent;
    ReleaseList = new dcParent;
    PressList->setParent(this);
    ReleaseList->setParent(this);
}

void dcKeyboardEvent::setKey(const char *key)
{
    if (key) mykey = key[0];
}

void dcKeyboardEvent::setKeyAscii(const char *key)
{
    if (key) mykey = StringToInteger(key, 0);
}

void dcKeyboardEvent::handleKeyPress(char key)
{
    if (mykey == key) this->PressList->handleEvent();
}

void dcKeyboardEvent::handleKeyRelease(char key)
{
    if (mykey == key) this->ReleaseList->handleEvent();
}
