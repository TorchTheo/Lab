#ifndef __ASSEMBLE_C__
#define __ASSEMBLE_C__
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "include/type.h"
#include "include/constant.h"
#include "include/regdesc.h"
#include "include/utils/asmhelper.h"
#include "include/utils/hashtable.h"

extern FILE* file_out;

static int stack_off = 0;

static AsmOp newAsmOp(int, ...);
static AsmList newAsmCode(int, ...);
static AsmList append(AsmList, AsmList);
static int getVarReg(char*);
static int getImmReg(int);
inline static int getReg(Operand);
static void storeReg(uint8_t, AsmList);
static void clearReg(uint8_t, AsmList);
static void loadReg(uint8_t, char*, AsmList);
static void loadRegImm(uint8_t, int, AsmList);
static void addVar(char*, uint8_t);
static void clearVar(char*);
static void prepareVarInStack(char*, AsmList);
static void getVarInStack(char*, int);
static AsmList generateAsmList(ICList);
static void printAsmOp(AsmOp);
void printAsmList(AsmList);
AsmList start_assemble(ICList);

static AsmOp newAsmOp(int type, ...) {
    AsmOp op = calloc(1, SIZEOF(AsmOp_));
    va_list list;
    va_start(list, type);
    op->op_kind = type;
    switch (type) {
        case IMM: case REG:
            op->u.val = va_arg(list, int);
            break;
        case NAME:
            op->u.name = va_arg(list, char *);
            break;
        case ADDR:
            op->u.addr.offset = va_arg(list, int);
            op->u.addr.base_reg = va_arg(list, int);
            break;
        default:
            assert(0);
            break;
    }
    va_end(list);
    return op;
}

static AsmList newAsmCode(int type, ...) {
    AsmList alist = calloc(1, SIZEOF(AsmList_));
    va_list list;
    va_start(list, type);
    alist->asm_code.asm_kind = type;
    switch (type) {
        case ASM_LABEL: case ASM_MFLO: case ASM_J: case ASM_JAL: case ASM_JR:
            // uniop
            alist->asm_code.u.uniop.op = va_arg(list, AsmOp);
            break;
        case ASM_LI: case ASM_LW: case ASM_SW: case ASM_MOVE: case ASM_DIV:
            // binop
            alist->asm_code.u.binop.op1 = va_arg(list, AsmOp);
            alist->asm_code.u.binop.op2 = va_arg(list, AsmOp);
            break;
        case ASM_ADD: case ASM_ADDI: case ASM_SUB: case ASM_MUL:
        case ASM_BEQ: case ASM_BNE: case ASM_BGT: case ASM_BLT: case ASM_BGE: case ASM_BLE:
            // triop
            alist->asm_code.u.triop.op1 = va_arg(list, AsmOp);
            alist->asm_code.u.triop.op2 = va_arg(list, AsmOp);
            alist->asm_code.u.triop.op3 = va_arg(list, AsmOp);
            break;
        case ASM_SYS:
            break;
        default:
            assert(0);
            break;
    }
    alist->next = alist->prev = NULL, alist->tail = alist;
    va_end(list);
    return alist;
}

