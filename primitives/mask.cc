#include "app_data.hh"
#include "mask.hh"

extern appdata AppData;

dcMask::dcMask(dcParent *myparent)
{
    myparent->addChild(this);
}

dcMask::~dcMask()
{
    return;
}

void dcMask::processPreCalculations()
{
    for (StencilList stencil : stencils) {
        stencil.stencils->processPreCalculations();
    }
}

void dcMask::processPostCalculations()
{
    for (StencilList stencil : stencils) {
        stencil.stencils->processPostCalculations();
    }
}

void dcMask::draw(void)
{
    stencil_begin();        // enable stencil, clear existing buffer
    for (StencilList stencil : stencils) {
        switch(stencil.type) {
            case MASK_STENCIL_DEST_ADD:
                stencil_init_dest_add();
                break;
            case MASK_STENCIL_DEST_SUB:
                stencil_init_dest_sub();
                break;
            case MASK_STENCIL_PROJ:
                stencil_init_proj();
                break;
        }
        stencil.stencils->draw();
    }
    stencil_end();
}
