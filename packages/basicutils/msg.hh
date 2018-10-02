#ifndef _MSG_HH_
#define _MSG_HH_

#include <iostream>
#include <sstream>

class Message
{
    public:
        static void setLabel(const std::string in) { label = in; };
        static std::string getLabel(void) { return label; };
        static void enableDebugging(void) { debugFlag = true; };
        static bool debuggingEnabled(void) { return debugFlag; };
    private:
        static std::string label;
        static bool debugFlag;
};

#define user_msg(a) \
do { \
    std::stringstream _MyTmpStr; \
    _MyTmpStr << Message::getLabel().c_str() << ": " << a; \
    std::cout << _MyTmpStr.str() << std::endl; \
} while(0)
#define debug_msg(a) \
do { \
    if (Message::debuggingEnabled()) \
    { \
        std::stringstream _MyTmpStr; \
        _MyTmpStr << Message::getLabel().c_str() << ": function=" << __FUNCTION__ << ", file=" << __FILE__ << ", line=" << __LINE__ << ": " << a; \
        std::cout << _MyTmpStr.str() << std::endl; \
    } \
} while(0)
#define warning_msg(a) \
do { \
    std::stringstream _MyTmpStr; \
    _MyTmpStr << Message::getLabel().c_str() << " WARNING: function=" << __FUNCTION__ << ", file=" << __FILE__ << ", line=" << __LINE__ << ": " << a; \
    std::cout << _MyTmpStr.str() << std::endl; \
} while(0)
#define error_msg(a) \
do { \
    std::stringstream _MyTmpStr; \
    _MyTmpStr << Message::getLabel().c_str() << " ERROR: function=" << __FUNCTION__ << ", file=" << __FILE__ << ", line=" << __LINE__ << ": " << a; \
    std::cout << _MyTmpStr.str() << std::endl; \
} while(0)

#endif
