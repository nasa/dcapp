#ifndef _KOLOR_HH_
#define _KOLOR_HH_

#include <string>
#include <vector>
#include <cctype>
#include "constants.hh"
#include "values.hh"

class Kolor
{
    public:
        Kolor()
        {
            R = getConstantFromDecimal(0.0);
            G = getConstantFromDecimal(0.0);
            B = getConstantFromDecimal(0.0);
            A = getConstantFromDecimal(1.0);
        }
        void set(double r, double g, double b)
        {
            R = getConstantFromDecimal(r);
            G = getConstantFromDecimal(g);
            B = getConstantFromDecimal(b);
            A = getConstantFromDecimal(1.0);
        }
        void set(double r, double g, double b, double a)
        {
            R = getConstantFromDecimal(r);
            G = getConstantFromDecimal(g);
            B = getConstantFromDecimal(b);
            A = getConstantFromDecimal(a);
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

            if (count > 0) R = getValue(mylist[0].c_str());
            else R = getConstantFromDecimal(0.0);
            if (count > 1) G = getValue(mylist[1].c_str());
            else G = getConstantFromDecimal(0.0);
            if (count > 2) B = getValue(mylist[2].c_str());
            else B = getConstantFromDecimal(0.0);
            if (count > 3) A = getValue(mylist[3].c_str());
            else A = getConstantFromDecimal(1.0);
        }

        Value *R;
        Value *G;
        Value *B;
        Value *A;
};

#endif
