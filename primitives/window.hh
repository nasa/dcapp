#ifndef _WINDOW_HH_
#define _WINDOW_HH_

#include "parent.hh"
#include "panel.hh"

class dcWindow : public dcParent
{
    public:
        dcWindow(int *);
        void setCurrentPanel(void);
        void handleKeyboard(char);
        void handleBezelPress(int);
        void handleBezelRelease(int);

    private:
        int *displayID;
        dcPanel *currentPanel;
};

#endif
