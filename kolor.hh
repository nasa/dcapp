#ifndef _KOLOR_HH_
#define _KOLOR_HH_

#include <string>
#include <vector>
#include <cctype>
#include "valuedata.hh"
#include "constants.hh"
#include "varlist.hh"

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

            if (count > 0) R = getValueData(mylist[0].c_str());
            else R = getConstantValue(0.0);
            if (count > 1) G = getValueData(mylist[1].c_str());
            else G = getConstantValue(0.0);
            if (count > 2) B = getValueData(mylist[2].c_str());
            else B = getConstantValue(0.0);
            if (count > 3) A = getValueData(mylist[3].c_str());
            else A = getConstantValue(1.0);
        }

        ValueData *R;
        ValueData *G;
        ValueData *B;
        ValueData *A;
};

#endif
