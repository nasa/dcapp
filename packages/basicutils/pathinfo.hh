#ifndef _PATHINFO_HH_
#define _PATHINFO_HH_

#include <cstdlib>
#include <cstring>
#include <libgen.h>
#include "tidy.hh"

class PathInfo
{
    public:
        PathInfo(const char *fullpath) : directory(0x0), file(0x0)
        {
            if (fullpath)
            {
                char *myabspath = (char *)calloc(PATH_MAX, sizeof(char));
                realpath(fullpath, myabspath);

                char *dirc = strdup(myabspath);
                char *basec = strdup(myabspath);
                directory = strdup(dirname(dirc));
                file = strdup(basename(basec));

                free(myabspath);
                free(dirc);
                free(basec);
            }
        };
        virtual ~PathInfo() { TIDY(directory); TIDY(file); };

        char *getDirectory(void) { return directory; };
        char *getFile(void) { return file; };

    private:
        char *directory;
        char *file;
};

#endif
