#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include "basicutils/pathinfo.hh"

std::vector<std::string> parseUserPath(void)
{
    std::vector<std::string> retvec;

    char *mypath = getenv("PATH");
    if (!mypath) return retvec;

    std::string userpath = mypath;
    if (userpath.empty()) return retvec;

    size_t start = 0, end = userpath.find(':');
    while (end != std::string::npos)
    {
        retvec.push_back(userpath.substr(start, end-start));
        start = end + 1;
        end = userpath.find(':', start);
    }
    retvec.push_back(userpath.substr(start));

    return retvec;
}

std::string findExecutablePath(const std::string &inpath)
{
    if (inpath.find('/') != std::string::npos)
    {
        PathInfo pinfo(inpath);
        if (pinfo.isExecutableFile()) return pinfo.getDirectory();
        else return "";
    }

    std::vector<std::string> upath = parseUserPath();
    std::vector<std::string>::iterator item;
    for (item = upath.begin(); item != upath.end(); item++)
    {
        PathInfo pinfo(*item + '/' + inpath);
        if (pinfo.isExecutableFile()) return pinfo.getDirectory();
    }
    return "";
}

std::string getScriptResult(const std::string &myscript, const std::string &myarg)
{
    std::string tmpstr, cmd = myscript + " " + myarg;

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    char mychar;
    while (fread(&mychar, 1, 1, pipe)) tmpstr += mychar;

    // strip out leading and trailing white space
    size_t i, startblanks=0, endblanks=0;
    for (i=0; i<tmpstr.length(); i++)
    {
        if (isspace(tmpstr[i])) startblanks++;
        else break;
    }
    for (i=tmpstr.length()-1; (int)i>=0; i--)
    {
        if (isspace(tmpstr[i])) endblanks++;
        else break;
    }

    return tmpstr.substr(startblanks, tmpstr.length()-endblanks);
}
