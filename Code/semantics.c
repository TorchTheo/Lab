#define __SEMATICS_C__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "include/type.h"
#include "include/constant.h"

extern int error;
// Hash Table & Table Stack
static SymbolList hash_table[HASH_TABLE_SIZE];
static SymbolList frame_stack[FRAME_STACK_SIZE];
static uint32_t st_top = 0;

// global return type
static Type ret_type = NULL;

// struct flag
static uint8_t struct_state = 0; // 0 is vardec, 1 is struct define
static uint32_t anonymous_struct_cnt = 0;
static uint32_t anonymous_field_cnt = 0;

void init();
void analyse(Node*);
void s_error(int, int, ...);
void type2str(Type, char*);
void processDef(Node*);
void processStmt(Node*);
uint32_t hash_pjw(char*);
void insertSymbol(char*, Type, uint32_t, uint32_t);
Type lookUpSymbol(char*);
void pushStack();
void popStack();
SymbolList getStackTop();
uint8_t typeComp(Type, Type);
void registerStruct(Node*);
Type getSpecifierType(Node*);
Type getVarDecType(Node*, Node*, char**);
Type getExpType(Node*);
FieldList getFieldType(Node*, uint32_t);
void deleteType(Type);
Type copyType(Type);


void init() {
    static struct Type_ read_type, write_type, static_int_type;

    static_int_type.kind = BASIC;
    static_int_type.u.basic = TYPE_INT;
    static_int_type.size = 4;

    static struct Func_ read_func_type;
    read_type.kind = FUNCTION;
    read_type.u.function = &read_func_type;
    read_func_type.ret = &static_int_type;

    static struct Func_ write_func_type;
    static struct FieldList_ write_params_type;
    write_type.kind = FUNCTION;
    write_type.u.function = &write_func_type;
    write_func_type.ret = &static_int_type;
    write_func_type.parameters = &write_params_type;
    write_params_type.name = "WRITE_PARAM";
    write_params_type.type = &static_int_type;

    insertSymbol("read", &read_type, FUNCDEF, (uint32_t)(-1));
    insertSymbol("write", &write_type, FUNCDEF, (uint32_t)(-1));
}

void analyse(Node *node) {
    pushStack();

    /* [x]: main process */
    if(node == NULL)
        return;
    if(node->type == T_NTERMINAL) {
        if(!strcmp(node->name, "ExtDef"))
            for(; node != NULL; node = node->next)
                processDef(node->sons);
        else if(!strcmp(node->name, "DefList")) {
            for(Node *def = node->sons; def != NULL; def = def->next)
                processDef(def->sons);
            for(Node *stmt = node->next->sons; stmt != NULL; stmt = stmt->next)
                processStmt(stmt->sons);
        }
    }

    // check function declaration
    for(SymbolList head = frame_stack[st_top]; head != NULL; head = head->fs_next) {
        if(head->func_type == DEC && head->type->kind == FUNCTION) {
            int has_def = 0;
            for(SymbolList p_id = hash_table[head->key]; p_id != head; p_id = p_id->tb_next) {
                if(!strcmp(head->name, p_id->name))
                    has_def = (p_id->func_type == DEF);
                if(has_def)
                    break;
            }
            if(!has_def)
                s_error(18, head->line, head->name);
        }
    }

    popStack();
}

void s_error(int type, int line, ...) {
    error = 1;
    va_list args;
    va_start(args, line);
    switch (type)
    {
        case 0:
            assert(0);
            break;
        case 1 ... 4: case 9 ... 11: case 14 ... 19:
            printf(formats[type], type, line, va_arg(args, char *));
            break;
        default:
            printf(formats[type], type, line);
            break;
    }
    va_end(args);
}

