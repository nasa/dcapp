#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <strings.h>
#include <libgen.h>
#include <fcntl.h>
#include <dlfcn.h>
#include "basicutils/msg.hh"
#include "basicutils/pathinfo.hh"
#include "basicutils/stringutils.hh"
#include "trick/trickcomm.hh"
#include "edge/edgecomm.hh"
#include "can/CAN.hh"
#include "uei/UEI.hh"
#include "hagstrom/Hagstrom.hh"
#include "primitives/primitives.hh"
#include "variables.hh"
#include "app_data.hh"
#include "xml_data.hh"
#include "xml_utils.hh"
#include "xml_stringsub.hh"

extern void window_init(bool, int , int, int, int);

extern void DisplayPreInitStub(void *(*)(const char *));
extern void DisplayInitStub(void);
extern void DisplayLogicStub(void);
extern void DisplayCloseStub(void);

static int process_elements(dcParent *, xmlNodePtr);

extern appdata AppData;

static TrickCommModule *trickcomm = 0x0;
static EdgeCommModule *edgecomm = 0x0;
static xmldata indid, indonval, activeid, activetrueval, transitionid, key, keyascii, bezelkey;
static bool preprocessing = true;

int ParseXMLFile(const char *fullpath)
{
    int mycwd;
    xmlDocPtr mydoc;
    xmlNodePtr root_element;

    if (!fullpath)
    {
        error_msg("No XML file specified");
        return (-1);
    }

    PathInfo mypath(fullpath);
    if (!(mypath.isValid()))
    {
        error_msg("Unable to open XML file " << fullpath);
        return (-1);
    }

    setenv("dcappDisplayHome", mypath.getDirectory().c_str(), 1);

    // Store cwd for future use
    mycwd = open(".", O_RDONLY);

    // Move to directory containing the specfile by default
    chdir(mypath.getDirectory().c_str());

    if (XMLFileOpen(&mydoc, &root_element, mypath.getFile())) return (-1);

    if (!NodeCheck(root_element, "DCAPP"))
    {
        error_msg("Bad root element in XML file: \"" << root_element->name << "\" (should be \"DCAPP\")");
        return (-1);
    }

    // Set generic display logic handlers, in case the user doesn't specify a DisplayLogic element
    AppData.DisplayPreInit = &DisplayPreInitStub;
    AppData.DisplayInit = &DisplayInitStub;
    AppData.DisplayLogic = &DisplayLogicStub;
    AppData.DisplayClose = &DisplayCloseStub;

    if (process_elements(0x0, root_element->children)) return (-1);

    XMLFileClose(mydoc);
    XMLEndParsing();

    // Return to the original working directory
    fchdir(mycwd);
    close(mycwd);

    return 0;
}