static AsmList append(AsmList head, AsmList tail) {
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

static int getVarReg(char *name) {
    AddrDesc addr = (AddrDesc)get_value(name);
    for (int i = 0; i < NR_REGS; i++)
        if (CHECK_REG(addr->reg_vec, i))
            return i;
    
    for (int i = NR_REGS - 1; i >= 0; i--)
        if (CHECK_USE_FREE(i) && regs[i].content.lst->next == NULL)
            return i;
    
    static uint8_t next_reg = 0;
    do {
        next_reg = (next_reg + 1) % NR_REGS;
    } while(!CHECK_USE_FREE(next_reg));
    return next_reg;
}

static int getImmReg(int imm) {
    if (imm == 0)
        return REG_ZERO;
    for (int i = 0; i < NR_REGS; i++)
        if (CHECK_USE_FREE(i) && regs[i].content.lst->next == NULL)
            return i;

    static uint8_t next_reg = 0;
    do {
        next_reg = (next_reg + 1) % NR_REGS;
    } while(!CHECK_USE_FREE(next_reg));
    return next_reg;
}

inline static int getReg(Operand op) {
    if (op->kind == OP_CONST)
        return getImmReg(op->u.val);
    return getVarReg(op->u.var_name);
}

static void storeReg(uint8_t reg_index, AsmList alist) {
    for (strLinkList name_node = regs[reg_index].content.lst->next; name_node != NULL; name_node = name_node->next) {
        AddrDesc addr = (AddrDesc)get_value(name_node->value);
        if (addr == NULL) {
            assert(
                name_node->value != NULL &&
                strlen(name_node->value) == 0 &&
                name_node->next == NULL &&
                regs[reg_index].content.lst->next == name_node
            );
            continue;
        }
        alist = append(
            alist, 
            newAsmCode(
                ASM_SW,
                newAsmOP(REG, reg_index),
                newAsmOP(ADDR, addr->addr_list->value, REG_FP)
            )
        );
        intLinklistInsert(addr->addr_list, addr->addr_list->value);
    }
}

static void clearReg(uint8_t reg_index, AsmList alist) {
    storeReg(reg_index, alist);
    for (strLinkList name_node = regs[reg_index].content.lst->next; name_node != NULL; name_node = name_node->next) {
        AddrDesc addr = (AddrDesc)get_value(name_node->value);
        if (addr == NULL) {
            assert(
                name_node->value != NULL &&
                strlen(name_node->value) == 0 &&
                name_node->next == NULL &&
                regs[reg_index].content.lst->next == name_node
            );
            continue;
        }
        CLEAR_REG(addr->reg_vec, reg_index);
    }
    intLinklistClear(regs[reg_index].content.lst);
    regs[reg_index].content.imm = 0;
}

static void loadReg(uint8_t reg_index, char *name, AsmList alist) {
    AddrDesc addr = (AddrDesc)get_value(name);
    if (addr == NULL) {
        assert(name != NULL && strlen(name) == 0);
        clearReg(reg_index, alist);
        strLinklistInsert(regs[reg_index].content.lst, name);
        return;
    }
    if (CHECK_REG(addr->reg_vec, reg_index))
        return;
    clearReg(reg_index, alist);
    if (addr->reg_vec != 0) {
        for (int i = 0; i < NR_REGS; i++)
            if (CHECK_REG(addr->reg_vec, i)) {
                alist = append(
                    alist,
                    newAsmCode(
                        ASM_MOVE,
                        newAsmOP(REG, reg_index),
                        newAsmOP(REG, i)
                    )
                );
                break;
            }
    } else {
        assert(addr->addr_list->next != NULL);
        alist = append(
            alist, 
            newAsmCode(
                ASM_LW,
                newAsmOP(REG, reg_index),
                newAsmOp(ADDR, addr->addr_list->next->value, REG_FP)
            )
        );
    }

    strLinklistInsert(regs[reg_index].content.lst, name);
    SET_REG(addr->reg_vec, reg_index);
}

static void loadRegImm(uint8_t reg_index, int imm, AsmList alist) {
    if (regs[reg_index].content.lst->next == NULL && regs[reg_index].content.imm == imm && imm)
        return;
    clearReg(reg_index, alist);
    alist = append(
        alist,
        newAsmCode(
            ASM_LI,
            newAsmOP(REG, reg_index),
            newAsmOP(IMM, imm)
        )
    );
    regs[reg_index].content.imm = imm;
}

static void addVar(char *name, uint8_t reg_index) {
    AddrDesc addr = (AddrDesc)get_value(name);
    if (!CHECK_REG(addr->reg_vec, reg_index)) {
        SET_REG(addr->reg_vec, reg_index);
        strLinklistInsert(regs[reg_index].content.lst, name);
    }
}

static void clearVar(char *name) {
    AddrDesc addr = (AddrDesc)get_value(name);
    intLinklistClear(addr->addr_list);
    for (int i = 0; i < NR_REGS; i++)
        if (CHECK_REG(addr->reg_vec, i))
            strLinklistDelete(regs[i].content.lst, name);
    addr->reg_vec = 0;
}

static void prepareVarInStack(char *name, AsmList alist) {
    AddrDesc var_desc = (AddrDesc)get_value(name);
    if (var_desc == NULL) {
        alist = append(
            alist, 
            PUSH_STACK(1)
        );
        stack_off -= 4;

        var_desc = calloc(1, SIZEOF(AddrDesc_));
        var_desc->var_name = name;
        var_desc->addr_list = intNewLinkList();
        var_desc->addr_list->value = stack_off;
        *insert(name) = (uint64_t)var_desc;
    }
}

static void getVarInStack(char *name, int offset) {
    AddrDesc var_desc = (AddrDesc)get_value(name);
    assert(var_desc == NULL);

    var_desc = calloc(1, SIZEOF(AddrDesc_));
    var_desc->var_name = name;
    var_desc->addr_list = intNewLinkList();
    var_desc->addr_list->value = offset;
    intLinklistInsert(var_desc->addr_list, offset);
    *insert(name) = (uint64_t)var_desc;
}

static AsmList generateAsmList(ICList icode_list) {
    AsmList alist = NULL;
    clear_table();
    for (int i = 0; i < NR_REGS; i++)
        regs[i].content.lst = strNewLinkList();

    for (ICList line = icode_list; line != NULL; line = line->next) {
        switch (line->code.kind) {
            case IC_ADD: {
                GENERATE_ADDSUB_ASM(ASM_ADD, +);
                break;
            }
            case IC_SUB: {
                GENERATE_ADDSUB_ASM(ASM_SUB, -);
                break;
            }
            case IC_MUL: {
                GENERATE_MULDIV_ASM(
                    newAsmCode(
                        ASM_MUL,
                        newAsmOP(REG, reg_res),
                        newAsmOP(REG, reg_op1),
                        newAsmOP(REG, reg_op2)
                    )
                );
                break;
            }
            case IC_DIV: {
                GENERATE_MULDIV_ASM(
                    append(
                        newAsmCode(
                            ASM_DIV,
                            newAsmOP(REG, reg_op1),
                            newAsmOP(REG, reg_op2)
                        ),
                        newAsmCode(
                            ASM_MFLO,
                            newAsmOP(REG, reg_res)
                        )
                    )
                );
                break;
            }
            case IC_ASSIGN: {
                Operand left = NULL, right = NULL;
                if (line->code.u.assign.left->kind == OP_VAR)
                    left = line->code.u.assign.left;
                if (line->code.u.assign.right->kind == OP_VAR || line->code.u.assign.right->kind == OP_CONST || line->code.u.assign.right->kind == OP_FUNC)
                    right = line->code.u.assign.right;
                assert(left != NULL); assert(right != NULL);

                prepareVarInStack(left->u.var_name, alist);

                uint8_t reg_right;

                switch (right->kind) {
                    case OP_VAR:
                        prepareVarInStack(right->u.var_name, alist);
                        reg_right = getReg(right);
                        loadReg(reg_right, right->u.var_name, alist);
                        break;
                    case OP_CONST:
                        reg_right = getReg(right);
                        loadRegImm(reg_right, right->u.val, alist);
                        break;
                    case OP_FUNC: {
                        int arg_index = 0;
                        for (ICList prev_line = line->prev; prev_line != NULL; prev_line = prev_line->prev, arg_index++) {
                            if (prev_line->code.kind != IC_ARG)
                                break;
                            Operand arg = prev_line->code.u.uniop.op;
                            if (arg_index < 4) {
                                if (arg->kind == OP_VAR) {
                                    prepareVarInStack(arg->u.var_name, alist);
                                    loadReg(REG_ARG_0 + arg_index, arg->u.var_name, alist);
                                } else {
                                    assert(arg->kind == OP_CONST);
                                    loadRegImm(REG_ARG_0 + arg_index, arg->u.val, alist);
                                }
                            } else {
                                if (arg->kind == OP_VAR) {
                                    prepareVarInStack(arg->u.var_name, alist);
                                    uint8_t reg_arg = getReg(arg);
                                    loadReg(reg_arg, arg->u.var_name, alist);
                                    alist = append(
                                        alist,
                                        PUSH_REG_CODE(reg_arg)
                                    );
                                } else {
                                    assert(arg->kind == OP_CONST);
                                    uint8_t reg_arg = getReg(arg);
                                    loadRegImm(reg_arg, arg->u.val, alist);
                                    alist = append(
                                        alist,
                                        PUSH_REG_CODE(reg_arg)
                                    );
                                }
                            }
                        }
                        for (int i = 0; i < NR_REGS; i++) {
                            clearReg(i, alist);
                        }
                        alist = append(
                            alist,
                            PUSH_REG_CODE(REG_RET)
                        );
                        alist = append(
                            alist,
                            newAsmCode(
                                ASM_JAL,
                                newAsmOP(NAME, right->u.var_name)
                            )
                        );
                        reg_right = REG_VALUE_0;
                        alist = append(
                            alist,
                            POP_REG_CODE(REG_RET)
                        );
                        if (arg_index >= 4)
                            alist = append(
                                alist,
                                POP_STACK(arg_index - 4)
                            );
                        break;
                    }
                    
                    default:
                        break;
                }
                clearVar(left->u.var_name);
                addVar(left->u.var_name, reg_right);
                break;
            }
            case IC_LABEL: case IC_FUNCTION: {
                alist = append(
                    alist,
                    newAsmCode(ASM_LABEL, newAsmOp(NAME, line->code.u.uniop.op->u.var_name))
                );
                if (line->code.kind == IC_FUNCTION) {
                    stack_off = 0;
                    alist = append(
                        alist,
                        PUSH_REG_CODE(REG_FP)
                    );
                    alist = append(
                        alist,
                        newAsmCode(
                            ASM_MOVE,
                            newAsmOP(REG, REG_FP),
                            newAsmOP(REG, REG_SP)
                        )
                    );
                    int param_index = 0;
                    for (ICList next_line = line->next; next_line != NULL; next_line = next_line->next, param_index++) {
                        if (next_line->code.kind != IC_PARAM)
                            break;
                        Operand param = next_line->code.u.uniop.op;
                        assert(param->kind == OP_VAR);
                        if (param_index < 4) {
                            prepareVarInStack(param->u.var_name, alist);
                            addVar(param->u.var_name, REG_ARG_0 + param_index);
                        } else
                            getVarInStack(param->u.var_name, 8 + (param_index - 4) * 4);
                    }
                }
                break;
            }
            case IC_GOTO:
                alist = append(
                    alist,
                    newAsmCode(ASM_J, newAsmOp(NAME, line->code.u.uniop.op->u.var_name))
                );
                break;
            case IC_IFGOTO: {
                Operand op[2] = { line->code.u.cond_goto.op1, line->code.u.cond_goto.op2 };
                assert(op[0]->kind == OP_VAR || op[0]->kind == OP_CONST);
                assert(op[1]->kind == OP_VAR || op[1]->kind == OP_CONST);
                int relop = -1;
                for (int i = 0; i < 6; i++)
                    if (!strcmp(relop_str[i], line->code.u.cond_goto.relop)) {
                        relop = i;
                        break;
                    }
                
                if (op[0]->kind == OP_CONST) {
                    if (op[1]->kind == OP_CONST) {
                        op[0]->u.val -= op[1]->u.val;
                        op[1]->u.val = 0;
                    } else {
                        Operand tmp = op[0];
                        op[0] = op[1], op[1] = tmp;
                        if (relop >= 2)
                            relop += (relop % 2 ? -1 : 1);
                    }
                }

                uint8_t op_reg[2];
                for (int i = 0; i < 2; i++) {
                    if (op[i]->kind == OP_CONST) {
                        op_reg[i] = getReg(op[i]);
                        loadRegImm(op_reg[i], op[i]->u.val, alist);
                    } else {
                        prepareVarInStack(op[i]->u.var_name, alist);
                        op_reg[i] = getReg(op[i]);
                        loadReg(op_reg[i], op[i]->u.var_name, alist);
                    }
                }

                assert(relop >= 0);
                alist = append(
                    alist,
                    newAsmCode(
                        relop + ASM_BEQ,
                        newAsmOp(REG, op_reg[0]),
                        newAsmOp(REG, op_reg[1]),
                        newAsmOp(NAME, line->code.u.cond_goto.target->u.var_name)
                    )
                );
                break;
            }
            // TODO
            case IC_RETURN: {
                Operand rs = line->code.u.uniop.op;
                clearReg(REG_VALUE_0, alist);
                switch (rs->kind) {
                    case OP_VAR:
                        prepareVarInStack(rs->u.var_name, alist);
                        loadReg(REG_VALUE_0, rs->u.var_name, alist);
                        break;
                    case OP_CONST:
                        loadRegImm(REG_VALUE_0, rs->u.val, alist);
                        break;
                    case OP_FUNC:
                        assert(0);
                        break;
                }
                alist = append(
                    alist,
                    append(
                        append(
                            newAsmCode(
                                ASM_MOVE,
                                newAsmOP(REG, REG_SP),
                                newAsmOp(REG, REG_FP)
                            ),
                            POP_REG_CODE(REG_FP)
                        ),
                        newAsmCode(
                            ASM_JR,
                            newAsmOP(REG, REG_RET)
                        )
                    )
                );
                break;
            }
            case IC_DEC:
                assert(0);
                break;
            case IC_ARG:
                break;
            case IC_PARAM:
                break;
            case IC_READ: {
                Operand dest = NULL;
                if (line->code.u.uniop.op->kind == OP_VAR)
                    dest = line->code.u.uniop.op;
                assert(dest != NULL);
                prepareVarInStack(dest->u.var_name, alist);
                clearReg(REG_VALUE_0, alist);
                loadRegImm(REG_VALUE_0, 5, alist);

                alist = append(
                    alist,
                    newAsmCode(ASM_SYS)
                );

                addVar(dest->u.var_name, REG_VALUE_0);
                break;
            }
            case IC_WRITE:{
                Operand src = NULL;
                if (line->code.u.uniop.op->kind == OP_VAR || line->code.u.uniop.op->kind == OP_CONST)
                    src = line->code.u.uniop.op;
                assert(src != NULL);
                clearReg(REG_VALUE_0, alist);
                loadRegImm(REG_VALUE_0, 1, alist);
                if (src->kind == OP_VAR) {
                    prepareVarInStack(src->u.var_name, alist);
                    clearReg(REG_ARG_0, alist);
                    loadReg(REG_ARG_0, src->u.var_name, alist);
                } else {
                    clearReg(REG_ARG_0, alist);
                    loadRegImm(REG_ARG_0, src->u.val, alist);
                }
                alist = appewdn(
                    alist,
                    newAsmCode(ASM_SYS)
                );
                
                clearReg(REG_VALUE_0, alist);
                loadRegImm(REG_VALUE_0, 11, alist);
                clearReg(REG_ARG_0, alist);
                loadRegImm(REG_ARG_0, '\n', alist);

                alist = append(
                    alist,
                    newAsmCode(ASM_SYS)
                );

                break;
            }
            default:
                assert(0);
                break;
        }

        if (
            line->next != NULL && (
                line->next->code.kind == IC_LABEL ||
                line->next->code.kind == IC_FUNCTION ||
                line->next->code.kind == IC_GOTO ||
                line->next->code.kind == IC_IFGOTO ||
                line->next->code.kind == IC_RETURN
            )
        )
            for (int i = 0; i < NR_REGS; i++)
                clearReg(i, alist);
    }

    return alist;
}

static void printAsmOp(AsmOp op) {
    if (file_out != NULL) {
        switch (op->op_kind) {
            case NAME:
                fprintf(file_out, "%s", op->u.name);
                break;
            case IMM:
                fprintf(file_out, "%d", op->u.val);
                break;
            case REG:
                fprintf(file_out, "%s", reg_name[op->u.val]);
                break;
            case ADDR:
                fprintf(file_out, "%d(%s)", op->u.addr.offset, reg_name[op->u.addr.base_reg]);
                break;
            default:
                assert(0);
                break;
        }
    }
}

void printAsmList(AsmList alist) {
    if (file_out != NULL) {
        fprintf(file_out, ".globl main\n.text\n");
        for (AsmList line = alist; line != NULL; line = line->next) {
            switch (line->asm_code.asm_kind) {
                case ASM_SYS:
                    fprintf(file_out, "    %s", instrs[line->asm_code.asm_kind]);
                    break;
                case ASM_LABEL:
                    fprintf(file_out, "%s:", line->asm_code.u.uniop.op->u.name);
                    break;
                case ASM_MFLO: case ASM_J: case ASM_JAL: case ASM_JR:
                    fprintf(file_out, "    %s ", instrs[line->asm_code.asm_kind]);
                    printAsmOp(line->asm_code.u.uniop.op);
                    break;
                case ASM_LI: case ASM_LW: case ASM_SW: case ASM_MOVE: case ASM_DIV:
                    fprintf(file_out, "    %s ", instrs[line->asm_code.asm_kind]);
                    printAsmOp(line->asm_code.u.binop.op1);
                    fputs(", ", file_out);
                    printAsmOp(line->asm_code.u.binop.op2);
                    break;
                case ASM_ADD: case ASM_ADDI: case ASM_SUB: case ASM_MUL:
                case ASM_BEQ: case ASM_BNE: case ASM_BGT: case ASM_BLT: case ASM_BGE: case ASM_BLE:
                    fprintf(file_out, "    %s ", instrs[line->asm_code.asm_kind]);
                    printAsmOp(line->asm_code.u.triop.op1);
                    fputs(", ", file_out);
                    printAsmOp(line->asm_code.u.triop.op2);
                    fputs(", ", file_out);
                    printAsmOp(line->asm_code.u.triop.op3);
                    break;
                default:
                    assert(0);
                    break;
            }
            fputc('\n', file_out);
        }
    }
}

AsmList start_assemble(ICList ic) {
    return generateAsmList(ic);
}

#endif