void type2str(Type type, char *str) {
    switch (type->kind)
    {
        case BASIC:
            sprintf(str, "%s", type->u.basic == TYPE_INT ? "int" : "float");
            break;
        case ARRAY:
            while(type->kind == ARRAY) {
                sprintf(str + strlen(str), "[%d]", type->u.array.size);
                type = type->u.array.elem;
            }
            type2str(type, str + strlen(str));
            break;
        case FUNCTION: {
            sprintf(str, "(");

            FieldList param = type->u.function->parameters;
            for(; param != NULL; param = param->next) {
                type2str(param->type, str + strlen(str));
                if(param->next != NULL)
                    sprintf(str + strlen(str), ", ");
            }

            sprintf(str + strlen(str), ")->");
            type2str(type->u.function->ret, str + strlen(str));
            break;
        }
        case STRUCTURE: {
            sprintf(str, "{");

            FieldList field = type->u.structure;
            for(; field != NULL; field = field->next) {
                type2str(field->type, str + strlen(str));
                sprintf(str + strlen(str), ": %s", field->name);
                if(field->next != NULL)
                    sprintf(str + strlen(str), "; ");
            }

            sprintf(str + strlen(str), "}");
            break;
        }
        default:
            break;
    }
}

void processDef(Node *node) {
    assert(node != NULL);

    Node *specifier = node, *dec = node->next;
    if(dec == NULL) {
        registerStruct(specifier);
    }
    else if(!strcmp(dec->name, "FunDec")) {
        Node *id = dec->sons, *param_dec_list = id->next, *param_dec;
        Type type = malloc(SIZEOF(Type_));
        type->kind = FUNCTION;
        type->u.function = malloc(SIZEOF(Func_));
        type->u.function->ret = getSpecifierType(specifier);
        if(param_dec_list != NULL)
            type->u.function->parameters = getFieldType(param_dec_list, FIELD_TYPE_PARAM);

        if(dec->next == NULL)
            insertSymbol(id->val.type_str, type, FUNCDEC, dec->line);
        else {
            FieldList param_field = type->u.function->parameters;
            insertSymbol(id->val.type_str, type, FUNCDEF, dec->line);
            pushStack();
            if(param_dec_list != NULL) {
                param_dec = id->next->sons;
                for(; param_dec != NULL; param_dec = param_dec->next, param_field = param_field->next)
                    insertSymbol(param_field->name, copyType(param_field->type), VARDEC, param_dec->line);
            }

            ret_type = type->u.function->ret;
            st_top--;
            Node *def_list = (Node *)(((Node *)(dec->next))->sons);
            analyse(def_list);
            ret_type = NULL;
        }
    }
    else {
        for(; dec != NULL; dec = dec->next) {
            if(!strcmp(dec->name, "VarDec")) {
                Node *vardec = dec->sons;
                char *name;
                insertSymbol(name, getVarDecType(specifier, vardec, &name), VARDEC, vardec->line);
            }
            else {
                Type exp_type = getExpType(dec);
                if(typeComp(exp_type, getStackTop()->type))
                    s_error(5, dec->line);
                deleteType(exp_type);
            }
        }
    }
}

// [x]: process Stmt
void processStmt(Node *node) {
    if(!strcmp(node->name, "CompSt"))
        analyse(node->sons);
    else if(!strcmp(node->name, "IF")) {
        Node *cond = node->sons, *if_stmt = cond->next;
        Type cond_type = getExpType(cond->sons);
        if(cond_type->kind != BASIC || cond_type->u.basic != 0)
            s_error(7, cond->line);
        
        deleteType(cond_type);
        processStmt(if_stmt->sons);
        if(node->next != NULL)
            processStmt(node->next->sons->sons);
    }
    else if(!strcmp(node->name, "WHILE")) {
        Node *cond = node->sons, *while_stmt = cond->next;
        Type cond_type = getExpType(cond->sons);
        if(cond_type->kind != BASIC || cond_type->u.basic != 0)
            s_error(7, cond->line);
        
        deleteType(cond_type);
        processStmt(while_stmt->sons);
    }
    else if(!strcmp(node->name, "RETURN")) {
        Type exp_type = getExpType(node->sons);
        if(typeComp(exp_type, ret_type))
            s_error(8, node->line);
        deleteType(exp_type);
    }
    else {
        Type exp_type = getExpType(node);
        deleteType(exp_type);
    }
}

