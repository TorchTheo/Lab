#include "include/type.h"
#include "include/constant.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
extern Node* root;

void popDelete(SymbolList);
void pushStack();
void popStack();
void initTable();
Type getSpeciferType(Node*);
void insertSymbol(char*, Type, uint32_t, int);
void processDef(Node*);
void analyse(Node*);

int stack_top = -1;
SymbolList symbol_table[HASH_TABLE_SIZE];
SymbolList frame_stack[FRAME_STACK_SIZE];

uint32_t hash(char *name) {
    uint32_t val = 0, i;
    for(; *name; ++name) {
        val = (val << 2) + *name;
        if(i = val & ~HASH_TABLE_SIZE)
            val = (val ^ (i >> 12)) & HASH_TABLE_SIZE;
    }
    return val;
}

void popDelete(SymbolList node) {
    if(node == NULL)
        return;
    popDelete(node->fs_next);
    node->tb_prev->tb_next = node->tb_next;
    if(node->tb_next != NULL)
        node->tb_next->tb_prev = node->tb_prev;
    free(node);
}

void pushStack() {
    stack_top++;
    SymbolList head = malloc(SIZEOF(SymbolList_));
    frame_stack[stack_top] = head;
    // TODO: 完善
}

void popStack() {
    if(stack_top < 0) {
        // TODO: error
    }

    // debug info
    printf("Frame %d:\n", stack_top);
    for(SymbolList head = frame_stack[stack_top]->fs_next; head != NULL; head = head->fs_next) {
        switch (head->type->kind)
        {
            case BASIC:
                printf("%s: %s\n", (head->type->u.basic == TYPE_INT ? "int" : "float"), head->name);
                break;
            case ARRAY: {
                Type type = head->type;
                while(type->kind == ARRAY) {
                    printf("[%d]", type->u.array.size);
                    type = type->u.array.elem;
                }
                printf("%s: %s\n", (type->u.basic == TYPE_INT ? "int" : "float"), head->name);
                break;
            }
        
            default:
                break;
        }
    }
    printf("\n");
    //
    if(frame_stack[stack_top]->fs_next != NULL)
        popDelete(frame_stack[stack_top]->fs_next);
    if(frame_stack[stack_top] != NULL) {
        free(frame_stack[stack_top]);
        frame_stack[stack_top] = NULL;
    }
    stack_top--;
    // TODO: 完善
}

void initTable() {
    for(int i = 0; i < HASH_TABLE_SIZE; i++) {
        SymbolList head = malloc(SIZEOF(SymbolList_));
        symbol_table[i] = head;
        // symbol_table[i] = NULL;
    }
}

Type getSpeciferType(Node* specifier) {
    Type t = malloc(SIZEOF(Type_));
    t->kind = BASIC;
    if(!strcmp(specifier->val.type_str, "int"))
        t->u.basic = TYPE_INT;
    else
        t->u.basic = TYPE_FLOAT;
    return t;
}

void insertSymbol(char* name, Type type, uint32_t kind, int line) {
    if(type->kind == FUNCTION) {
        // TODO:
    }
    uint32_t key = hash(name);
    SymbolList symbol = malloc(SIZEOF(SymbolList_));

    switch(kind) {
        case VARDEC:
            for(SymbolList head = symbol_table[key]->tb_next; head != NULL; head = head->tb_next) {
                if(head->dep != stack_top)
                    continue;
                if(!strcmp(head->name, name)) {
                    // TODO: 同层重定义
                    printf("%s重定义: %d\n", name, head->line);
                    return;
                }
            }
            break;
        default:
            break;
    }

    symbol->name = name;
    symbol->dep = stack_top;
    symbol->key = key;
    symbol->line = line;
    symbol->type = type;

    // 添加到符号表中
    if(symbol_table[key]->tb_next != NULL)
        symbol_table[key]->tb_next->tb_prev = symbol;
    symbol->tb_next = symbol_table[key]->tb_next;
    symbol->tb_prev = symbol_table[key];
    symbol_table[key]->tb_next = symbol;
    // 添加到层栈中
    symbol->fs_next = frame_stack[stack_top]->fs_next;
    frame_stack[stack_top]->fs_next = symbol;
}

void processDef(Node* node) {
    // TODO: 完善
    Node *specifier = node, *dec = node->next;
    if(specifier->type == T_TYPE) {
        for (; dec != NULL; dec = dec->next) {
            if(!strcmp(dec->name, "VarDec")) {
                Node *info = dec->sons;
                Type t = NULL, *tail = &t;
                while(info->type != T_ID) {
                    *tail = malloc(SIZEOF(Type_));
                    (*tail)->kind = ARRAY;
                    (*tail)->u.array.size = info->next->val.type_int;
                    info = info->sons;
                    tail = &((*tail)->u.array.elem);
                }
                (*tail) = getSpeciferType(specifier);
                insertSymbol(info->val.type_str, t, VARDEC, info->line);
            }
        }
    }
}

void analyse(Node* root) {
    pushStack();
    if(root == NULL || root->type == T_NULL)
        return;
    if(root->type == T_NTERMINAL) {
        if(!strcmp(root->name, "ExtDef"))
            for(; root != NULL; root = root->next)
                processDef(root->sons);
    }
    popStack();
}

void start_semantics(Node *root) {
    initTable();
    analyse(root);
}