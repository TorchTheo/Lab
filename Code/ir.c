#define __IR_C__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#include "include/constant.h"
#include "include/type.h"
#include "include/utils/hashtable.h"
#include "include/utils/optimize.h"

extern FILE* file_out;
extern void registerStruct(Node*);
extern Type lookUpSymbol(char*);
extern SymbolList getStackTop();
extern Type getExpType(Node*);
extern Type getVarDecType(Node*, Node*, char**);
extern Type getSpecifierType(Node*);
extern FieldList getFieldType(Node*, uint32_t);
extern void pushStack();
extern void popStack();
extern void insertSymbol(char*, Type, uint32_t, uint32_t);

static uint32_t label_cnt = 0;
static uint32_t temp_var_cnt = 0;
static uint8_t value_type = RIGHT_VALUE;
static uint8_t print_stdout = TRUE;

void start_ir(Node*);
Operand new_var(char*);
Operand new_func(char*);
Operand new_const(int);
Operand new_addr(char*);
ICList new_ic(int, ...);
char *new_label_name();
char *get_temp_var_name();
ICList append(ICList, ICList);
ICList translate_exp(Node*, char*);
ICList translate_cond(Node*, char*, char*);
ICList translate_stmt(Node*);
ICList translate_def(Node*);
ICList translate_deflist(Node*, Node*);
Operand copy_operand(Operand);
ICList optimizeICList(ICList);
void print_operand(Operand);
void print_ic(ICList);

void start_ir(Node *root) {
    if(file_out != NULL) {
        print_stdout = FALSE;
    }
    // print_ic(translate_deflist(root, NULL));
    print_ic(optimizeICList(translate_deflist(root, NULL)));
}

Operand new_var(char *name) {
    Operand ret = malloc(SIZEOF(Operand_));
    ret->kind = OP_VAR;
    if(name == NULL)
        ret->u.var_name = get_temp_var_name();
    else
        ret->u.var_name = name;
    return ret;
}

Operand new_func(char* name) {
    Operand ret = malloc(SIZEOF(Operand_));
    ret->kind = OP_FUNC;
    ret->u.var_name = name;
    return ret;
}

Operand new_const(int val) {
    Operand ret = malloc(SIZEOF(Operand_));
    ret->kind = OP_CONST;
    ret->u.val = val;
    return ret;
}

Operand new_addr(char* name) {
    Operand ret = malloc(SIZEOF(Operand_));
    ret->kind = OP_ADDR;
    ret->u.var_name = name;
    return ret;
}

ICList new_ic(int code_type, ...) {
    ICList ic = malloc(SIZEOF(ICList_));
    va_list list;
    va_start(list, code_type);
    ic->code.kind = code_type;
    switch(code_type) {
        case IC_ASSIGN: 
            ic->code.u.assign.left = va_arg(list, Operand);
            ic->code.u.assign.right = va_arg(list, Operand);
            break;
        case IC_ADD: case IC_SUB: case IC_MUL: case IC_DIV:
            ic->code.u.binop.left = va_arg(list, Operand);
            ic->code.u.binop.right = va_arg(list, Operand);
            ic->code.u.binop.res = va_arg(list, Operand);
            break;
        case IC_LABEL: case IC_GOTO: case IC_FUNCTION:
            ic->code.u.uniop.op = new_var(va_arg(list, char*));
            break;
        case IC_RETURN: case IC_READ: case IC_WRITE: case IC_ARG: case IC_PARAM:
            ic->code.u.uniop.op = va_arg(list, Operand);
            break;
        case IC_IFGOTO:
            ic->code.u.cond_goto.op1 = va_arg(list, Operand);
            ic->code.u.cond_goto.op2 = va_arg(list, Operand);
            ic->code.u.cond_goto.target = new_var(va_arg(list, char *));
            ic->code.u.cond_goto.relop = va_arg(list, char*);
            break;
        case IC_DEC:
            ic->code.u.declr.first_byte = va_arg(list, char*);
            ic->code.u.declr.size = va_arg(list, uint32_t);
            break;
        default:
            break;
    }
    ic->prev = ic->next = NULL, ic->tail = ic;
    return ic;
}

char *new_label_name() {
    char *name = malloc(30 * sizeof(char));
    sprintf(name, "__LABEL_@%u", label_cnt++);
    return name;
}

