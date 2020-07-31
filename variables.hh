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

        unsigned compareToValue(Value &);

        double getDecimal(void);
        int getInteger(void);
        std::string getString(std::string = "");
        bool getBoolean(void);

        void setType(int);
        void setType(std::string &);
        void setAttributes(Variable &);

        void *getPointer(void);

        bool isDecimal(void);
        bool isInteger(void);
        bool isString(void);

        bool isVariable(void);

    private:
        int type;
};

extern void registerVariable(std::string, std::string, std::string);
extern Variable *getVariable(const char *);
extern Variable *getVariableSSTR(std::string &);
extern void *get_pointer(const char *);
extern char *create_virtual_variable(const char *, const char *);

#endif
