#ifndef _KOLOR_HH_
#define _KOLOR_HH_

#include <string>
#include <vector>
#include <cctype>
#include "constants.hh"
#include "varlist.hh"

class Kolor
{
    public:
        Kolor()
        {
            R = dcLoadConstant(0.0);
            G = dcLoadConstant(0.0);
            B = dcLoadConstant(0.0);
            A = dcLoadConstant(1.0);
        }
        void set(double r, double g, double b)
        {
            R = dcLoadConstant(r);
            G = dcLoadConstant(g);
            B = dcLoadConstant(b);
            A = dcLoadConstant(1.0);
        }
        void set(double r, double g, double b, double a)
        {
            R = dcLoadConstant(r);
            G = dcLoadConstant(g);
            B = dcLoadConstant(b);
            A = dcLoadConstant(a);
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
            else R = dcLoadConstant(0.0);
            if (count > 1) G = color_element(mylist[1]);
            else G = dcLoadConstant(0.0);
            if (count > 2) B = color_element(mylist[2]);
            else B = dcLoadConstant(0.0);
            if (count > 3) A = color_element(mylist[3]);
            else A = dcLoadConstant(1.0);
        }

        double *R;
        double *G;
        double *B;
        double *A;

    private:
        double *color_element(std::string strval)
        {
            if (check_dynamic_element(strval.c_str())) return (double *)get_pointer(strval.c_str());
            else return dcLoadConstant(std::stod(strval));
        }
};

#endif
