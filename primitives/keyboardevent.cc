#include <string>
#include "basicutils/stringutils.hh"
#include "keyboardevent.hh"

dcKeyboardEvent::dcKeyboardEvent(dcParent *myparent) : mykey(0)
{
    coreConstructor(myparent);
}

dcKeyboardEvent::dcKeyboardEvent(dcParent *myparent, const std::string &key)
{
    coreConstructor(myparent);
    if (!key.empty()) mykey = key[0];
}

dcKeyboardEvent::dcKeyboardEvent(dcParent *myparent, const std::string &key, const std::string &keyascii)
{
    coreConstructor(myparent);
    if (!key.empty()) mykey = key[0];
    else if (!keyascii.empty()) mykey = StringToInteger(keyascii);
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

void dcKeyboardEvent::setKey(const std::string &key)
{
    if (!key.empty()) mykey = key[0];
}

void dcKeyboardEvent::setKeyAscii(const std::string &key)
{
    if (!key.empty()) mykey = StringToInteger(key);
}

void dcKeyboardEvent::handleKeyPress(char key)
{
    if (mykey == key) this->PressList->handleEvent();
}

void dcKeyboardEvent::handleKeyRelease(char key)
{
    if (mykey == key) this->ReleaseList->handleEvent();
}