uint32_t hash_pjw(char *name) {
    uint32_t val = 0, i;
    for(; '\0' != *name; ++name) {
        val = (val << 2) + (*name);
        if((i = val) & (~(HASH_TABLE_SIZE - 1)))
            val = (val ^ (i >> 12)) & HASH_TABLE_SIZE;
    }
    return val;
}

void insertSymbol(char *name, Type type, uint32_t kind, uint32_t line) {
    if(strcmp(name, "read") && strcmp(name, "write"))
        assert(st_top);

    uint32_t key = hash_pjw(name);
    SymbolList p_id;

    switch (kind)
    {
        case VARDEC:
            for(SymbolList head = hash_table[key]; head != NULL; head = head->tb_next) {
                if(!strcmp(name, head->name)) {
                    if(head->type->kind == BASIC && head->dep < st_top)
                        break;
                    s_error(3, line, head->name);
                    return;
                }
            }
            break;
        case FUNCDEC:
            for(SymbolList head = hash_table[key]; head != NULL; head = head->tb_next) {
                if(!strcmp(name, head->name)) {
                    if(head->type->kind == FUNCTION && 0 == typeComp(type, head->type))
                        return;
                    s_error(19, line, head->name);
                    return;
                }
            }
            break;
        case FUNCDEF:
            for(SymbolList head = hash_table[key]; head != NULL; head = head->tb_next) {
                if(!strcmp(name, head->name)) {
                    if(head->func_type == DEC & head->type->kind == FUNCTION) {
                        if(0 == typeComp(type, head->type))
                            break;
                        s_error(19, line, head->name);
                        return;
                    }
                    s_error(4, line, head->name);
                    return;
                }
            }
            break;
        case PARAMDEC: {
            for(SymbolList head = hash_table[key]; head != NULL; head = head->tb_next) {
                if(!strcmp(name, head->name)) {
                    if(head->type->kind == BASIC && head->dep < st_top)
                        break;
                    s_error(3, line, head->name);
                    name = malloc(30 * sizeof(char));
                    sprintf(name, "AnonymousField@%d", anonymous_field_cnt++);
                }
            }
            break;
        }
        case FIELDDEC: {
            for(SymbolList head = hash_table[key]; head != NULL; head = head->tb_next) {
                if(!strcmp(name, head->name)) {
                    if(head->dep == st_top) {
                        s_error(15, line, head->name);
                        name = malloc(30 * sizeof(char));
                        sprintf(name, "AnonymousField@%d", anonymous_field_cnt++);
                    }
                    break;
                }
            }
            break;
        }
        case STRUCTDEF:
            for(SymbolList head = hash_table[key]; head != NULL; head = head->tb_next) {
                if(!strcmp(name, head->name)) {
                    s_error(16, line, head->name);
                    return;
                }
            }
            break;

        default:
            break;
    }

    p_id = malloc(SIZEOF(SymbolList_));
    p_id->name = name;
    p_id->dep = st_top;
    p_id->key = key;
    p_id->line = line;
    p_id->type = type;

    switch (kind)
    {
        case VARDEC: case FUNCDEC: case PARAMDEC: case FIELDDEC:
            p_id->func_type = DEC;
            break;
        case FUNCDEF: case STRUCTDEF:
            p_id->func_type = DEF;
            break;
        default:
            break;
    }

    p_id->tb_next = hash_table[key];
    hash_table[key] = p_id;
    p_id->fs_next = frame_stack[st_top];
    frame_stack[st_top] = p_id;
}

