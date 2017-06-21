#ifndef _PARENT_HH_
#define _PARENT_HH_

#include <list>
#include "object.hh"

class dcParent : public dcObject
{
    public:
        dcParent() { };
        virtual ~dcParent();

        void handleKeyboard(char);
        void updateData(void);
        void processAnimation(void);

        void addChild(dcObject *);

    protected:
        std::list<dcObject *> children;
};

#endif
