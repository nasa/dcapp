#ifndef _VALUEDATA_HH_
#define _VALUEDATA_HH_

#include <string>

class ValueData
{
    public:
        ValueData();
        virtual ~ValueData();

        bool operator == (const ValueData &);
        bool operator != (const ValueData &);

        void setType(int);
        void setType(const char *);
        void setValue(const char *);
        void setValue(const char *, unsigned);
        void setValue(const ValueData &);
        void setValue(double);
        void setValue(int);
        void setValue(std::string);
        void setBoolean(bool);
        void makeGeneric(void);

        int getType(void);
        bool getBoolean(void);
        double getDecimal(void);
        int getInteger(void);
        std::string getString(void);
        void *getPointer(void);

        bool isDecimal(void);
        bool isInteger(void);
        bool isString(void);
        
        double decval;
        int intval;
        std::string strval;

    private:
        int type;
};

#endif
