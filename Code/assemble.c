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
    
    assert(0);
}

static int getImmReg(int imm) {
    if (imm == 0)
        return REG_ZERO;
    for (int i = 0; i < NR_REGS; i++)
        if (CHECK_USE_FREE(i) && regs[i].content.lst->next == NULL)
            return i;

    assert(0);
}

inline static int getReg(Operand op) {
    if (op->kind == OP_CONST)
        return getImmReg(op->u.val);
    return getVarReg(op->u.var_name);
}

static void storeReg(uint8_t reg_index, AsmList alist) {
    for (LinkList name_node = regs[reg_index].content.lst->next; name_node != NULL; name_node = name_node->next) {
        AddrDesc addr = (AddrDesc)get_value((char*)(name_node->value));
        alist = append(
            alist, 
            newAsmCode(
                ASM_SW,
                newAsmOP(REG, reg_index),
                newAsmOP(ADDR, addr->addr_list->value, REG_FP)
            )
        );
        linklistInsert(addr->addr_list, addr->addr_list->value);
    }
}

static void clearReg(uint8_t reg_index, AsmList alist) {
    storeReg(reg_index, alist);
    for (LinkList name_node = regs[reg_index].content.lst->next; name_node != NULL; name_node = name_node->next) {
        AddrDesc addr = (AddrDesc)get_value((char*)(name_node->value));
        CLEAR_REG(addr->reg_vec, reg_index);
    }
    linklistClear(regs[reg_index].content.lst);
}

static void loadReg(uint8_t reg_index, char *name, AsmList alist) {
    AddrDesc addr = (AddrDesc)get_value(name);
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

    linklistInsert(regs[reg_index].content.lst, (int64_t)name);
    SET_REG(addr->reg_vec, reg_index);
}

static void loadRegImm(uint8_t reg_index, int imm, AsmList alist) {
    if (regs[reg_index].content.lst->next == NULL && regs[reg_index].content.imm == imm)
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
        linklistInsert(regs[reg_index].content.lst, (int64_t)(name));
    }
}

static void clearVar(char *name) {
    AddrDesc addr = (AddrDesc)get_value(name);
    linklistClear(addr->addr_list);
    for (int i = 0; i < NR_REGS; i++)
        if (CHECK_REG(addr->reg_vec, i))
            linklistDelete(regs[i].content.lst, (int64_t)(name));
    addr->reg_vec = 0;
}

static void prepareVarInStack(char *name, AsmList alist) {
    AddrDesc var_desc = (AddrDesc)get_value(name);
    if (var_desc == NULL) {
        alist = append(
            alist, 
            PUSH_STACK
        );
        stack_off -= 4;

        var_desc = calloc(1, SIZEOF(AddrDesc_));
        var_desc->var_name = name;
        var_desc->addr_list = newLinkList();
        var_desc->addr_list->value = stack_off;
        *insert(name) = (uint64_t)var_desc;
    }
}

static AsmList generateAsmList(ICList icode_list) {
    AsmList alist = NULL;
    clear_table();
    for (int i = 0; i < NR_REGS; i++)
        regs[i].content.lst = newLinkList();

    for (ICList line = icode_list; line != NULL; line = line->next) {
        switch (line->code.kind) {
            case IC_ADD: {
                CHECK_AND_GET_BINOP(res, op1, op2);
                prepareVarInStack(res->u.var_name, alist);
                prepareVarInStack(op1->u.var_name, alist);

                uint8_t reg_res, reg_op1;

                reg_op1 = getReg(op1);
                loadReg(reg_op1, op1->u.var_name, alist);

                reg_res = getReg(res);
                clearReg(reg_res, alist);

                if (op2->kind == OP_VAR) {
                    prepareVarInStack(op2->u.var_name, alist);
                    uint8_t reg_op2 = getReg(op2);
                    loadReg(reg_op2, op2->u.var_name, alist);

                    alist = append(
                        alist,
                        newAsmCode(
                            ASM_ADD,
                            newAsmOP(REG, reg_res),
                            newAsmOP(REG, reg_op1),
                            newAsmOP(REG, reg_op2)
                        )
                    );
                } else
                    alist = append(
                        alist,
                        newAsmCode(
                            ASM_ADD,
                            newAsmOP(REG, reg_res),
                            newAsmOP(REG, reg_op1),
                            newAsmOP(IMM, op2->u.val)
                        )
                    );
                clearVar(res->u.var_name);
                addVar(res->u.var_name, reg_res);
                break;
            }
            case IC_SUB: {
                CHECK_AND_GET_BINOP(res, op1, op2);
                break;
            }
            case IC_MUL: {
                CHECK_AND_GET_BINOP(res, op1, op2);
                break;
            }
            case IC_DIV: {
                CHECK_AND_GET_BINOP(res, op1, op2);
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

                uint8_t reg_left = getReg(left), reg_right;

                clearReg(reg_left, alist);
                switch (right->kind) {
                    case OP_VAR:
                        prepareVarInStack(right->u.var_name, alist);
                        reg_right = getReg(right);
                        loadReg(reg_right, right->u.var_name, alist);
                        alist = append(
                            alist,
                            newAsmCode(
                                ASM_MOVE, 
                                newAsmOp(REG, reg_left), 
                                newAsmOp(REG, reg_right)
                            )
                        );
                        clearVar(left->u.var_name);
                        addVar(left->u.var_name, reg_left);

                        addVar(left->u.var_name, reg_right);
                        addVar(right->u.var_name, reg_left);
                        break;
                    case OP_CONST:
                        alist = append(
                            alist,
                            newAsmCode(
                                ASM_LI,
                                newAsmOp(REG, reg_left),
                                newAsmOp(IMM, right->u.val)
                            )
                        );
                        clearVar(left->u.var_name);
                        addVar(left->u.var_name, reg_left);
                        break;
                    case OP_FUNC:
                        assert(0);
                        break;
                    
                    default:
                        break;
                }
                break;
            }
            case IC_LABEL: case IC_FUNCTION:
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
                }
                break;
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
                        relop = ASM_BEQ + i;
                        break;
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
                        relop,
                        newAsmOp(REG, op_reg[0]),
                        newAsmOp(REG, op_reg[1]),
                        newAsmOp(NAME, line->code.u.cond_goto.target->u.var_name)
                    )
                );
                break;
            }
            // TODO
            case IC_RETURN:
                break;
            case IC_DEC:
                break;
            case IC_ARG:
                break;
            case IC_PARAM:
                break;
            case IC_READ:
                break;
            case IC_WRITE:
                break;
            default:
                assert(0);
                break;
        }

        if (
            line->next != NULL && (
                line->next->code.kind == IC_LABEL ||
                line->next->code.kind == IC_FUNCTION ||
                line->next->code.kind == IC_GOTO ||
                line->next->code.kind == IC_IFGOTO
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
        for (AsmList line = alist; line != NULL; line = line->next) {
            switch (line->asm_code.asm_kind) {
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