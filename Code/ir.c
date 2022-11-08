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
#include "include/utils/basic.h"

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
static uint8_t print_stream = STD_OUT;

void start_ir(Node*);
Operand new_operand(uint32_t, ...);
Operand new_temp();
ICList new_ic(int, ...);
char *new_label_name();
char *get_temp_var_name();
ICList append(ICList, ICList);
ICList translate_exp(Node*, char*);
ICList translate_cond(Node*, char*, char*);
ICList translate_stmt(Node*);
ICList translate_def(Node*);
ICList translate_deflist(Node*, Node*);
ICList translate_copy_arr(char*, char*, uint32_t);
Operand copy_operand(Operand);
ICList optimizeICList(ICList);
void print_operand(Operand);
void print_ic(ICList);

void start_ir(Node *root) {
    if(file_out != NULL) {
        print_stream = FILE_OUT;
    }
    // print_ic(translate_deflist(root, NULL));
    print_ic(optimizeICList(translate_deflist(root, NULL)));
}

Operand new_operand(uint32_t op_type, ...) {
    Operand ret = malloc(SIZEOF(Operand_));
    va_list list;
    va_start(list, op_type);
    ret->kind = op_type;
    switch (ret->kind)
    {
        case OP_VAR: case OP_ADDR: case OP_DEREF: case OP_FUNC:
            ret->u.var_name = va_arg(list, char*);
            break;
        case OP_CONST:
            ret->u.val = va_arg(list, int);
            break;
        default:
            break;
    }
    va_end(list);
    return ret;
}

Operand new_temp() {
    return new_operand(OP_VAR, get_temp_var_name());
}

ICList new_ic(int code_type, ...) {
    ICList ic = calloc(1, sizeof(struct ICList_));
    va_list list;
    va_start(list, code_type);

    ic->code.kind = code_type;
    switch(code_type)
    {
        case IC_ASSIGN:
            ic->code.u.assign.left = va_arg(list, Operand);
            ic->code.u.assign.right = va_arg(list, Operand);
            break;
        case IC_ADD: case IC_SUB: case IC_MUL: case IC_DIV:
            ic->code.u.binop.res = va_arg(list, Operand);
            ic->code.u.binop.left = va_arg(list, Operand);
            ic->code.u.binop.right = va_arg(list, Operand);
            break;
        case IC_LABEL: case IC_GOTO: case IC_FUNCTION:
            ic->code.u.uniop.op = new_operand(OP_VAR, va_arg(list, char *));
            break;
        case IC_RETURN: case IC_READ: case IC_WRITE: case IC_ARG: case IC_PARAM:
            ic->code.u.uniop.op = va_arg(list, Operand);
            break;
        case IC_IFGOTO:
            ic->code.u.cond_goto.op1 = va_arg(list, Operand);
            ic->code.u.cond_goto.op2 = va_arg(list, Operand);
            ic->code.u.cond_goto.target = new_operand(OP_VAR, va_arg(list, char *));
            ic->code.u.cond_goto.relop = va_arg(list, char *);
            break;
        case IC_DEC:
            ic->code.u.declr.first_byte = va_arg(list, char *);
            ic->code.u.declr.size = va_arg(list, uint32_t);
            break;

        default:
            break;
    }
    ic->prev = ic->next = NULL, ic->tail = ic;

    va_end(list);
    return ic;
}

char *new_label_name() {
    char *name = malloc(30 * sizeof(char));
    sprintf(name, "LABEL%u", label_cnt++);
    return name;
}

