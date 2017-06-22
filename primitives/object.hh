#ifndef _OBJECT_HH_
#define _OBJECT_HH_

class dcObject
{
    public:
        dcObject() : parent(0x0) { };
        virtual ~dcObject() { };

        virtual void draw(void) { };
        virtual void handleKeyboard(char) { };
        virtual void handleMousePress(float, float) { };
        virtual void handleMouseRelease(void) { };
        virtual void handleBezelPress(int) { };
        virtual void handleBezelRelease(int) { };
        virtual void updateData(void) { };
        virtual void processAnimation(void) { };
        virtual void setParent(dcObject *parentid) { parent = parentid; };
        virtual dcObject *getParent(void) { return parent; };
        virtual bool checkID(int) { return false; }; // TODO: this probably doesn't fit here

    private:
        dcObject *parent;
};

#endif
