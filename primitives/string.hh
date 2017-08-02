#ifndef _STRING_HH_
#define _STRING_HH_

#include <string>
#include <vector>
#include "fontlib/fontlib.hh"
#include "kolor.hh"
#include "types.hh"
#include "geometric.hh"
#include "parent.hh"

#include <iostream>

class VarString
{
    public:
        VarString(int a, void *b, const char *c)
        {
            if (a == UNDEFINED_TYPE) datatype = STRING_TYPE;
            else datatype = a;
            value = b;
            if (c) format = c;
            else
            {
                switch (datatype)
                {
                    case FLOAT_TYPE:   format = "%g"; break;
                    case INTEGER_TYPE: format = "%d";   break;
                    case STRING_TYPE:  format = "%s";   break;
                }
            }
        };
        std::string get(void)
        {
            char *tmp_str = 0x0;
            switch (datatype)
            {
                case FLOAT_TYPE: asprintf(&tmp_str, format.c_str(), *(float *)(value));   break;
                case INTEGER_TYPE: asprintf(&tmp_str, format.c_str(), *(int *)(value));   break;
                case STRING_TYPE:  asprintf(&tmp_str, format.c_str(), (char *)value);     break;
            }
            std::string ret_str = tmp_str;
            free(tmp_str);
            return ret_str;
        };
    private:
        int datatype;
        void *value;
        std::string format;
};

class dcString : public dcGeometric
{
    public:
        dcString(dcParent *);
        void setColor(const char *);
        void setBackgroundColor(const char *);
        void setFont(const char *, const char *, const char *, const char *);
        void setShadowOffset(const char *);
        void setString(std::string);
        void draw(void);

    private:
        size_t parse_var(std::string);
        unsigned count_lines(std::string);

        bool background;
        Kolor color;
        Kolor bgcolor;
        std::vector<VarString *> vstring;
        std::vector<std::string> filler;
        flFont *fontID;
        float *fontSize;
        float *shadowOffset;
        flMonoOption forcemono;
};

#endif