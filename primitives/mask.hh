#ifndef _MASK_HH_
#define _MASK_HH_

#include "object.hh"
#include "parent.hh"

class dcMask : public dcObject
{
    public:
        dcMask(dcParent *);
        virtual ~dcMask();

        void processPreCalculations();
        void processPostCalculations();
        void draw(void);

        dcParent *stencilList;
        dcParent *projectionList;
};

#endif