char *get_temp_var_name() {
    char *name = malloc(30 * sizeof(char));
    sprintf(name, "TEMPVAR%u", temp_var_cnt++);
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
    if(exp->type == T_INT)
        return new_ic(IC_ASSIGN, new_operand(OP_VAR, place), new_operand(OP_CONST, exp->val.type_int));
    if(exp->type == T_ID) {
        Type id_type = lookUpSymbol(exp->val.type_str);
        if(id_type->kind != BASIC && id_type->kind != POINTER)
            return new_ic(IC_ASSIGN, new_operand(OP_VAR, place), new_operand(OP_ADDR, exp->val.type_str));
        return new_ic(IC_ASSIGN, new_operand(OP_VAR, place), new_operand(OP_VAR, exp->val.type_str));
    }
    if(!strcmp(exp->name, "FuncCall")) {
        Node *func_id_node = exp->sons, *args_node = func_id_node->next;
        char *func_name = func_id_node->val.type_str;
        ICList get_args = NULL, arg_list = NULL;
        if(0 == strcmp(func_name, "read"))
            return new_ic(IC_READ, new_operand(OP_VAR, place));
        while(args_node != NULL) {
            Operand arg = new_temp();
            // [x]: right value
            uint8_t last_value_type = value_type;
            Type arg_type = getExpType(args_node);
            if(arg_type->kind != BASIC)
                value_type = LEFT_VALUE;

            get_args = append(get_args, translate_exp(args_node, arg->u.var_name));
            arg_list = append(new_ic(IC_ARG, arg), arg_list);

            value_type = last_value_type;
            args_node = args_node->next;
        }
        if(0 == strcmp(func_name, "write")) {
            assert(arg_list != NULL && arg_list->next == NULL);
            arg_list->code.kind = IC_WRITE;
            return append(
                append(
                    get_args,
                    arg_list
                ),
                new_ic(IC_ASSIGN, new_operand(OP_VAR, place), new_operand(OP_CONST, 0))
            );
        }
        return append(
            append(
                get_args,
                arg_list
            ),
            new_ic(IC_ASSIGN, new_operand(OP_VAR, place), new_operand(OP_FUNC, func_name))
        );
    }
    if(0 == strcmp(exp->name, "ArrayEval")) {
        Node *arr_node = exp->sons, *ind_node = arr_node->next;
        Type arr_type = getExpType(arr_node);
        Operand addr = new_temp(), index = new_temp();
        ICList get_addr, get_index, ret;

        uint8_t last_value_type = value_type;
        value_type = LEFT_VALUE;
        get_addr = translate_exp(arr_node, addr->u.var_name);
        value_type = last_value_type;

        get_index = translate_exp(ind_node, index->u.var_name);
        ret = append(
            new_ic(IC_MUL, new_operand(OP_VAR, index->u.var_name), new_operand(OP_VAR, index->u.var_name), new_operand(OP_CONST, arr_type->u.array.elem->size)),
            append(
                new_ic(IC_ADD, new_operand(OP_VAR, addr->u.var_name), new_operand(OP_VAR, addr->u.var_name), index),
                new_ic(IC_ASSIGN, new_operand(OP_VAR, place), addr)
            )
        );
        if(value_type == RIGHT_VALUE)
            addr->kind = OP_DEREF;
        return append(append(get_addr, get_index), ret);
    }

    if(0 == strcmp(exp->name, "PLUS")) {
        Node *left = exp->sons, *right = left->next;
        Operand op1 = new_temp(), op2 = new_temp();
        ICList get_op1 = translate_exp(left, op1->u.var_name),
               get_op2 = translate_exp(right, op2->u.var_name),
               add = new_ic(IC_ADD, new_operand(OP_VAR, place), op1, op2);
        return append(append(get_op1, get_op2), add);
    }

    if(0 == strcmp(exp->name, "MINUS")) {
        Node *left = exp->sons, *right = left->next;
        if(right != NULL) {
            Operand op1 = new_temp(), op2 = new_temp();
            ICList get_op1 = translate_exp(left, op1->u.var_name),
                   get_op2 = translate_exp(right, op2->u.var_name),
                   add = new_ic(IC_SUB, new_operand(OP_VAR, place), op1, op2);
            return append(append(get_op1, get_op2), add);
        }
        else {
            Operand op1 = new_operand(OP_CONST, 0), op2 = new_temp();
            ICList get_op2 = translate_exp(left, op2->u.var_name),
                   add = new_ic(IC_SUB, new_operand(OP_VAR, place), op1, op2);
            return append(get_op2, add);
        }
    }

    if(0 == strcmp(exp->name, "STAR")) {
        Node *left = exp->sons, *right = left->next;
        Operand op1 = new_temp(), op2 = new_temp();
        ICList get_op1 = translate_exp(left, op1->u.var_name),
               get_op2 = translate_exp(right, op2->u.var_name),
               add = new_ic(IC_MUL, new_operand(OP_VAR, place), op1, op2);
        return append(append(get_op1, get_op2), add);
    }

    if(0 == strcmp(exp->name, "DIV")) {
        Node *left = exp->sons, *right = left->next;
        Operand op1 = new_temp(), op2 = new_temp();
        ICList get_op1 = translate_exp(left, op1->u.var_name),
               get_op2 = translate_exp(right, op2->u.var_name),
               add = new_ic(IC_DIV, new_operand(OP_VAR, place), op1, op2);
        return append(append(get_op1, get_op2), add);
    }

    if(0 == strcmp(exp->name, "AND") ||
       0 == strcmp(exp->name, "OR") ||
       0 == strcmp(exp->name, "NOT") ||
       0 == strcmp(exp->name, "RELOP")) {
        char *label_true = new_label_name(), *label_false = new_label_name();
        ICList init = new_ic(IC_ASSIGN, new_operand(OP_VAR, place), new_operand(OP_CONST, 0)),
               inter = translate_cond(exp, label_true, label_false),
               end = append(
                   append(
                       new_ic(IC_LABEL, label_true),
                       new_ic(IC_ASSIGN, new_operand(OP_VAR, place), new_operand(OP_CONST, 1))
                   ),
                   new_ic(IC_LABEL, label_false)
               );
        return append(append(init, inter), end);
    }

    if(0 == strcmp(exp->name, "ASSIGNOP")) {
        Node *left = exp->sons, *right = left->next;
        Operand right_value = new_temp();
        ICList assign_to_place = NULL, get_right = NULL;
        Type op_type[2];
        op_type[0] = getExpType(left), op_type[1] = getExpType(right);
        switch (op_type[0]->kind)
        {
            case BASIC: case POINTER: {
                get_right = translate_exp(right, right_value->u.var_name);
                if(left->type == T_ID) {
                    assign_to_place = append(
                        new_ic(IC_ASSIGN, new_operand(OP_VAR, left->val.type_str), right_value),
                        new_ic(IC_ASSIGN, new_operand(OP_VAR, place), new_operand(OP_VAR, right_value->u.var_name))
                    );

                }
                else {
                    Operand addr = new_temp();
                    uint8_t last_value_type = value_type;
                    value_type = LEFT_VALUE;

                    addr->kind = OP_DEREF;
                    assign_to_place = append(
                        translate_exp(left, addr->u.var_name),
                        append(
                            new_ic(IC_ASSIGN, addr, right_value),
                            new_ic(IC_ASSIGN, new_operand(OP_VAR, place), new_operand(OP_VAR, right_value->u.var_name))
                        )
                    );

                    value_type = last_value_type;
                }
                break;
            }
            case ARRAY: {
                Operand left_value = new_temp();
                ICList get_left = NULL;
                uint32_t size = GET_MIN(op_type[0]->size, op_type[1]->size);

                uint8_t last_value_type = value_type;
                value_type = LEFT_VALUE;
                get_left = translate_exp(left, left_value->u.var_name);
                get_right = translate_exp(right, right_value->u.var_name);
                value_type = last_value_type;

                assign_to_place = append(
                    get_left,
                    translate_copy_arr(left_value->u.var_name, right_value->u.var_name, size)
                );

                if(place == NULL) {
                    break;
                }
                Type place_type = lookUpSymbol(place);
                if(place_type != NULL) {
                    // XXX: cannot handle a = b = c;
                    if(place_type->kind == POINTER) {
                        size = GET_MIN(place_type->u.pointer->size, size);
                    }
                    else {
                        size = GET_MIN(place_type->size, size);
                    }
                    assign_to_place = append(
                        assign_to_place,
                        translate_copy_arr(place, right_value->u.var_name, size)
                    );
                }
                break;
            }

            default:
                break;
        }

        return append(get_right, assign_to_place);
    }

    if(0 == strcmp(exp->name, "DOT")) {
        Node *struct_node = exp->sons, *field_node = exp->sons->next;
        Type struct_type = getExpType(struct_node);
        FieldList field_list = struct_type->u.structure;
        Operand addr = new_temp();
        ICList get_addr;

        uint8_t last_value_type = value_type;
        value_type = LEFT_VALUE;
        get_addr = translate_exp(struct_node, addr->u.var_name);
        value_type = last_value_type;

        for(; field_list != NULL; field_list = field_list->next) {
            if(0 == strcmp(field_node->val.type_str, field_list->name)) {
                get_addr = append(
                    get_addr,
                    new_ic(IC_ADD, new_operand(OP_VAR, addr->u.var_name), new_operand(OP_VAR, addr->u.var_name), new_operand(OP_CONST, field_list->offset))
                );
                if(value_type == RIGHT_VALUE) {
                    addr->kind = OP_DEREF;
                }
                break;
            }
        }
        return append(get_addr, new_ic(IC_ASSIGN, new_operand(OP_VAR, place), addr));
    }
}

