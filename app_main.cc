#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cctype>
#include <unistd.h>
#include <libgen.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include "basicutils/msg.hh"
#include "basicutils/timer.hh"
#include "basicutils/tidy.hh"
#include "basicutils/pathinfo.hh"
#include "basicutils/shellutils.hh"
#include "basicutils/stringutils.hh"
#include "comm.hh"
#include "device.hh"
#include "app_data.hh"
#include "xml_stringsub.hh"
#include "PixelStream/curlLib.hh"

#define MINIMUM_REFRESH 0.05 // 50 ms = 20 Hz

extern void mainloop(void);
extern void UpdateDisplay(void);
extern void SetNeedsRedraw(void);
extern void CheckMouseBounce(void);
extern void ui_terminate(void);
extern std::vector<std::string> OSgetArgs(int, char **);
extern int ParseXMLFile(const std::string &);
extern void DisplayPreInitStub(void *(*)(const char *));
extern void DisplayInitStub(void);
extern void DisplayLogicStub(void);
extern void DisplayCloseStub(void);
extern void start_logging(std::string &);
extern void end_logging(void);

void Terminate(int);

static void SetDefaultEnvironment(std::string);
static void ProcessArgs(int, char **);

appdata AppData;

int main(int argc, char **argv)
{
    // Set up signal handlers
    signal(SIGINT, Terminate);
    signal(SIGTERM, Terminate);
    signal(SIGPIPE, SIG_IGN);

    Message::setLabel(basename(argv[0]));
#ifdef DEBUG
    Message::enableDebugging();
#endif

    AppData.master_timer = new Timer;
    AppData.last_update = new Timer;

    AppData.DisplayPreInit = &DisplayPreInitStub;
    AppData.DisplayInit = &DisplayInitStub;
    AppData.DisplayLogic = &DisplayLogicStub;
    AppData.DisplayClose = &DisplayCloseStub;

    AppData.toplevel = 0;
    SetDefaultEnvironment(argv[0]);
    ProcessArgs(argc, argv);
    if (!(AppData.toplevel))
    {
        error_msg("A Window element must be defined in the specfile");
        Terminate(0);
    }

    curlLibInit();
    AppData.DisplayPreInit(get_pointer);
    AppData.DisplayInit();
    UpdateDisplay();

    // Do forever
    mainloop();

    return(0);
}

void Idle(void)
{
    static Timer *mytime = new Timer;
    int status;
    std::list<DeviceModule *>::iterator deviceitem;
    std::list<CommModule *>::iterator commitem;
    std::list<Animation *>::iterator animitem;

    float elapsed = mytime->getSeconds();
    mytime->restart();

    // hibernate until the next period of time defined by MINIMUM_REFRESH to temporarily release the CPU
    if (elapsed < MINIMUM_REFRESH) hibernate(MINIMUM_REFRESH - elapsed);

    for (deviceitem = AppData.devicelist.begin(); deviceitem != AppData.devicelist.end(); deviceitem++)
    {
        (*deviceitem)->read();
    }

    for (commitem = AppData.commlist.begin(); commitem != AppData.commlist.end(); commitem++)
    {
        status = (*commitem)->write();
        if (status == CommModule::Terminate) Terminate(0);

        status = (*commitem)->read();
        if (status == CommModule::Success) UpdateDisplay();
        else if (status == CommModule::Terminate) Terminate(0);

        (*commitem)->updateConnectedVariable();
    }

    if (!AppData.animators.empty())
    {
        SetNeedsRedraw();
        for (animitem = AppData.animators.begin(); animitem != AppData.animators.end(); animitem++)
        {
            if ((*animitem)->update(AppData.master_timer->getSeconds()))
            {
                delete *animitem;
                *animitem = 0x0;
            }
        }
        AppData.animators.remove(0x0);
    }

    CheckMouseBounce();

    if (AppData.last_update->getSeconds() > AppData.force_update) UpdateDisplay();
}

