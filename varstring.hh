#ifndef _VARSTRING_HH_
#define _VARSTRING_HH_

#include "types.hh"

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
                case FLOAT_TYPE:   asprintf(&tmp_str, format.c_str(), *(float *)(value)); break;
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

#endif