static int process_elements(dcParent *myparent, xmlNodePtr startnode)
{
    xmlNodePtr node;

    for (node = startnode; node; node = node->next)
    {
        if (NodeCheck(node, "Dummy")) process_elements(myparent, node->children);
        if (NodeCheck(node, "Constant")) processConstantNode(node);
        if (NodeCheck(node, "Style")) processStyleNode(node);
        if (NodeCheck(node, "Defaults")) processDefaultsNode(node);
        if (NodeCheck(node, "Include"))
        {
            xmlDocPtr include_file;
            xmlNodePtr include_element;
            xmldata include_filename = get_node_content(node);

            if (!include_filename.empty())
            {
                PathInfo mypath(include_filename);
                if (!mypath.isValid())
                {
                    warning_msg("Couldn't open include file " << include_filename);
                }
                else
                {
                    // Store cwd for future use
                    int mycwd = open(".", O_RDONLY);

                    // Move to directory containing the new file
                    chdir(mypath.getDirectory().c_str());

                    if (XMLFileOpen(&include_file, &include_element, mypath.getFile()))
                    {
                        warning_msg("Couldn't open include file " << include_filename);
                    }
                    else
                    {
                        process_elements(myparent, include_element);
                        XMLFileClose(include_file);
                    }

                    // Return to the original working directory
                    fchdir(mycwd);
                    close(mycwd);
                }
            }
            else warning_msg("No include file specified");
        }
        if (NodeCheck(node, "If"))
        {
            xmldata val = get_element_data(node, "Value");
            xmldata val1 = get_element_data(node, "Value1");
            xmldata val2 = get_element_data(node, "Value2");
            xmldata myoperator = get_element_data(node, "Operator");
            bool subparent_found = false;

            if (!val1.defined()) val1 = val;

            if (preprocessing || (!check_dynamic_element(val1) && !check_dynamic_element(val2)))
            {
                dcCondition *myitem = new dcCondition(0x0, myoperator, val1, val2);
                bool myflag = myitem->checkCondition();
                delete myitem;

                for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
                {
                    if (myflag && NodeCheck(subnode, "True"))
                    {
                        process_elements(myparent, subnode->children);
                        subparent_found = true;
                    }
                    if (!myflag && NodeCheck(subnode, "False"))
                    {
                        process_elements(myparent, subnode->children);
                        subparent_found = true;
                    }
                }
                // Assume "True" if no subparent is found
                if (myflag && !subparent_found) process_elements(myparent, node->children);
            }
            else
            {
                dcCondition *myitem = new dcCondition(myparent, myoperator, val1, val2);

                for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
                {
                    if (NodeCheck(subnode, "True"))
                    {
                        process_elements(myitem->TrueList, subnode->children);
                        subparent_found = true;
                    }
                    if (NodeCheck(subnode, "False"))
                    {
                        process_elements(myitem->FalseList, subnode->children);
                        subparent_found = true;
                    }
                }
                // Assume "True" if no subparent is found
                if (!subparent_found) process_elements(myitem->TrueList, node->children);
            }
        }
        if (NodeCheck(node, "Set"))
        {
            if (preprocessing)
            {
                dcSetValue *myitem = new dcSetValue(0x0, get_element_data(node, "Variable"), get_node_content(node));
                myitem->setOperator(get_element_data(node, "Operator"));
                myitem->setRange(get_element_data(node, "MinimumValue"), get_element_data(node, "MaximumValue"));
                myitem->updateData();
                delete myitem;
            }
            else
            {
                dcSetValue *myitem = new dcSetValue(myparent, get_element_data(node, "Variable"), get_node_content(node));
                myitem->setOperator(get_element_data(node, "Operator"));
                myitem->setRange(get_element_data(node, "MinimumValue"), get_element_data(node, "MaximumValue"));
            }
        }
        if (NodeCheck(node, "Animation"))
        {
            dcAnimate *myitem = new dcAnimate(myparent);
            myitem->setDuration(get_element_data(node, "Duration"));
            process_elements(myitem, node->children);
        }
        if (NodeCheck(node, "Variable"))
        {
            registerVariable(get_node_content(node), get_element_data(node, "Type"), get_element_data(node, "InitialValue"));
        }
        if (NodeCheck(node, "TrickIo"))
        {
            trickcomm = new TrickCommModule;
            trickcomm->setHost(get_element_data(node, "Host"));
            trickcomm->setPort(get_element_data(node, "Port"));
            trickcomm->setDataRate(get_element_data(node, "DataRate"));
            trickcomm->setConnectedVariable(get_element_data(node, "ConnectedVariable"));
            if (get_element_data(node, "DisconnectAction") == "Reconnect") trickcomm->setReconnectOnDisconnect();
            process_elements(myparent, node->children);
            trickcomm->finishInitialization();
            AppData.commlist.push_back(trickcomm);
        }
        if (NodeCheck(node, "FromTrick"))
        {
            if (trickcomm)
            {
                trickcomm->activateFromList();
                process_elements(myparent, node->children);
                trickcomm->deactivateList();
            }
        }
        if (NodeCheck(node, "ToTrick"))
        {
            if (trickcomm)
            {
                trickcomm->activateToList();
                process_elements(myparent, node->children);
                trickcomm->deactivateList();
            }
        }
        if (NodeCheck(node, "TrickVariable"))
        {
            if (trickcomm)
            {
                trickcomm->addParameter(get_node_content(node), get_element_data(node, "Name"), get_element_data(node, "Units"), get_element_data(node, "InitializationOnly"), false);
            }
        }
        if (NodeCheck(node, "TrickMethod"))
        {
            if (trickcomm)
            {
                trickcomm->addParameter(get_node_content(node), get_element_data(node, "Name"), "", "", true);
            }
        }
        if (NodeCheck(node, "EdgeIo"))
        {
            edgecomm = new EdgeCommModule;
            edgecomm->setConnectedVariable(get_element_data(node, "ConnectedVariable"));
            process_elements(myparent, node->children);
            edgecomm->finishInitialization(get_element_data(node, "Host"), get_element_data(node, "Port"), StringToDecimal(get_element_data(node, "DataRate"), 1.0));
            AppData.commlist.push_back(edgecomm);
        }
        if (NodeCheck(node, "FromEdge"))
        {
            if (edgecomm)
            {
                edgecomm->activateFromList();
                process_elements(myparent, node->children);
                edgecomm->deactivateList();
            }
        }
        if (NodeCheck(node, "ToEdge"))
        {
            if (edgecomm)
            {
                edgecomm->activateToList();
                process_elements(myparent, node->children);
                edgecomm->deactivateList();
            }
        }
        if (NodeCheck(node, "EdgeVariable"))
        {
            if (edgecomm)
            {
                edgecomm->addParameter(get_node_content(node), get_element_data(node, "RcsCommand"));
            }
        }
        if (NodeCheck(node, "CAN"))
        {
            CanDevice *can = new CanDevice;
            can->initialize(get_element_data(node, "Network"), get_element_data(node, "ButtonID"), get_element_data(node, "ControlID"), get_element_data(node, "InhibitVariable"));
            AppData.devicelist.push_back(can);
        }
        if (NodeCheck(node, "UEI"))
        {
            UeiDevice *uei = new UeiDevice;
            uei->connect(get_element_data(node, "Port"), get_element_data(node, "BezelID"));
            AppData.devicelist.push_back(uei);
        }
        if (NodeCheck(node, "Hagstrom"))
        {
            HagstromDevice *hagstrom = new HagstromDevice;
            hagstrom->setSerialNumber(get_element_data(node, "SerialNumber"));
            AppData.devicelist.push_back(hagstrom);
        }
        if (NodeCheck(node, "DisplayLogic"))
        {
            xmldata myfile = get_node_content(node);
            void *so_handler;
            char *error;

            debug_msg("Loading " << myfile << "...");

            // If path isn't specified, prepend with "./" to avoid searching LD_LIBRARY_PATH, DYLD_LIBRARY_PATH, etc.
            if (myfile.find('/') == xmldata::npos) myfile.insert(0, "./");

            so_handler = dlopen(myfile.c_str(), RTLD_NOW);
            if (so_handler)
            {
                AppData.DisplayPreInit = (void (*)(void *(*)(const char *)))dlsym(so_handler, "DisplayPreInit");
                if ((error = dlerror()))
                {
                    debug_msg(error);
                    AppData.DisplayPreInit = &DisplayPreInitStub;
                }

                AppData.DisplayInit = (void (*)())dlsym(so_handler, "DisplayInit");
                if ((error = dlerror()))
                {
                    debug_msg(error);
                    AppData.DisplayInit = &DisplayInitStub;
                }

                AppData.DisplayLogic = (void (*)())dlsym(so_handler, "DisplayLogic");
                if ((error = dlerror()))
                {
                    debug_msg(error);
                    AppData.DisplayLogic = &DisplayLogicStub;
                }

                AppData.DisplayClose = (void (*)())dlsym(so_handler, "DisplayClose");
                if ((error = dlerror()))
                {
                    debug_msg(error);
                    AppData.DisplayClose = &DisplayCloseStub;
                }
            }
            else warning_msg(dlerror());
        }
        if (NodeCheck(node, "Window"))
        {
            AppData.force_update = StringToDecimal(get_element_data(node, "ForceUpdate"), 60);
            window_init(StringToBoolean(get_element_data(node, "FullScreen")),
                        StringToInteger(get_element_data(node, "X"), 0),
                        StringToInteger(get_element_data(node, "Y"), 0),
                        StringToInteger(get_element_data(node, "Width"), 800),
                        StringToInteger(get_element_data(node, "Height"), 800));
            AppData.toplevel = new dcWindow();
            AppData.toplevel->setActiveDisplay(get_element_data(node, "ActiveDisplay"));
            process_elements(AppData.toplevel, node->children);
        }
        if (NodeCheck(node, "Panel"))
        {
            dcPanel *myitem = new dcPanel(myparent);
            myitem->setID(get_element_data(node, "DisplayIndex"));
            myitem->setColor(get_element_data(node, "BackgroundColor"));
            myitem->setOrtho(get_element_data(node, "VirtualWidth"), get_element_data(node, "VirtualHeight"));
            preprocessing = false;
            process_elements(myitem, node->children);
            preprocessing = true;
        }
        if (NodeCheck(node, "Container"))
        {
            dcContainer *myitem = new dcContainer(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setOrigin(get_element_data(node, "OriginX"), get_element_data(node, "OriginY"));
            myitem->setVirtualSize(get_element_data(node, "VirtualWidth"), get_element_data(node, "VirtualHeight"));
            process_elements(myitem, node->children);
        }
        if (NodeCheck(node, "Vertex"))
        {
            dcVertex *myitem = new dcVertex(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setOrigin(get_element_data(node, "OriginX"), get_element_data(node, "OriginY"));
        }
        if (NodeCheck(node, "Line"))
        {
            dcLine *myitem = new dcLine(myparent);
            myitem->setLineWidth(get_element_data(node, "LineWidth"));
            myitem->setColor(get_element_data(node, "Color"));
            process_elements(myitem, node->children);
        }
        if (NodeCheck(node, "Polygon"))
        {
            dcPolygon *myitem = new dcPolygon(myparent);
            myitem->setFillColor(get_element_data(node, "FillColor"));
            myitem->setLineColor(get_element_data(node, "LineColor"));
            myitem->setLineWidth(get_element_data(node, "LineWidth"));
            process_elements(myitem, node->children);
            for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
            {
                if (NodeCheck(subnode, "OnPress")) process_elements(myitem->PressList, subnode->children);
                if (NodeCheck(subnode, "OnRelease")) process_elements(myitem->ReleaseList, subnode->children);
            }
        }
        if (NodeCheck(node, "Rectangle"))
        {
            dcRectangle *myitem = new dcRectangle(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setOrigin(get_element_data(node, "OriginX"), get_element_data(node, "OriginY"));
            myitem->setFillColor(get_element_data(node, "FillColor"));
            myitem->setLineColor(get_element_data(node, "LineColor"));
            myitem->setLineWidth(get_element_data(node, "LineWidth"));
            for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
            {
                if (NodeCheck(subnode, "OnPress")) process_elements(myitem->PressList, subnode->children);
                if (NodeCheck(subnode, "OnRelease")) process_elements(myitem->ReleaseList, subnode->children);
            }
        }
        if (NodeCheck(node, "Circle"))
        {
            dcCircle *myitem = new dcCircle(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setOrigin(get_element_data(node, "OriginX"), get_element_data(node, "OriginY"));
            myitem->setFillColor(get_element_data(node, "FillColor"));
            myitem->setLineColor(get_element_data(node, "LineColor"));
            myitem->setLineWidth(get_element_data(node, "LineWidth"));
            myitem->setRadius(get_element_data(node, "Radius"));
            myitem->setSegments(get_element_data(node, "Segments"));
            for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
            {
                if (NodeCheck(subnode, "OnPress")) process_elements(myitem->PressList, subnode->children);
                if (NodeCheck(subnode, "OnRelease")) process_elements(myitem->ReleaseList, subnode->children);
            }
        }
        if (NodeCheck(node, "String"))
        {
            dcString *myitem = new dcString(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize("0", get_element_data(node, "Size"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setOrigin(get_element_data(node, "OriginX"), get_element_data(node, "OriginY"));
            myitem->setColor(get_element_data(node, "Color"));
            myitem->setBackgroundColor(get_element_data(node, "BackgroundColor"));
            myitem->setFont(get_element_data(node, "Font"), get_element_data(node, "Face"), get_element_data(node, "Size"), get_element_data(node, "ForceMono"));
            myitem->setShadowOffset(get_element_data(node, "ShadowOffset"));
            myitem->setString(get_node_content(node));
        }
        if (NodeCheck(node, "Image"))
        {
            dcImage *myitem = new dcImage(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setOrigin(get_element_data(node, "OriginX"), get_element_data(node, "OriginY"));
            xmldata myfile = get_element_data(node, "File");
            if (myfile.empty()) myfile = get_node_content(node);
            myitem->setTexture(myfile);
            for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
            {
                if (NodeCheck(subnode, "OnPress")) process_elements(myitem->PressList, subnode->children);
                if (NodeCheck(subnode, "OnRelease")) process_elements(myitem->ReleaseList, subnode->children);
            }
        }
        if (NodeCheck(node, "PixelStream"))
        {
            dcPixelStream *myitem = new dcPixelStream(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setOrigin(get_element_data(node, "OriginX"), get_element_data(node, "OriginY"));
            myitem->setProtocol(get_element_data(node, "Protocol"), get_element_data(node, "Host"), get_element_data(node, "Port"), get_element_data(node, "Path"), get_element_data(node, "Username"), get_element_data(node, "Password"), get_element_data(node, "SharedMemoryKey"), get_element_data(node, "File"), get_element_data(node, "Camera"));
            myitem->setTestPattern(get_element_data(node, "TestPattern"));
        }
        if (NodeCheck(node, "Button"))
        {
            key = get_element_data(node, "Key");
            keyascii = get_element_data(node, "KeyASCII");
            bezelkey = get_element_data(node, "BezelKey");
            xmldata type = get_element_data(node, "Type");
            xmldata buttonid = get_element_data(node, "Variable");
            xmldata buttononval = get_element_data(node, "On");
            xmldata buttonoffval = get_element_data(node, "Off");
            xmldata switchid = get_element_data(node, "SwitchVariable");
            xmldata switchonval = get_element_data(node, "SwitchOn");
            xmldata switchoffval = get_element_data(node, "SwitchOff");
            indid = get_element_data(node, "IndicatorVariable");
            indonval = get_element_data(node, "IndicatorOn");
            activeid = get_element_data(node, "ActiveVariable");
            activetrueval = get_element_data(node, "ActiveOn");

            if (!switchonval.defined()) switchonval = buttononval;
            if (!switchoffval.defined()) switchoffval = buttonoffval;
            if (!indonval.defined()) indonval = switchonval;

            if (buttonid.empty() && switchid.empty()) buttonid = create_virtual_variable("Integer", "0");
            if (switchid.empty()) switchid = buttonid;
            if (indid.empty()) indid = switchid;
            if (switchid != indid) transitionid = create_virtual_variable("Integer", "0");
            else transitionid.reset();

            dcContainer *myitem = new dcContainer(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setOrigin(get_element_data(node, "OriginX"), get_element_data(node, "OriginY"));

            bool toggle = false, momentary = false;
            xmldata offval;

            if (!type.empty())
            {
                if (type == "Toggle") toggle = true;
                else if (type == "Momentary") momentary = true;
            }

            dcParent *mySublist = myitem;
            if (!activeid.empty())
            {
                if (!activetrueval.defined()) activetrueval = "1";
                dcCondition *mycond = new dcCondition(myitem, "eq", activeid, activetrueval);
                mySublist = mycond->TrueList;
            }

            if (!switchonval.defined()) switchonval = "1";
            if (toggle || momentary)
            {
                if (switchoffval.defined()) offval = switchoffval;
                else offval = "0";
            }

            if (toggle)
            {
                dcCondition *mycond = new dcCondition(mySublist, "eq", indid, indonval);
                dcMouseEvent *mymouse = new dcMouseEvent(mycond->TrueList);
                new dcSetValue(mymouse->PressList, switchid, offval);
                if (!transitionid.empty()) new dcSetValue(mymouse->PressList, transitionid, "-1");
                dcMouseEvent *mymouse1 = new dcMouseEvent(mycond->FalseList);
                new dcSetValue(mymouse1->PressList, switchid, switchonval);
                if (!transitionid.empty()) new dcSetValue(mymouse1->PressList, transitionid, "1");
                if (!key.empty() || !keyascii.empty())
                {
                    dcKeyboardEvent *myevent = new dcKeyboardEvent(mycond->TrueList, key, keyascii);
                    new dcSetValue(myevent->PressList, switchid, offval);
                    if (!transitionid.empty()) new dcSetValue(myevent->PressList, transitionid, "-1");
                    myevent = new dcKeyboardEvent(mycond->FalseList, key, keyascii);
                    new dcSetValue(myevent->PressList, switchid, switchonval);
                    if (!transitionid.empty()) new dcSetValue(myevent->PressList, transitionid, "1");
                }
                if (!bezelkey.empty())
                {
                    dcBezelEvent *myevent1 = new dcBezelEvent(mycond->TrueList, bezelkey);
                    new dcSetValue(myevent1->PressList, switchid, offval);
                    if (!transitionid.empty()) new dcSetValue(myevent1->PressList, transitionid, "-1");
                    dcBezelEvent *myevent2 = new dcBezelEvent(mycond->FalseList, bezelkey);
                    new dcSetValue(myevent2->PressList, switchid, switchonval);
                    if (!transitionid.empty()) new dcSetValue(myevent2->PressList, transitionid, "1");
                }
            }
            else
            {
                dcMouseEvent *mymouse = new dcMouseEvent(mySublist);
                new dcSetValue(mymouse->PressList, switchid, switchonval);
                if (!transitionid.empty()) new dcSetValue(mymouse->PressList, transitionid, "1");
                if (momentary)
                {
                    new dcSetValue(mymouse->ReleaseList, switchid, offval);
                    if (!transitionid.empty()) new dcSetValue(mymouse->ReleaseList, transitionid, "-1");
                }
                if (!key.empty() || !keyascii.empty())
                {
                    dcKeyboardEvent *myevent = new dcKeyboardEvent(mySublist, key, keyascii);
                    new dcSetValue(myevent->PressList, switchid, switchonval);
                    if (!transitionid.empty()) new dcSetValue(myevent->PressList, transitionid, "1");
                    if (momentary)
                    {
                        new dcSetValue(myevent->ReleaseList, switchid, offval);
                        if (!transitionid.empty()) new dcSetValue(myevent->ReleaseList, transitionid, "-1");
                    }
                }
                if (!bezelkey.empty())
                {
                    dcBezelEvent *myevent1 = new dcBezelEvent(mySublist, bezelkey);
                    new dcSetValue(myevent1->PressList, switchid, switchonval);
                    if (!transitionid.empty()) new dcSetValue(myevent1->PressList, transitionid, "1");
                    if (momentary)
                    {
                        new dcSetValue(myevent1->ReleaseList, switchid, offval);
                        if (!transitionid.empty()) new dcSetValue(myevent1->ReleaseList, transitionid, "-1");
                    }
                }
            }

            if (!transitionid.empty())
            {
                dcCondition *mylist1, *mylist2, *mylist3;

                mylist1 = new dcCondition(myitem, "eq", transitionid, "1");
                mylist2 = new dcCondition(mylist1->TrueList, "eq", indid, indonval);
                new dcSetValue(mylist2->TrueList, transitionid, "0");
                mylist3 = new dcCondition(mylist2->FalseList, "eq", switchid, switchonval);
                new dcSetValue(mylist3->FalseList, transitionid, "0");

                mylist1 = new dcCondition(myitem, "eq", transitionid, "-1");
                mylist2 = new dcCondition(mylist1->TrueList, "eq", indid, indonval);
                new dcSetValue(mylist2->FalseList, transitionid, "0");
                mylist3 = new dcCondition(mylist2->FalseList, "eq", switchid, switchoffval);
                new dcSetValue(mylist3->TrueList, transitionid, "0");
            }

            process_elements(myitem, node->children);
            transitionid.reset();
        }
        if (NodeCheck(node, "OnPress"))
        {
            dcParent *mySublist = myparent;
            if (!activeid.empty())
            {
                dcCondition *mycond = new dcCondition(myparent, "eq", activeid, activetrueval);
                mySublist = mycond->TrueList;
            }
            dcMouseEvent *mymouse = new dcMouseEvent(mySublist);
            process_elements(mymouse->PressList, node->children);
            if (!key.empty() || !keyascii.empty())
            {
                dcKeyboardEvent *myitem = new dcKeyboardEvent(mySublist, key, keyascii);
                process_elements(myitem->PressList, node->children);
            }
            if (!bezelkey.empty())
            {
                dcBezelEvent *myitem = new dcBezelEvent(mySublist, bezelkey);
                process_elements(myitem->PressList, node->children);
            }
        }
        if (NodeCheck(node, "OnRelease"))
        {
            dcParent *mySublist = myparent;
            if (!activeid.empty())
            {
                dcCondition *mycond = new dcCondition(myparent, "eq", activeid, activetrueval);
                mySublist = mycond->TrueList;
            }
            dcMouseEvent *mymouse = new dcMouseEvent(mySublist);
            process_elements(mymouse->ReleaseList, node->children);
            if (!key.empty() || !keyascii.empty())
            {
                dcKeyboardEvent *myitem = new dcKeyboardEvent(mySublist, key, keyascii);
                process_elements(myitem->PressList, node->children);
            }
            if (!bezelkey.empty())
            {
                dcBezelEvent *myitem = new dcBezelEvent(mySublist, bezelkey);
                process_elements(myitem->PressList, node->children);
            }
        }
        if (NodeCheck(node, "Active"))
        {
            dcCondition *myitem = new dcCondition(myparent, "eq", activeid, activetrueval);
            process_elements(myitem->TrueList, node->children);
        }
        if (NodeCheck(node, "Inactive"))
        {
            dcCondition *myitem = new dcCondition(myparent, "eq", activeid, activetrueval);
            process_elements(myitem->FalseList, node->children);
        }
        if (NodeCheck(node, "On"))
        {
            if (!transitionid.empty())
            {
                dcCondition *myitem = new dcCondition(myparent, "eq", transitionid, "0");
                dcCondition *mychild = new dcCondition(myitem->TrueList, "eq", indid, indonval);
                process_elements(mychild->TrueList, node->children);
            }
            else
            {
                dcCondition *myitem = new dcCondition(myparent, "eq", indid, indonval);
                process_elements(myitem->TrueList, node->children);
            }
        }
        if (NodeCheck(node, "Transition"))
        {
            if (!transitionid.empty())
            {
                dcCondition *myitem;
                myitem = new dcCondition(myparent, "eq", transitionid, "1");
                process_elements(myitem->TrueList, node->children);
                myitem = new dcCondition(myparent, "eq", transitionid, "-1");
                process_elements(myitem->TrueList, node->children);
            }
        }
        if (NodeCheck(node, "Off"))
        {
            if (!transitionid.empty())
            {
                dcCondition *myitem = new dcCondition(myparent, "eq", transitionid, "0");
                dcCondition *mychild = new dcCondition(myitem->TrueList, "eq", indid, indonval);
                process_elements(mychild->FalseList, node->children);
            }
            else
            {
                dcCondition *myitem = new dcCondition(myparent, "eq", indid, indonval);
                process_elements(myitem->FalseList, node->children);
            }
        }
        if (NodeCheck(node, "ADI"))
        {
            dcADI *myitem = new dcADI(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setOrigin(get_element_data(node, "OriginX"), get_element_data(node, "OriginY"));
            myitem->setBackgroundTexture(get_element_data(node, "CoverFile"));
            myitem->setBallTexture(get_element_data(node, "BallFile"));
            myitem->setRPY(get_element_data(node, "Roll"), get_element_data(node, "Pitch"), get_element_data(node, "Yaw"));
            myitem->setRPYerrors(get_element_data(node, "RollError"), get_element_data(node, "PitchError"), get_element_data(node, "YawError"));
            myitem->setRadius(get_element_data(node, "OuterRadius"), get_element_data(node, "BallRadius"));
            myitem->setChevron(get_element_data(node, "ChevronWidth"), get_element_data(node, "ChevronHeight"));
        }
        if (NodeCheck(node, "MouseMotion"))
        {
            new dcMouseMotion(myparent, get_element_data(node, "XVariable"), get_element_data(node, "YVariable"));
        }
        if (NodeCheck(node, "MouseEvent"))
        {
            dcMouseEvent *myitem = new dcMouseEvent(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setOrigin(get_element_data(node, "OriginX"), get_element_data(node, "OriginY"));
            bool subparent_found = false;
            for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
            {
                if (NodeCheck(subnode, "OnPress"))
                {
                    process_elements(myitem->PressList, subnode->children);
                    subparent_found = true;
                }
                if (NodeCheck(subnode, "OnRelease"))
                {
                    process_elements(myitem->ReleaseList, subnode->children);
                    subparent_found = true;
                }
            }
            // Assume "Press" if no subparent is found
            if (!subparent_found) process_elements(myitem->PressList, node->children);
        }
        if (NodeCheck(node, "KeyboardEvent"))
        {
            dcKeyboardEvent *myitem = new dcKeyboardEvent(myparent);
            myitem->setKey(get_element_data(node, "Key"));
            myitem->setKeyAscii(get_element_data(node, "KeyASCII"));
            bool subparent_found = false;
            for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
            {
                if (NodeCheck(subnode, "OnPress"))
                {
                    process_elements(myitem->PressList, subnode->children);
                    subparent_found = true;
                }
                if (NodeCheck(subnode, "OnRelease"))
                {
                    process_elements(myitem->ReleaseList, subnode->children);
                    subparent_found = true;
                }
            }
            // Assume "Press" if no subparent is found
            if (!subparent_found) process_elements(myitem->PressList, node->children);
        }
        if (NodeCheck(node, "BezelEvent"))
        {
            dcBezelEvent *myitem = new dcBezelEvent(myparent);
            myitem->setKey(get_element_data(node, "Key"));
            bool subparent_found = false;
            for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
            {
                if (NodeCheck(subnode, "OnPress"))
                {
                    process_elements(myitem->PressList, subnode->children);
                    subparent_found = true;
                }
                if (NodeCheck(subnode, "OnRelease"))
                {
                    process_elements(myitem->ReleaseList, subnode->children);
                    subparent_found = true;
                }
            }
            // Assume "Press" if no subparent is found
            if (!subparent_found) process_elements(myitem->PressList, node->children);
        }
    }

    return 0;
}
