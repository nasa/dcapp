#ifndef _CONSTANTS_HH_
#define _CONSTANTS_HH_

#include <string>
#include "values.hh"

class Constant : public Value
{
    public:
        Constant();
        virtual ~Constant();

        bool operator == (const Constant &);
        bool operator != (const Constant &);

        void setToDecimal(double);
        void setToInteger(int);
        void setToCharstr(const char *);
        void setToBoolean(bool);

        unsigned compareToValue(Value &);

        double getDecimal(void);
        int getInteger(void);
        std::string getString(std::string = "");
        bool getBoolean(void);

        bool isConstant(void);

    private:
        bool boolval;
};

extern Constant *getConstantFromDecimal(double);
extern Constant *getConstantFromInteger(int);
extern Constant *getConstantFromCharstr(const char *);
extern Constant *getConstantFromBoolean(bool);

#endif
