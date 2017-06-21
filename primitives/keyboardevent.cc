#include "keyboardevent.hh"

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
printf("found a key press item = %p, panel = %p, key = %c\n", this, this->getParent(), key);
//        actionList.push_back(current->object.ke.PressList);
//        actionList.push_back(current->object.ke.ReleaseList);
    }
}
