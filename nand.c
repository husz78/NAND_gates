#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "nand.h"
#include "structs.h"

#define MAX(x, y) (x < y ? y : x)

// disconnects the signal from the k-th input of gate g
void delete_kth_entry_signal(unsigned k, nand_t* g) { 
    void *entry = g->entries[k];
    if (entry == NULL) return;
    if (g->entry_type[k]) { // if the k-th input is connected to a nand_t
        nand_t *nand_entry = (nand_t *)entry;
        // remove gate g from the outputs of nand_entry
        del_node(&(nand_entry->exits), g); 
    }
    g->entries[k] = NULL; // needs further consideration
    g->entry_type[k] = 0;
}

// returns the input number of gate g_out that is connected to gate g_in
unsigned entry_number(nand_t *g_in, nand_t *g_out) {
    for (unsigned i = 0; i < g_out->n_of_entries; i++) {
        if (g_out->entries[i] != NULL) {
            if (g_out->entry_type[i]) {
                if (g_out->entries[i] == g_in) return i;
            }
        }
    }
    return 0;
}

nand_t* nand_new(unsigned n) {
    nand_t* New_nand = (nand_t*)malloc(sizeof(nand_t));
    if (New_nand) {
        New_nand->n_of_entries = n;
        New_nand->signal = false;
        New_nand->process = pre;
        New_nand->path_len = 0;
        New_nand->exits = NULL;
        New_nand->entries = (void **)malloc(sizeof(void *) * n);
        New_nand->entry_type = (bool*)malloc(sizeof(bool) * n);
        // Allocation errors
        if (New_nand->entries == NULL && New_nand->entry_type == NULL) {
            errno = ENOMEM;
            free(New_nand);
            return NULL;
        }
        // Allocation errors
        else if (New_nand->entries == NULL && New_nand->entry_type) {
            errno = ENOMEM;
            free(New_nand->entry_type);
            free(New_nand);
            return NULL;
        }
        // Allocation errors
        else if (New_nand->entries && New_nand->entry_type == NULL) { 
            errno = ENOMEM;
            free(New_nand->entries);
            free(New_nand);
            return NULL;
        }
        // no input is connected to anything, hence NULL
        for (unsigned i = 0; i < n; i++) New_nand->entries[i] = NULL;
    } else { // Allocation error
        errno = ENOMEM;
        return NULL;
    }
    return New_nand;
}

void nand_delete(nand_t *g) {
    if (g == NULL) return;
    // disconnect input signals
    for (unsigned i = 0; i < g->n_of_entries; i++)
        delete_kth_entry_signal(i, g);
    
    // disconnect output signals
    list tmp = g->exits;
    while (tmp != NULL && tmp->gate != NULL) {
        unsigned entry_num = entry_number(g, tmp->gate);
        tmp->gate->entries[entry_num] = NULL;
        tmp = tmp->next;
    }
    del_list(&(g->exits)); 
    
    // free memory
    free(g->entry_type);
    free(g->entries);
    free(g);
}

