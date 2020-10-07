#ifndef _VARIABLES_HH_
#define _VARIABLES_HH_

#include <string>
#include "values.hh"

class Variable : public Value
{
    public:
        Variable();
        virtual ~Variable();

        bool operator == (const Variable &);
        bool operator != (const Variable &);

        void setToString(const std::string &);
        void setToDecimal(double);
        void setToInteger(int);
        void setToBoolean(bool);
        void setToValue(Value &);

        void incrementByValue(Value &);
        void decrementByValue(Value &);
        void applyMinimumByValue(Value &);
        void applyMaximumByValue(Value &);

        unsigned compareToValue(Value &);

        double getDecimal(void);
        int getInteger(void);
        std::string getString(std::string = "");
        bool getBoolean(void);

        void setType(int);
        void setType(const std::string &);
        void setAttributes(Variable &);

        void *getPointer(void);

        bool isDecimal(void);
        bool isInteger(void);
        bool isString(void);

        bool isVariable(void);

    private:
        int type;
};

extern void registerVariable(const std::string &, const std::string &, const std::string &);
extern Variable *getVariable(const std::string &);
extern void *get_pointer(const char *);
extern std::string create_virtual_variable(const char *, const char *);

#endif
