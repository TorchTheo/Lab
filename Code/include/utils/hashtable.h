#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include "../type.h"
#include "../constant.h"
#define HASH_SIZE 0x4000

typedef struct ListNode *List;
struct ListNode;

uint64_t* insert(char *key);
uint64_t *get_value_pointer(char *key);
uint64_t get_value(char *key);
uint8_t delete_key_pointer(List list_node);
uint8_t delete_key(char *key);
void clear_table();

#endif