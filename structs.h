#ifndef STRUCTS_H
#define STRUCTS_H

#include "nand.h"

typedef struct node {
    nand_t *gate;
    struct node *next;
} node;
typedef struct node* list;

typedef enum Process {pre, in, post} Process;
struct nand {
    unsigned n_of_entries; // number of gate inputs
    // array of pointers to gates connected to the gate's input
    void **entries;
    
    // array, where at the i-th position there is a 0 if a boolean signal
    // is connected to the i-th input, and 1 if a nand gate is connected to the i-th input
    bool *entry_type; 
    bool signal; // value of the gate's signal
    Process process; // indicates the stage of critical path computation
    // the gate is in: pre - before computation,
    // in - during, post - after computation

    ssize_t path_len; // length of the gate's critical path
    list exits; // list of pointers to
    // gates connected to the output of this gate
};

// creates a list node that contains
// a pointer to gate, returns a pointer to this node or NULL
list create_node(nand_t *gate); 

void list_push(list head, list item); // inserts item into the head list
void del_node(list *head, nand_t* g); // removes gate g from the head list

// removes the entire list, where head is a pointer to the first element
void del_list(list *head); 

#endif
