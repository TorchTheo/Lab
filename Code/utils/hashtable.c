#include "../include/utils/hashtable.h"

#include <stdlib.h>
#include <string.h>

static List hash_table[HASH_SIZE];

static uint32_t hash(char *name) {
    uint32_t val = 0, i;
    for(; '\0' != *name; ++name) {
        val = (val << 2) + (*name);
        if((i = val) & (~(HASH_SIZE - 1))) {
            val = (val ^ (i >> 12)) & HASH_SIZE;
        }
    }
    return val;
}

uint32_t* insert(char *key) {
    uint32_t index = hash(key);
    for(List head = hash_table[index]; head != NULL; head = head->next)
        if(!strcmp(key, head->key))
            return &(head->value);
    List list_node = malloc(SIZEOF(ListNode));
    list_node->key = key, list_node->value = 0, list_node->next = hash_table[index];
    if(hash_table[index] != NULL)
        hash_table[index]->prev = list_node;
    hash_table[index] = list_node;
    return &(list_node->value);
}

uint32_t *get_value_pointer(char *key) {
    uint32_t index = hash(key);
    for(List head = hash_table[index]; head != NULL; head = head->next)
        if(!strcmp(head->key, key))
            return &(head->value);
    return NULL;
}

uint32_t get_value(char *key) {
    uint32_t index = hash(key);
    for(List head = hash_table[index]; head != NULL; head = head->next)
        if(!strcmp(head->key, key))
            return head->value;
    return 0;
}

uint8_t delete_key_pointer(List list_node) {
    if(list_node == NULL)
        return FALSE;
    if(list_node->next != NULL)
        list_node->next->prev = list_node->prev;
    if(list_node->prev != NULL)
        list_node->prev->next = list_node->next;
    else {
        uint32_t index = hash(list_node->key);
        hash_table[index] = list_node->next;
    }
    free(list_node);
    return TRUE;
}

uint8_t delete_key(char *key) {
    uint32_t index = hash(key);
    for(List head = hash_table[index]; head != NULL; head = head->next) {
        if(!strcmp(head->key, key)) {
            if(head->next != NULL)
                head->next->prev = head->prev;
            if(head->prev != NULL)
                head->prev->next = head->next;
            else
                hash_table[index] = head->next;
            free(head);
            return TRUE;
        }
    }
}
