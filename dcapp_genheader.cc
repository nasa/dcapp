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
#include "xml_utils.hh"
#include "xml_stringsub.hh"

typedef struct
{
    std::string name;
    std::string type;
} vitem;

static std::list<vitem> vlist;

static void process_elements(xmlNodePtr);
static void UsageError(xmlDocPtr);

int main(int argc, char **argv)
{
    xmlDocPtr mydoc;
    xmlNodePtr root_element;
    FILE *p_file;
    static std::list<vitem>::iterator myvitem;

    Message::setLabel(basename(argv[0]));

    if (argc < 2) UsageError(0x0);

    if (XMLFileOpen(&mydoc, &root_element, argv[1])) UsageError(0x0);
    if (!NodeCheck(root_element, "DCAPP"))
    {
        error_msg("Bad root element in XML file: \"" << root_element->name << "\" (should be \"DCAPP\")");
        return (-1);
    }

    if (argc < 3) p_file = fopen("dcapp.h", "w");
    else p_file = fopen(argv[2], "w");
    if (!p_file) UsageError(mydoc);

    // move to the folder containing the specfile and set $dcappDisplayHome
    PathInfo mypath(argv[1]);
    if (mypath.isValid())
    {
        setenv("dcappDisplayHome", mypath.getDirectory().c_str(), 1);
        chdir(mypath.getDirectory().c_str());
    }

    process_elements(root_element->children);

    std::string configscript = PathInfo(findExecutablePath(argv[0]) + "/../dcapp-config").getFullPath();

    fprintf(p_file, "// ********************************************* //\n");
    fprintf(p_file, "// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //\n");
    fprintf(p_file, "// ********************************************* //\n\n");
    // the line below preceded the use of DCAPP_MAJOR_VERSION and DCAPP_MINOR_VERSION, so retain it for backward compatibility
    fprintf(p_file, "#define DCAPP_VERSION_1_0\n\n");
    fprintf(p_file, "#define DCAPP_MAJOR_VERSION %s\n", getScriptResult(configscript, "--version_major").c_str());
    fprintf(p_file, "#define DCAPP_MINOR_VERSION %s\n", getScriptResult(configscript, "--version_minor").c_str());
    fprintf(p_file, "\n#include <string>\n\n");
    fprintf(p_file, "#ifndef _DCAPP_EXTERNALS_\n#define _DCAPP_EXTERNALS_\n\n");
    fprintf(p_file, "void *(*get_pointer)(const char *);\n\n");

    for (myvitem = vlist.begin(); myvitem != vlist.end(); myvitem++)
    {
        if (myvitem->type == "Decimal" || myvitem->type == "Float") fprintf(p_file, "double *");
        else if (myvitem->type == "Integer") fprintf(p_file, "int *");
        else fprintf(p_file, "std::string *");
        fprintf(p_file, "%s;\n", myvitem->name.c_str());
    }

    fprintf(p_file, "\n#ifdef __cplusplus\n");
    fprintf(p_file, "extern \"C\" void DisplayPreInit(void *(*get_pointer_arg)(const char *))\n");
    fprintf(p_file, "#else\n");
    fprintf(p_file, "void DisplayPreInit(void *(*get_pointer_arg)(const char *))\n");
    fprintf(p_file, "#endif\n{\n");
    fprintf(p_file, "    get_pointer = get_pointer_arg;\n\n");

    for (myvitem = vlist.begin(); myvitem != vlist.end(); myvitem++)
    {
        fprintf(p_file, "    %s = ", myvitem->name.c_str());
        if (myvitem->type == "Decimal" || myvitem->type == "Float") fprintf(p_file, "(double *)");
        else if (myvitem->type == "Integer") fprintf(p_file, "(int *)");
        else fprintf(p_file, "(std::string *)");
        fprintf(p_file, "get_pointer(\"%s\");\n", myvitem->name.c_str());
    }

    fprintf(p_file, "}\n\n");
    fprintf(p_file, "#else\n\n");
    fprintf(p_file, "extern void *(*get_pointer)(const char *);\n\n");

    for (myvitem = vlist.begin(); myvitem != vlist.end(); myvitem++)
    {
        if (myvitem->type == "Decimal" || myvitem->type == "Float") fprintf(p_file, "extern double *");
        else if (myvitem->type == "Integer") fprintf(p_file, "extern int *");
        else fprintf(p_file, "extern std::string *");
        fprintf(p_file, "%s;\n", myvitem->name.c_str());
    }

    fprintf(p_file, "\n#endif\n");
    fclose(p_file);
    vlist.clear();
    XMLFileClose(mydoc);
    XMLEndParsing();

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
        if (NodeCheck(node, "Variable"))
        {
            vitem newitem;
            std::string myname = get_node_content(node);
            if (myname[0] == '@') myname.erase(0, 1);
            newitem.name = myname;
            newitem.type = get_element_data(node, "Type");
            vlist.push_back(newitem);
        }
    }
}

static void UsageError(xmlDocPtr infile)
{
    if (infile) XMLFileClose(infile);

    printf("Usage: dcapp_genheader <infile> <outfile>, where:\n");
    printf("    <infile> is a dcapp specfile or a dcapp global variable file\n");
    printf("    <outfile> is a the name of the header file to be generated\n");

    exit(-1);
}
