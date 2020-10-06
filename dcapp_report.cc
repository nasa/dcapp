#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <list>
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include "basicutils/msg.hh"
#include "basicutils/pathinfo.hh"
#include "basicutils/shellutils.hh"
#include "basicutils/stringutils.hh"
#include "xml_data.hh"
#include "xml_utils.hh"
#include "xml_stringsub.hh"

static void process_elements(xmlNodePtr);
static void UsageError(xmlDocPtr);

#include <fstream>
std::ofstream logfile;
int indent=0;

void log_start_tag(const std::string &instr, bool sameline=false)
{
    for (int i=0; i<indent; i++) logfile << "    ";
    logfile << "<" << instr << ">";
    if (!sameline)
    {
        logfile << std::endl;
        indent++;
    }
}

void log_open_start_tag(const std::string &instr)
{
    for (int i=0; i<indent; i++) logfile << "    ";
    logfile << "<" << instr;
}

void log_close_start_tag(bool sameline=false)
{
    logfile << ">";
    if (!sameline)
    {
        logfile << std::endl;
        indent++;
    }
}

void log_element(const std::string &label, const std::string &value, bool printifempty=false)
{
    if (!value.empty() || printifempty) logfile << " " << label << "=\"" << value << "\"";
}

void log_content(const std::string &instr)
{
    logfile << instr;
}

void log_end_tag(const std::string &instr, bool sameline=false)
{
    if (!sameline)
    {
        indent--;
        for (int i=0; i<indent; i++) logfile << "    ";
    }
    logfile << "</" << instr << ">" << std::endl;
}

bool check_dynamic_element(const std::string &spec)
{
    if (!spec.empty())
    {
        if (spec[0] == '@') return true;
    }
    return false;
}

bool checkCondition(const std::string &myoperator, const std::string &val1, const std::string &val2)
{
    if (myoperator == "eq" && val1 == val2) return true;
    else if (myoperator == "ne" && val1 != val2) return true;
    else if (myoperator == "gt" && val1 > val2) return true;
    else if (myoperator == "lt" && val1 < val2) return true;
    else if (myoperator == "ge" && val1 >= val2) return true;
    else if (myoperator == "le" && val1 <= val2) return true;
    else return StringToBoolean(val1);
}

int main(int argc, char **argv)
{
    xmlDocPtr mydoc;
    xmlNodePtr root_element;

    Message::setLabel(basename(argv[0]));

    if (argc < 3) UsageError(0x0);

    logfile.open(argv[2]);
    log_start_tag("DCAPPreport");

    if (XMLFileOpen(&mydoc, &root_element, argv[1])) UsageError(0x0);
    if (!NodeCheck(root_element, "DCAPP"))
    {
        error_msg("Bad root element in XML file: \"" << root_element->name << "\" (should be \"DCAPP\")");
        return (-1);
    }

    // move to the folder containing the specfile and set $dcappDisplayHome
    PathInfo mypath(argv[1]);
    if (mypath.isValid())
    {
        setenv("dcappDisplayHome", mypath.getDirectory().c_str(), 1);
        chdir(mypath.getDirectory().c_str());
    }

    process_elements(root_element->children);

    XMLFileClose(mydoc);
    XMLEndParsing();

    log_end_tag("DCAPPreport");
    logfile.close();

    return 0;
}

static void process_elements(xmlNodePtr startnode)
{
    for (xmlNodePtr node = startnode; node; node = node->next)
    {
        if (NodeCheck(node, "Dummy")) process_elements(node->children);
        if (NodeCheck(node, "Constant")) processConstantNode(node);
        if (NodeCheck(node, "Style")) processStyleNode(node);
        if (NodeCheck(node, "Defaults")) processDefaultsNode(node);
        if (NodeCheck(node, "Include"))
        {
            xmlDocPtr include_file;
            xmlNodePtr include_element;
            std::string include_filename = get_node_content(node);

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
                        process_elements(include_element);
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

            if (check_dynamic_element(val1) || check_dynamic_element(val2))
            {
                log_open_start_tag("If");
                log_element("Value1", val1);
                log_element("Operator", myoperator);
                log_element("Value2", val2);
                log_close_start_tag();

                for (xmlNodePtr subnode = node->children; subnode; subnode = subnode->next)
                {
                    if (NodeCheck(subnode, "True"))
                    {
                        log_start_tag("True");
                        process_elements(subnode->children);
                        log_end_tag("True");
                        subparent_found = true;
                    }
                    if (NodeCheck(subnode, "False"))
                    {
                        log_start_tag("False");
                        process_elements(subnode->children);
                        log_end_tag("False");
                        subparent_found = true;
                    }
                }
                // Assume "True" if no subparent is found
                if (!subparent_found) process_elements(node->children);
                log_end_tag("If");
            }
            else
            {
                xmldata myval1, myval2;
                if (val1.defined()) myval1 = val1;
                if (val2.defined()) myval2 = val2;
                if (checkCondition(myoperator, myval1, myval2)) process_elements(node->children);
            }
        }
        if (NodeCheck(node, "TrickIo"))
        {
            log_open_start_tag("TrickIo");
            log_element("Host", get_element_data(node, "Host"));
            log_element("Port", get_element_data(node, "Port"));
            log_element("DataRate", get_element_data(node, "DataRate"));
            log_element("ConnectedVariable", get_element_data(node, "ConnectedVariable"));
            log_element("DisconnectAction", get_element_data(node, "DisconnectAction"));
            log_close_start_tag();
            process_elements(node->children);
            log_end_tag("TrickIo");
        }
        if (NodeCheck(node, "FromTrick"))
        {
            log_start_tag("FromTrick");
            process_elements(node->children);
            log_end_tag("FromTrick");
        }
        if (NodeCheck(node, "ToTrick"))
        {
            log_start_tag("ToTrick");
            process_elements(node->children);
            log_end_tag("ToTrick");
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
        if (NodeCheck(node, "EdgeIo"))
        {
            log_open_start_tag("EdgeIo");
            log_element("Host", get_element_data(node, "Host"));
            log_element("Port", get_element_data(node, "Port"));
            log_element("DataRate", get_element_data(node, "DataRate"));
            log_element("ConnectedVariable", get_element_data(node, "ConnectedVariable"));
            log_close_start_tag();
            process_elements(node->children);
            log_end_tag("EdgeIo");
        }
        if (NodeCheck(node, "FromEdge"))
        {
            log_start_tag("FromEdge");
            process_elements(node->children);
            log_end_tag("FromEdge");
        }
        if (NodeCheck(node, "ToEdge"))
        {
            log_start_tag("ToEdge");
            process_elements(node->children);
            log_end_tag("ToEdge");
        }
        if (NodeCheck(node, "EdgeVariable"))
        {
            log_open_start_tag("EdgeVariable");
            log_element("RcsCommand", get_element_data(node, "RcsCommand"));
            log_close_start_tag(true);
            log_content(get_node_content(node));
            log_end_tag("EdgeVariable", true);
        }
    }
}

static void UsageError(xmlDocPtr infile)
{
    if (infile) XMLFileClose(infile);

    printf("Usage: dcapp_report <infile> <outfile>, where:\n");
    printf("    <infile> is a dcapp specfile\n");
    printf("    <outfile> is a the name of the report file to be generated\n");

    exit(-1);
}