ICList translate_cond(Node* exp, char* label_true, char* label_false) {
    if(!strcmp(exp->name, "RELOP")) {
        Node *left_exp = exp->sons, *right_exp = left_exp->next;
        Operand t1 = new_temp(), t2 = new_temp();
        ICList left_val = translate_exp(left_exp, t1->u.var_name), right_val = translate_exp(right_exp, t2->u.var_name);
        return append(append(left_val, right_val), append(new_ic(IC_IFGOTO, t1, t2, label_true, exp->val.type_str), new_ic(IC_GOTO, label_false)));
    } else if(!strcmp(exp->name, "NOT"))
        return translate_cond(exp->sons, label_false, label_true);
    else if(!strcmp(exp->name, "AND")) {
        Node *left_exp = exp->sons, *right_exp = left_exp->next;
        char *next = new_label_name();
        ICList calc_left = translate_cond(left_exp, next, label_false),
               calc_right = translate_cond(right_exp, label_true, label_false);
        return append(
            calc_left,
            append(
                new_ic(IC_LABEL, next),
                calc_right
            )
        );
    }
    else if(!strcmp(exp->name, "OR")) {
        Node *left_exp = exp->sons, *right_exp = left_exp->next;
        char *next = new_label_name();
        ICList calc_left = translate_cond(left_exp, label_true, next),
               calc_right = translate_cond(right_exp, label_true, label_false);
        return append(
            calc_left,
            append(
                new_ic(IC_LABEL, next),
                calc_right
            )
        );
    }
    Operand t1 = new_temp();
    ICList code1 = translate_exp(exp, t1->u.var_name);
    ICList code2 = new_ic(IC_IFGOTO, t1, new_operand(OP_CONST, 0), label_true, "!=");
    return append(code1, append(code2, new_ic(IC_GOTO, label_false)));
}