char *get_temp_var_name() {
    char *name = malloc(30 * sizeof(char));
    sprintf(name, "__TEMP_@%u", temp_var_cnt++);
    return name;
}

ICList append(ICList head, ICList tail) {
    // assert(head != NULL && tail != NULL);
    if(head == NULL)
        return tail;
    if(tail == NULL)
        return head;
    assert(head->tail->next == NULL);
    head->tail->next = tail;
    tail->prev = head->tail;
    head->tail = tail->tail;
    return head;
}

ICList translate_exp(Node* exp, char* place) {
    if(exp == NULL)
        return NULL;
    if(exp->type == T_INT) {
        // ICList ic = malloc(SIZEOF(ICList_));
        // ic->code.kind = IC_ASSIGN;
        // ic->code.u.assign.left = new_var(place);
        // ic->code.u.assign.right = new_const(exp->val.type_int);
        // ic->prev = ic->next = NULL, ic->tail = ic;
        return new_ic(IC_ASSIGN, new_var(place), new_const(exp->val.type_int));
    }
    if(exp->type == T_ID) {
        // ICList ic = malloc(SIZEOF(ICList_));
        // ic->code.kind = IC_ASSIGN;
        // ic->code.u.assign.left = new_var(place);
        // ic->code.u.assign.right = new_var(exp->val.type_str);
        // ic->prev = ic->next = NULL, ic->tail = ic;
        Type id_type = lookUpSymbol(exp->val.type_str);
        if(id_type->kind != BASIC && id_type->kind != POINTER) 
            return new_ic(IC_ASSIGN, new_var(place), new_addr(exp->val.type_str));
        return new_ic(IC_ASSIGN, new_var(place), new_var(exp->val.type_str));
    }
    if(!strcmp(exp->name, "FuncCall")) {
        Node *func_id_node = exp->sons, *args_node = func_id_node->next;
        char *func_name = func_id_node->val.type_str;
        if(args_node == NULL) {
            if(!strcmp(func_name, "read"))
                return new_ic(IC_READ, new_var(place));
            return new_ic(IC_ASSIGN, new_var(place), new_func(func_name));
        } else {
            ICList code1 = NULL, code2 = NULL;
            while(args_node != NULL) {
                Operand arg = new_var(NULL);
                uint8_t last_value_type = value_type;
                Type arg_type = getExpType(args_node);
                if(arg_type->kind != BASIC) 
                    value_type = LEFT_VALUE;
                code1 = append(code1, translate_exp(args_node, arg->u.var_name));
                code2 = append(code2, new_ic(IC_ARG, arg));
                value_type = last_value_type;
                args_node = args_node->next;
            }
            if(!strcmp(func_name, "write")) {
                code2->code.kind = IC_WRITE;
                return append(code1, append(code2, new_ic(IC_ASSIGN, new_var(place), new_const(0))));
            }
            return append(code1, append(code2, new_ic(IC_ASSIGN, new_var(place), new_func(func_name))));
        }
    }
    if(!strcmp(exp->name, "ArrayEval")) {
        Node *arr_node = (Node *)(exp->sons), *ind_node = (Node *)(arr_node->next);
        Type arr_type = getExpType(arr_node);
        Operand addr = new_var(NULL), index = new_var(NULL);
        ICList get_addr, get_index, ret;

        uint8_t last_value_type = value_type;
        value_type = LEFT_VALUE;
        get_addr = translate_exp(arr_node, addr->u.var_name);
        value_type = last_value_type;

        get_index = translate_exp(ind_node, index->u.var_name);
        ret = append(
            new_ic(IC_MUL, new_var(index->u.var_name), new_var(index->u.var_name), new_const(arr_type->u.array.elem->size)),
            append(
                new_ic(IC_ADD, new_var(addr->u.var_name), new_var(addr->u.var_name), index),
                new_ic(IC_ASSIGN, new_var(place), addr)
            )
        );
        if(value_type == RIGHT_VALUE)
            addr->kind = OP_DEREF;
        return append(append(get_addr, get_index), ret);
    }

    if(!strcmp(exp->name, "PLUS")) {
        Node *left = exp->sons, *right = left->next;
        Operand op1 = new_var(NULL), op2 = new_var(NULL);
        ICList get_op1 = translate_exp(left, op1->u.var_name),
               get_op2 = translate_exp(right, op2->u.var_name),
               res = new_ic(IC_ADD, op1, op2, new_var(place));
        return append(append(get_op1, get_op2), res);
    }

    if(!strcmp(exp->name, "MINUS")) {
        Node *left = exp->sons, *right = left->next;
        if(right != NULL) {
            Operand op1 = new_var(NULL), op2 = new_var(NULL);
            ICList get_op1 = translate_exp(left, op1->u.var_name),
                get_op2 = translate_exp(right, op2->u.var_name),
                res = new_ic(IC_SUB, op1, op2, new_var(place));
            return append(append(get_op1, get_op2), res);
        } else {
            Operand op1 = new_const(0), op2 = new_var(NULL);
            ICList get_op2 = translate_exp(left, op2->u.var_name),
                   res = new_ic(IC_SUB, op1, op2, new_var(place));
            return append(get_op2, res);
        }
    }

    if(!strcmp(exp->name, "STAR")) {
        Node *left = exp->sons, *right = left->next;
        Operand op1 = new_var(NULL), op2 = new_var(NULL);
        ICList get_op1 = translate_exp(left, op1->u.var_name),
               get_op2 = translate_exp(right, op2->u.var_name),
               res = new_ic(IC_MUL, op1, op2, new_var(place));
        return append(append(get_op1, get_op2), res);
    }

    if(!strcmp(exp->name, "DIV")) {
        Node *left = exp->sons, *right = left->next;
        Operand op1 = new_var(NULL), op2 = new_var(NULL);
        ICList get_op1 = translate_exp(left, op1->u.var_name),
               get_op2 = translate_exp(right, op2->u.var_name),
               res = new_ic(IC_DIV, op1, op2, new_var(place));
        return append(append(get_op1, get_op2), res);
    }

    if(!strcmp(exp->name, "AND") ||
       !strcmp(exp->name, "OR") ||
       !strcmp(exp->name, "NOT") ||
       !strcmp(exp->name, "RELOP")) {
        char *label_true = new_label_name(), *label_false = new_label_name();
        ICList init = new_ic(IC_ASSIGN, new_var(place), new_const(0)),
               inter = translate_cond(exp, label_true, label_false),
               end = append(append(new_ic(IC_LABEL, label_true), new_ic(IC_ASSIGN, new_var(place), new_const(1))), new_ic(IC_LABEL, label_false));
        return append(append(init, inter), end);
    }

    if(!strcmp(exp->name, "ASSIGNOP")) {
        Node *left = exp->sons, *right = exp->sons->next;
        Operand right_value = new_var(NULL);
        ICList assign_to_place, get_right = translate_exp(right, right_value->u.var_name);
        if(left->type == T_ID) {
            assign_to_place = append(new_ic(IC_ASSIGN, new_var(left->val.type_str), right_value), 
                                    new_ic(IC_ASSIGN, new_var(place), new_var(right_value->u.var_name)));
            return append(get_right, assign_to_place);
        } else {
            Operand addr = new_var(NULL);
            uint32_t last_value_type = value_type;
            value_type = LEFT_VALUE;
            addr->kind = OP_DEREF;
            assign_to_place = append(
                translate_exp(left, addr->u.var_name),
                append(
                    new_ic(IC_ASSIGN, addr, right_value),
                    new_ic(IC_ASSIGN, new_var(place), new_var(right_value->u.var_name))
                )
            );
        }
        return append(get_right, assign_to_place);
    }

    if(!strcmp(exp->name, "DOT")) {
        Node *struct_node = exp->sons, *field_node = struct_node->next;
        Type struct_type = getExpType(struct_node);
        FieldList field_list = struct_type->u.structure;
        Operand addr = new_var(NULL);
        ICList get_addr;
        uint8_t last_value_type = value_type;
        value_type = LEFT_VALUE;
        get_addr = translate_exp(struct_node, addr->u.var_name);
        value_type = last_value_type;
        for(; field_list != NULL; field_list = field_list->next) {
            if(!strcmp(field_list->name, field_node->val.type_str)) {
                get_addr = append(get_addr, new_ic(IC_ADD, new_var(addr->u.var_name), new_const(field_list->offset), new_var(addr->u.var_name)));
                if(value_type == RIGHT_VALUE)
                    addr->kind = OP_DEREF;
                break;
            }
        }
        return append(get_addr, new_ic(IC_ASSIGN, new_var(place), addr));
    }
}

