#ifndef STRUCTS_H
#define STRUCTS_H

#include "nand.h"

// typedef struct node {
//     nand_t *gate;
//     struct node *next, *prev;
// }node;
typedef struct node {
    nand_t *gate;
    struct node *next;
}node;
typedef struct node* list;

typedef enum Process {pre, in, post} Process;
struct nand {
    unsigned n_of_entries; // liczba wejsc bramki
    // tablica wskaznikow do bramek podlaczonych do wejscia bramki
    void **entries;
    
    // tablica, na i-tym miejscu ma 0 gdy pod i-te wejscie jest podlaczony
    // sygnal boolowski, a 1 gdy pod i-te wejscie jest podlaczona bramka nand
    bool *entry_type; 
    bool signal; // wartosc sygnalu bramki
    Process process; // oznacza w jakim stadium obliczania sciezki krytycznej
    // sie znajduje bramka: pre - przed obliczeniem,
    //  in - w trakcie, post - po obliczeniu

    ssize_t path_len ; // dlugosc sciezki krytycznej danej bramki
    list exits; // lista wskaznikow do 
    // bramek podlaczonych do wyjscia bramki
};

// tworzy element listy który zawiera 
// wskaznik na gate, zwraca wskaznik do tego node'a lub NULL
list create_node(nand_t *gate); 

void list_push(list head, list item); // wstawia item do listy head
void del_node(list *head, nand_t* g); // usuwa bramke g z listy head

// usuwa cala liste, gdzie head to wskaznik na pierwszy element
void del_list(list *head); 

#endif 