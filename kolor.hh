#ifndef _KOLOR_HH_
#define _KOLOR_HH_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "constants.hh"
#include "varlist.hh"

class Kolor
{
    public:
        Kolor()
        {
            R = dcLoadConstant(0.0f);
            G = dcLoadConstant(0.0f);
            B = dcLoadConstant(0.0f);
            A = dcLoadConstant(1.0f);
        }
        void set(double r, double g, double b)
        {
            R = dcLoadConstant(r);
            G = dcLoadConstant(g);
            B = dcLoadConstant(b);
            A = dcLoadConstant(1.0f);
        }
        void set(double r, double g, double b, double a)
        {
            R = dcLoadConstant(r);
            G = dcLoadConstant(g);
            B = dcLoadConstant(b);
            A = dcLoadConstant(a);
        }
        void set (const char *cspec)
        {
            if (!cspec) return;
            size_t itemlen = strlen(cspec) + 1;
            char *rstr, *gstr, *bstr, *astr;
            rstr = (char *)malloc(itemlen);
            gstr = (char *)malloc(itemlen);
            bstr = (char *)malloc(itemlen);
            astr = (char *)malloc(itemlen);
            int count = sscanf(cspec, "%s %s %s %s", rstr, gstr, bstr, astr);
            R = color_element(count-1, rstr, 0);
            G = color_element(count-2, gstr, 0);
            B = color_element(count-3, bstr, 0);
            A = color_element(count-4, astr, 1);
            free(rstr);
            free(gstr);
            free(bstr);
            free(astr);
        }

        double *R;
        double *G;
        double *B;
        double *A;

    private:
        double *color_element(int index, const char *strval, double defval)
        {
            if (index < 0) return dcLoadConstant(defval);
            else if (check_dynamic_element(strval)) return (double *)get_pointer(strval);
            else return dcLoadConstant(strtof(strval, 0x0));
        }
};

#endif
