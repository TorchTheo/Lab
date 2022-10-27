#include "include/type.h"
#include "include/constant.h"
#include <stdio.h>
#include <stdlib.h>
extern Node* root;

void pushStack(); // TODO
void popStack(); // TODO
int stack_top = 0;

uint32_t hash(char *name) {
    uint32_t val = 0, i;
    for(; *name; ++name) {
        val = (val << 2) + *name;
        if(i = val & ~HASH_TABLE_SIZE)
            val = (val ^ (i >> 12)) & HASH_TABLE_SIZE;
    }
    return val;
}