Type lookUpSymbol(char *name) {
    for(SymbolList head = hash_table[hash_pjw(name)]; head != NULL; head = head->tb_next)
        if(!strcmp(name, head->name))
            return head->type;
    return NULL;
}

void pushStack() {
    st_top++;
}

void popStack() {
    for(SymbolList head = frame_stack[st_top], next; head != NULL; head = next) {
        next = head->fs_next;
        hash_table[head->key] = head->tb_next;
        deleteType(head->type);
        free(head);
    }
    frame_stack[st_top--] = NULL;
}

SymbolList getStackTop() {
    return frame_stack[st_top];
}

// return 0 if two types is structural equivalence
uint8_t typeComp(Type a, Type b) {
    if(a == NULL || b == NULL)
        return 1;
    if(a->kind != b->kind)
        return 1;
    switch (a->kind)
    {
        case BASIC:
            return a->u.basic != b->u.basic;
        case ARRAY:
            return typeComp(a->u.array.elem, b->u.array.elem);
        case STRUCTURE: {
            FieldList lst[2] = {
                a->u.structure,
                b->u.structure
            };
            while(lst[0] != NULL || lst[1] != NULL) {
                if(lst[0] == NULL || lst[1] == NULL)
                    return 1;
                if(typeComp(lst[0]->type, lst[1]->type))
                    return 1;
                lst[0] = lst[0]->next;
                lst[1] = lst[1]->next;
            }
            break;
        }
        case FUNCTION: {
            if(typeComp(a->u.function->ret, b->u.function->ret))
                return 1;
            struct Type_ params[2];
            params[0].kind = params[1].kind = STRUCTURE;
            params[0].u.structure = a->u.function->parameters;
            params[1].u.structure = b->u.function->parameters;
            if(typeComp(&params[0], &params[1]))
                return 1;
            break;
        }

        default:
            break;
    }
    return 0;
}

void registerStruct(Node *specifier) {
    uint8_t last_state = struct_state;
    struct_state = 1;
    getSpecifierType(specifier);
    struct_state = last_state;
}

Type getSpecifierType(Node *specifier) {
    Type type = malloc(SIZEOF(Type_));
    if(specifier->type == T_TYPE) {
        type->kind = BASIC;
        if(!strcmp(specifier->val.type_str, "int")) {
            type->u.basic = TYPE_INT;
        }
        else {
            type->u.basic = TYPE_FLOAT;
        }
        type->size = 4;
    }
    else {
        assert(!strcmp(specifier->name, "StructSpecifier"));

        type->kind = STRUCTURE;
        type->size = 0;

        Node *details = specifier->sons, *id = NULL, *def_list = NULL;
        if(details->type == T_ID)
            id = details;
        else
            def_list = details, id = (Node *)(def_list->next);

        char *name;
        if(id != NULL)
            name = id->val.type_str;
        else {
            // generate a name
            name = malloc(30 * sizeof(char));
            sprintf(name, "AnonymousStruct@%d", anonymous_struct_cnt++);
        }

        if(def_list != NULL) {
            uint8_t last_state = struct_state;
            struct_state = 0;
            type->u.structure = getFieldType(def_list, FIELD_TYPE_STRUCT);
            struct_state = last_state;
        }

        for(FieldList lst = type->u.structure; lst != NULL; lst = lst->next)
            type->size += lst->type->size;

        switch (struct_state)
        {
            case 0: {
                uint32_t key = hash_pjw(name);
                SymbolList p_id = NULL;
                for(SymbolList head = hash_table[key]; head != NULL; head = head->tb_next) {
                    if(!strcmp(name, head->name)) {
                        p_id = head;
                    }
                }
                if(p_id == NULL) {
                    if(def_list == NULL) { // struct A var;
                        s_error(17, specifier->line, name);
                    }
                    else { // struct A { int a; } var;
                        insertSymbol(name, copyType(type), STRUCTDEF, specifier->line);
                    }
                }
                else {
                    if(def_list == NULL) { // struct A var;
                        if(p_id->type->kind != STRUCTURE) {
                            s_error(16, specifier->line, name);
                        }
                        else {
                            free(type);
                            type = copyType(p_id->type);
                        }
                    }
                    else { // struct A { int a; } var;
                        s_error(16, specifier->line, name);
                    }
                }
                break;
            }
            case 1:
                insertSymbol(name, type, STRUCTDEF, specifier->line);
                break;

            default:
                assert(0);
                break;
        }
    }
    return type;
}

