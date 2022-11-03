#define __IR_C__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#include "include/constant.h"
#include "include/type.h"

static uint32_t label_cnt = 0;
static uint32_t temp_var_cnt = 0;
static ICList ir = NULL;


Operand new_var(char*);
Operand new_func(char*);
Operand new_const(int);
ICList new_ic(int, ...);
char *new_label_name();
char *get_temp_var_name();
ICList append(ICList, ICList);
ICList translate_exp(Node*, char*);
ICList translate_cond(Node*, char*, char*);
ICList translate_stmt(Node*);
ICList translate_compst(Node*);
void append_ir(ICList);
void print_operand(Operand);
void print_ir();

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
                code1 = append(code1, translate_exp(args_node, arg->u.var_name));
                code2 = append(code2, new_ic(IC_ARG, arg));
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
        if(left->type == T_ID) {
            Operand temp = new_var(NULL);
            ICList get_right = translate_exp(right, temp->u.var_name),
                   assign_to_place = append(new_ic(IC_ASSIGN, new_var(left->val.type_str), temp), 
                                            new_ic(IC_ASSIGN, new_var(place), new_var(left->val.type_str)));
            return append(get_right, assign_to_place);
        }
    }

    if(!strcmp(exp->name, "DOT")) {

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
        return translate_compst(stmt);
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

ICList translate_compst(Node* compst) {
    return NULL;
}

void append_ir(ICList ic) {
    if(ir == NULL)
        ir = ic;
    else
        ir = append(ir, ic);
}

void print_operand(Operand op) {
    switch(op->kind) {
        case OP_VAR:
            printf("%s", op->u.var_name);
            break;
        case OP_CONST:
            printf("#%d", op->u.val);
            break;
        case OP_ADDR:
            printf("&%s", op->u.var_name);
            break;
        case OP_FUNC:
            printf("CALL %s", op->u.var_name);
            break;
        default:
            break;
    }
}

void print_ir() {
    for(ICList head = ir; head != NULL; head = head->next) {
        switch (head->code.kind)
        {
            case IC_ASSIGN: {
                Operand left = head->code.u.assign.left, right = head->code.u.assign.right;
                assert(left->u.var_name != NULL);
                print_operand(left);
                printf(" := ");
                print_operand(right);
                break;
            }
            case IC_ADD: case IC_SUB: case IC_MUL: case IC_DIV: {
                Operand left = head->code.u.binop.left, right = head->code.u.binop.right, res = head->code.u.binop.res;
                print_operand(res);
                printf(" := ");
                print_operand(left);
                printf(" %c ", "+-*/"[head->code.kind]);
                print_operand(right);
                break;
            }
            case IC_LABEL: case IC_FUNCTION:
                printf("%s ", icode_name[head->code.kind]);
                print_operand(head->code.u.uniop.op);
                printf(" :");
                break;
            case IC_GOTO: case IC_RETURN: case IC_ARG: case IC_PARAM: case IC_READ: case IC_WRITE:
                printf("%s ", icode_name[head->code.kind]);
                print_operand(head->code.u.uniop.op);
                break;
            case IC_IFGOTO:
                printf("IF ");
                print_operand(head->code.u.cond_goto.op1);
                printf(" %s ", head->code.u.cond_goto.relop);
                print_operand(head->code.u.cond_goto.op2);
                printf(" GOTO ");
                print_operand(head->code.u.cond_goto.target);
                break;
            default:
                printf("Padding!");
                break;
        }
        putchar('\n');
    }
}