ICList translate_cond(Node* exp, char* label_true, char* label_false) {
    if(!strcmp(exp->name, "RELOP")) {
        Node *left_exp = exp->sons, *right_exp = left_exp->next;
        Operand t1 = new_var(NULL), t2 = new_var(NULL);
        ICList left_val = translate_exp(left_exp, t1->u.var_name), right_val = translate_exp(right_exp, t2->u.var_name);
        return append(append(left_val, left_val), append(new_ic(IC_IFGOTO, t1, t2, label_true, exp->val.type_str), new_ic(IC_GOTO, label_false)));
    } else if(!strcmp(exp->name, "NOT"))
        return translate_cond(exp->sons, label_false, label_true);
    else if(!strcmp(exp->name, "AND")) {
        char *label1 = new_label_name();
        return append(append(translate_cond(exp->sons, label1, label_false), new_ic(IC_LABEL, label1)), translate_cond(exp->sons->next, label_true, label_false));
    }
    else if(!strcmp(exp->name, "OR")) {
        char *label1 = new_label_name();
        return append(append(translate_cond(exp->sons, label_true, label1), new_ic(IC_LABEL, label1)), translate_cond(exp->sons->next, label_true, label_false));
    } else {
        Operand t1 = new_var(NULL);
        ICList code1 = translate_exp(exp, t1->u.var_name);
        ICList code2 = new_ic(IC_IFGOTO, t1, new_const(0), label_true, "!=");
        return append(append(code1, code2), new_ic(IC_GOTO, label_false));
    }
}

