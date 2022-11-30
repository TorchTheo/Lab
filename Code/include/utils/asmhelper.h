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

#define PUSH_STACK(num)                                                                 \
    newAsmCode(                                                                         \
        ASM_ADDI,                                                                       \
        newAsmOp(REG, REG_SP),                                                          \
        newAsmOp(REG, REG_SP),                                                          \
        newAsmOP(IMM, -4 * (num))                                                       \
    )

#define POP_STACK(num)                                                                  \
    newAsmCode(                                                                         \
        ASM_ADDI,                                                                       \
        newAsmOP(REG, REG_SP),                                                          \
        newAsmOp(REG, REG_SP),                                                          \
        newAsmOp(IMM, 4 * (num))                                                        \
    )

#define PUSH_REG_CODE(reg)                                                              \
    append(                                                                             \
        PUSH_STACK(1),                                                                  \
        newAsmCode(                                                                     \
            ASM_SW,                                                                     \
            newAsmOP(REG, reg),                                                         \
            newAsmOP(ADDR, 0, REG_SP)                                                   \
        )                                                                               \
    )

#define POP_REG_CODE(reg)                                                               \
    append(                                                                             \
        newAsmCode(                                                                     \
            ASM_LW,                                                                     \
            newAsmOp(REG, reg),                                                         \
            newAsmOp(ADDR, 0, REG_SP)                                                   \
        ),                                                                              \
        POP_STACK(1)                                                                    \
    )                                                                                   

#define GENERATE_ADDSUB_ASM(KIND, SIGN)                                                 \
    CHECK_AND_GET_BINOP(res, op1, op2);                                                 \
                                                                                        \
    prepareVarInStack(res->u.var_name, alist);                                          \
    prepareVarInStack(op1->u.var_name, alist);                                          \
                                                                                        \
    uint8_t reg_res, reg_op1;                                                           \
                                                                                        \
    reg_op1 = getReg(op1);                                                              \
    loadReg(reg_op1, op1->u.var_name, alist);                                           \
                                                                                        \
    if (op2->kind == OP_VAR) {                                                          \
        prepareVarInStack(op2->u.var_name, alist);                                      \
        uint8_t reg_op2 = getReg(op2);                                                  \
        loadReg(reg_op2, op2->u.var_name, alist);                                       \
                                                                                        \
        reg_res = getReg(res);                                                          \
        clearReg(reg_res, alist);                                                       \
                                                                                        \
        alist = append(                                                                 \
            alist,                                                                      \
            newAsmCode(                                                                 \
                KIND,                                                                   \
                newAsmOp(REG, reg_res),                                                 \
                newAsmOp(REG, reg_op1),                                                 \
                newAsmOp(REG, reg_op2)                                                  \
            )                                                                           \
        );                                                                              \
    } else {                                                                            \
        reg_res = getReg(res);                                                          \
        clearReg(reg_res, alist);                                                       \
                                                                                        \
        alist = append(                                                                 \
            alist,                                                                      \
            newAsmCode(                                                                 \
                ASM_ADDI,                                                               \
                newAsmOp(REG, reg_res),                                                 \
                newAsmOp(REG, reg_op1),                                                 \
                newAsmOp(IMM, SIGN (op2->u.val))                                        \
            )                                                                           \
        );                                                                              \
    }                                                                                   \
    clearVar(res->u.var_name);                                                          \
    addVar(res->u.var_name, reg_res);                                                   

#define GENERATE_MULDIV_ASM(INSTR)                                                      \
    CHECK_AND_GET_BINOP(res, op1, op2);                                                 \
                                                                                        \
    prepareVarInStack(res->u.var_name, alist);                                          \
    prepareVarInStack(op1->u.var_name, alist);                                          \
                                                                                        \
    uint8_t reg_res, reg_op1, reg_op2;                                                  \
    reg_op1 = getReg(op1);                                                              \
    loadReg(reg_op1, op1->u.var_name, alist);                                           \
                                                                                        \
                                                                                        \
    if (op2->kind == OP_VAR) {                                                          \
        prepareVarInStack(op2->u.var_name, alist);                                      \
        reg_op2 = getReg(op2);                                                          \
        loadReg(reg_op2, op2->u.var_name, alist);                                       \
    } else {                                                                            \
        reg_op2 = getReg(op2);                                                          \
        loadRegImm(reg_op2, op2->u.val, alist);                                         \
    }                                                                                   \
    reg_res = getReg(res);                                                              \
    clearReg(reg_res, alist);                                                           \
    alist = append(                                                                     \
        alist,                                                                          \
        INSTR                                                                           \
    );                                                                                  \
    clearVar(res->u.var_name);                                                          \
    addVar(res->u.var_name, reg_res);                                                     

#endif