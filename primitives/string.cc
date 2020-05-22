#include <string>
#include <vector>
#include "nodes.hh"
#include "RenderLib/RenderLib.hh"
#include "basicutils/msg.hh"
#include "basicutils/stringutils.hh"
#include "valuedata.hh"
#include "constants.hh"
#include "alignment.hh"
#include "varlist.hh"
#include "commonutils.hh"
#include "string.hh"

extern appdata AppData;

dcString::dcString(dcParent *myparent) : dcGeometric(myparent), background(false), fontID(0x0), forcemono(flMonoNone)
{
    color.set(1, 1, 1);
    fontSize = getConstantFromDecimal(12);
    shadowOffset = getConstantFromDecimal(0);
}

void dcString::setColor(const char *cspec)
{
    if (cspec) color.set(cspec);
}

void dcString::setBackgroundColor(const char *cspec)
{
    if (cspec)
    {
        bgcolor.set(cspec);
        background = true;
    }
}

void dcString::setFont(const char *font, const char *face, const char *size, const char *mono)
{
    std::string myface;
    if (face) myface = face;

    if (font) fontID = tdLoadFont(font, myface);
    if (!fontID) fontID = tdLoadFont(AppData.defaultfont, myface);

    if (size) fontSize = getValue(size);

    if (mono)
    {
        std::string mymono = mono;
        if (mymono == "Numeric") forcemono = flMonoNumeric;
        else if (mymono == "AlphaNumeric") forcemono = flMonoAlphaNumeric;
        else if (mymono == "All") forcemono = flMonoAll;
    }
}

void dcString::setShadowOffset(const char *inval)
{
    if (inval) shadowOffset = getValue(inval);
}

void dcString::setString(std::string mystr)
{
    size_t vstart, vlen, curpos = 0;

    do
    {
        vstart = mystr.find('@', curpos);
        filler.push_back(mystr.substr(curpos, vstart-curpos));
        if (vstart == std::string::npos) return;
        vlen = parse_var(mystr.substr(vstart, std::string::npos));
        if (vlen == std::string::npos)
        {
            filler.push_back("");
            return;
        }
        curpos = vstart + vlen;
    } while (curpos < mystr.size());

    filler.push_back("");
}

void dcString::draw(void)
{
    if (!fontID) return;

    computeGeometry();

    double myleft, myright, mybottom, mytop;
    unsigned i;
    float stringWidth;

    std::string mystring;
    for (i = 0; i < vstring.size(); i++) mystring += filler[i] + vstring[i]->get();
    mystring += filler[vstring.size()];

    // break mystring into a vector of 1-line substrings
    std::vector <std::string> lines;
    size_t endptr, strptr=0;
    do
    {
        endptr = mystring.find("\\n", strptr);
        lines.push_back(mystring.substr(strptr, endptr - strptr));
        strptr = endptr + 2;
    } while (endptr != std::string::npos);

    for (i=0; i<lines.size(); i++)
    {
        if (background || halign == AlignCenter || halign == AlignRight)
            stringWidth = fontID->getAdvance(lines[i], forcemono) * fontSize->getDecimal() / fontID->getBaseSize();

        switch (halign)
        {
            case AlignLeft:
                myleft = 0;
                break;
            case AlignCenter:
                myleft = -0.5 * stringWidth;
                break;
            case AlignRight:
                myleft = -stringWidth;
                break;
        }

        switch (valign)
        {
            case AlignBottom:
                mybottom = (fontSize->getDecimal()) * (double)(lines.size() - (i + 1));
                break;
            case AlignMiddle:
                mybottom = (fontSize->getDecimal()) * (((double)lines.size()/2) - (double)(i + 1));
                break;
            case AlignTop:
                mybottom = -(fontSize->getDecimal()) * (double)(i + 1);
                break;
        }

        translate_start(refx, refy);
        rotate_start(rotate->getDecimal());

        if (background)
        {
            mytop = mybottom + fontSize->getDecimal();
            myright = myleft + stringWidth;

            std::vector<float> pointsL;
            addPoint(pointsL, myleft, mybottom);
            addPoint(pointsL, myleft, mytop);
            addPoint(pointsL, myright, mytop);
            addPoint(pointsL, myright, mybottom);

            draw_quad(pointsL, bgcolor.R->getDecimal(), bgcolor.G->getDecimal(), bgcolor.B->getDecimal(), bgcolor.A->getDecimal());
        }
        if (shadowOffset->getBoolean())
        {
            draw_string(myleft + shadowOffset->getDecimal(), mybottom - shadowOffset->getDecimal(), fontSize->getDecimal(), 0, 0, 0, 1, fontID, forcemono, lines[i]);
        }
        draw_string(myleft, mybottom, fontSize->getDecimal(), color.R->getDecimal(), color.G->getDecimal(), color.B->getDecimal(), color.A->getDecimal(), fontID, forcemono, lines[i]);

        rotate_end();
        translate_end();
    }
}

// TODO: The parsing in this file should be simplified and/or combined with string parsing in xml_stringsub.cc
size_t dcString::parse_var(std::string mystr)
{
    size_t var_start, var_end;
    size_t fmt_start = mystr.find('(');
    size_t fmt_end = mystr.find(')');
    bool braced;
    std::string varstr = "@";

    if (mystr[1] == '{') 
    {
        braced = true;
        var_start = 2;
    }
    else
    {
        braced = false;
        var_start = 1;
    }

    if (fmt_start != std::string::npos) var_end = fmt_start;
    else if (braced) var_end = mystr.find('}');
    else var_end = mystr.find(' ');

    if (var_end == std::string::npos) varstr += mystr.substr(var_start, std::string::npos);
    else varstr += mystr.substr(var_start, var_end - var_start);

    Variable *myvalue = getVariable(varstr.c_str());
    if (myvalue)
    {
        if (fmt_start != std::string::npos && fmt_end != std::string::npos)
            vstring.push_back(new VarString(myvalue, mystr.substr(fmt_start+1, fmt_end-fmt_start-1)));
        else
            vstring.push_back(new VarString(myvalue, ""));
    }
    else warning_msg("Invalid variable label: " << varstr);

    if (braced) return mystr.find('}') + 1;
    else if (fmt_end != std::string::npos) return fmt_end + 1;
    else return mystr.find(' ');
}
