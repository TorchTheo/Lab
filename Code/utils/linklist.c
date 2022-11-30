#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../include/utils/linklist.h"

inline intLinkList intNewLinkList() {
    return calloc(1, SIZEOF(intLinkNode));
}

uint8_t intLinklistInsert(intLinkList lst, int64_t value) {
    assert(lst != NULL);
    for (intLinkList node = lst->next; node != NULL; node = node->next)
        if (node->value == value)
            return 0;
    
    intLinkList new_node = calloc(1, SIZEOF(intLinkNode));
    new_node->value = value;
    new_node->prev = lst, new_node->next = lst->next;
    if (lst->next != NULL)
        lst->next->prev = new_node;
    lst->next = new_node;
    return 1;
}

uint8_t intLinklistDelete(intLinkList lst, int64_t value) {
    assert(lst != NULL);
    for (intLinkList node = lst->next; node != NULL; node = node->next)
        if (node->value == value) {
            node->prev->next = node->next;
            if (node->next != NULL)
                node->next->prev = node->prev;
            free(node);
            return 1;
        }
    return 0;
}

uint8_t intLinklistClear(intLinkList lst) {
    while (lst->next != NULL) {
        intLinkList to_delete = lst->next;
        lst->next = to_delete->next;
        free(to_delete);
    }
    return 1;
}

strLinkList strNewLinkList() {
    return calloc(1, SIZEOF(strLinkNode));
}

uint8_t strLinklistInsert(strLinkList lst, char *value) {
    assert(lst != NULL);
    for (strLinkList node = lst->next; node != NULL; node = node->next)
        if (!strcmp(node->value, value))
            return 0;
        
    strLinkList new_node = calloc(1, SIZEOF(strLinkNode));
    new_node->value = value;
    new_node->prev = lst, new_node->next = lst->next;

    if (lst->next != NULL)
        lst->next->prev = new_node;
    
    lst->next = new_node;
    return 1;
}

uint8_t strLinklistDelete(strLinkList lst, char *value) {
    assert(lst != NULL);
    for (strLinkList node = lst->next; node != NULL; node = node->next)
        if (!strcmp(node->value, value)) {
            node->prev->next = node->next;
            if (node->next != NULL)
                node->next->prev = node->prev;
            free(node);
            return 1;
        }
    return 0;
}

uint8_t strLinklistClear(strLinkList lst) {
    while (lst->next != NULL) {
        strLinkList to_delete = lst->next;
        lst->next = to_delete->next;
        free(to_delete);
    }
    return 1;
}