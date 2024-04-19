#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "nand.h"
#include "structs.h"

#define MAX(x, y) (x < y ? y : x)

// odlacza sygnal z k-tego wejscia bramki g
void delete_kth_entry_signal(unsigned k, nand_t* g) { 
    void *entry = g->entries[k];
    if (entry == NULL)return;
    if (g->entry_type[k]) { // jesli k-te wyjscie jest podlaczone pod nand_t
        nand_t *nand_entry = (nand_t *)entry;
        //if (nand_entry == g)return;
        // usuwamy bramke g z wyjsc bramki nand_entry
        del_node(&(nand_entry->exits), g); 
    }
    g->entries[k] = NULL; // trzeba sie nad tym zastanowic
    g->entry_type[k] = 0;
}

// zwraca numer wejscia bramki g_out, do ktorego jest podlaczona bramka g_in
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
        // Bledy alokowania
        if (New_nand->entries == NULL && New_nand->entry_type == NULL) {
            errno = ENOMEM;
            free(New_nand);
            return NULL;
        }
        // Bledy alokowania
        else if (New_nand->entries == NULL && New_nand->entry_type){
            errno = ENOMEM;
            free(New_nand->entry_type);
            free(New_nand);
            return NULL;
        }
        // Bledy alokowania
        else if (New_nand->entries && New_nand->entry_type == NULL) { 
            errno = ENOMEM;
            free(New_nand->entries);
            free(New_nand);
            return NULL;
        }
        // zadne wejscie nie jest podlaczone pod nic. Dlatego NULL
        for (unsigned i = 0; i < n; i++)New_nand->entries[i] = NULL;
    }
    else  { // Blad alokowania
        errno = ENOMEM;
        return NULL;
    }
    return New_nand;
}

void nand_delete(nand_t *g) {
    if (g == NULL)return;
    // odlaczamy sygnaly wejsciowe
    for (unsigned i = 0; i < g->n_of_entries; i++)
        delete_kth_entry_signal(i, g);
    list tmp = g->exits;
    while (tmp != NULL && tmp->gate != NULL) {
        unsigned entry_num = entry_number(g, tmp->gate);
        tmp->gate->entries[entry_num] = NULL;
        tmp = tmp->next;
    }
    del_list(&(g->exits)); // odlaczamy sygnaly wyjsciowe
    // sprawdzic czy g->exits = NULL
    free(g->entry_type);
    g->entry_type = NULL;
    free(g->entries);
    free(g);
    g = NULL;
}

