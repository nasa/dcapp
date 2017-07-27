#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <strings.h>
#include <libgen.h>
#include <fcntl.h>
#include <dlfcn.h>
#include "basicutils/msg.hh"
#include "trick/trickcomm.hh"
#include "edge/edgecomm.hh"
#include "ccsds/ccsds_udp_comm.hh"
#include "can/CAN.hh"
#include "uei/UEI.hh"
#include "varlist.hh"
#include "nodes.hh"
#include "opengl_draw.hh"
#include "string_utils.hh"
#include "xml_utils.hh"
#include "xml_stringsub.hh"

extern void window_init(bool, int , int, int, int);
extern int *getIntegerPointer(const char *);

extern struct node *new_container(struct node *, struct node **);
extern struct node *new_isequal(dcCondition **, struct node *, struct node **, const char *, const char *, const char *);
extern struct node *new_button(dcContainer *, struct node *, struct node **, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *,
                               const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *);
extern struct node *new_mouseevent(dcMouseEvent **, dcParent *, struct node *, struct node **, const char *, const char *, const char *, const char *, const char *, const char *);
extern struct node *new_keyboardevent(dcKeyboardEvent **, struct node *, struct node **, const char *, const char *);
extern struct node *new_bezelevent(dcBezelEvent **, struct node *, struct node **, const char *);
extern dcSetValue *new_setvalue(struct node *, struct node **, const char *, const char *, const char *, const char *, const char *);
extern struct ModifyValue get_setvalue_data(const char *, const char *, const char *, const char *, const char *);
extern bool CheckConditionLogic(int, int, const void *, int, const void *);
extern void UpdateValueLogic(int, int, void *, int, void *, int, void *, int, void *);
extern bool check_dynamic_element(const char *);

extern void DisplayPreInitStub(void *(*)(const char *));
extern void DisplayInitStub(void);
extern void DisplayLogicStub(void);
extern void DisplayCloseStub(void);

static int process_elements(dcParent *, struct node *, struct node **, xmlNodePtr);
static xmlNodePtr GetSubList(xmlNodePtr, const char *, const char *, const char *);

extern appdata AppData;

static TrickCommModule *trickcomm = 0x0;
static EdgeCommModule *edgecomm = 0x0;
static CcsdsUdpCommModule *ccsdsudpcomm = 0x0;
static char *transitionid;
static const char *indid, *indonval, *activeid, *activetrueval, *key, *keyascii, *bezelkey;
static int bufferID;
static bool preprocessing = true;


int ParseXMLFile(const char *fullpath)
{
    char *dirc, *basec, *bname, *dname;
    int mycwd;
    xmlDocPtr mydoc;
    xmlNodePtr root_element;

    if (!fullpath) return (-1);

    dirc = strdup(fullpath);
    basec = strdup(fullpath);
    dname = dirname(dirc);
    bname = basename(basec);

    // Store cwd for future use
    mycwd = open(".", O_RDONLY);

    // Move to directory containing the specfile by default
    if (dname) chdir(dname);

    if (XMLFileOpen(&mydoc, &root_element, bname)) return (-1);

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

    if (process_elements(0x0, 0, 0, root_element->children)) return (-1);

    XMLFileClose(mydoc);
    XMLEndParsing();

    // Return to the original working directory
    fchdir(mycwd);

    free(dirc);
    free(basec);

    return 0;
}

