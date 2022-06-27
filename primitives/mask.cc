#include "app_data.hh"
#include "mask.hh"

extern appdata AppData;

dcMask::dcMask(dcParent *myparent)
{
    myparent->addChild(this);

    stencilList = new dcParent;
    stencilList->setParent(this);
    projectionList = new dcParent;
    projectionList->setParent(this);
}

dcMask::~dcMask()
{
    return;
}

void dcMask::processPreCalculations()
{
    projectionList->processPreCalculations();
}

void dcMask::processPostCalculations()
{
    projectionList->processPostCalculations();
}

void dcMask::draw(void)
{
    stencil_begin();        // enable stencil, clear existing buffer
    stencil_init_dest();    // setup stencil test to write 1's into destination area 
    stencilList->draw();

    stencil_init_proj();    // set stencil to only keep fragments with reference != 0
    projectionList->draw();

    stencil_end();
}
