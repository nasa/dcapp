#include <cstdlib>
#include "nodes.hh"


struct node *NewList(struct node *parent)
{
    struct node *newNode = (struct node *)calloc(1, sizeof(struct node));

    newNode->p_next = NULL;                      // This is the first node of the list...
    newNode->p_prev = NULL;                      //    therefore there is nothing to point to.
    newNode->p_next_list = NULL;                 // This is the next display.
    newNode->p_prev_list = NULL;                 // This is the previous display.
    newNode->p_tail = newNode;                   // Make it the tail since there's one node.

    if (parent->p_head == NULL)
        parent->p_head = newNode;

    if (parent->p_tail == NULL)                  // If there are no other lists for this chain...
        parent->p_tail = newNode;                //    make the chain point to this list.
    else
    {
        parent->p_tail->p_next_list = newNode;   // Make the old last list in the chain point to your list.
        newNode->p_prev_list = parent->p_tail;   // Make your new list point back to the old last list.
        parent->p_tail = newNode;                // Make your list the last one in the chain.
    }

    parent->p_current = newNode;                 // This is the currently updating list.
    return newNode;
}

struct node *NewNode(struct node *parent, struct node **list)
{
    struct node *newNode = (struct node *)calloc(1, sizeof(struct node));

    if (*list == NULL)
    {
        *list = newNode;
        newNode->p_prev = NULL;
    }
    else
    {
        (*list)->p_tail->p_next = newNode;
        newNode->p_prev = (*list)->p_tail;
    }

    (*list)->p_tail = newNode;
    newNode->p_next = NULL;
    newNode->p_head = *list;
    newNode->p_parent = parent;

    return newNode;
}
