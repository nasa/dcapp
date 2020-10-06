#ifndef _VALUES_HH_
#define _VALUES_HH_

#include <string>

#define UNDEFINED_TYPE 0
#define STRING_TYPE    1
#define DECIMAL_TYPE   2
#define INTEGER_TYPE   3

enum { isEqual, isGreaterThan, isLessThan };

class Value
{
    public:
        Value();
        virtual ~Value();

        virtual unsigned compareToValue(Value &) { return isEqual; };

        virtual double getDecimal(void) { return 0.0; };
        virtual int getInteger(void) { return 0; };
        virtual std::string getString(std::string = "") { return ""; };
        virtual bool getBoolean(void) { return false; };

        virtual bool isConstant(void) { return false; };
        virtual bool isVariable(void) { return false; };

    protected:
        double decval;
        int intval;
        std::string strval;
};

extern bool check_dynamic_element(const std::string &);
extern Value *getValue(const std::string &);

#endif