Type getVarDecType(Node *specifier, Node *vardec, char **p_name) {
    Type type = getSpecifierType(specifier);
    while(vardec->type != T_ID) {
        Type array_type = malloc(SIZEOF(Type_));
        array_type->kind = ARRAY;
        array_type->u.array.elem = type;
        array_type->u.array.size = vardec->next->val.type_int;
        array_type->size = array_type->u.array.size * type->size;

        vardec = vardec->sons;
        type = array_type;
    }
    (*p_name) = vardec->val.type_str;
    return type;
}

Type getExpType(Node *exp) {
    if(exp == NULL) {
        return NULL;
    }
    if(exp->type == T_INT) {
        goto RETURN_DEFAULT_EXP_TYPE;
    }
    if(exp->type == T_FLOAT) {
        Type type = malloc(SIZEOF(Type_));
        type->kind = BASIC;
        type->u.basic = 1;
        return type;
    }
    if(exp->type == T_ID) {
        uint32_t key = hash_pjw(exp->val.type_str);
        SymbolList p_id = NULL;
        for(SymbolList head = hash_table[key]; head != NULL; head = head->tb_next) {
            if(!strcmp(exp->val.type_str, head->name)) {
                p_id = head;
                break;
            }
        }
        if(p_id == NULL) {
            s_error(1, exp->line, exp->val.type_str);
            goto RETURN_DEFAULT_EXP_TYPE;
        }
        if(p_id->type->kind == POINTER) {
            return copyType(p_id->type->u.pointer);
        }
        return copyType(p_id->type);
    }
    if(!strcmp(exp->name, "FuncCall")) {
        Node *func_id_node = exp->sons, *param_node = func_id_node->next;
        uint32_t key = hash_pjw(func_id_node->val.type_str);
        SymbolList p_id = NULL;
        FieldList param_type;
        for(SymbolList head = hash_table[key]; head != NULL; head = head->tb_next) {
            if(!strcmp(func_id_node->val.type_str, head->name)) {
                p_id = head;
                break;
            }
        }
        if(p_id == NULL) {
            s_error(2, func_id_node->line, func_id_node->val.type_str);
            goto RETURN_DEFAULT_EXP_TYPE;
        }
        if(p_id->type->kind != FUNCTION) {
            s_error(11, func_id_node->line, p_id->name);
            goto RETURN_DEFAULT_EXP_TYPE;
        }
        param_type = p_id->type->u.function->parameters;
        while(param_type != NULL) {
            if(param_node == NULL) {
                char *str = malloc(128 * sizeof(char));
                type2str(p_id->type, str);
                s_error(9, func_id_node->line, str);
                free(str);
                goto RETURN_DEFAULT_EXP_TYPE;
            }
            Type exp_type = getExpType(param_node);
            if(typeComp(param_type->type, exp_type)) {
                char *str = malloc(128 * sizeof(char));
                type2str(p_id->type, str);
                s_error(9, func_id_node->line, str);
                free(str);
                deleteType(exp_type);
                goto RETURN_DEFAULT_EXP_TYPE;
            }
            deleteType(exp_type);
            param_node = (Node *)(param_node->next);
            param_type = param_type->next;
        }
        if(param_node != NULL) {
            char *str = malloc(128 * sizeof(char));
            type2str(p_id->type, str);
            s_error(9, func_id_node->line, str);
            free(str);
            goto RETURN_DEFAULT_EXP_TYPE;
        }

        return copyType(p_id->type->u.function->ret);
    }
    if(!strcmp(exp->name, "ArrayEval")) {
        // [x]: evaluate array
        Node *arr_node = exp->sons, *ind_node = arr_node->next;
        Type arr_type = getExpType(arr_node), ind_type = getExpType(ind_node), ret_type;
        if(arr_type->kind != ARRAY) {
            s_error(10, arr_node->line, "some other variables");
            deleteType(arr_type), deleteType(ind_type);
            goto RETURN_DEFAULT_EXP_TYPE;
        }
        if(ind_type->kind != BASIC || ind_type->u.basic != 0)
            s_error(12, ind_node->line);
        ret_type = copyType(arr_type->u.array.elem);
        deleteType(arr_type), deleteType(ind_type);
        return ret_type;
    }

    if(!strcmp(exp->name, "PLUS") ||
       !strcmp(exp->name, "MINUS") ||
       !strcmp(exp->name, "STAR") ||
       !strcmp(exp->name, "DIV") ||
       !strcmp(exp->name, "RELOP")) {
        Type op_type[2];
        op_type[0] = getExpType(exp->sons);
        op_type[1] = getExpType(exp->sons->next);
        if(op_type[1] == NULL)
            return op_type[0];
        if(typeComp(op_type[0], op_type[1]))
            s_error(7, exp->line);
        else if(op_type[0]->kind != BASIC)
            s_error(7, exp->line);
        deleteType(op_type[1]);
        if(!strcmp(exp->name, "RELOP")) {
            deleteType(op_type[0]);
            goto RETURN_DEFAULT_EXP_TYPE;
        }
        return op_type[0];
    }

    if(!strcmp(exp->name, "AND") ||
       !strcmp(exp->name, "OR") ||
       !strcmp(exp->name, "NOT")) {
        Type op_type[2];
        op_type[0] = getExpType(exp->sons);
        op_type[1] = getExpType(exp->sons->next);
        if(op_type[1] != NULL) {
            if(op_type[1]->kind != BASIC || op_type[1]->u.basic != 0)
                s_error(7, exp->line);
            deleteType(op_type[1]);
        }
        if(op_type[0]->kind != BASIC || op_type[0]->u.basic != 0) {
            s_error(7, exp->line);
            deleteType(op_type[0]);
            goto RETURN_DEFAULT_EXP_TYPE;
        }
        return op_type[0];
    }

    if(!strcmp(exp->name, "ASSIGNOP")) {
        Type op_type[2];
        op_type[0] = getExpType(exp->sons);
        op_type[1] = getExpType(exp->sons->next);
        if(typeComp(op_type[0], op_type[1]))
            s_error(5, exp->line);
        deleteType(op_type[1]);

        Node *left_node = (Node *)(exp->sons);
        if(left_node->type != T_ID &&
           strcmp(left_node->name, "ArrayEval") &&
           strcmp(left_node->name, "DOT")) {
            s_error(6, left_node->line);
        }

        return op_type[0];
    }

    if(!strcmp(exp->name, "DOT")) {
        Type struct_type = getExpType(exp->sons);
        if(struct_type->kind != STRUCTURE)
            s_error(13, exp->line);
        else {
            Node *field_node = exp->sons->next;
            if(field_node->type != T_ID) {
                s_error(13, field_node->line);
                deleteType(getExpType(field_node));
            }
            else {
                FieldList field_list = struct_type->u.structure;
                for(; field_list != NULL; field_list = field_list->next)
                    if(!strcmp(field_node->val.type_str, field_list->name))
                        return copyType(field_list->type);
                s_error(14, field_node->line, field_node->val.type_str);
            }
        }
        return struct_type;
    }

    RETURN_DEFAULT_EXP_TYPE: { // Default type of exp
        Type type = malloc(SIZEOF(Type_));
        type->kind = BASIC;
        type->u.basic = 0;
        return type;
    }
}


