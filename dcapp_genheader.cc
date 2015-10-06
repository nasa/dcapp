#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <iostream>
#include <list>
#include "msg.hh"
#include "xml_utils.hh"

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
    if (XMLFileOpen(&mydoc, &root_element, argv[1], 0x0)) UsageError(0x0);
    if (argc < 3) p_file = fopen("dcapp.h", "w");
    else p_file = fopen(argv[2], "w");
    if (!p_file) UsageError(mydoc);

    process_elements(root_element->children);

    fprintf(p_file, "// ********************************************* //\n");
    fprintf(p_file, "// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //\n");
    fprintf(p_file, "// ********************************************* //\n\n");
    fprintf(p_file, "#ifndef _DCAPP_EXTERNALS_\n#define _DCAPP_EXTERNALS_\n\n");

    for (myvitem = vlist.begin(); myvitem != vlist.end(); myvitem++)
    {
        if (myvitem->type == "Float") fprintf(p_file, "float *");
        else if (myvitem->type == "Integer") fprintf(p_file, "int *");
        else fprintf(p_file, "char *");
        fprintf(p_file, "%s;\n", myvitem->name.c_str());
    }

    fprintf(p_file, "\n#ifdef __cplusplus\n");
    fprintf(p_file, "extern \"C\" void DisplayPreInit(void *(*get_pointer)(const char *))\n");
    fprintf(p_file, "#else\n");
    fprintf(p_file, "void DisplayPreInit(void *(*get_pointer)(const char *))\n");
    fprintf(p_file, "#endif\n{\n");

    for (myvitem = vlist.begin(); myvitem != vlist.end(); myvitem++)
    {
        fprintf(p_file, "    %s = ", myvitem->name.c_str());
        if (myvitem->type == "Float") fprintf(p_file, "(float *)");
        else if (myvitem->type == "Integer") fprintf(p_file, "(int *)");
        else fprintf(p_file, "(char *)");
        fprintf(p_file, "get_pointer(\"%s\");\n", myvitem->name.c_str());
    }

    fprintf(p_file, "}\n\n");
    fprintf(p_file, "#else\n\n");

    for (myvitem = vlist.begin(); myvitem != vlist.end(); myvitem++)
    {
        if (myvitem->type == "Float") fprintf(p_file, "extern float *");
        else if (myvitem->type == "Integer") fprintf(p_file, "extern int *");
        else fprintf(p_file, "extern char *");
        fprintf(p_file, "%s;\n", myvitem->name.c_str());
    }

    fprintf(p_file, "\n#endif\n");
    fclose(p_file);
    vlist.clear();
    XMLFileClose(mydoc);

    return 0;
}

static void process_elements(xmlNodePtr startnode)
{
    for (xmlNodePtr node = startnode; node; node = node->next)
    {
        if (NodeCheck(node, "Dummy")) process_elements(node->children);
        if (NodeCheck(node, "Variable"))
        {
            vitem newitem;
            newitem.name = std::string(get_XML_content(node));
            newitem.type = std::string(get_XML_attribute(node, "Type"));
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
