#ifndef _KOLOR_HH_
#define _KOLOR_HH_

#include <string>
#include <vector>
#include <cctype>
#include "valuedata.hh"
#include "constants.hh"
#include "varlist.hh"
#include "string_utils.hh"

class Kolor
{
    public:
        Kolor()
        {
            R = getConstantValue(0.0);
            G = getConstantValue(0.0);
            B = getConstantValue(0.0);
            A = getConstantValue(1.0);
        }
        void set(double r, double g, double b)
        {
            R = getConstantValue(r);
            G = getConstantValue(g);
            B = getConstantValue(b);
            A = getConstantValue(1.0);
        }
        void set(double r, double g, double b, double a)
        {
            R = getConstantValue(r);
            G = getConstantValue(g);
            B = getConstantValue(b);
            A = getConstantValue(a);
        }
        void set(std::string mystring)
        {
            size_t pos = 0, startpos;
            bool started = false;
            std::vector<std::string> mylist;
            size_t string_len = mystring.size(), count;

            for (pos = 0; pos < string_len; pos++)
            {
                if (!isspace(mystring[pos]))
                {
                    if (!started)
                    {
                        started = true;
                        startpos = pos;
                    }
                }
                else if (started)
                {
                    started = false;
                    mylist.push_back(mystring.substr(startpos, pos-startpos));
                }
            }
            if (started) mylist.push_back(mystring.substr(startpos));

            count = mylist.size();

            if (count > 0) R = color_element(mylist[0]);
            else R = getConstantValue(0.0);
            if (count > 1) G = color_element(mylist[1]);
            else G = getConstantValue(0.0);
            if (count > 2) B = color_element(mylist[2]);
            else B = getConstantValue(0.0);
            if (count > 3) A = color_element(mylist[3]);
            else A = getConstantValue(1.0);
        }

        ValueData *R;
        ValueData *G;
        ValueData *B;
        ValueData *A;

    private:
        ValueData *color_element(std::string &strval)
        {
            if (check_dynamic_element(strval.c_str())) return getVariableValue(strval.c_str());
            else return getConstantValue(StringToDecimal(strval));
        }
};

#endif