int nand_connect_nand(nand_t *g_out, nand_t *g_in, unsigned k) {
    // if any parameter is invalid
    if (g_out == NULL || g_in == NULL || k >= g_in->n_of_entries) {
        errno = EINVAL;
        return -1;
    }
    if (g_out->exits == NULL) { // if output is an empty list
        g_out->exits = create_node(g_in);
        if (g_out->exits == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }
    else { // if something is already connected to the output of g_out
        list new_list = create_node(g_in);
        if (new_list == NULL) { // if memory allocation failed
            errno = ENOMEM;
            return -1;
        }
        list_push(g_out->exits, new_list);
    }
    
    delete_kth_entry_signal(k, g_in);
    g_in->entries[k] = (nand_t*)g_out;
    g_in->entry_type[k] = 1;

    return 0;
}

int nand_connect_signal(bool const *s, nand_t *g, unsigned k) {
    // if any parameter is invalid
    if (s == NULL || g == NULL || k >= g->n_of_entries) {
        errno = EINVAL;
        return -1;
    }
    delete_kth_entry_signal(k, g);
    g->entries[k] = (bool*)s;
    g->entry_type[k] = 0;
    return 0;
}

ssize_t nand_fan_out(nand_t const *g) {
    if (g == NULL) { 
        errno = EINVAL;
        return -1;
    }
    ssize_t counter = 1;
    if (g->exits == NULL) return 0;
    if (g->exits->gate == NULL) return 0;
    list tmp = g->exits;
    while (tmp->next != NULL) {
        counter++;
        tmp = tmp->next;
    }
    return counter;
}

void* nand_input(nand_t const *g, unsigned k) {
    // if any of the parameters is invalid
    if (g == NULL || k >= g->n_of_entries) { 
        errno = EINVAL;
        return NULL;
    }

    if (g->entries[k] == NULL) { // if nothing is connected to input k
        errno = 0;
        return NULL;
    }

    if (g->entry_type[k]) return (nand_t*)(g->entries[k]);
    else return (bool*)(g->entries[k]);
}

nand_t* nand_output(nand_t const *g, ssize_t k) {
    if (g == NULL || k >= nand_fan_out(g)) { // if any of the parameters is invalid
        errno = EINVAL;
        return NULL;
    }
    list tmp = g->exits;
    while (k > 0) {
        tmp = tmp->next;
        k--;
    }
    return tmp->gate;
}

// sets the process of gate g and all preceding gates to pre
void unprocess_gate(nand_t* g) {
    if (g == NULL) return;
    if (g->process == pre) return;
    g->process = pre;
    g->signal = false;
    g->path_len = 0;
    for (unsigned i = 0; i < g->n_of_entries; i++)
        // if the input exists and is connected to a nand gate
        if (g->entries[i] != NULL && g->entry_type[i])
            unprocess_gate((nand_t*)g->entries[i]);
}

// sets the process of all gates in the array gates,
// and all preceding gates, to pre
void unprocess_gates(nand_t **gates, size_t m) { 
    for (size_t i = 0; i < m; i++) unprocess_gate(gates[i]);
}

// returns the computed signal of gate g and sets path_len
// to the critical path length of gate g
bool eval_gate(nand_t* g, ssize_t* path_len) { 
    if (*path_len == -1) return false;
    if (g->process == post) { // if the gate has already been evaluated
        *path_len = g->path_len;
        return g->signal;
    }
    // if the gate is already being evaluated, we have a cycle
    if (g->process == in) { 
        errno = ECANCELED;
        *path_len = -1;
        g->path_len = -1;
        return false;
    }
    if (g->n_of_entries == 0) { // when the gate has no inputs
        g->signal = false;
        g->path_len = 0;
        *path_len = 0;
        g->process = post;
        return false;
    }
    g->process = in;

    // maximum critical path length from all inputs
    ssize_t max_path = 0; 
    bool eval = false; // result of eval_gate for the input gates
    g->signal = false;

    // iterate over the inputs of gate g
    for (unsigned i = 0; i < g->n_of_entries; i++) { 
        // if nothing is connected to the i-th input
        if (g->entries[i] == NULL) { 
            *path_len = -1;
            g->path_len = -1;
            return false;
        }
        // if the i-th input is connected to a nand gate
        if (g->entry_type[i]) { 
            eval = eval_gate((nand_t*)g->entries[i], path_len);
            if (*path_len == -1) {
                g->path_len = -1;
                return false;
            }
            else {
                if (!eval) g->signal = true;
                max_path = MAX(max_path, *path_len);
            }
        }
        else { // if the i-th input is connected to a boolean signal
            bool *tmp = (bool *)g->entries[i];
            if (!(*tmp)) g->signal = true;
            *path_len = 0;
        }
    }
    max_path++; // increment the path length according to the formula
    *path_len = max_path;
    g->path_len = max_path;
    g->process = post;
    return g->signal;
}

ssize_t nand_evaluate(nand_t **g, bool *s, size_t m) {
    if (m == 0 || g == NULL || s == NULL) {
        errno = EINVAL;
        return -1;
    }
    for (size_t i = 0; i < m; i++)
        if (g[i] == NULL) {
            errno = EINVAL;
            return -1;
        }
    unprocess_gates(g, m);
    ssize_t max_path = 0;
    // tmp_path will store the lengths of individual gate paths
    ssize_t tmp_path = 0;

    // iterate over the array of gates, compute the path length
    // of the gate system, and fill the array s
    for (size_t i = 0; i < m; i++) { 
        s[i] = eval_gate(g[i], &tmp_path);
        if (g[i]->path_len == -1) {
            unprocess_gates(g, m);
            errno = ECANCELED;
            return -1;
        }
        max_path = MAX(max_path, g[i]->path_len);
    }
    unprocess_gates(g, m);
    return max_path;
}
