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
    if (argc < 3) p_file = fopen("dcapp.h", "w");
    else p_file = fopen(argv[2], "w");
    if (!p_file) UsageError(mydoc);

    // move to the folder containing the specfile and set $dcappDisplayHome
    PathInfo *mypath = new PathInfo(argv[1]);
    setenv("dcappDisplayHome", mypath->getDirectory(), 1);
    if (mypath->getDirectory()) chdir(mypath->getDirectory());
    delete mypath;

    process_elements(root_element->children);

    fprintf(p_file, "// ********************************************* //\n");
    fprintf(p_file, "// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //\n");
    fprintf(p_file, "// ********************************************* //\n\n");
fprintf(p_file, "#define float double\n#define fabsf fabs\n");
    fprintf(p_file, "#ifndef _DCAPP_EXTERNALS_\n#define _DCAPP_EXTERNALS_\n\n");
    fprintf(p_file, "void *(*get_pointer)(const char *);\n\n");

    for (myvitem = vlist.begin(); myvitem != vlist.end(); myvitem++)
    {
        if (myvitem->type == "Float") fprintf(p_file, "double *");
        else if (myvitem->type == "Integer") fprintf(p_file, "int *");
        else fprintf(p_file, "char *");
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
        if (myvitem->type == "Float") fprintf(p_file, "(double *)");
        else if (myvitem->type == "Integer") fprintf(p_file, "(int *)");
        else fprintf(p_file, "(char *)");
        fprintf(p_file, "get_pointer(\"%s\");\n", myvitem->name.c_str());
    }

    fprintf(p_file, "}\n\n");
    fprintf(p_file, "#else\n\n");
    fprintf(p_file, "extern void *(*get_pointer)(const char *);\n\n");

    for (myvitem = vlist.begin(); myvitem != vlist.end(); myvitem++)
    {
        if (myvitem->type == "Float") fprintf(p_file, "extern double *");
        else if (myvitem->type == "Integer") fprintf(p_file, "extern int *");
        else fprintf(p_file, "extern char *");
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
            const char *include_filename = get_node_content(node);

            PathInfo *mypath = new PathInfo(include_filename);

            // Store cwd for future use
            int mycwd = open(".", O_RDONLY);

            // Move to directory containing the new file
            if (mypath->getDirectory()) chdir(mypath->getDirectory());

            if (XMLFileOpen(&include_file, &include_element, mypath->getFile()))
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

            delete mypath;
        }
        if (NodeCheck(node, "Variable"))
        {
            vitem newitem;
            const char *myname = get_node_content(node);
            if (myname[0] == '@') myname++;
            newitem.name = std::string(myname);
            newitem.type = std::string(get_element_data(node, "Type"));
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
