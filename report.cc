#include <fstream>
#include <map>
#include <string>
#include "xml_utils.hh"
#include "xml_stringsub.hh"

static std::ofstream logfile;
static std::map<char, std::string> xforms;
static std::string xmlreserved;
static int indent=0;

static void log_start_tag(const std::string &instr, bool sameline=false)
{
    for (int i=0; i<indent; i++) logfile << "    ";
    logfile << "<" << instr << ">";
    if (!sameline)
    {
        logfile << std::endl;
        indent++;
    }
}

static void log_open_start_tag(const std::string &instr)
{
    for (int i=0; i<indent; i++) logfile << "    ";
    logfile << "<" << instr;
}

static void log_close_start_tag(bool sameline=false)
{
    logfile << ">";
    if (!sameline)
    {
        logfile << std::endl;
        indent++;
    }
}

static void log_element(const std::string &label, std::string value, bool printifempty=false)
{
    if (!value.empty() || printifempty)
    {
        size_t pos = 0;
        while (std::string::npos != (pos = value.find_first_of(xmlreserved, pos)))
        {
            value.replace(pos, 1, xforms[value[pos]]);
            pos++;
        }

        logfile << " " << label << "=\"" << value << "\"";
    }
}

static void log_content(std::string instr)
{
    size_t pos = 0;
    while (std::string::npos != (pos = instr.find_first_of(xmlreserved, pos)))
    {
        instr.replace(pos, 1, xforms[instr[pos]]);
        pos++;
    }

    logfile << instr;
}

static void log_end_tag(const std::string &instr, bool sameline=false)
{
    if (!sameline)
    {
        indent--;
        for (int i=0; i<indent; i++) logfile << "    ";
    }
    logfile << "</" << instr << ">" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

void start_logging(std::string &fname)
{
    // Set up XML character transformation map
    xforms['&']  = std::string("&amp;");
    xforms['\''] = std::string("&apos;");
    xforms['"']  = std::string("&quot;");
    xforms['>']  = std::string("&gt;");
    xforms['<']  = std::string("&lt;");

    for (auto it = xforms.begin(); it != xforms.end(); it++) xmlreserved += it->first;

    logfile.open(fname);
    log_start_tag("DCAPP");
}

void end_logging(void)
{
    log_end_tag("DCAPP");
    logfile.close();
}

void log_node_start(xmlNodePtr node)
{
    if (logfile.is_open())
    {
        if (NodeCheck(node, "TrickIo"))
        {
            log_open_start_tag("TrickIo");
            log_element("Host", get_element_data(node, "Host"));
            log_element("Port", get_element_data(node, "Port"));
            log_element("DataRate", get_element_data(node, "DataRate"));
            log_element("ConnectedVariable", get_element_data(node, "ConnectedVariable"));
            log_element("DisconnectAction", get_element_data(node, "DisconnectAction"));
            log_close_start_tag();
        }
        if (NodeCheck(node, "FromTrick"))
        {
            log_start_tag("FromTrick");
        }
        if (NodeCheck(node, "ToTrick"))
        {
            log_start_tag("ToTrick");
        }
        if (NodeCheck(node, "EdgeIo"))
        {
            log_open_start_tag("EdgeIo");
            log_element("Host", get_element_data(node, "Host"));
            log_element("Port", get_element_data(node, "Port"));
            log_element("DataRate", get_element_data(node, "DataRate"));
            log_element("ConnectedVariable", get_element_data(node, "ConnectedVariable"));
            log_close_start_tag();
        }
        if (NodeCheck(node, "FromEdge"))
        {
            log_start_tag("FromEdge");
        }
        if (NodeCheck(node, "ToEdge"))
        {
            log_start_tag("ToEdge");
        }
        if (NodeCheck(node, "Window"))
        {
            log_open_start_tag("Window");
            log_element("X", get_element_data(node, "X"));
            log_element("Y", get_element_data(node, "Y"));
            log_element("Width", get_element_data(node, "Width"));
            log_element("Height", get_element_data(node, "Height"));
            log_element("FullScreen", get_element_data(node, "FullScreen"));
            log_element("ForceUpdate", get_element_data(node, "ForceUpdate"));
            log_element("ActiveDisplay", get_element_data(node, "ActiveDisplay"));
            log_close_start_tag();
        }
        if (NodeCheck(node, "Panel"))
        {
            log_open_start_tag("Panel");
            log_element("DisplayIndex", get_element_data(node, "DisplayIndex"));
            log_element("BackgroundColor", get_element_data(node, "BackgroundColor"));
            log_element("VirtualWidth", get_element_data(node, "VirtualWidth"));
            log_element("VirtualHeight", get_element_data(node, "VirtualHeight"));
            log_close_start_tag();
        }
    }
}

void log_node_data(xmlNodePtr node)
{
    if (logfile.is_open())
    {
        if (NodeCheck(node, "Variable"))
        {
            log_open_start_tag("Variable");
            log_element("Type", get_element_data(node, "Type"));
            log_element("InitialValue", get_element_data(node, "InitialValue"));
            log_close_start_tag(true);
            log_content(get_node_content(node));
            log_end_tag("Variable", true);
        }
        if (NodeCheck(node, "TrickVariable"))
        {
            log_open_start_tag("TrickVariable");
            log_element("Name", get_element_data(node, "Name"));
            log_element("Units", get_element_data(node, "Units"));
            log_element("InitializationOnly", get_element_data(node, "InitializationOnly"));
            log_close_start_tag(true);
            log_content(get_node_content(node));
            log_end_tag("TrickVariable", true);
        }
        if (NodeCheck(node, "TrickMethod"))
        {
            log_open_start_tag("TrickMethod");
            log_element("Name", get_element_data(node, "Name"));
            log_close_start_tag(true);
            log_content(get_node_content(node));
            log_end_tag("TrickMethod", true);
        }
        if (NodeCheck(node, "EdgeVariable"))
        {
            log_open_start_tag("EdgeVariable");
            log_element("RcsCommand", get_element_data(node, "RcsCommand"));
            log_close_start_tag(true);
            log_content(get_node_content(node));
            log_end_tag("EdgeVariable", true);
        }
        if (NodeCheck(node, "String"))
        {
            log_open_start_tag("String");
            log_element("X", get_element_data(node, "X"));
            log_element("Y", get_element_data(node, "Y"));
            log_element("Rotate", get_element_data(node, "Rotate"));
            log_element("HorizontalAlign", get_element_data(node, "HorizontalAlign"));
            log_element("VerticalAlign", get_element_data(node, "VerticalAlign"));
            log_element("OriginX", get_element_data(node, "OriginX"));
            log_element("OriginY", get_element_data(node, "OriginY"));
            log_element("Color", get_element_data(node, "Color"));
            log_element("BackgroundColor", get_element_data(node, "BackgroundColor"));
            log_element("Font", get_element_data(node, "Font"));
            log_element("Face", get_element_data(node, "Face"));
            log_element("Size", get_element_data(node, "Size"));
            log_element("ForceMono", get_element_data(node, "ForceMono"));
            log_element("ShadowOffset", get_element_data(node, "ShadowOffset"));
            log_close_start_tag(true);
            log_content(get_node_content(node));
            log_end_tag("String", true);
        }
    }
}

void log_node_end(xmlNodePtr node)
{
    if (logfile.is_open()) log_end_tag(get_node_type(node));
}
