#ifndef _XML_DATA_HH_
#define _XML_DATA_HH_

#include <string>

class xmldata : public std::string
{
    public:
        xmldata();
        xmldata(std::string);
        xmldata(const char *);
        ~xmldata() {};

        void reset(void);
        bool defined(void) const { return isDefined; };

        bool isDefined;
};

#endif
