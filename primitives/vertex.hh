#ifndef _VERTEX_HH_
#define _VERTEX_HH_

#include <string>
#include "values.hh"
#include "object.hh"
#include "parent.hh"

class dcVertex : public dcObject
{
    public:
        dcVertex(dcParent *);
        void setPosition(const std::string &, const std::string &);
        void setOrigin(const std::string &, const std::string &);
        void draw(void);

    private:
        Value *x;
        Value *y;
        Value *containerw;
        Value *containerh;
        unsigned originx;
        unsigned originy;
};

#endif