FieldList getFieldType(Node *node, uint32_t field_type) {
    Node *def = node->sons;
    FieldList field_list = NULL;

    pushStack();

    for(; def != NULL; def = def->next) {
        Node *specifier = def->sons, *dec = specifier->next;
        for(; dec != NULL; dec = dec->next) {
            if(!strcmp(dec->name, "VarDec")) {
                Node *vardec = dec->sons;
                char *name;
                switch (field_type)
                {
                    case FIELD_TYPE_PARAM:
                        insertSymbol(name, getVarDecType(specifier, vardec, &name), PARAMDEC, vardec->line);
                        break;
                    case FIELD_TYPE_STRUCT:
                        insertSymbol(name, getVarDecType(specifier, vardec, &name), FIELDDEC, vardec->line);
                        break;

                    default:
                        break;
                }
            }
            else {
                s_error(15, dec->line, frame_stack[st_top]->name);
                deleteType(getExpType(dec));
            }
        }
    }

    for(SymbolList head = frame_stack[st_top], next; head != NULL; head = next) {
        if(head->func_type != DEF) {
            FieldList field = malloc(SIZEOF(FieldList_));
            field->name = head->name;
            field->next = field_list;
            field->type = head->type;
            field_list = field;
        }
        next = head->fs_next;
        hash_table[head->key] = head->tb_next;
        free(head);
    }
    frame_stack[st_top--] = NULL;

    uint32_t offset = 0;
    for(FieldList lst = field_list; lst != NULL; lst = lst->next) {
        lst->offset = offset;
        offset += lst->type->size;
    }

    return field_list;
}