static int process_elements(dcParent *myparent, struct node *parent, struct node **list, xmlNodePtr startnode)
{
    xmlNodePtr node;
    struct node *data;

    for (node = startnode; node; node = node->next)
    {
        if (NodeCheck(node, "Dummy")) process_elements(myparent, parent, list, node->children);
        if (NodeCheck(node, "Constant")) processConstantNode(node);
        if (NodeCheck(node, "Style")) processStyleNode(node);
        if (NodeCheck(node, "Defaults")) processDefaultsNode(node);
        if (NodeCheck(node, "Include"))
        {
            xmlDocPtr include_file;
            xmlNodePtr include_element;
            const char *include_filename = get_node_content(node);
            if (XMLFileOpen(&include_file, &include_element, include_filename))
            {
                warning_msg("Couldn't open include file " << include_filename);
            }
            else
            {
                process_elements(myparent, parent, list, include_element);
                XMLFileClose(include_file);
            }
        }
        if (NodeCheck(node, "If"))
        {
            const char *val = get_element_data(node, "Value");
            const char *val1 = get_element_data(node, "Value1");
            const char *val2 = get_element_data(node, "Value2");
            const char *myoperator = get_element_data(node, "Operator");

            if (!val1) val1 = val;

            if (preprocessing || (!check_dynamic_element(val1) && !check_dynamic_element(val2)))
                process_elements(myparent, parent, list, GetSubList(node, myoperator, val1, val2));
            else
            {
                bool subparent_found = false;

dcCondition *myitem;
                data = new_isequal(&myitem, parent, list, myoperator, val1, val2);
myparent->addChild(myitem);

                for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
                {
                    if (NodeCheck(subnode, "True"))
                    {
                        process_elements(myitem->TrueList, data, &(data->object.cond.TrueList), subnode->children);
                        subparent_found = true;
                    }
                    if (NodeCheck(subnode, "False"))
                    {
                        process_elements(myitem->FalseList, data, &(data->object.cond.FalseList), subnode->children);
                        subparent_found = true;
                    }
                }
                // Assume "True" if no subparent is found
                if (!subparent_found) process_elements(myitem->TrueList, data, &(data->object.cond.TrueList), node->children);
            }
        }
        if (NodeCheck(node, "Animation"))
        {
            dcAnimate *myitem = new dcAnimate(myparent);
            myitem->setDuration(get_element_data(node, "Duration"));
            process_elements(myitem, data, &(data->object.anim.SubList), node->children);
        }
        if (NodeCheck(node, "Set"))
        {
            if (preprocessing)
            {
                struct ModifyValue myset = get_setvalue_data(get_element_data(node, "Variable"),
                                                             get_element_data(node, "Operator"),
                                                             get_element_data(node, "MinimumValue"),
                                                             get_element_data(node, "MaximumValue"),
                                                             get_node_content(node));
                if (myset.datatype1 != UNDEFINED_TYPE) UpdateValueLogic(myset.optype,
                                                                   myset.datatype1, myset.var,
                                                                   myset.datatype2, myset.val,
                                                                   myset.mindatatype, myset.min,
                                                                   myset.maxdatatype, myset.max);
            }
            else
{
dcSetValue *myitem = new_setvalue(parent, list,
                                     get_element_data(node, "Variable"),
                                     get_element_data(node, "Operator"),
                                     get_element_data(node, "MinimumValue"),
                                     get_element_data(node, "MaximumValue"),
                                     get_node_content(node));
myparent->addChild(myitem);
}
        }
        if (NodeCheck(node, "Variable"))
        {
            varlist_append(get_node_content(node), get_element_data(node, "Type"), get_element_data(node, "InitialValue"));
        }
        if (NodeCheck(node, "TrickIo"))
        {
            trickcomm = new TrickCommModule;
            trickcomm->setHost(get_element_data(node, "Host"));
            trickcomm->setPort(StrToInt(get_element_data(node, "Port"), 0));
            trickcomm->setDataRate(get_element_data(node, "DataRate"));
            const char *d_a = get_element_data(node, "DisconnectAction");
            if (d_a)
            {
                if (!strcasecmp(d_a, "Reconnect")) trickcomm->setReconnectOnDisconnect();
            }
            trickcomm->activeID = (int *)get_pointer(get_element_data(node, "ConnectedVariable"));
            process_elements(myparent, 0, 0, node->children);
            trickcomm->finishInitialization();
            AppData.commlist.push_back(trickcomm);
        }
        if (NodeCheck(node, "FromTrick"))
        {
            if (trickcomm)
            {
                bufferID = TrickCommModule::FromTrick;
                process_elements(myparent, 0, 0, node->children);
            }
        }
        if (NodeCheck(node, "ToTrick"))
        {
            if (trickcomm)
            {
                bufferID = TrickCommModule::ToTrick;
                process_elements(myparent, 0, 0, node->children);
            }
        }
        if (NodeCheck(node, "TrickVariable"))
        {
            if (trickcomm)
            {
                trickcomm->addParameter(bufferID, get_node_content(node), get_element_data(node, "Name"), get_element_data(node, "Units"), get_element_data(node, "InitializationOnly"), 0);
            }
        }
        if (NodeCheck(node, "TrickMethod"))
        {
            if (trickcomm && bufferID == TrickCommModule::ToTrick)
            {
                trickcomm->addParameter(bufferID, get_node_content(node), get_element_data(node, "Name"), 0x0, 0x0, 1);
            }
        }
        if (NodeCheck(node, "EdgeIo"))
        {
            edgecomm = new EdgeCommModule;
            edgecomm->activeID = (int *)get_pointer(get_element_data(node, "ConnectedVariable"));
            process_elements(myparent, 0, 0, node->children);
            edgecomm->finishInitialization(get_element_data(node, "Host"), get_element_data(node, "Port"), StrToFloat(get_element_data(node, "DataRate"), 1.0));
            AppData.commlist.push_back(edgecomm);
        }
        if (NodeCheck(node, "FromEdge"))
        {
            if (edgecomm)
            {
                bufferID = EDGEIO_FROMEDGE;
                process_elements(myparent, 0, 0, node->children);
            }
        }
        if (NodeCheck(node, "ToEdge"))
        {
            if (edgecomm)
            {
                bufferID = EDGEIO_TOEDGE;
                process_elements(myparent, 0, 0, node->children);
            }
        }
        if (NodeCheck(node, "EdgeVariable"))
        {
            if (edgecomm)
            {
                edgecomm->addParameter(bufferID, get_node_content(node), get_element_data(node, "RcsCommand"));
            }
        }
        if (NodeCheck(node, "CcsdsUdpIo"))
        {
            ccsdsudpcomm = new CcsdsUdpCommModule;
            ccsdsudpcomm->activeID = (int *)get_pointer(get_element_data(node, "ConnectedVariable"));
            process_elements(myparent, 0, 0, node->children);
            AppData.commlist.push_back(ccsdsudpcomm);
        }
        if (NodeCheck(node, "CcsdsUdpRead"))
        {
            if (ccsdsudpcomm)
            {
                ccsdsudpcomm->read_initialize(get_element_data(node, "Host"), StrToInt(get_element_data(node, "Port"), 0), StrToInt(get_element_data(node, "ThreadID"), 0), StrToFloat(get_element_data(node, "Rate"), 0), StrToBool(get_element_data(node, "LittleEndian"), false));
            }
        }
        if (NodeCheck(node, "CcsdsUdpWrite"))
        {
            if (ccsdsudpcomm)
            {
                ccsdsudpcomm->write_initialize(get_element_data(node, "Host"), StrToInt(get_element_data(node, "Port"), 0));
            }
        }
        if (NodeCheck(node, "CAN"))
        {
            CAN_init(get_element_data(node, "Network"), get_element_data(node, "ButtonID"), get_element_data(node, "ControlID"));
            AppData.canbus_inhibited = (int *)get_pointer(get_element_data(node, "InhibitVariable"));
        }
        if (NodeCheck(node, "UEI"))
        {
            UEI_init(get_element_data(node, "Host"), get_element_data(node, "Port"), get_element_data(node, "BezelID"));
        }
        if (NodeCheck(node, "DisplayLogic"))
        {
            char *error;
            void *so_handler;

            debug_msg("Loading " << get_node_content(node) << "...");
            so_handler = dlopen(get_node_content(node), RTLD_NOW);
            if (!so_handler)
            {
                error_msg(dlerror());
                return (-1);
            }

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
        if (NodeCheck(node, "Window"))
        {
            AppData.force_update = StrToFloat(get_element_data(node, "ForceUpdate"), 60);
            window_init(StrToBool(get_element_data(node, "FullScreen"), false),
                        StrToInt(get_element_data(node, "X"), 0),
                        StrToInt(get_element_data(node, "Y"), 0),
                        StrToInt(get_element_data(node, "Width"), 800),
                        StrToInt(get_element_data(node, "Height"), 800));
            graphics_init();
            AppData.toplevel = new dcWindow();
            AppData.toplevel->setActiveDisplay(get_element_data(node, "ActiveDisplay"));
            process_elements(AppData.toplevel, 0, 0, node->children);
        }
        if (NodeCheck(node, "Panel"))
        {
            dcPanel *myitem = new dcPanel(myparent);
            myitem->setID(get_element_data(node, "DisplayIndex"));
            myitem->setColor(get_element_data(node, "BackgroundColor"));
            myitem->setOrtho(get_element_data(node, "VirtualWidth"), get_element_data(node, "VirtualHeight"));
            preprocessing = false;
data = (struct node *)calloc(1, sizeof(struct node)); // TODO: remove once sublists are gone
            process_elements(myitem, data, &(data->object.panel.SubList), node->children);
            preprocessing = true;
        }
        if (NodeCheck(node, "Container"))
        {
            dcContainer *myitem = new dcContainer(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setVirtualSize(get_element_data(node, "VirtualWidth"), get_element_data(node, "VirtualHeight"));
data = new_container(parent, list);
            process_elements(myitem, data, &(data->object.cont.SubList), node->children);
        }
        if (NodeCheck(node, "Vertex"))
        {
            dcVertex *myitem = new dcVertex(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
        }
        if (NodeCheck(node, "Line"))
        {
            dcLine *myitem = new dcLine(myparent);
            myitem->setLineWidth(get_element_data(node, "LineWidth"));
            myitem->setColor(get_element_data(node, "Color"));
            process_elements(myitem, data, &(data->object.line.Vertices), node->children);
        }
        if (NodeCheck(node, "Polygon"))
        {
            dcPolygon *myitem = new dcPolygon(myparent);
            myitem->setFillColor(get_element_data(node, "FillColor"));
            myitem->setLineColor(get_element_data(node, "LineColor"));
            myitem->setLineWidth(get_element_data(node, "LineWidth"));
            process_elements(myitem, data, &(data->object.poly.Vertices), node->children);
        }
        if (NodeCheck(node, "Rectangle"))
        {
            dcRectangle *myitem = new dcRectangle(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setFillColor(get_element_data(node, "FillColor"));
            myitem->setLineColor(get_element_data(node, "LineColor"));
            myitem->setLineWidth(get_element_data(node, "LineWidth"));
        }
        if (NodeCheck(node, "Circle"))
        {
            dcCircle *myitem = new dcCircle(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setFillColor(get_element_data(node, "FillColor"));
            myitem->setLineColor(get_element_data(node, "LineColor"));
            myitem->setLineWidth(get_element_data(node, "LineWidth"));
            myitem->setRadius(get_element_data(node, "Radius"));
            myitem->setSegments(get_element_data(node, "Segments"));
        }
        if (NodeCheck(node, "String"))
        {
            dcString *myitem = new dcString(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize("0", get_element_data(node, "Size"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
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
            myitem->setTexture(get_node_content(node));
        }
        if (NodeCheck(node, "PixelStream"))
        {
            dcPixelStream *myitem = new dcPixelStream(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setProtocol(get_element_data(node, "Protocol"), get_element_data(node, "Host"), get_element_data(node, "Port"), get_element_data(node, "SharedMemoryKey"), get_element_data(node, "File"));
        }
        if (NodeCheck(node, "Button"))
        {
            key = get_element_data(node, "Key");
            keyascii = get_element_data(node, "KeyASCII");
            bezelkey = get_element_data(node, "BezelKey");
            const char *buttonid = get_element_data(node, "Variable");
            const char *buttononval = get_element_data(node, "On");
            const char *buttonoffval = get_element_data(node, "Off");
            const char *switchid = get_element_data(node, "SwitchVariable");
            const char *switchonval = get_element_data(node, "SwitchOn");
            const char *switchoffval = get_element_data(node, "SwitchOff");
            indid = get_element_data(node, "IndicatorVariable");
            indonval = get_element_data(node, "IndicatorOn");
            activeid = get_element_data(node, "ActiveVariable");
            activetrueval = get_element_data(node, "ActiveOn");

            if (!switchonval) switchonval = buttononval;
            if (!switchoffval) switchoffval = buttonoffval;
            if (!indonval) indonval = switchonval;

            if (!buttonid && !switchid) buttonid = strdup(create_virtual_variable("Integer", "0"));
            if (!switchid) switchid = buttonid;
            if (!indid) indid = switchid;
            if (switchid != indid) transitionid = create_virtual_variable("Integer", "0");
            else transitionid = 0x0;

            dcContainer *myitem = new dcContainer(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
data = new_button(myitem, parent, list,
      get_element_data(node, "X"),
      get_element_data(node, "Y"),
      get_element_data(node, "Width"),
      get_element_data(node, "Height"),
      get_element_data(node, "HorizontalAlign"),
      get_element_data(node, "VerticalAlign"),
      get_element_data(node, "Rotate"),
      get_element_data(node, "Type"),
      switchid,
      switchonval,
      switchoffval,
      indid,
      indonval,
      activeid,
      activetrueval,
      transitionid,
      key,
      keyascii,
      bezelkey);
            process_elements(myitem, data, &(data->object.cont.SubList), node->children);
            if (transitionid) free(transitionid);
        }
        if (NodeCheck(node, "OnPress"))
        {
            struct node *curlist = parent;
            struct node **sublist = list;
dcParent *mySublist = myparent;
dcCondition *mycond;
            if (activeid)
            {
                curlist = new_isequal(&mycond, parent, list, "eq", activeid, activetrueval);
myparent->addChild(mycond);
mySublist = mycond->TrueList;
                sublist = &(curlist->object.cond.TrueList);
            }
dcMouseEvent *mymouse;
            data = new_mouseevent(&mymouse, mySublist, curlist, sublist, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
//need to do this for functionality
            process_elements(mymouse->PressList, data, &(data->object.me.PressList), node->children);
            if (key || keyascii)
{
dcKeyboardEvent *myitem;
                new_keyboardevent(&myitem, curlist, sublist, key, keyascii);
//                data->object.ke.PressList = data->object.me.PressList;
mySublist->addChild(myitem);
//set KeyboardEvent PressList to MouseEvent PressList (see the hack above to remove the line below)
process_elements(myitem->PressList, data, &(data->object.me.PressList), node->children);
}
            if (bezelkey)
            {
dcBezelEvent *myitem;
                new_bezelevent(&myitem, curlist, sublist, bezelkey);
//                data->object.be.PressList = data->object.me.PressList;
mySublist->addChild(myitem);
//set BezelEvent PressList to MouseEvent PressList (see the hack above to remove the line below)
process_elements(myitem->PressList, data, &(data->object.me.PressList), node->children);
            }
        }
        if (NodeCheck(node, "OnRelease"))
        {
            struct node *curlist = parent;
            struct node **sublist = list;
dcParent *mySublist = myparent;
dcCondition *mycond;
            if (activeid)
            {
                curlist = new_isequal(&mycond, parent, list, "eq", activeid, activetrueval);
myparent->addChild(mycond);
mySublist = mycond->TrueList;
                sublist = &(curlist->object.cond.TrueList);
            }
dcMouseEvent *mymouse;
            data = new_mouseevent(&mymouse, mySublist, curlist, sublist, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
//need to do this for functionality
            process_elements(mymouse->ReleaseList, data, &(data->object.me.ReleaseList), node->children);
            if (key || keyascii)
{
dcKeyboardEvent *myitem;
                new_keyboardevent(&myitem, curlist, sublist, key, keyascii);
//                data->object.ke.PressList = data->object.me.ReleaseList;
mySublist->addChild(myitem);
//set KeyboardEvent PressList to MouseEvent PressList (see the hack above to remove the line below)
process_elements(myitem->PressList, data, &(data->object.me.PressList), node->children);
}
            if (bezelkey)
            {
dcBezelEvent *myitem;
                new_bezelevent(&myitem, curlist, sublist, bezelkey);
//                data->object.be.PressList = data->object.me.ReleaseList;
mySublist->addChild(myitem);
//set BezelEvent PressList to MouseEvent PressList (see the hack above to remove the line below)
process_elements(myitem->PressList, data, &(data->object.me.PressList), node->children);
            }
        }
        if (NodeCheck(node, "Active"))
        {
dcCondition *myitem;
            data = new_isequal(&myitem, parent, list, "eq", activeid, activetrueval);
myparent->addChild(myitem);
            process_elements(myitem->TrueList, data, &(data->object.cond.TrueList), node->children);
        }
        if (NodeCheck(node, "Inactive"))
        {
dcCondition *myitem;
            data = new_isequal(&myitem, parent, list, "eq", activeid, activetrueval);
myparent->addChild(myitem);
            process_elements(myitem->FalseList, data, &(data->object.cond.FalseList), node->children);
        }
        if (NodeCheck(node, "On"))
        {
            if (transitionid)
            {
dcCondition *myitem, *mychild;
                data = new_isequal(&myitem, parent, list, "eq", transitionid, "0");
myparent->addChild(myitem);
                struct node *iseq = new_isequal(&mychild, data, &(data->object.cond.TrueList), "eq", indid, indonval);
myitem->TrueList->addChild(mychild);
                process_elements(mychild->TrueList, iseq, &(iseq->object.cond.TrueList), node->children);
            }
            else
            {
dcCondition *myitem;
                data = new_isequal(&myitem, parent, list, "eq", indid, indonval);
myparent->addChild(myitem);
                process_elements(myitem->TrueList, data, &(data->object.cond.TrueList), node->children);
            }
        }
        if (NodeCheck(node, "Transition"))
        {
            if (transitionid)
            {
dcCondition *myitem;
                data = new_isequal(&myitem, parent, list, "eq", transitionid, "1");
myparent->addChild(myitem);
                process_elements(myitem->TrueList, data, &(data->object.cond.TrueList), node->children);
                data = new_isequal(&myitem, parent, list, "eq", transitionid, "-1");
myparent->addChild(myitem);
                process_elements(myitem->TrueList, data, &(data->object.cond.TrueList), node->children);
            }
        }
        if (NodeCheck(node, "Off"))
        {
            if (transitionid)
            {
dcCondition *myitem, *mychild;
                data = new_isequal(&myitem, parent, list, "eq", transitionid, "0");
myparent->addChild(myitem);
                struct node *iseq = new_isequal(&mychild, data, &(data->object.cond.TrueList), "eq", indid, indonval);
myitem->TrueList->addChild(mychild);
                process_elements(mychild->FalseList, iseq, &(iseq->object.cond.FalseList), node->children);
            }
            else
            {
dcCondition *myitem;
                data = new_isequal(&myitem, parent, list, "eq", indid, indonval);
myparent->addChild(myitem);
                process_elements(myitem->FalseList, data, &(data->object.cond.FalseList), node->children);
            }
        }
        if (NodeCheck(node, "ADI"))
        {
            dcADI *myitem = new dcADI(myparent);
            myitem->setPosition(get_element_data(node, "X"), get_element_data(node, "Y"));
            myitem->setSize(get_element_data(node, "Width"), get_element_data(node, "Height"));
            myitem->setRotation(get_element_data(node, "Rotate"));
            myitem->setAlignment(get_element_data(node, "HorizontalAlign"), get_element_data(node, "VerticalAlign"));
            myitem->setBackgrountTexture(get_element_data(node, "CoverFile"));
            myitem->setBallTexture(get_element_data(node, "BallFile"));
        }
        if (NodeCheck(node, "MouseEvent"))
        {
            dcMouseEvent *mymouse;
            data = new_mouseevent(&mymouse, myparent, parent, list,
                                  get_element_data(node, "X"),
                                  get_element_data(node, "Y"),
                                  get_element_data(node, "Width"),
                                  get_element_data(node, "Height"),
                                  get_element_data(node, "HorizontalAlign"),
                                  get_element_data(node, "VerticalAlign"));
            bool subparent_found = false;
            for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
            {
                if (NodeCheck(subnode, "OnPress"))
                {
                    process_elements(mymouse->PressList, data, &(data->object.me.PressList), subnode->children);
                    subparent_found = true;
                }
                if (NodeCheck(subnode, "OnRelease"))
                {
                    process_elements(mymouse->ReleaseList, data, &(data->object.me.ReleaseList), subnode->children);
                    subparent_found = true;
                }
            }
            // Assume "Press" if no subparent is found
            if (!subparent_found) process_elements(mymouse->PressList, data, &(data->object.me.PressList), node->children);
        }
        if (NodeCheck(node, "KeyboardEvent"))
        {
dcKeyboardEvent *myitem;
            data = new_keyboardevent(&myitem, parent, list, get_element_data(node, "Key"), get_element_data(node, "KeyASCII"));
myparent->addChild(myitem);
            bool subparent_found = false;
            for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
            {
                if (NodeCheck(subnode, "OnPress"))
                {
                    process_elements(myitem->PressList, data, &(data->object.ke.PressList), subnode->children);
                    subparent_found = true;
                }
                if (NodeCheck(subnode, "OnRelease"))
                {
                    process_elements(myitem->ReleaseList, data, &(data->object.ke.ReleaseList), subnode->children);
                    subparent_found = true;
                }
            }
            // Assume "Press" if no subparent is found
            if (!subparent_found) process_elements(myitem->PressList, data, &(data->object.ke.PressList), node->children);
        }
        if (NodeCheck(node, "BezelEvent"))
        {
dcBezelEvent *myitem;
            data = new_bezelevent(&myitem, parent, list, get_element_data(node, "Key"));
myparent->addChild(myitem);
            bool subparent_found = false;
            for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
            {
                if (NodeCheck(subnode, "OnPress"))
                {
                    process_elements(myitem->PressList, data, &(data->object.be.PressList), subnode->children);
                    subparent_found = true;
                }
                if (NodeCheck(subnode, "OnRelease"))
                {
                    process_elements(myitem->ReleaseList, data, &(data->object.be.ReleaseList), subnode->children);
                    subparent_found = true;
                }
            }
            // Assume "Press" if no subparent is found
            if (!subparent_found) process_elements(myitem->PressList, data, &(data->object.be.PressList), node->children);
        }
    }

    return 0;
}

static xmlNodePtr GetSubList(xmlNodePtr node, const char *opspec, const char *val1, const char *val2)
{
    bool myflag = 0;
    xmlNodePtr subnode;
    int optype = Simple;

    if (opspec)
    {
        if (!strcasecmp(opspec, "eq")) optype = IfEquals;
        else if (!strcasecmp(opspec, "ne")) optype = IfNotEquals;
        else if (!strcasecmp(opspec, "gt")) optype = IfGreaterThan;
        else if (!strcasecmp(opspec, "lt")) optype = IfLessThan;
        else if (!strcasecmp(opspec, "ge")) optype = IfGreaterOrEquals;
        else if (!strcasecmp(opspec, "le")) optype = IfLessOrEquals;
    }

    myflag = CheckConditionLogic(optype, STRING_TYPE, val1, STRING_TYPE, val2);

    for (subnode = node->children; subnode; subnode = subnode->next)
    {
        if (myflag && NodeCheck(subnode, "True")) return subnode->children;
        if (!myflag && NodeCheck(subnode, "False")) return subnode->children;
    }

    if (myflag) return node->children;  // Assume "True" if no subparent is found
    else return 0x0;
}
