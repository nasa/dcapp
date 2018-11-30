#ifndef _PATHINFO_HH_
#define _PATHINFO_HH_

#include <string>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>

class PathInfo
{
    public:
        PathInfo(std::string pathspec) : fullpath(0x0), directory(0x0), file(0x0)
        {
            if (!pathspec.empty())
            {
                char *myabspath = (char *)calloc(PATH_MAX, sizeof(char));
                realpath(pathspec.c_str(), myabspath);
                fullpath = strdup(myabspath);

                // note that we create dirc and basec because dirname and basename can mangle the passed string
                char *dirc = strdup(myabspath);
                char *basec = strdup(myabspath);
                directory = strdup(dirname(dirc));
                file = strdup(basename(basec));

                free(myabspath);
                free(dirc);
                free(basec);
            }
        };
        virtual ~PathInfo() { if (directory) free(directory); if (file) free(file); };

        char *getDirectory(void) { return directory; };
        char *getFile(void) { return file; };
        bool isExecutableFile(void)
        {
            if (!access(fullpath, X_OK))
            {
                struct stat finfo;
                if (!stat(fullpath, &finfo))
                {
                    if (finfo.st_mode & S_IFREG) return true;
                }
            }
            return false;
        };

    private:
        char *fullpath;
        char *directory;
        char *file;
};

#endif