void deleteType(Type type) {
    if(type == NULL)
        return;
    
    switch (type->kind)
    {
        case ARRAY:
            deleteType(type->u.array.elem);
            break;
        case STRUCTURE: {
            FieldList field = type->u.structure;
            while(field != NULL) {
                FieldList pre_field = field;
                deleteType(field->type);
                field = field->next;
                free(pre_field);
            }
            break;
        }
        case FUNCTION: {
            FieldList field = type->u.function->parameters;
            deleteType(type->u.function->ret);
            while(field != NULL) {
                FieldList pre_field = field;
                deleteType(field->type);
                field = field->next;
                free(pre_field);
            }
            break;
        }

        case BASIC: default:
            break;
    }
    free(type);
}

Type copyType(Type type) {
    if(type == NULL) {
        return NULL;
    }
    Type copied_type = malloc(SIZEOF(Type_));
    copied_type->kind = type->kind;
    copied_type->size = type->size;
    switch (type->kind)
    {
        case BASIC:
            copied_type->u = type->u;
            break;
        case ARRAY:
            copied_type->u.array.size = type->u.array.size;
            copied_type->u.array.elem = copyType(type->u.array.elem);
            break;
        case STRUCTURE: {
            FieldList *p_list = &(copied_type->u.structure), src = type->u.structure;
            while(src != NULL) {
                (*p_list) = calloc(1, sizeof(struct FieldList_));
                (*p_list)->type = copyType(src->type);
                (*p_list)->name = src->name;
                (*p_list)->offset = src->offset;

                p_list = &((*p_list)->next), src = src->next;
            }
            break;
        }
        case FUNCTION: {
            FieldList *p_list = &(copied_type->u.function->parameters), src = type->u.function->parameters;
            copied_type->u.function->ret = copyType(type->u.function->ret);
            while(src != NULL) {
                (*p_list) = calloc(1, sizeof(struct FieldList_));
                (*p_list)->type = copyType(src->type);

                p_list = &((*p_list)->next), src = src->next;
            }
            break;
        }

        default:
            break;
    }

    return copied_type;
}