void Terminate(int flag)
{
    std::list<DeviceModule *>::iterator deviceitem;
    std::list<CommModule *>::iterator commitem;
    std::list<Animation *>::iterator animitem;
    std::list<PixelStreamItem *>::iterator psitem;

    if (AppData.DisplayClose) AppData.DisplayClose();

    ui_terminate();

    for (deviceitem = AppData.devicelist.begin(); deviceitem != AppData.devicelist.end(); deviceitem++)
    {
        delete (*deviceitem);
    }
    for (commitem = AppData.commlist.begin(); commitem != AppData.commlist.end(); commitem++)
    {
        delete (*commitem);
    }
    for (psitem = AppData.pixelstreams.begin(); psitem != AppData.pixelstreams.end(); psitem++)
    {
        delete (*psitem);
    }
    for (animitem = AppData.animators.begin(); animitem != AppData.animators.end(); animitem++)
    {
        delete (*animitem);
    }

    curlLibTerm();
    exit(flag);
}

/* Set the dcappOSTYPE, dcappOSSPEC, dcappOBJDIR, and dcappBINDIR environment       */
/* variables in case the user needs them.  Likewise, set the following for the user */
/* if they have not been set: USER, LOGNAME, HOME, OSTYPE, MACHTYPE, and HOST.      */
static void SetDefaultEnvironment(std::string pathspec)
{
    unsigned i;
    struct utsname minfo;
    struct passwd *pw = getpwuid(getuid());
    long hsize = sysconf(_SC_HOST_NAME_MAX)+1;
    char myhost[hsize];

    std::string mypath = findExecutablePath(pathspec);

    AppData.dcapphome = PathInfo(mypath + "/../../../").getFullPath();
    AppData.defaultfont = AppData.dcapphome + "/dcapp.app/Contents/Resources/fonts/defaultfont";

    std::string configscript = PathInfo(mypath + "/../dcapp-config").getFullPath();

    setenv("dcappOSTYPE", getScriptResult(configscript, "--ostype").c_str(), 1);
    setenv("dcappOSSPEC", getScriptResult(configscript, "--osspec").c_str(), 1);
    setenv("dcappOBJDIR", getScriptResult(configscript, "--objdir").c_str(), 1);
    setenv("dcappBINDIR", getScriptResult(configscript, "--bindir").c_str(), 1);
    setenv("dcappVERSION", getScriptResult(configscript, "--version_short").c_str(), 1);

    uname(&minfo);

    std::string lcos, os = minfo.sysname;
    for (i = 0; i < os.length(); i++) lcos += tolower(os[i]);
    setenv("OSTYPE", lcos.c_str(), 0);

    setenv("USER", pw->pw_name, 0);
    setenv("LOGNAME", pw->pw_name, 0);
    setenv("HOME", pw->pw_dir, 0);
    setenv("MACHTYPE", minfo.machine, 0);

    if (!gethostname(myhost, hsize)) setenv("HOST", myhost, 0);
}

static void ProcessArgs(int argc, char **argv)
{
    std::vector<std::string> arglist;
    std::string outfile;

    arglist = OSgetArgs(argc, argv);

    if (arglist.empty())
    {
        user_msg("USAGE: dcapp <specfile> [optional arguments]");
        Terminate(-1);
    }

    for (std::vector<std::string>::iterator it = arglist.begin()+1; it != arglist.end(); it++)
    {
        size_t findequal = it->find_first_of('=');
        if (findequal == std::string::npos)
        {
            if (*it == "-debug") Message::enableDebugging();
            else if (*it == "-o" && it+1 != arglist.end()) outfile = *(++it);
         }
        else
        {
            std::string key = it->substr(0, findequal);
            std::string value = it->substr(findequal+1, std::string::npos);
            processArgument(key, value);
        }
    }

    start_logging(outfile);

    if (ParseXMLFile(arglist[0])) Terminate(-1);

    end_logging();
}
