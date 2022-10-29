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
Type getExpType(Node*); // 得到Exp对应的类型
Type copyType(Type);
boolean typeComp(Type, Type); // 比较是否结构相同
void insertSymbol(char*, Type, uint32_t, int); // 将符号插入表中
void processDef(Node*); 
void analyse(Node*);
void type2str(Type, char*); // 将type转换为字符串便于输出调试

int stack_top = -1;
SymbolList symbol_table[HASH_TABLE_SIZE];
SymbolList frame_stack[FRAME_STACK_SIZE];

uint32_t hash(char *name) {
    uint32_t val = 0, i;
    for(; *name; ++name) {
        val = (val << 2) + *name;
        if(i = val & ~(HASH_TABLE_SIZE - 1))
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
    char str[128];
    for(int i = 0; i < stack_top; i++)
        printf("    ");
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
        for(int i = 0; i < stack_top + 1; i++)
            printf("    ");
        memset(str, 0, sizeof(str));
        type2str(head->type, str);
        printf("%s: %s\n", str, head->name);
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
    Type t = getSpeciferType(specifier);
    while(var_dec->type != T_ID) {
        Type arr_type = malloc(SIZEOF(Type_));
        arr_type->kind = ARRAY;
        arr_type->u.array.base = getSpeciferType(specifier);
        arr_type->u.array.elem = t;
        arr_type->u.array.size = var_dec->next->val.type_int;
        var_dec = var_dec->sons;
        t = arr_type;
    }
    (*name) = var_dec->val.type_str;
    return t;
}

Type getExpType(Node* exp) {
    if(exp == NULL)
        return NULL;
    if(exp->type == T_INT) {
        goto RETURN_DEFAULT_EXP_TYPE;
    }
    if(exp->type == T_FLOAT) {
        Type t = malloc(SIZEOF(Type_));
        t->kind = BASIC;
        t->u.basic = TYPE_FLOAT;
        return t;
    }
    if(exp->type == T_ID) {
        uint32_t key = hash(exp->val.type_str);
        SymbolList symbol = NULL;
        for(SymbolList head = symbol_table[key]->tb_next; head != NULL; head = head->tb_next) {
            if(!strcmp(exp->val.type_str, head->name)) {
                symbol = head;
                break;
            }
        }
        if(symbol == NULL) {
            printf("未定义变量: %d", exp->line);
            goto RETURN_DEFAULT_EXP_TYPE;
        }
        return copyType(symbol->type);
    }
    if(!strcmp(exp->name, "FuncCall")) {
        Node *func_symbol = exp->sons, *params = func_symbol->next;
        uint32_t key = hash(func_symbol->val.type_str);
        SymbolList symbol = NULL;
        Param param_type;
        for(SymbolList head = symbol_table[key]->tb_next; head != NULL; head = head->tb_next)
            if(!strcmp(head->name, func_symbol->val.type_str)) {
                symbol = head;
                break;
            }
        if(symbol == NULL) {
            printf("函数未定义: %d\n", func_symbol->line);
            goto RETURN_DEFAULT_EXP_TYPE;
        }
        if(symbol->type->kind != FUNCTION) {
            printf("对普通变量使用函数调用: %d\n", func_symbol->line);
            goto RETURN_DEFAULT_EXP_TYPE;
        }
        param_type = symbol->type->u.func->parameters;
        while(param_type != NULL) {
            if(params == NULL) {
                printf("调用函数时形参不匹配: %d\n", func_symbol->line);
                goto RETURN_DEFAULT_EXP_TYPE;
            }
            Type exp_type = getExpType(params);
            if(!typeComp(param_type->type, exp_type)) {
                printf("调用函数时形参不匹配: %d\n", func_symbol->line);
                goto RETURN_DEFAULT_EXP_TYPE;
            }
            params = params->next;
            param_type = param_type->next;
        }
        if(params != NULL) {
            printf("调用函数时形参不匹配: %d\n", func_symbol->line);
            goto RETURN_DEFAULT_EXP_TYPE;
        }
        return copyType(symbol->type->u.func->ret);
    }
    if(!strcmp(exp->name, "ArrayEval")) {
        // TODO:
        char *arr_name;
        uint32_t key;
        Node *arr_eval = exp;
        Type arr_type = NULL, arr_symbol_type;
        SymbolList symbol = NULL;
        while(!strcmp(arr_eval->name, "ArrayEval"))
            arr_eval = arr_eval->sons;
        arr_name = arr_eval->val.type_str;
        key = hash(arr_name);
        for(SymbolList head = symbol_table[key]->tb_next; head != NULL; head = head->tb_next) {
            if(!strcmp(arr_name, head->name)) {
                symbol = head;
                break;
            }
        }
        if(symbol == NULL) {
            printf("未定义变量: %d", arr_eval->line);
            goto RETURN_DEFAULT_EXP_TYPE;
        }
        if(symbol->type->kind != ARRAY) {
            printf("非数组变量使用数组取值: %d\n", arr_eval->line);
            goto RETURN_DEFAULT_EXP_TYPE;
        }

        arr_type = NULL, arr_symbol_type = symbol->type;
        arr_eval = exp->sons;
        while(arr_eval != NULL) {
            Type outer_arr_type = malloc(SIZEOF(Type_)), i;
            outer_arr_type->kind = ARRAY;
            outer_arr_type->u.array.elem = arr_type;
            outer_arr_type->u.array.size = 0;
            i = getExpType(arr_eval->next);
            if(i->kind != BASIC || i->u.basic != TYPE_INT) {
                printf("数组下标不合法: %d\n", arr_eval->next->line);
            }

            arr_type = outer_arr_type;
            arr_eval = arr_eval->sons;
        }
        while(arr_type != NULL) {
            if(arr_symbol_type->kind != ARRAY) {
                printf("数组下标不合法: %d\n", symbol->line);
                return arr_type;
            }

            arr_type = arr_type->u.array.elem;
            arr_symbol_type = arr_symbol_type->u.array.elem;
        }
        return copyType(arr_symbol_type);
    }
    if(!strcmp(exp->name, "PLUS") || !strcmp(exp->name, "MINUS") || !strcmp(exp->name, "STAR") || !strcmp(exp->name, "DIV")) {
        Type op[2];
        op[0] = getExpType(exp->sons);
        op[1] = getExpType(exp->sons->next);
        if(op[1] == NULL) {
            return op[0];
        }
        if(!typeComp(op[1], op[2]) || op[0]->kind != BASIC) {
            printf("类型不匹配: %d\n", exp->line);
        }
        return op[0];
    }

    RETURN_DEFAULT_EXP_TYPE: {
        Type t = malloc(SIZEOF(Type_));
        t->kind = BASIC;
        t->u.basic = 0;
        return t;
    }
}

Type copyType(Type t) {
    if(t == NULL)
        return NULL;
    Type ret_type = malloc(SIZEOF(Type_));
    switch (t->kind)
    {
    case BASIC:
        ret_type->kind = BASIC;
        ret_type->u = t->u;
        break;
    case ARRAY:
        ret_type->kind = ARRAY;
        ret_type->u.array.size = t->u.array.size;
        ret_type->u.array.base = copyType(t->u.array.base);
        ret_type->u.array.elem = copyType(t->u.array.elem);
        break;
    case STRUCTURE: {
        ret_type->kind = STRUCTURE;
        FieldList *p_list = &ret_type->u.structure, src = t->u.structure;
        while(src != NULL) {
            (*p_list) = malloc(SIZEOF(FieldList_));
            (*p_list)->type = copyType(src->type);
            p_list = &((*p_list)->next), src = src->next;
        }
    }
    case FUNCTION: {
        ret_type->kind = FUNCTION;
        Param *p_list = &(ret_type->u.func->parameters), src = t->u.func->parameters;
        ret_type->u.func->ret = copyType(t->u.func->ret);
        while(src != NULL) {
            (*p_list) = malloc(SIZEOF(Param_));
            (*p_list)->type = copyType(src->type);

            p_list = &((*p_list)->next), src = src->next;
        } 
    }
    default:
        break;
    }

    return ret_type;
}

boolean typeComp(Type a, Type b) {
    if(a == NULL || b == NULL)
        return FALSE;
    if(a->kind != b->kind)
        return FALSE;
    switch(a->kind) {
        case BASIC:
            return a->u.basic == b->u.basic;
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

            if(dec->next != NULL) { // 函数定义开新栈
                pushStack();
            } 

            for(; params != NULL; params = params->next) {
                Node *param_specifier = params->sons, *param = param_specifier->next, *param_vardec = param->sons;
                char *name;
                (*tail) = malloc(SIZEOF(Param_));
                (*tail)->type = getVarDecType(param_specifier, param_vardec, &name);
                (*tail)->name = name;
                if(dec->next != NULL) 
                    insertSymbol(name, (*tail)->type, VARDEC, param_vardec->line);
                tail = &((*tail)->next);
            }

            if(dec->next == NULL) {
                insertSymbol(id->val.type_str, t, FUNCDEC, dec->line);
            } else {
                Node *def_list = dec->next->sons, *stmt_list = def_list->next;
                stack_top--;
                insertSymbol(id->val.type_str, t, FUNCDEF, dec->line);
                analyse(def_list);
            }
        } else {
            for (; dec != NULL; dec = dec->next) {
                if(!strcmp(dec->name, "VarDec")) {
                    Node *var_dec = dec->sons;
                    char *name;
                    Type t = getVarDecType(specifier, var_dec, &name);
                    insertSymbol(name, t, VARDEC, var_dec->line);
                } else {
                    Type t = getExpType(dec);
                    if(!typeComp(t, frame_stack[stack_top]->fs_next->type))
                        printf("类型不匹配: %d\n", dec->line);
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
        else if(!strcmp(root->name, "DefList")) {
            for(Node* def = root->sons; def != NULL; def = def->next)
                processDef(def->sons);
            if(root->next != NULL) {
                // TODO: 处理Stmtlist
            }
        }
    }
    popStack();
}

void start_semantics(Node *root) {
    initTable();
    analyse(root);
}

void type2str(Type t, char* str) {
    switch (t->kind)
    {
        case BASIC:
            sprintf(str, "%s", t->u.basic == TYPE_INT ? "int" : "float");
            break;
        case ARRAY: {
            while(t->kind == ARRAY) {
                sprintf(str + strlen(str), "[%d]", t->u.array.size);
                t = t->u.array.elem;
            }
            type2str(t, str + strlen(str));
            break;
        }
        case FUNCTION: {
            sprintf(str, "(");
            Param params = t->u.func->parameters;
            for(; params != NULL; params = params->next) {
                type2str(params->type, str + strlen(str));
                if(params->next != NULL)
                    sprintf(str + strlen(str), ", ");
            }
            sprintf(str + strlen(str), ") -> ");
            type2str(t->u.func->ret, str + strlen(str));
            break;
        }
        case STRUCTURE: {
            // TODO:
        }
        default:
            break;
    }
}