ICList translate_stmt(Node* stmt) {
    if(!strcmp(stmt->name, "CompSt"))
        return translate_deflist(stmt->sons, NULL);
    else if(!strcmp(stmt->name, "RETURN")) {
        Operand t1 = new_temp();
        ICList code1 = translate_exp(stmt->sons, t1->u.var_name);
        ICList code2 = new_ic(IC_RETURN, t1);
        return append(code1, code2);
    } else if(!strcmp(stmt->name, "IF")) {
        Node *cond = stmt->sons, *if_stmt = cond->next;
        char *cond_fin_label = new_label_name(), *if_fin_label = new_label_name();
        ICList  calc_cond = append(
                   translate_cond(cond->sons, cond_fin_label, if_fin_label),
                   new_ic(IC_LABEL, cond_fin_label)
               ),
               if_icode = translate_stmt(if_stmt->sons),
               if_fin_icode = new_ic(IC_LABEL, if_fin_label);

        if(stmt->next != NULL) {
            Node *else_stmt = stmt->next;
            char *else_fin_label = new_label_name();

            if_icode = append(if_icode, new_ic(IC_GOTO, else_fin_label));

            if_fin_icode = append(
                if_fin_icode,
                append(
                    translate_stmt(else_stmt->sons->sons),
                    new_ic(IC_LABEL, else_fin_label)
                )
            );
        }

        return append(calc_cond, append(if_icode, if_fin_icode));
    } else if(!strcmp(stmt->name, "WHILE")) {
        Node *cond = stmt->sons, *while_stmt = cond->next;
        char *cond_start_label = new_label_name(),
             *cond_fin_label = new_label_name(),
             *stmt_fin_label = new_label_name();
        ICList calc_cond = append(
                   new_ic(IC_LABEL, cond_start_label),
                   translate_cond(cond->sons, cond_fin_label, stmt_fin_label)
               ),
               while_icode = append(
                   new_ic(IC_LABEL, cond_fin_label),
                   translate_stmt(while_stmt->sons)
               ),
               while_fin_icode = append(
                   new_ic(IC_GOTO, cond_start_label),
                   new_ic(IC_LABEL, stmt_fin_label)
               );

        return append(calc_cond, append(while_icode, while_fin_icode));
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
                    param_type = pointer_type;
                }
                insertSymbol(name, param_type, VARDEC, var_dec->line);
                ic = append(ic, new_ic(IC_PARAM, new_operand(OP_VAR, name)));
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

ICList translate_copy_arr(char* left_name, char* right_name, uint32_t size) {
    if(left_name == NULL)
        return NULL;
    ICList copyAll = NULL;
    while(size != 0) {
        size -= 4;
        copyAll = append(
            copyAll,
            append(
                new_ic(IC_ASSIGN, new_operand(OP_DEREF, left_name), new_operand(OP_DEREF, right_name)),
                append(
                    new_ic(IC_ADD, new_operand(OP_VAR, left_name), new_operand(OP_VAR, left_name), new_operand(OP_CONST, 4)),
                    new_ic(IC_ADD, new_operand(OP_VAR, right_name), new_operand(OP_VAR, right_name), new_operand(OP_CONST, 4))
                )
            )
        );
    }
    return copyAll;
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
            switch (line->code.kind)
            {
            case IC_ASSIGN: {
                Operand left = line->code.u.assign.left, right = line->code.u.assign.right;
                if(left->kind == OP_VAR && left->u.var_name == NULL ||
                   left->kind == OP_VAR && right->kind == OP_VAR && 0 == strcmp(left->u.var_name, right->u.var_name)) {
                    DELETE_LINE(prev)
                }
                if(right->kind == OP_VAR) {
                    FIND_LAST_ASSIGN(u.assign.right, right, 3)
                }
                break;
            }
            case IC_ADD: case IC_SUB: case IC_MUL: case IC_DIV: {
                Operand res = line->code.u.binop.res, left = line->code.u.binop.left, right = line->code.u.binop.right;
                if(left->kind == OP_VAR) {
                    FIND_LAST_ASSIGN(u.binop.left, left, 1)
                }
                if(right->kind == OP_VAR) {
                    FIND_LAST_ASSIGN(u.binop.right, right, 1)
                }
                if(left->kind == OP_CONST && right->kind == OP_CONST) {
                    line->code.u.assign.left = res;
                    switch (line->code.kind)
                    {
                    case IC_ADD:
                        line->code.u.assign.right = new_operand(OP_CONST, left->u.val + right->u.val);
                        break;
                    case IC_SUB:
                        line->code.u.assign.right = new_operand(OP_CONST, left->u.val - right->u.val);
                        break;
                    case IC_MUL:
                        line->code.u.assign.right = new_operand(OP_CONST, left->u.val * right->u.val);
                        break;
                    case IC_DIV:
                        line->code.u.assign.right = new_operand(OP_CONST, left->u.val / right->u.val);
                        break;

                    default:
                        break;
                    }
                    line->code.kind = IC_ASSIGN;
                    free(left), free(right);
                    break;
                }
                if(left->kind == OP_CONST && left->u.val == 0 && line->code.kind != IC_SUB) {
                    line->code.u.assign.left = res;
                    switch (line->code.kind)
                    {
                    case IC_ADD:
                        line->code.u.assign.right = right;
                        break;
                    case IC_MUL: case IC_DIV:
                        line->code.u.assign.right = new_operand(OP_CONST, 0);
                        free(right);
                        break;

                    default:
                        break;
                    }
                    line->code.kind = IC_ASSIGN;
                    free(left);
                    break;
                }
                if(right->kind == OP_CONST && right->u.val == 0) {
                    line->code.u.assign.left = res;
                    switch (line->code.kind)
                    {
                    case IC_ADD: case IC_SUB:
                        line->code.u.assign.right = left;
                        break;
                    case IC_MUL:
                        line->code.u.assign.right = new_operand(OP_CONST, 0);
                        free(left);
                        break;
                    case IC_DIV:
                        assert(0);
                        break;

                    default:
                        break;
                    }
                    line->code.kind = IC_ASSIGN;
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
                    FIND_LAST_ASSIGN(u.uniop.op, op, 1);
                }
                break;
            }
            case IC_IFGOTO: {
                Operand op1 = line->code.u.cond_goto.op1, op2 = line->code.u.cond_goto.op2;
                if(op1->kind == OP_VAR) {
                    FIND_LAST_ASSIGN(u.cond_goto.op1, op1, 1);
                }
                if(op2->kind == OP_VAR) {
                    FIND_LAST_ASSIGN(u.cond_goto.op2, op2, 1);
                }
                break;
            }
            case IC_LABEL: {
                Operand label = line->code.u.uniop.op;
                for(ICList next_line = line->prev; next_line != NULL; next_line = next_line->prev) {
                    if(next_line->code.kind != IC_LABEL) {
                        break;
                    }
                    char *other_label_name = next_line->code.u.uniop.op->u.var_name;
                    for(ICList other_line = icode; other_line != NULL; other_line = other_line->next) {
                        Operand other_label = NULL;
                        switch (other_line->code.kind)
                        {
                        case IC_LABEL: case IC_GOTO:
                            other_label = other_line->code.u.uniop.op;
                            break;
                        case IC_IFGOTO:
                            other_label = other_line->code.u.cond_goto.target;
                            break;

                        default:
                            break;
                        }
                        if(other_label == NULL) {
                            continue;
                        }
                        if(0 == strcmp(other_label->u.var_name, other_label_name)) {
                            other_label->u.var_name = label->u.var_name;
                        }
                    }
                    DELETE_LINE(next)
                }
                break;
            }
            case IC_GOTO: case IC_PARAM: case IC_READ:
                break;
            case IC_DEC:
                break;

            default:
                break;
            }
        }
    }
    for(ICList line = icode; line != NULL; line = line->next) {
        switch (line->code.kind)
        {
        case IC_ASSIGN: {
            Operand left = line->code.u.assign.left, right = line->code.u.assign.right;
            COUNT_USED_VARIABLE(left)
            COUNT_USED_VARIABLE(right)
            break;
        }
        case IC_ADD: case IC_SUB: case IC_MUL: case IC_DIV: {
            Operand res = line->code.u.binop.res, left = line->code.u.binop.left, right = line->code.u.binop.right;
            COUNT_USED_VARIABLE(res)
            COUNT_USED_VARIABLE(left)
            COUNT_USED_VARIABLE(right)
            break;
        }
        case IC_RETURN: case IC_ARG: case IC_PARAM: case IC_READ: case IC_WRITE: {
            Operand op = line->code.u.uniop.op;
            COUNT_USED_VARIABLE(op)
            break;
        }
        case IC_DEC: {
            Operand first_byte = new_operand(OP_VAR, line->code.u.declr.first_byte);
            COUNT_USED_VARIABLE(first_byte);
            free(first_byte);
            break;
        }
        case IC_IFGOTO: {
            // TODO: LABEL
            Operand op1 = line->code.u.cond_goto.op1, op2 = line->code.u.cond_goto.op2;
            COUNT_USED_VARIABLE(op1)
            COUNT_USED_VARIABLE(op2)
            break;
        }
        case IC_LABEL: case IC_GOTO:
            break;

        default:
            break;
        }
    }
    for(ICList line = icode; line != NULL; line = line->next) {
        switch (line->code.kind)
        {
        case IC_ASSIGN: {
            Operand left = line->code.u.assign.left, right = line->code.u.assign.right;
            if(left->kind == OP_VAR && get_value(left->u.var_name) <= 1) {
                DELETE_LINE(prev);
            }
            break;
        }
        case IC_DEC: {
            char *var_name = line->code.u.declr.first_byte;
            if(get_value(var_name) <= 1) {
                DELETE_LINE(prev)
            }
            break;
        }

        default:
            break;
        }
    }
    return icode;
}

