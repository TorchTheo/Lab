#ifndef __ASSEMBLE_C__
#define __ASSEMBLE_C__
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "include/type.h"
#include "include/constant.h"
#include "include/utils/asmhelper.h"

extern FILE* file_out;

static AsmOp newAsmOp(int, ...);
static AsmList newAsmCode(int, ...);
static AsmList append(AsmList, AsmList);
static int getVarReg(char*, AsmList);
static int getImmReg(int, AsmList);
static int getReg(Operand, AsmList);
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

inline static int getVarReg(char *name, AsmList alist) {
    static int reg_index = 0;
    return reg_index++;
}

inline static int getImmReg(int imm, AsmList alist) {
    return getVarReg("", alist);
}

static int getReg(Operand op, AsmList alist) {
    if(op->kind == OP_CONST)
        return getImmReg(op->u.val, alist);
    return getVarReg(op->u.var_name, alist);
}

static AsmList generateAsmList(ICList icode_list) {
    AsmList asm_list = NULL;

    for (ICList line = icode_list; line != NULL; line = line->next) {
        switch (line->code.kind) {
            case IC_ADD: {
                CHECK_AND_GET_BINOP(res, op1, op2);
                COMPARE_AND_APPEND(ASM_ADD, ASM_ADDI, IMM, op2->u.val);
                break;
            }
            case IC_SUB: {
                CHECK_AND_GET_BINOP(res, op1, op2);
                COMPARE_AND_APPEND(ASM_SUB, ASM_ADDI, IMM, -op2->u.val);
                break;
            }
            case IC_MUL: {
                CHECK_AND_GET_BINOP(res, op1, op2);
                COMPARE_AND_APPEND(
                    ASM_MUL,
                    ASM_MUL,
                    REG,
                    getReg(op2, asm_list)
                );
                break;
            }
            case IC_DIV: {
                CHECK_AND_GET_BINOP(res, op1, op2);
                asm_list = append(
                    asm_list,
                    newAsmCode(
                        ASM_DIV,
                        newAsmOp(REG, getReg(op1, asm_list)),
                        newAsmOp(REG, getReg(op2, asm_list))
                    )
                );

                asm_list = append(
                    asm_list,
                    newAsmCode(
                        ASM_MFLO,
                        newAsmOp(REG, getReg(res, asm_list))
                    )
                );
                break;
            }
            case IC_ASSIGN: {
                Operand left = NULL, right = NULL;
                if(line->code.u.assign.left->kind == OP_VAR)
                    left = line->code.u.assign.left;
                if(line->code.u.assign.right->kind == OP_VAR || line->code.u.assign.right->kind == OP_CONST)
                    right = line->code.u.assign.right;
                assert(left != NULL); assert(right != NULL);

                if(right->kind == OP_VAR)
                    asm_list = append(
                        asm_list,
                        newAsmCode(
                            ASM_MOVE, 
                            newAsmOp(REG, getReg(left, asm_list)), 
                            newAsmOp(REG, getReg(right, asm_list))
                        )
                    );
                else
                    asm_list = append(
                        asm_list,
                        newAsmCode(
                            ASM_LI,
                            newAsmOp(REG, getReg(left, asm_list)),
                            newAsmOp(IMM, right->u.val)
                        )
                    );
                break;
            }
            case IC_LABEL: case IC_FUNCTION:
                asm_list = append(
                    asm_list,
                    newAsmCode(ASM_LABEL, newAsmOp(NAME, line->code.u.uniop.op->u.var_name))
                );
                break;
            case IC_GOTO:
                asm_list = append(
                    asm_list,
                    newAsmCode(ASM_J, newAsmOp(NAME, line->code.u.uniop.op->u.var_name))
                );
                break;
            case IC_IFGOTO: {
                Operand op1 = line->code.u.cond_goto.op1, op2 = line->code.u.cond_goto.op2;
                assert(op1->kind == OP_VAR || op1->kind == OP_CONST);
                assert(op2->kind == OP_VAR || op2->kind == OP_CONST);
                int relop = -1;
                for (int i = 0; i < 6; i++)
                    if (!strcmp(relop_str[i], line->code.u.cond_goto.relop)) {
                        relop = ASM_BEQ + i;
                        break;
                    }
                
                assert(relop >= 0);
                asm_list = append(
                    asm_list,
                    newAsmCode(
                        relop,
                        newAsmOp(REG, getReg(op1, asm_list)),
                        newAsmOp(REG, getReg(op2, asm_list)),
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
    }

    return asm_list;
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
                // TODO
                fprintf(file_out, "$t%d", op->u.val);
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
                    fputc(' ', file_out);
                    printAsmOp(line->asm_code.u.binop.op2);
                    break;
                case ASM_ADD: case ASM_ADDI: case ASM_SUB: case ASM_MUL:
                case ASM_BEQ: case ASM_BNE: case ASM_BGT: case ASM_BLT: case ASM_BGE: case ASM_BLE:
                    fprintf(file_out, "    %s ", instrs[line->asm_code.asm_kind]);
                    printAsmOp(line->asm_code.u.triop.op1);
                    fputc(' ', file_out);
                    printAsmOp(line->asm_code.u.triop.op2);
                    fputc(' ', file_out);
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