#ifndef _PANEL_HH_
#define _PANEL_HH_

#include "parent.hh"

class dcPanel : public dcParent
{
    public:
        dcPanel(int);
        bool checkID(int);
    
    private:
        int displayID;
};

#endif
