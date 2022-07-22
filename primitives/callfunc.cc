#include "xml_data.hh"
#include "app_data.hh"
#include "variables.hh"
#include "values.hh"
#include "functions.hh"
#include "callfunc.hh"

extern appdata AppData;


dcCallFunc::dcCallFunc(dcParent *myparent, const std::string &name)
{
    myparent->addChild(this);

    if (!name.empty())
    {
        this->funcname = getValue(name);
    }
    else
    {
        printf("dcCallFunc: missing function name\n");
    }
}

void dcCallFunc::draw(void)
{
    Function* f = getFunction(this->funcname->getString());
    f->runFunction();
}