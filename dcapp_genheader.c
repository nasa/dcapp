#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xml_utils.h"

typedef struct _vitem
{
    char *name;
    char *type;
    struct _vitem *next;
} vitem;

static void process_elements(xmlNodePtr);
static vitem *vlist_add(vitem **);
static void vlist_clean(vitem *);
static void UsageError(void);

static vitem *vlist_start = 0;

int main(int argc, char **argv)
{
    xmlDocPtr mydoc;
    xmlNodePtr root_element;
    FILE *p_file;
    vitem *myvitem;

    if (XMLFileOpen(&mydoc, &root_element, argv[1], NULL)) UsageError();

    process_elements(root_element->children);

    p_file = fopen("dcapp.h", "w");

    fprintf(p_file, "// ********************************************* //\n");
    fprintf(p_file, "// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //\n");
    fprintf(p_file, "// ********************************************* //\n\n");
    fprintf(p_file, "#ifndef _DCAPP_H_\n#define _DCAPP_H_\n\n");

    for (myvitem = vlist_start; myvitem; myvitem = myvitem->next)
    {
        if (!strcmp(myvitem->type, "Float")) fprintf(p_file, "static float *");
        else if (!strcmp(myvitem->type, "Integer")) fprintf(p_file, "static int *");
        else fprintf(p_file, "static char *");
        fprintf(p_file, "%s;\n", myvitem->name);
    }

    fprintf(p_file, "\nvoid DisplayPreInit(void *(*get_pointer)(char *))\n{\n");

    for (myvitem = vlist_start; myvitem; myvitem = myvitem->next)
    {
        fprintf(p_file, "    %s = get_pointer(\"%s\");\n", myvitem->name, myvitem->name);
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
    vitem *new;
    static vitem *prev = 0;

    new = malloc(sizeof(vitem));
    if (!*start) *start = new;
    if (prev) prev->next = new;
    new->next = 0;
    prev = new;

    return new;
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

static void UsageError(void)
{
    printf("Usage: dcapp_genheader <file>, where <file> is a dcapp specfile or a dcapp global variable file\n");
    exit(-1);
}