void print_operand(Operand op) {
    switch(op->kind) {
        case OP_VAR: {
            if(!print_stream)
                printf("%s", op->u.var_name);
            else
                fprintf(file_out, "%s", op->u.var_name);
            break;
        }
        case OP_CONST: {
            if(!print_stream)
                printf("#%d", op->u.val);
            else
                fprintf(file_out, "#%d", op->u.val);
            break;
        }
        case OP_ADDR: {
            if(!print_stream)
                printf("&%s", op->u.var_name);
            else
                fprintf(file_out, "&%s", op->u.var_name);
            break;
        }
        case OP_FUNC: {
            if(!print_stream)
                printf("CALL %s", op->u.var_name);
            else
                fprintf(file_out, "CALL %s", op->u.var_name);
            break;
        }
        case OP_DEREF: {
            if(!print_stream)
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
                print_operand(left);
                if(!print_stream)
                    printf(" := ");
                else
                    fprintf(file_out, " := ");
                print_operand(right);
                break;
            }
            case IC_ADD: case IC_SUB: case IC_MUL: case IC_DIV: {
                Operand left = head->code.u.binop.left, right = head->code.u.binop.right, res = head->code.u.binop.res;
                print_operand(res);
                if(!print_stream)
                    printf(" := ");
                else
                    fprintf(file_out, " := ");
                print_operand(left);
                if(!print_stream)
                    printf(" %c ", "+-*/"[head->code.kind]);
                else
                    fprintf(file_out, " %c ", "+-*/"[head->code.kind]);
                print_operand(right);
                break;
            }
            case IC_LABEL: case IC_FUNCTION: {
                if(!print_stream)
                    printf("%s ", icode_name[head->code.kind]);
                else
                    fprintf(file_out, "%s ", icode_name[head->code.kind]);
                print_operand(head->code.u.uniop.op);
                if(!print_stream)
                    printf(" :");
                else
                    fprintf(file_out, " :");
                break;
            }
            case IC_GOTO: case IC_RETURN: case IC_ARG: case IC_PARAM: case IC_READ: case IC_WRITE: {
                if(!print_stream)
                    printf("%s ", icode_name[head->code.kind]);
                else
                    fprintf(file_out, "%s ", icode_name[head->code.kind]);
                print_operand(head->code.u.uniop.op);
                break;
            }
            case IC_IFGOTO: {
                if(!print_stream)
                    printf("IF ");
                else
                    fprintf(file_out, "IF ");
                print_operand(head->code.u.cond_goto.op1);
                if(!print_stream)
                    printf(" %s ", head->code.u.cond_goto.relop);
                else
                    fprintf(file_out, " %s ", head->code.u.cond_goto.relop);
                print_operand(head->code.u.cond_goto.op2);
                if(!print_stream)
                    printf(" GOTO ");
                else
                    fprintf(file_out, " GOTO ");
                print_operand(head->code.u.cond_goto.target);
                break;
            }
            case IC_DEC: {
                if(!print_stream)
                    printf("DEC %s %u", head->code.u.declr.first_byte, head->code.u.declr.size);
                else
                    fprintf(file_out, "DEC %s %u", head->code.u.declr.first_byte, head->code.u.declr.size);
                break;
            }
            default: {
                if(!print_stream)
                    printf("Padding!");
                else
                    fprintf(file_out, "Padding!");
                break;
            }
        }
        if(!print_stream)
            putchar('\n');
        else
            fputc('\n', file_out);
    }
}