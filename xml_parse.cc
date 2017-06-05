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
#include "mappings.hh"
#include "string_utils.hh"
#include "xml_utils.hh"
#include "xml_stringsub.hh"

extern void new_window(bool, int, int, int, int, char *);
extern struct node *new_panel(char *, char *, char *, char *);
extern struct node *new_container(struct node *, struct node **, char *, char *, char *, char *, char *, char *, char *, char *, char *);
extern struct node *new_isequal(struct node *, struct node **, const char *, char *, const char *);
extern struct node *new_vertex(struct node *, struct node **, char *, char *);
extern struct node *new_line(struct node *, struct node **, char *, char *);
extern struct node *new_polygon(struct node *, struct node **, char *, char *, char *);
extern struct node *new_rectangle(struct node *, struct node **, char *, char *, char *, char *, char *, char *, char *, char *, char *, char *);
extern struct node *new_circle(struct node *, struct node **, char *, char *, char *, char *, char *, char *, char *, char *, char *);
extern struct node *new_string(struct node *, struct node **, char *, char *, char *, char *, char *,
                               char *, char *, char *, char *, char *, char *, char *, char *);
extern struct node *new_image(struct node *, struct node **, char *, char *, char *, char *, char *, char *, char *, char *);
extern struct node *new_pixel_stream(struct node *, struct node **, char *, char *, char *, char *, char *, char *, char *, char *, char *, char *, char *, char *);
extern struct node *new_button(struct node *, struct node **, char *, char *, char *, char *, char *, char *, char *, char *,
                               char *, char *, char *, char *, char *, char *, char *, char *, char *, char *, char *);
extern struct node *new_adi(struct node *, struct node **, char *, char *, char *, char *, char *, char *, char *, char *, char *,
                                 char *, char *, char *, char *, char *, char *, char *, char *, char *);
extern struct node *new_mouseevent(struct node *, struct node **, char *, char *, char *, char *, char *, char *);
extern struct node *new_keyboardevent(struct node *, struct node **, char *, char *);
extern struct node *new_bezelevent(struct node *, struct node **, char *);
extern struct node *new_animation(struct node *, struct node **, char *);
extern struct node *new_setvalue(struct node *, struct node **, char *, char *, char *, char *, const char *);
extern struct ModifyValue get_setvalue_data(char *, char *, char *, char *, const char *);
extern bool CheckConditionLogic(int, int, void *, int, void *);
extern void UpdateValueLogic(int, int, void *, int, void *, int, void *, int, void *);
extern bool check_dynamic_element(const char *);

extern void DisplayPreInitStub(void *(*)(const char *));
extern void DisplayInitStub(void);
extern void DisplayLogicStub(void);
extern void DisplayCloseStub(void);

static int process_elements(struct node *, struct node **, xmlNodePtr);
static xmlNodePtr GetSubList(xmlNodePtr, char *, char *, char *);

extern appdata AppData;

static TrickCommModule *trickcomm = 0x0;
static EdgeCommModule *edgecomm = 0x0;
static CcsdsUdpCommModule *ccsdsudpcomm = 0x0;
static char *switchid, *switchonval, *switchoffval, *indid, *indonval, *activeid, *activetrueval, *transitionid, *key, *keyascii, *bezelkey;
static int id_count = 0, bufferID;
static bool preprocessing = true;


int ParseXMLFile(char *fullpath)
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

    if (process_elements(0, 0, root_element->children)) return (-1);

    XMLFileClose(mydoc);
    XMLEndParsing();

    // Return to the original working directory
    fchdir(mycwd);

    free(dirc);
    free(basec);

    return 0;
}

