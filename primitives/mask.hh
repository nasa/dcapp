#ifndef _MASK_HH_
#define _MASK_HH_

#include "object.hh"
#include "parent.hh"

#include <vector>

class dcMask : public dcObject
{
    public:
        dcMask(dcParent *);
        virtual ~dcMask();

        void processPreCalculations();
        void processPostCalculations();
        void draw(void);

        enum MaskStencilType {
            MASK_STENCIL_DEST_ADD,
            MASK_STENCIL_DEST_SUB,
            MASK_STENCIL_PROJ
        };

        typedef struct _stencilList {
            MaskStencilType type;
            dcParent* stencils;
        } StencilList;

        std::vector<StencilList> stencils;
};

#endif
