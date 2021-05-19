#include <string>
#include <vector>
#include "app_data.hh"
#include "RenderLib/RenderLib.hh"
#include "basicutils/msg.hh"
#include "basicutils/stringutils.hh"
#include "constants.hh"
#include "variables.hh"
#include "values.hh"
#include "commonutils.hh"
#include "geometric.hh"
#include "string.hh"

extern appdata AppData;

dcString::dcString(dcParent *myparent) :
dcGeometric(myparent), background(false), fontID(0x0), outlineFontID(0x0), forcemono(flMonoNone), trimDefined(false), rateDefined(false), outlineDefined(false)
{
    outlineColor.set(1,1,1);
    color.set(1, 1, 1);
    fontSize = getConstantFromDecimal(12);
    shadowOffset = getConstantFromDecimal(0);
    zeroTrim = getConstantFromDecimal(0);
    updateRate = getConstantFromDecimal(0);
}

void dcString::setColor(const std::string &cspec)
{
    if (!cspec.empty()) color.set(cspec);
}

void dcString::setBackgroundColor(const std::string &cspec)
{
    if (!cspec.empty())
    {
        bgcolor.set(cspec);
        background = true;
    }
}

void dcString::setFont(const std::string &font, const std::string &face, const std::string &size, const std::string &mono)
{
    if (!font.empty()) fontID = tdLoadFont(font, face);
    if (!fontID) fontID = tdLoadFont(AppData.defaultfont, face);

    if (!size.empty()) fontSize = getValue(size);

    if (!mono.empty())
    {
        if (CaseInsensitiveCompare(mono, "Numeric")) forcemono = flMonoNumeric;
        else if (CaseInsensitiveCompare(mono, "AlphaNumeric")) forcemono = flMonoAlphaNumeric;
        else if (CaseInsensitiveCompare(mono, "All")) forcemono = flMonoAll;
    }

    // get file for outline 
    std::string outlinefont = font;
    if(!font.empty()) {
        if (outlinefont[outlinefont.length() - 1] == '/')
            outlinefont = outlinefont.substr(0, outlinefont.length()-1);

        if (outlinefont.substr(outlinefont.length() - 4) == ".ttf")
            outlinefont = outlinefont.substr(0, outlinefont.length()-4) + "-Outline.ttf";
        else
            outlinefont = outlinefont + "-Outline";
    }
    setOutlineFont(outlinefont, face);
}

void dcString::setOutlineFont(const std::string &font, const std::string &face)
{
    if (!font.empty()) outlineFontID = tdLoadFont(font, face);
    if (!outlineFontID) outlineFontID = tdLoadFont(AppData.defaultoutlinefont, face);
}

void dcString::setShadowOffset(const std::string &inval)
{
    if (!inval.empty()) shadowOffset = getValue(inval);
}

void dcString::setUpdateRate(const std::string &inval)
{
    if (!inval.empty())
    {
        rateDefined = true;
        updateRate = getValue(inval);
    }
}

void dcString::setZeroTrim(const std::string &inval)
{
    if (!inval.empty())
    {
        trimDefined = true;
        zeroTrim = getValue(inval);
    }
}

void dcString::setString(const std::string &mystr)
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

void dcString::setOutlineColor(const std::string &cspec)
{
    if (!cspec.empty())
    {
        outlineColor.set(cspec);
        outlineDefined = true;
    }
}

void dcString::draw(void)
{
    if (!fontID) return;

    computeGeometry();

    double myleft, myright, mybottom, mytop;
    unsigned i;
    float stringWidth;

    std::string mystring;

    if (rateDefined)
    {
        if (lastUpdate.getSeconds() > updateRate->getDecimal())
        {
            for (i = 0; i < vstring.size(); i++)
            {
                if (trimDefined)
                    mystring += filler[i] + vstring[i]->get(zeroTrim);
                else
                    mystring += filler[i] + vstring[i]->get();
            }
            mystring += filler[vstring.size()];
            lastUpdate.restart();
            storedString = mystring;
        }
        else mystring = storedString;
    }
    else
    {
        for (i = 0; i < vstring.size(); i++)
        {
                if (trimDefined)
                    mystring += filler[i] + vstring[i]->get(zeroTrim);
                else
                    mystring += filler[i] + vstring[i]->get();
        }
        mystring += filler[vstring.size()];
    }

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
        if (background || halign == dcCenter || halign == dcRight)
            stringWidth = fontID->getAdvance(lines[i], forcemono) * fontSize->getDecimal() / fontID->getBaseSize();

        switch (halign)
        {
            case dcLeft:
                myleft = 0;
                break;
            case dcCenter:
                myleft = -0.5 * stringWidth;
                break;
            case dcRight:
                myleft = -stringWidth;
                break;
        }

        switch (valign)
        {
            case dcBottom:
                mybottom = (fontSize->getDecimal()) * (double)(lines.size() - (i + 1));
                break;
            case dcMiddle:
                mybottom = (fontSize->getDecimal()) * (((double)lines.size()/2) - (double)(i + 1));
                break;
            case dcTop:
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

        // draw outline
        if (outlineDefined) 
        {
            draw_string(myleft, mybottom, fontSize->getDecimal(), outlineColor.R->getDecimal(), outlineColor.G->getDecimal(), outlineColor.B->getDecimal(), 
                outlineColor.A->getDecimal(), outlineFontID, forcemono, lines[i]);
        }

        rotate_end();
        translate_end();
    }
}

// TODO: The parsing in this file should be simplified and/or combined with string parsing in xml_stringsub.cc
size_t dcString::parse_var(const std::string &mystr)
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

    Variable *myvalue = getVariable(varstr);
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
