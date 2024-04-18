#include <stdio.h>
#include <stdlib.h>
#include "structs.h"

list create_node(nand_t *gate) {
    list new_node = (list)malloc(sizeof(node));
    if (new_node) {
        new_node->gate = gate;
        new_node->next = NULL;
    }
    return new_node;
}

void list_push(list head, list item) {
    list tmp = head;
    while (tmp->next != NULL)
        tmp = tmp->next;
    tmp->next = item;
}

void del_node(list *head, nand_t* g) {
    if (g == NULL) return;
    if (*head == NULL)return;
    list cur = *head;
    list prev = cur->next;
    if (cur->gate == g) {
        free(cur);
        *head = prev;
        return;
    }
    while (cur != NULL && cur->gate != g) {
        prev = cur;
        cur = cur->next;
    }
    if (cur == NULL) return;
    prev->next = cur->next;
    free(cur);
    cur = NULL;
}

void del_list(list *head) {
    if (*head == NULL)return;
    list tmp = *head;
    del_list(&(tmp->next));
    free(tmp);
    *head = NULL;
}