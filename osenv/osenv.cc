#include <vector>
#include <string>

extern void ui_init(const std::string &);

std::vector<std::string> OSgetArgs(int argc, char **argv)
{
    std::vector<std::string> arglist;
    std::string xdisplay;

    arglist.assign(argv + 1, argv + argc);

    if (!arglist.empty())
    {
        for (std::vector<std::string>::iterator it = arglist.begin(); it != arglist.end(); it++)
        {
            if (*it == "-x" && it+1 != arglist.end()) xdisplay = *(++it);
        }

        ui_init(xdisplay);
    }

    return arglist;
}
