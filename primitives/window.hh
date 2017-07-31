#ifndef _WINDOW_HH_
#define _WINDOW_HH_

#include "parent.hh"
#include "panel.hh"

class dcWindow : public dcParent
{
    public:
        dcWindow();
        void setActiveDisplay(const char *);
        void setCurrentPanel(void);
        void draw(void);
        void handleKeyPress(char);
        void handleKeyRelease(char);
        void handleMousePress(float, float);
        void handleMouseRelease(void);
        void handleBezelPress(int);
        void handleBezelRelease(int);
        void updateStreams(unsigned);

    private:
        int *displayID;
        dcPanel *currentPanel;
};

#endif