ICList translate_stmt(Node* stmt) {
    if(!strcmp(stmt->name, "CompSt"))
        return translate_deflist(stmt->sons, NULL);
    else if(!strcmp(stmt->name, "RETURN")) {
        Operand t1 = new_var(NULL);
        ICList code1 = translate_exp(stmt->sons, t1->u.var_name);
        ICList code2 = new_ic(IC_RETURN, t1);
        return append(code1, code2);
    } else if(!strcmp(stmt->name, "IF")) {
        Node *cond = stmt->sons, *if_stmt = cond->next;
        char *label1 = new_label_name(), *label2 = new_label_name();
        ICList code1 = translate_cond(cond, label1, label2);
        ICList code2 = translate_stmt(if_stmt);
        if(stmt->next == NULL)
            return append(code1, append(new_ic(IC_LABEL, label1), append(code2, new_ic(IC_LABEL, label2))));
        else {
            Node *else_stmt = stmt->next;
            char *label3 = new_label_name();
            ICList code3 = translate_stmt(else_stmt);
            return append(code1, append(
                    new_ic(IC_LABEL, label1), append(
                        code2, append(
                            new_ic(IC_GOTO, label3), append(
                                new_ic(IC_LABEL, label2), append(
                                    code3, new_ic(IC_LABEL, label3)))))));
        }
    } else if(!strcmp(stmt->name, "WHILE")) {
        char *label1 = new_label_name(), *label2 = new_label_name(), *label3 = new_label_name();
        Node *cond = stmt->sons, *while_stmt = cond->next;
        ICList code1 = translate_cond(cond, label2, label3);
        ICList code2 = translate_stmt(while_stmt);
        return append(new_ic(IC_LABEL, label1), append(
                code1, append(
                    new_ic(IC_LABEL, label2), append(
                        code2, append(
                            new_ic(IC_GOTO, label1), new_ic(IC_LABEL, label3))))));
    } else 
        return translate_exp(stmt, NULL);
}

