#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xml_utils.hh"

typedef struct _vitem
{
    char *name;
    char *type;
    struct _vitem *next;
} vitem;

static void process_elements(xmlNodePtr);
static vitem *vlist_add(vitem **);
static void vlist_clean(vitem *);
static void UsageError(xmlDocPtr);

static vitem *vlist_start = 0;

int main(int argc, char **argv)
{
    xmlDocPtr mydoc;
    xmlNodePtr root_element;
    FILE *p_file;
    vitem *myvitem;

    if (argc < 2) UsageError(NULL);
    if (XMLFileOpen(&mydoc, &root_element, argv[1], NULL)) UsageError(NULL);
    if (argc < 3) p_file = fopen("dcapp.h", "w");
    else p_file = fopen(argv[2], "w");
    if (!p_file) UsageError(mydoc);

    process_elements(root_element->children);

    fprintf(p_file, "// ********************************************* //\n");
    fprintf(p_file, "// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //\n");
    fprintf(p_file, "// ********************************************* //\n\n");
    fprintf(p_file, "#ifndef _DCAPP_EXTERNALS_\n#define _DCAPP_EXTERNALS_\n\n");

    for (myvitem = vlist_start; myvitem; myvitem = myvitem->next)
    {
        if (!strcmp(myvitem->type, "Float")) fprintf(p_file, "float *");
        else if (!strcmp(myvitem->type, "Integer")) fprintf(p_file, "int *");
        else fprintf(p_file, "char *");
        fprintf(p_file, "%s;\n", myvitem->name);
    }

    fprintf(p_file, "\n#ifdef __cplusplus\n");
    fprintf(p_file, "extern \"C\" void DisplayPreInit(void *(*get_pointer)(const char *))\n");
    fprintf(p_file, "#else\n");
    fprintf(p_file, "void DisplayPreInit(void *(*get_pointer)(const char *))\n");
    fprintf(p_file, "#endif\n{\n");

    for (myvitem = vlist_start; myvitem; myvitem = myvitem->next)
    {
        fprintf(p_file, "    %s = ", myvitem->name);
        if (!strcmp(myvitem->type, "Float")) fprintf(p_file, "(float *)");
        else if (!strcmp(myvitem->type, "Integer")) fprintf(p_file, "(int *)");
        else fprintf(p_file, "(char *)");
        fprintf(p_file, "get_pointer(\"%s\");\n", myvitem->name);
    }

    fprintf(p_file, "}\n\n");
    fprintf(p_file, "#else\n\n");

    for (myvitem = vlist_start; myvitem; myvitem = myvitem->next)
    {
        if (!strcmp(myvitem->type, "Float")) fprintf(p_file, "extern float *");
        else if (!strcmp(myvitem->type, "Integer")) fprintf(p_file, "extern int *");
        else fprintf(p_file, "extern char *");
        fprintf(p_file, "%s;\n", myvitem->name);
    }

    fprintf(p_file, "\n#endif\n");

    fclose(p_file);

    vlist_clean(vlist_start);

    XMLFileClose(mydoc);

    return 0;
}

static void process_elements(xmlNodePtr startnode)
{
    xmlNodePtr node;
    vitem *myvitem;

    for (node = startnode; node != NULL; node = node->next)
    {
        if (NodeCheck(node, "Dummy")) process_elements(node->children);
        if (NodeCheck(node, "Variable"))
        {
            myvitem = vlist_add(&vlist_start);
            myvitem->name = get_XML_content(node);
            myvitem->type = get_XML_attribute(node, "Type");
        }
    }
}

static vitem *vlist_add(vitem **start)
{
    vitem *newitem;
    static vitem *previtem = 0;

    newitem = (vitem *)malloc(sizeof(vitem));
    if (!*start) *start = newitem;
    if (previtem) previtem->next = newitem;
    newitem->next = 0;
    previtem = newitem;

    return newitem;
}

static void vlist_clean(vitem *start)
{
    vitem *current, *tmp;

    for (current = start; current;)
    {
        tmp = current->next;
        free(current);
        current = tmp;
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