static int process_elements(struct node *parent, struct node **list, xmlNodePtr startnode)
{
    xmlNodePtr node, node1;
    struct node *curlist, **sublist, *data, *data1;
    char *id, *onval, *offval;
    int subnode_found;

    for (node = startnode; node; node = node->next)
    {
        if (NodeCheck(node, "Dummy")) process_elements(parent, list, node->children);
        if (NodeCheck(node, "Constant")) processConstantNode(node);
        if (NodeCheck(node, "Style")) processStyleNode(node);
        if (NodeCheck(node, "Defaults")) processDefaultsNode(node);
        if (NodeCheck(node, "Include"))
        {
            xmlDocPtr include_file;
            xmlNodePtr include_element;
            if (XMLFileOpen(&include_file, &include_element, get_node_content(node)))
            {
                warning_msg("Couldn't open include file " << get_node_content(node));
            }
            else
            {
                process_elements(parent, list, include_element);
                XMLFileClose(include_file);
            }
        }
        if (NodeCheck(node, "If"))
        {
            char *val = get_element_data(node, "Value");
            char *val1 = get_element_data(node, "Value1");
            char *val2 = get_element_data(node, "Value2");
            char *myoperator = get_element_data(node, "Operator");
            bool staticlogic = true;

            if (!val1) val1 = val;

            if (check_dynamic_element(val1) || check_dynamic_element(val2)) staticlogic = false;

            if (preprocessing || staticlogic) process_elements(parent, list, GetSubList(node, myoperator, val1, val2));
            else
            {
                subnode_found = 0;

                data = new_isequal(parent, list, myoperator, val1, val2);

                for (node1 = node->children; node1; node1 = node1->next)
                {
                    if (NodeCheck(node1, "True"))
                    {
                        process_elements(data, &(data->object.cond.TrueList), node1->children);
                        subnode_found = 1;
                    }
                    if (NodeCheck(node1, "False"))
                    {
                        process_elements(data, &(data->object.cond.FalseList), node1->children);
                        subnode_found = 1;
                    }
                }
                if (!subnode_found) // Assume "True" if no subnode is found
                {
                    process_elements(data, &(data->object.cond.TrueList), node->children);
                }
            }
        }
        if (NodeCheck(node, "Animation"))
        {
            data = new_animation(parent, list, get_element_data(node, "Duration"));
            process_elements(data, &(data->object.anim.SubList), node->children);
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
            else data = new_setvalue(parent, list,
                                     get_element_data(node, "Variable"),
                                     get_element_data(node, "Operator"),
                                     get_element_data(node, "MinimumValue"),
                                     get_element_data(node, "MaximumValue"),
                                     get_node_content(node));
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
            char *d_a = get_element_data(node, "DisconnectAction");
            if (d_a)
            {
                if (!strcasecmp(d_a, "Reconnect")) trickcomm->setReconnectOnDisconnect();
            }
            char *connectedvariable = get_element_data(node, "ConnectedVariable");
            if (connectedvariable) trickcomm->activeID = (int *)get_pointer(&connectedvariable[1]);
            process_elements(0, 0, node->children);
            trickcomm->finishInitialization();
            AppData.commlist.push_back(trickcomm);
        }
        if (NodeCheck(node, "FromTrick"))
        {
            if (trickcomm)
            {
                bufferID = TrickCommModule::FromTrick;
                process_elements(0, 0, node->children);
            }
        }
        if (NodeCheck(node, "ToTrick"))
        {
            if (trickcomm)
            {
                bufferID = TrickCommModule::ToTrick;
                process_elements(0, 0, node->children);
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
            char *connectedvariable = get_element_data(node, "ConnectedVariable");
            if (connectedvariable) edgecomm->activeID = (int *)get_pointer(&connectedvariable[1]);
            process_elements(0, 0, node->children);
            edgecomm->finishInitialization(get_element_data(node, "Host"), get_element_data(node, "Port"), StrToFloat(get_element_data(node, "DataRate"), 1.0));
            AppData.commlist.push_back(edgecomm);
        }
        if (NodeCheck(node, "FromEdge"))
        {
            if (edgecomm)
            {
                bufferID = EDGEIO_FROMEDGE;
                process_elements(0, 0, node->children);
            }
        }
        if (NodeCheck(node, "ToEdge"))
        {
            if (edgecomm)
            {
                bufferID = EDGEIO_TOEDGE;
                process_elements(0, 0, node->children);
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
            char *connectedvariable = get_element_data(node, "ConnectedVariable");
            if (connectedvariable) ccsdsudpcomm->activeID = (int *)get_pointer(&connectedvariable[1]);
            process_elements(0, 0, node->children);
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
            char *inhibitvariable = get_element_data(node, "InhibitVariable");
            AppData.canbus_inhibited = (int *)get_pointer(&inhibitvariable[1]);
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
            new_window(StrToBool(get_element_data(node, "FullScreen"), false),
                       StrToInt(get_element_data(node, "X"), 0),
                       StrToInt(get_element_data(node, "Y"), 0),
                       StrToInt(get_element_data(node, "Width"), 800),
                       StrToInt(get_element_data(node, "Height"), 800),
                       get_element_data(node, "ActiveDisplay"));
            process_elements(0, 0, node->children);
        }
        if (NodeCheck(node, "Panel"))
        {
            data = new_panel(get_element_data(node, "DisplayIndex"),
                                             get_element_data(node, "BackgroundColor"),
                                             get_element_data(node, "VirtualWidth"),
                                             get_element_data(node, "VirtualHeight"));
            preprocessing = false;
            process_elements(data, &(data->object.panel.SubList), node->children);
            preprocessing = true;
        }
        if (NodeCheck(node, "Container"))
        {
            data = new_container(parent, list,
                                 get_element_data(node, "X"),
                                 get_element_data(node, "Y"),
                                 get_element_data(node, "Width"),
                                 get_element_data(node, "Height"),
                                 get_element_data(node, "HorizontalAlign"),
                                 get_element_data(node, "VerticalAlign"),
                                 get_element_data(node, "VirtualWidth"),
                                 get_element_data(node, "VirtualHeight"),
                                 get_element_data(node, "Rotate"));
            process_elements(data, &(data->object.cont.SubList), node->children);
        }
        if (NodeCheck(node, "Vertex"))
        {
            data = new_vertex(parent, list,
                              get_element_data(node, "X"),
                              get_element_data(node, "Y"));
        }
        if (NodeCheck(node, "Line"))
        {
            data = new_line(parent, list,
                            get_element_data(node, "LineWidth"),
                            get_element_data(node, "Color"));
            process_elements(data, &(data->object.line.Vertices), node->children);
        }
        if (NodeCheck(node, "Polygon"))
        {
            data = new_polygon(parent, list,
                                 get_element_data(node, "FillColor"),
                                 get_element_data(node, "LineColor"),
                                 get_element_data(node, "LineWidth"));
            process_elements(data, &(data->object.poly.Vertices), node->children);
        }
        if (NodeCheck(node, "Rectangle"))
        {
            data = new_rectangle(parent, list,
                                 get_element_data(node, "X"),
                                 get_element_data(node, "Y"),
                                 get_element_data(node, "Width"),
                                 get_element_data(node, "Height"),
                                 get_element_data(node, "HorizontalAlign"),
                                 get_element_data(node, "VerticalAlign"),
                                 get_element_data(node, "Rotate"),
                                 get_element_data(node, "FillColor"),
                                 get_element_data(node, "LineColor"),
                                 get_element_data(node, "LineWidth"));
        }
        if (NodeCheck(node, "Circle"))
        {
            data = new_circle(parent, list,
                                 get_element_data(node, "X"),
                                 get_element_data(node, "Y"),
                                 get_element_data(node, "HorizontalAlign"),
                                 get_element_data(node, "VerticalAlign"),
                                 get_element_data(node, "Radius"),
                                 get_element_data(node, "Segments"),
                                 get_element_data(node, "FillColor"),
                                 get_element_data(node, "LineColor"),
                                 get_element_data(node, "LineWidth"));
        }
        if (NodeCheck(node, "String"))
        {
            data = new_string(parent, list,
                            get_element_data(node, "X"),
                            get_element_data(node, "Y"),
                            get_element_data(node, "Rotate"),
                            get_element_data(node, "Size"),
                            get_element_data(node, "HorizontalAlign"),
                            get_element_data(node, "VerticalAlign"),
                            get_element_data(node, "Color"),
                            get_element_data(node, "BackgroundColor"),
                            get_element_data(node, "ShadowOffset"),
                            get_element_data(node, "Font"),
                            get_element_data(node, "Face"),
                            get_element_data(node, "ForceMono"),
                            get_node_content(node));
        }
        if (NodeCheck(node, "Image"))
        {
            data = new_image(parent, list,
                      get_element_data(node, "X"),
                      get_element_data(node, "Y"),
                      get_element_data(node, "Width"),
                      get_element_data(node, "Height"),
                      get_element_data(node, "HorizontalAlign"),
                      get_element_data(node, "VerticalAlign"),
                      get_element_data(node, "Rotate"),
                      get_node_content(node));
        }
        if (NodeCheck(node, "PixelStream"))
        {
            data = new_pixel_stream(parent, list,
                      get_element_data(node, "X"),
                      get_element_data(node, "Y"),
                      get_element_data(node, "Width"),
                      get_element_data(node, "Height"),
                      get_element_data(node, "HorizontalAlign"),
                      get_element_data(node, "VerticalAlign"),
                      get_element_data(node, "Rotate"),
                      get_element_data(node, "Protocol"),
                      get_element_data(node, "Host"),
                      get_element_data(node, "Port"),
                      get_element_data(node, "SharedMemoryKey"),
                      get_element_data(node, "File"));
        }
        if (NodeCheck(node, "Button"))
        {
            key = get_element_data(node, "Key"),
            keyascii = get_element_data(node, "KeyASCII"),
            bezelkey = get_element_data(node, "BezelKey"),
            id = get_element_data(node, "Variable");
            onval = get_element_data(node, "On");
            offval = get_element_data(node, "Off");
            switchid = get_element_data(node, "SwitchVariable");
            switchonval = get_element_data(node, "SwitchOn");
            switchoffval = get_element_data(node, "SwitchOff");
            indid = get_element_data(node, "IndicatorVariable");
            indonval = get_element_data(node, "IndicatorOn");
            activeid = get_element_data(node, "ActiveVariable");
            activetrueval = get_element_data(node, "ActiveOn");

            if (!switchonval) switchonval = onval;
            if (!switchoffval) switchoffval = offval;
            if (!indonval) indonval = switchonval;

            if (!id && !switchid)
            {
                asprintf(&id, "@dcappVirtualVariable%d", id_count);
                id_count++;
                varlist_append(&id[1], "Integer", "0");
            }
            if (!switchid) switchid = id;
            if (!indid) indid = switchid;
            if (switchid != indid)
            {
                asprintf(&transitionid, "@dcappVirtualVariable%d", id_count);
                id_count++;
                varlist_append(&transitionid[1], "Integer", "0");
            }
            else transitionid = 0x0;

            data = new_button(parent, list,
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
            process_elements(data, &(data->object.cont.SubList), node->children);
            if (transitionid) free(transitionid);
        }
        if (NodeCheck(node, "OnPress"))
        {
            curlist = parent;
            sublist = list;
            if (activeid)
            {
                curlist = new_isequal(parent, list, "eq", activeid, activetrueval);
                sublist = &(curlist->object.cond.TrueList);
            }
            data = new_mouseevent(curlist, sublist, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
            process_elements(data, &(data->object.me.PressList), node->children);
            if (key || keyascii)
            {
                data1 = new_keyboardevent(curlist, sublist, key, keyascii);
                data1->object.ke.PressList = data->object.me.PressList;
            }
            if (bezelkey)
            {
                data1 = new_bezelevent(curlist, sublist, bezelkey);
                data1->object.be.PressList = data->object.me.PressList;
            }
        }
        if (NodeCheck(node, "OnRelease"))
        {
            curlist = parent;
            sublist = list;
            if (activeid)
            {
                curlist = new_isequal(parent, list, "eq", activeid, activetrueval);
                sublist = &(curlist->object.cond.TrueList);
            }
            data = new_mouseevent(curlist, sublist, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
            process_elements(data, &(data->object.me.ReleaseList), node->children);
            if (key || keyascii)
            {
                data1 = new_keyboardevent(curlist, sublist, key, keyascii);
                data1->object.ke.PressList = data->object.me.ReleaseList;
            }
            if (bezelkey)
            {
                data1 = new_bezelevent(curlist, sublist, bezelkey);
                data1->object.be.PressList = data->object.me.ReleaseList;
            }
        }
        if (NodeCheck(node, "Active"))
        {
            data = new_isequal(parent, list, "eq", activeid, activetrueval);
            process_elements(data, &(data->object.cond.TrueList), node->children);
        }
        if (NodeCheck(node, "Inactive"))
        {
            data = new_isequal(parent, list, "eq", activeid, activetrueval);
            process_elements(data, &(data->object.cond.FalseList), node->children);
        }
        if (NodeCheck(node, "On"))
        {
            if (transitionid)
            {
                data = new_isequal(parent, list, "eq", transitionid, "0");
                data1 = new_isequal(data, &(data->object.cond.TrueList), "eq", indid, indonval);
                process_elements(data1, &(data1->object.cond.TrueList), node->children);
            }
            else
            {
                data = new_isequal(parent, list, "eq", indid, indonval);
                process_elements(data, &(data->object.cond.TrueList), node->children);
            }
        }
        if (NodeCheck(node, "Transition"))
        {
            if (transitionid)
            {
                data = new_isequal(parent, list, "eq", transitionid, "1");
                process_elements(data, &(data->object.cond.TrueList), node->children);
                data = new_isequal(parent, list, "eq", transitionid, "-1");
                process_elements(data, &(data->object.cond.TrueList), node->children);
            }
        }
        if (NodeCheck(node, "Off"))
        {
            if (transitionid)
            {
                data = new_isequal(parent, list, "eq", transitionid, "0");
                data1 = new_isequal(data, &(data->object.cond.TrueList), "eq", indid, indonval);
                process_elements(data1, &(data1->object.cond.FalseList), node->children);
            }
            else
            {
                data = new_isequal(parent, list, "eq", indid, indonval);
                process_elements(data, &(data->object.cond.FalseList), node->children);
            }
        }
        if (NodeCheck(node, "ADI"))
        {
            data = new_adi(parent, list,
                           get_element_data(node, "X"),
                           get_element_data(node, "Y"),
                           get_element_data(node, "Width"),
                           get_element_data(node, "Height"),
                           get_element_data(node, "HorizontalAlign"),
                           get_element_data(node, "VerticalAlign"),
                           get_element_data(node, "OuterRadius"),
                           get_element_data(node, "BallRadius"),
                           get_element_data(node, "ChevronWidth"),
                           get_element_data(node, "ChevronHeight"),
                           get_element_data(node, "BallFile"),
                           get_element_data(node, "CoverFile"),
                           get_element_data(node, "Roll"),
                           get_element_data(node, "Pitch"),
                           get_element_data(node, "Yaw"),
                           get_element_data(node, "RollError"),
                           get_element_data(node, "PitchError"),
                           get_element_data(node, "YawError"));
        }
        if (NodeCheck(node, "MouseEvent"))
        {
            data = new_mouseevent(parent, list,
                                  get_element_data(node, "X"),
                                  get_element_data(node, "Y"),
                                  get_element_data(node, "Width"),
                                  get_element_data(node, "Height"),
                                  get_element_data(node, "HorizontalAlign"),
                                  get_element_data(node, "VerticalAlign"));
            subnode_found = 0;
            for (node1 = node->children; node1; node1 = node1->next)
            {
                if (NodeCheck(node1, "OnPress"))
                {
                    process_elements(data, &(data->object.me.PressList), node1->children);
                    subnode_found = 1;
                }
                if (NodeCheck(node1, "OnRelease"))
                {
                    process_elements(data, &(data->object.me.ReleaseList), node1->children);
                    subnode_found = 1;
                }
            }
            if (!subnode_found) // Assume "Press" if no subnode is found
            {
                process_elements(data, &(data->object.me.PressList), node->children);
            }
        }
        if (NodeCheck(node, "KeyboardEvent"))
        {
            data = new_keyboardevent(parent, list, get_element_data(node, "Key"), get_element_data(node, "KeyASCII"));
            subnode_found = 0;
            for (node1 = node->children; node1; node1 = node1->next)
            {
                if (NodeCheck(node1, "OnPress"))
                {
                    process_elements(data, &(data->object.ke.PressList), node1->children);
                    subnode_found = 1;
                }
                if (NodeCheck(node1, "OnRelease"))
                {
                    process_elements(data, &(data->object.ke.ReleaseList), node1->children);
                    subnode_found = 1;
                }
            }
            if (!subnode_found) // Assume "Press" if no subnode is found
            {
                process_elements(data, &(data->object.ke.PressList), node->children);
            }
        }
        if (NodeCheck(node, "BezelEvent"))
        {
            data = new_bezelevent(parent, list, get_element_data(node, "Key"));
            subnode_found = 0;
            for (node1 = node->children; node1; node1 = node1->next)
            {
                if (NodeCheck(node1, "OnPress"))
                {
                    process_elements(data, &(data->object.be.PressList), node1->children);
                    subnode_found = 1;
                }
                if (NodeCheck(node1, "OnRelease"))
                {
                    process_elements(data, &(data->object.be.ReleaseList), node1->children);
                    subnode_found = 1;
                }
            }
            if (!subnode_found) // Assume "Press" if no subnode is found
            {
                process_elements(data, &(data->object.be.PressList), node->children);
            }
        }
    }

    return 0;
}

static xmlNodePtr GetSubList(xmlNodePtr node, char *opspec, char *val1, char *val2)
{
    bool myflag = 0;
    xmlNodePtr node1;
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

    for (node1 = node->children; node1; node1 = node1->next)
    {
        if (myflag && NodeCheck(node1, "True")) return node1->children;
        if (!myflag && NodeCheck(node1, "False")) return node1->children;
    }

    if (myflag) return node->children;  // Assume "True" if no subnode is found
    else return 0x0;
}
