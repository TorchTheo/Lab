#ifndef __ASMHELPER_H__
#define __ASMHELPER_H__

#define CHECK_AND_GET_BINOP(res, op1, op2)                                              \
    Operand res = NULL, op1 = NULL, op2 = NULL;                                         \
    if (line->code.u.binop.res->kind == OP_VAR) {                                       \
        res = line->code.u.binop.res;                                                   \
    }                                                                                   \
    if (                                                                                \
        line->code.u.binop.left->kind == OP_CONST &&                                    \
        line->code.u.binop.right->kind == OP_VAR                                        \
    ) {                                                                                 \
        op1 = line->code.u.binop.right;                                                 \
        op2 = line->code.u.binop.left;                                                  \
    } else {                                                                            \
        op2 = line->code.u.binop.right;                                                 \
        op1 = line->code.u.binop.left;                                                  \
    }                                                                                   \
    assert(res != NULL);                                                                \
    assert(op1->kind == OP_VAR);                                                        \
    assert(op2->kind == OP_VAR || op2->kind == OP_CONST);

#define COMPARE_AND_APPEND(last_op_is_var, last_op_is_imm, last_op_type, last_op_body)  \
    if(op2->kind == OP_VAR) {                                                           \
        asm_list = append(                                                              \
            asm_list,                                                                   \
            newAsmCode(                                                                 \
                last_op_is_var,                                                         \
                newAsmOp(REG, getReg(res, asm_list)),                                   \
                newAsmOp(REG, getReg(op1, asm_list)),                                   \
                newAsmOp(REG, getReg(op2, asm_list))                                    \
            )                                                                           \
        );                                                                              \
    } else {                                                                            \
        asm_list = append(                                                              \
            asm_list,                                                                   \
            newAsmCode(                                                                 \
                last_op_is_imm,                                                         \
                newAsmOp(REG, getReg(res, asm_list)),                                   \
                newAsmOp(REG, getReg(op1, asm_list)),                                   \
                newAsmOp(last_op_type, last_op_body)                                    \
            )                                                                           \
        );                                                                              \
    }

#define PUSH_STACK                                                                      \
    newAsmCode(                                                                         \
        ASM_ADDI,                                                                       \
        newAsmOp(REG, REG_SP),                                                          \
        newAsmOp(REG, REG_SP),                                                          \
        newAsmOP(IMM, -4)                                                               \
    )

#define PUSH_REG_CODE(reg)                                                              \
    append(                                                                             \
        PUSH_STACK,                                                                     \
        newAsmCode(                                                                     \
            ASM_SW,                                                                     \
            newAsmOP(REG, reg),                                                         \
            newAsmOP(ADDR, 0, REG_SP)                                                   \
        )                                                                               \
    )

#endif