ICList translate_def(Node* node) {
    ICList ic = NULL;
    Node *specifier = node, *dec = node->next;
    if(dec == NULL) {
        // struct
        registerStruct(specifier);
    } else if(!strcmp(dec->name, "FunDec")) {
        Node *symbol = dec->sons, *param_dec_list = symbol->next;
        Type type = malloc(SIZEOF(Type_));
        type->kind = FUNCTION;
        type->u.function = malloc(SIZEOF(Func_));
        type->u.function->ret = getSpecifierType(specifier);
        if(param_dec_list != NULL)
            type->u.function->parameters = getFieldType(param_dec_list, VARDEC);
        if(dec->next == NULL) {
            // 函数声明
        }
        else {
            insertSymbol(symbol->val.type_str, type, FUNCDEF, symbol->line);
            ic = append(ic, new_ic(IC_FUNCTION, symbol->val.type_str));
            Node *def_list = dec->next->sons;
            ic = append(ic, translate_deflist(def_list, param_dec_list));
        }
    } else {
        // 变量声明
        for(;dec != NULL; dec = dec->next) {
            if(!strcmp(dec->name, "VarDec")) {
                Node *var_dec = dec->sons;
                char *name;
                Type t = getVarDecType(specifier, var_dec, &name);
                insertSymbol(name, t, VARDEC, var_dec->line);
                if(t->kind != BASIC) {
                    ic = append(ic, new_ic(IC_DEC, name, t->size));
                }
            } else {
                // var = exp
                ic = append(ic, translate_exp(dec, getStackTop()->name));
            }
        }
    }
    return ic;
}

ICList translate_deflist(Node* def_list, Node* param_list) {
    ICList ic = NULL;
    pushStack();
    if(param_list != NULL) {
        Node *param_def = param_list->sons;
        for(; param_def != NULL; param_def = param_def->next) {
            Node *param_specifier = param_def->sons, *param_dec = param_specifier->next;
            for(; param_dec != NULL; param_dec = param_dec->next) {
                Node *var_dec = param_dec->sons;
                char *name;
                Type param_type = getVarDecType(param_specifier, var_dec, &name);
                if(param_type->kind != BASIC) {
                    Type pointer_type = malloc(SIZEOF(Type_));
                    pointer_type->kind = POINTER;
                    pointer_type->size = 4;
                    pointer_type->u.pointer = param_type;
                }
                insertSymbol(name, param_type, PARAMDEC, var_dec->line);
                ic = append(ic, new_ic(IC_PARAM, new_var(name)));
            }
        }
    }
    if(!strcmp(def_list->name, "ExtDef")) {
        for(; def_list != NULL; def_list = def_list->next)
            ic = append(ic, translate_def(def_list->sons));
    } else if(!strcmp(def_list->name, "DefList")) {
        Node *def = def_list->sons;
        for(; def != NULL; def = def->next)
            ic = append(ic, translate_def(def->sons));
        Node *stmt = def_list->next->sons;
        for(; stmt != NULL; stmt = stmt->next) 
            ic = append(ic, translate_stmt(stmt->sons));
    }

    popStack();
    return ic;
}

Operand copy_operand(Operand op) {
    Operand ret = malloc(SIZEOF(Operand_));
    ret->kind = op->kind;
    ret->u = op->u;
    return ret;
}

