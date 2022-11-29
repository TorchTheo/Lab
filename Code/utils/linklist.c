#include <stdlib.h>
#include <assert.h>

#include "../include/utils/linklist.h"

inline LinkList newLinkList() {
    return calloc(1, SIZEOF(LinkNode));
}

uint8_t linklistInsert(LinkList lst, int64_t value) {
    assert(lst != NULL);
    for (LinkList node = lst->next; node != NULL; node = node->next)
        if (node->value == value)
            return 0;
    
    LinkList new_node = calloc(1, SIZEOF(LinkNode));
    new_node->value = value;
    new_node->prev = lst, new_node->next = lst->next;

    lst->next = new_node;
    return 1;
}

uint8_t linklistDelete(LinkList lst, int64_t value) {
    assert(lst != NULL);
    for (LinkList node = lst->next; node != NULL; node = node->next)
        if (node->value == value) {
            node->prev->next = node->next;
            if (node->next != NULL)
                node->next->prev = node->prev;
            free(node);
            break;
        }
    return 0;
}

uint8_t linklistClear(LinkList lst) {
    while (lst->next != NULL) {
        LinkList to_delete = lst->next;
        lst->next = to_delete->next;
        free(to_delete);
    }
    return 1;
}