#ifndef _PATHINFO_HH_
#define _PATHINFO_HH_

#include <string>
#include <cstdlib>
#include <cstring>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>

class PathInfo
{
    public:
        PathInfo(std::string pathspec) : fullpath(pathspec), valid(false)
        {
            if (!fullpath.empty())
            {
                char *myabspath = realpath(fullpath.c_str(), 0x0);
                if (myabspath)
                {
                    valid = true;

                    // note that we create dirc and basec because dirname and basename can mangle the passed string
                    char *dirc = strdup(myabspath);
                    char *basec = strdup(myabspath);

                    directory = dirname(dirc);
                    file = basename(basec);

                    free(dirc);
                    free(basec);
                    free(myabspath);
                }
            }
        };
        virtual ~PathInfo() { };

        std::string getFullPath(void) { return fullpath; };
        std::string getDirectory(void) { return directory; };
        std::string getFile(void) { return file; };
        bool isValid(void) { return valid; };
        bool isExecutableFile(void)
        {
            if (!access(fullpath.c_str(), X_OK))
            {
                struct stat finfo;
                if (!stat(fullpath.c_str(), &finfo))
                {
                    if (finfo.st_mode & S_IFREG) return true;
                }
            }
            return false;
        };

    private:
        std::string fullpath;
        std::string directory;
        std::string file;
        bool valid;
};

#endif