ICList optimizeICList(ICList icode) {
    int times = 5;
    while(times--) {
        for(ICList line = icode; line != NULL; line = line->next) {
            switch(line->code.kind) {
                case IC_ASSIGN: {
                    Operand left = line->code.u.assign.left, right = line->code.u.assign.right;
                    if(left->kind == OP_VAR && left->u.var_name == NULL ||
                       left->kind == OP_VAR && right->kind == OP_VAR && !strcmp(left->u.var_name, right->u.var_name))
                       DELETE_LINE
                    if(right->kind == OP_VAR) {
                        FIND_LAST_ASSIGN(u.assign.right, right);
                    }
                    break;
                }
                case IC_ADD: case IC_SUB: case IC_MUL: case IC_DIV:  {
                    Operand res = line->code.u.binop.res, left = line->code.u.binop.left, right = line->code.u.binop.right;
                    if(left->kind == OP_VAR) {
                        FIND_LAST_ASSIGN(u.binop.left, left);
                    }
                    if(right->kind == OP_VAR) {
                        FIND_LAST_ASSIGN(u.binop.right, right);
                    }
                    if(left->kind == OP_CONST && right->kind == OP_CONST) {
                        line->code.u.assign.left = res;
                        line->code.kind == IC_ASSIGN;
                        switch(line->code.kind) {
                            case IC_ADD:
                                line->code.u.assign.right = new_const(left->u.val + right->u.val);
                            case IC_SUB:
                                line->code.u.assign.right = new_const(left->u.val - right->u.val);
                            case IC_MUL:
                                line->code.u.assign.right = new_const(left->u.val * right->u.val);
                            case IC_DIV:
                                line->code.u.assign.right = new_const(left->u.val / right->u.val);
                        }
                        free(left);
                        free(right);
                        break;
                    }
                    if(left->kind == OP_CONST && left->u.val == 0 && line->code.kind != IC_SUB) {
                        line->code.kind = IC_ASSIGN;
                        line->code.u.assign.left = res;
                        switch(line->code.kind) {
                            case IC_ADD:
                                line->code.u.assign.right = right;
                                break;
                            case IC_MUL: case IC_DIV:
                                line->code.u.assign.right = new_const(0);
                                free(right);
                                break;
                        }
                        free(left);
                        break;
                    }
                    if(right->kind == OP_CONST && right->u.val == 0) {
                        line->code.kind = IC_ASSIGN;
                        line->code.u.assign.left = res;
                        switch(line->code.kind) {
                            case IC_ADD: case IC_SUB:
                                line->code.u.assign.right = left;
                                break;
                            case IC_MUL:
                                line->code.u.assign.right = new_const(0);
                                free(left);
                                break;
                            case IC_DIV:
                                assert(0);
                                break;
                        }
                        free(right);
                        break;
                    }
                    if(left->kind == OP_CONST && left->u.val == 1 && line->code.kind == IC_MUL) {
                        line->code.kind = IC_ASSIGN;
                        line->code.u.assign.left = res;
                        line->code.u.assign.right = right;
                        free(left);
                        break;
                    }
                    if(right->kind == OP_CONST && right->u.val == 1 && (line->code.kind == IC_MUL || line->code.kind == IC_DIV)) {
                        line->code.kind = IC_ASSIGN;
                        line->code.u.assign.left = res;
                        line->code.u.assign.right = left;
                        free(right);
                        break;
                    }
                    break;
                }
                case IC_RETURN: case IC_ARG: case IC_WRITE: {
                    Operand op = line->code.u.uniop.op;
                    if(op->kind == OP_VAR) {
                        FIND_LAST_ASSIGN(u.uniop.op, op);
                    }
                    break;
                }
                case IC_IFGOTO: {
                    Operand op1 = line->code.u.cond_goto.op1, op2 = line->code.u.cond_goto.op2;
                    if(op1->kind == OP_VAR) {
                        FIND_LAST_ASSIGN(u.cond_goto.op1, op1);
                    }
                    if(op2->kind == OP_VAR) {
                        FIND_LAST_ASSIGN(u.cond_goto.op2, op2);
                    }
                    break;
                }
                default:
                    break;
            }
            if(line == NULL)
                break;
        }
    }
    for(ICList line = icode; line != NULL; line = line->next) {
        switch(line->code.kind) {
            case IC_ASSIGN: {
                Operand left = line->code.u.assign.left, right = line->code.u.assign.right;
                COUNT_USED_VARIABLE(left);
                COUNT_USED_VARIABLE(right);
                break;
            }
            case IC_ADD: case IC_SUB: case IC_MUL: case IC_DIV: {
                Operand res = line->code.u.binop.res, left = line->code.u.binop.left, right = line->code.u.binop.right;
                COUNT_USED_VARIABLE(res);
                COUNT_USED_VARIABLE(left);
                COUNT_USED_VARIABLE(right);
                break;
            }
            case IC_RETURN: case IC_ARG: case IC_WRITE: case IC_READ: case IC_PARAM: {
                Operand op = line->code.u.uniop.op;
                COUNT_USED_VARIABLE(op);
                break;
            }
            case IC_DEC: {
                Operand first_byte = new_var(line->code.u.declr.first_byte);
                COUNT_USED_VARIABLE(first_byte);
                free(first_byte);
                break;
            }
            case IC_IFGOTO: {
                Operand op1 = line->code.u.cond_goto.op1, op2 = line->code.u.cond_goto.op2;
                COUNT_USED_VARIABLE(op1);
                COUNT_USED_VARIABLE(op2);
                break;
            }
            default:
                break;
        }
    }
    for(ICList line = icode; line != NULL; line = line->next) {
        switch(line->code.kind) {
            case IC_ASSIGN: {
                Operand left = line->code.u.assign.left;
                if(left->kind == OP_VAR && get_value(left->u.var_name) <= 1)
                    DELETE_LINE;
                break;
            }
            case IC_DEC: {
                char *var_name = line->code.u.declr.first_byte;
                if(get_value(var_name) <= 1) {                                                                       
                    ICList prev_line = line->prev;                                                      
                    if(line->prev != NULL)                                                              
                        line->prev->next = line->next;                                                  
                    if(line->next != NULL)                                                              
                        line->next->prev = line->prev;                                                  
                    free(line);                                                                         
                    line = prev_line;                                                                   
                    break;                                                                              
                }
                break;
            }
        }
        if(line == NULL)
            break;
    }
    return icode;
}