int nand_connect_nand(nand_t *g_out, nand_t *g_in, unsigned k) {
    // jesli ktorys parametr jest niepoprawny
    if (g_out == NULL || g_in == NULL || k >= g_in->n_of_entries) {
        errno = EINVAL;
        return -1;
    }
    if (g_out->exits == NULL) { // jesli wyjscie to pusta lista
        g_out->exits = create_node(g_in);
        if (g_out->exits == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }
    else { // jesli juz cos jest podlaczone do wyjscia g_out
        list new_list = create_node(g_in);
        if (new_list == NULL) { // jesli wystapil blad alokowania pamieci
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
    // jesli ktorys parametr jest niepoprawny
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
    if (g->exits == NULL)return 0;
    if (g->exits->gate == NULL)return 0;
    list tmp = g->exits;
    while (tmp->next != NULL) {
        counter++;
        tmp = tmp->next;
    }
    return counter;
}

void* nand_input(nand_t const *g, unsigned k) {
    // jesli ktorys z parametrow jest niepoprawny
    if (g == NULL || k >= g->n_of_entries) { 
        errno = EINVAL;
        return NULL;
    }

    if (g->entries[k] == NULL) {// jesli nic nie jest podlaczone do wejscia k
        errno = 0;
        return NULL;
    }

    if (g->entry_type[k]) return (nand_t*)(g->entries[k]);
    else return (bool*)(g->entries[k]);
}

nand_t* nand_output(nand_t const *g, ssize_t k) {
    if (g == NULL || k >= nand_fan_out(g)) { // jesli ktorys z parametrow jest niepoprawny
        errno = EINVAL;
        return NULL;
    } // co oznacza nieokreslonosc wyniku tutaj??
    list tmp = g->exits;
    while (k > 0) {
        tmp = tmp->next;
        k--;
    }
    return tmp->gate;
}

// ustawia process bramki g jak i wszystkie poprzedzajace ja bramki na pre
void unprocess_gate(nand_t* g) {
    if (g == NULL)return;
    if (g->process == pre)return;
    g->process = pre;
    g->signal = false;
    g->path_len = 0;
    for (unsigned i = 0; i < g->n_of_entries; i++)
        // jesli wejscie istnieje i jest podlaczone pod bramke nand
        if (g->entries[i] != NULL && g->entry_type[i])
            unprocess_gate((nand_t*)g->entries[i]);
}

// ustawia process wszystkich bramek w tablicy gates,
// jak i wszystkich poprzedzajacych bramek na pre 
void unprocess_gates(nand_t **gates, size_t m) { 
    for (size_t i = 0; i < m; i++) unprocess_gate(gates[i]);
}

// zwraca obliczony sygnal bramki g i ustawia path_len 
// na dlugosc sciezki krytycznej bramki g
bool eval_gate(nand_t* g, ssize_t* path_len) { 
    if (*path_len == -1)return false;
    if (g->process == post) { // jesli bramka zostala juz policzona
        *path_len = g->path_len;
        return g->signal;
    }
    // jesli bramka juz byla w trakcie liczenia to mamy cykl
    if (g->process == in) { 
        errno = ECANCELED;
        *path_len = -1;
        g->path_len = -1;
        return false;
    }
    if (g->n_of_entries == 0) {
        g->signal = false;
        g->path_len = 0;
        *path_len = 0;
        g->process = post;
        return false;
    }
    g->process = in;

    // maksimum dlugosci sciezek krytycznych ze wszystkich wejsc
    ssize_t max_path = 0; 
    bool eval = false; // wynik funkcji eval_gate dla bramek z wejscia
    g->signal = false;

    // iterujemy sie po wejsciach bramki g
    for (unsigned i = 0; i < g->n_of_entries; i++) { 
        // jesli nic nie jest podlaczone pod i-te wejscie
        if (g->entries[i] == NULL) { 
            *path_len = -1;
            g->path_len = -1;
            return false;
        }
        // jesli pod i-te wejscie jest podlaczona bramka nand
        if (g->entry_type[i]) { 
            eval = eval_gate((nand_t*)g->entries[i], path_len);
            if (*path_len == -1) {
                // errno = ECANCELED;
                g->path_len = -1;
                return false;
            }
            else {
                if (!eval) g->signal = true;
                max_path = MAX(max_path, *path_len);
            }
        }
        else { // jesli pod i-te wejscie jest podlaczony sygnal boolowski   
            
            bool *tmp = (bool *)g->entries[i];
            if (!(*tmp)) g->signal = true;
            *path_len = 0;
        }
    }
    max_path++; // dodajemy jeden do dlugosci sciezki zgodnie ze wzorem
    *path_len = max_path;
    g->path_len = max_path;
    g->process = post;
    return g->signal;
}

ssize_t nand_evaluate(nand_t **g, bool *s, size_t m) {
    if (m == 0 || g == NULL || s == NULL){
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
    // tmp_path bedzie ustawiane na dlugosci sciezek poszczegolnych bramek
    ssize_t tmp_path = 0;

    // przechodzimy po tablicy bramek i obliczamy dlugosc
    // sciezki ukladu bramek i wypelniamy tablice s
    for (size_t i = 0; i < m; i++) { 
        // if (g[i] == NULL) {
        //     errno = EINVAL;
        //     return -1;
        // }
        s[i] = eval_gate(g[i], &tmp_path);
        if (g[i]->path_len == -1) {
            errno = ECANCELED;
            return -1;
        }
        max_path = MAX(max_path, g[i]->path_len);
    }
    unprocess_gates(g, m);
    return max_path;
}
