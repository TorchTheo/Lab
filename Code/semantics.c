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
Type getSpeciferType(Node*); // 从Specifier中获取类型
Type getVarDecType(Node*, Node*, char**); // 从VarDec中获得类型
boolean typeComp(Type, Type); // 比较是否结构相同
void insertSymbol(char*, Type, uint32_t, int); // 将符号插入表中
void processDef(Node*); 
void analyse(Node*);
char *type2str(Type); // 将type转换为字符串便于输出调试

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
        if(head->func_type == DEC && head->type->kind == FUNCTION) {
            boolean has_def = FALSE;
            for(SymbolList symbol = symbol_table[head->key]->tb_next; symbol != NULL; symbol = symbol->tb_next) {
                if(!strcmp(head->name, symbol->name) && symbol->func_type == DEF) {
                    has_def = TRUE;
                    break;
                }
            }
            if(!has_def) {
                printf("函数'%s'声明未定义: %d\n", head->name, head->line);
                // return;
            }
        }
        printf("%s: %s\n", type2str(head->type), head->name);
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

Type getVarDecType(Node* specifier, Node* var_dec, char **name) {
    Type t = NULL, *tail = &t;
    while(var_dec->type != T_ID) {
        *tail = malloc(SIZEOF(Type_));
        (*tail)->kind = ARRAY;
        (*tail)->u.array.size = var_dec->next->val.type_int;
        var_dec = var_dec->sons;
        tail = &((*tail)->u.array.elem);
    }
    (*tail) = getSpeciferType(specifier);
    (*name) = var_dec->val.type_str;
    return t;
}

boolean typeComp(Type a, Type b) {
    if(a->kind != b->kind)
        return FALSE;
    switch(a->kind) {
        case ARRAY:
            if(a->u.array.size != b->u.array.size)
                return FALSE;
            return typeComp(a->u.array.elem, b->u.array.elem);
        case STRUCTURE:{
            FieldList l[2] = {a->u.structure, b->u.structure};
            while(l[0] != NULL || l[1] != NULL) {
                if(l[0] == NULL || l[1] == NULL)
                    return FALSE;
                if(!typeComp(l[0]->type, l[1]->type))
                    return FALSE;
                l[0] = l[0]->next;
                l[1] = l[1]->next;
            }
            break;
        }  
        case FUNCTION: {
            if(!typeComp(a->u.func->ret, b->u.func->ret))
                return FALSE;
            Param l[2] = {a->u.func->parameters, b->u.func->parameters};
            while(l[0] != NULL || l[1] != NULL) {
                if(l[0] == NULL || l[1] == NULL)
                    return FALSE;
                if(!typeComp(l[0]->type, l[1]->type))
                    return FALSE;
                l[0] = l[0]->next;
                l[1] = l[1]->next;
            }
            break;
        }
    }
    return TRUE;
}

void insertSymbol(char* name, Type type, uint32_t kind, int line) {
    uint32_t key = hash(name);
    SymbolList symbol = malloc(SIZEOF(SymbolList_));

    switch(kind) {
        case VARDEC:
            for(SymbolList head = symbol_table[key]->tb_next; head != NULL; head = head->tb_next) {
                if(head->dep != stack_top)
                    continue;
                if(!strcmp(head->name, name)) {
                    // TODO: 同层重定义
                    printf("变量'%s'重定义: %d\n", name, head->line);
                    return;
                }
            }
            break;
        case FUNCDEC:
            for(SymbolList head = symbol_table[key]->tb_next; head != NULL; head = head->tb_next) {
                if(!strcmp(head->name, name)) {
                    if(head->type->kind == FUNCTION && typeComp(head->type, type)) {
                        printf("函数'%s'重复声明: %d\n", name, head->line);
                        return;
                    }
                    
                }
            }
        case FUNCDEF:
            for(SymbolList head = symbol_table[key]->tb_next; head != NULL; head = head->tb_next) {
                if(!strcmp(head->name, name)) {
                    if(head->type->kind == FUNCTION && head->func_type == DEC && !typeComp(head->type, type)) {
                        printf("函数'%s'定义声明不匹配: 定义位于line%d, 声明位于line%d\n", name, line, head->line);
                        return;
                    }
                    
                }
            }
        default:
            break;
    }
    if(type->kind == FUNCTION)
        symbol->func_type = (kind == FUNCDEC ? DEC : DEF);

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
        if(!strcmp(dec->name, "FunDec")) {
            Node *id = dec->sons, *params = id->next;
            Type t = malloc(SIZEOF(Type_));
            t->kind = FUNCTION;
            t->u.func = malloc(SIZEOF(Func_));
            t->u.func->ret = getSpeciferType(specifier);
            Param *tail = &(t->u.func->parameters);

            for(; params != NULL; params = params->next) {
                Node *param_specifier = params->sons, *param = param_specifier->next, *param_vardec = param->sons;
                char *name;
                (*tail) = malloc(SIZEOF(Param_));
                (*tail)->type = getVarDecType(param_specifier, param_vardec, &name);
                (*tail)->name = name;
                tail = &((*tail)->next);
            }

            if(dec->next == NULL) {
                insertSymbol(id->val.type_str, t, FUNCDEC, dec->line);
            } else {
                insertSymbol(id->val.type_str, t, FUNCDEF, dec->line);
            }
        } else {
            for (; dec != NULL; dec = dec->next) {
                if(!strcmp(dec->name, "VarDec")) {
                    Node *var_dec = dec->sons;
                    char *name;
                    Type t = getVarDecType(specifier, var_dec, &name);
                    insertSymbol(name, t, VARDEC, var_dec->line);
                }
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

char *type2str(Type t) {
    switch (t->kind)
    {
        case BASIC:
            return (t->u.basic == TYPE_INT ? "int" : "float");
            break;
        case ARRAY: {
            char *ret = type2str(t->u.array.elem);
            if(t->u.array.elem->kind != ARRAY)
                printf("%s", ret);
            printf("[%d]", t->u.array.size);
            return "";
        }
        case FUNCTION: {
            printf("(");
            Param params = t->u.func->parameters;
            for(; params != NULL; params = params->next) {
                printf("%s", type2str(params->type));
                if(params->next != NULL)
                    printf(", ");
            }
            printf(") -> ");
            return type2str(t->u.func->ret);
        }
        default:
            break;
    }
}