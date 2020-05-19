#ifndef _VALUEDATA_HH_
#define _VALUEDATA_HH_

#include <string>

#define UNDEFINED_TYPE 0
#define STRING_TYPE    1
#define DECIMAL_TYPE   2
#define INTEGER_TYPE   3

class Value
{
    public:
        Value();
        virtual ~Value();

        virtual double getDecimal(void) { return 0.0; };
        virtual int getInteger(void) { return 0; };
        virtual std::string getString(std::string = "") { return ""; };
        virtual bool getBoolean(void) { return false; };

// this shouldn't be here, it's a hack
virtual int getType(void) { return 0; };

    protected:
        double decval;
        int intval;
        std::string strval;
};

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

        double getDecimal(void);
        int getInteger(void);
        std::string getString(std::string = "");
        bool getBoolean(void);

    private:
        bool boolval;
};

class Variable : public Value
{
    public:
        Variable();
        virtual ~Variable();

        bool operator == (const Variable &);
        bool operator != (const Variable &);

        void setToCharstr(const char *);
        void setToString(std::string &);
        void setToDecimal(double);
        void setToInteger(int);
        void setToBoolean(bool);
        void setToValue(Value &);

        void incrementByValue(Value &);
        void decrementByValue(Value &);
        void applyMinimumByValue(Value &);
        void applyMaximumByValue(Value &);

        double getDecimal(void);
        int getInteger(void);
        std::string getString(std::string = "");
        bool getBoolean(void);

        void setType(int);
        void setType(const char *);
        void setAttributes(Variable &);

        int getType(void);
        void *getPointer(void);

        bool isDecimal(void);
        bool isInteger(void);
        bool isString(void);

    private:
        int type;
};

#endif