void print_operand(Operand op) {
    switch(op->kind) {
        case OP_VAR: {
            if(print_stdout)
                printf("%s", op->u.var_name);
            else
                fprintf(file_out, "%s", op->u.var_name);
            break;
        }
        case OP_CONST: {
            if(print_stdout)
                printf("#%d", op->u.val);
            else
                fprintf(file_out, "#%d", op->u.val);
            break;
        }
        case OP_ADDR: {
            if(print_stdout)
                printf("&%s", op->u.var_name);
            else
                fprintf(file_out, "&%s", op->u.var_name);
            break;
        }
        case OP_FUNC: {
            if(print_stdout)
                printf("CALL %s", op->u.var_name);
            else
                fprintf(file_out, "CALL %s", op->u.var_name);
            break;
        }
        case OP_DEREF: {
            if(print_stdout)
                printf("*%s", op->u.var_name);
            else
                fprintf(file_out, "*%s", op->u.var_name);
            break;
        }
        default:
            break;
    }
}

void print_ic(ICList ic) {
    for(ICList head = ic; head != NULL; head = head->next) {
        switch (head->code.kind)
        {
            case IC_ASSIGN: {
                Operand left = head->code.u.assign.left, right = head->code.u.assign.right;
                assert(left->u.var_name != NULL);
                print_operand(left);
                if(print_stdout)
                    printf(" := ");
                else
                    fprintf(file_out, " := ");
                print_operand(right);
                break;
            }
            case IC_ADD: case IC_SUB: case IC_MUL: case IC_DIV: {
                Operand left = head->code.u.binop.left, right = head->code.u.binop.right, res = head->code.u.binop.res;
                print_operand(res);
                if(print_stdout)
                    printf(" := ");
                else
                    fprintf(file_out, " := ");
                print_operand(left);
                if(print_stdout)
                    printf(" %c ", "+-*/"[head->code.kind]);
                else
                    fprintf(file_out, " %c ", "+-*/"[head->code.kind]);
                print_operand(right);
                break;
            }
            case IC_LABEL: case IC_FUNCTION: {
                if(print_stdout)
                    printf("%s ", icode_name[head->code.kind]);
                else
                    fprintf(file_out, "%s ", icode_name[head->code.kind]);
                print_operand(head->code.u.uniop.op);
                if(print_stdout)
                    printf(" :");
                else
                    fprintf(file_out, " :");
                break;
            }
            case IC_GOTO: case IC_RETURN: case IC_ARG: case IC_PARAM: case IC_READ: case IC_WRITE: {
                if(print_stdout)
                    printf("%s ", icode_name[head->code.kind]);
                else
                    fprintf(file_out, "%s ", icode_name[head->code.kind]);
                print_operand(head->code.u.uniop.op);
                break;
            }
            case IC_IFGOTO: {
                if(print_stdout)
                    printf("IF ");
                else
                    fprintf(file_out, "IF ");
                print_operand(head->code.u.cond_goto.op1);
                if(print_stdout)
                    printf(" %s ", head->code.u.cond_goto.relop);
                else
                    fprintf(file_out, " %s ", head->code.u.cond_goto.relop);
                print_operand(head->code.u.cond_goto.op2);
                if(print_stdout)
                    printf(" GOTO ");
                else
                    fprintf(file_out, " GOTO ");
                print_operand(head->code.u.cond_goto.target);
                break;
            }
            case IC_DEC: {
                if(print_stdout)
                    printf("DEC %s %u", head->code.u.declr.first_byte, head->code.u.declr.size);
                else
                    fprintf(file_out, "DEC %s %u", head->code.u.declr.first_byte, head->code.u.declr.size);
                break;
            }
            default: {
                if(print_stdout)
                    printf("Padding!");
                else
                    fprintf(file_out, "Padding!");
                break;
            }
        }
        if(print_stdout)
            putchar('\n');
        else
            fputc('\n', file_out);
    }
}