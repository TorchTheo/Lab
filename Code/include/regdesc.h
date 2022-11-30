#ifndef __REGDESC_H__
#define __REGDESC_H__
#include "utils/linklist.h"
#include "constant.h"

#define CHECK_USE_FREE(reg_index)               \
    ((USE_FREE >> reg_index) & 0x1)
#define CHECK_SAVE_BEFORE_USE(reg_index)        \
    ((SAVE_BEFORE_USE >> reg_index) & 0x1)    
#define CLEAR_REG(vec, reg_index)               \
    vec = (vec & (~(0x1 << reg_index)))
#define SET_REG(vec, reg_index)                 \
    vec = (vec | (0x1 << reg_index))
#define CHECK_REG(vec, reg_index)               \
    ((vec >> reg_index) & 0x1)

typedef struct AddrDesc_* AddrDesc;
typedef enum {
    REG_ZERO        =  0,   // $zero
    REG_VALUE_0     =  2,   // $v0
    REG_VALUE_1         ,   // $v1
    REG_ARG_0           ,   // $a0
    REG_ARG_1           ,   // $a1
    REG_ARG_2           ,   // $a2
    REG_ARG_3           ,   // $a3
    REG_TEMP_0          ,   // $t0
    REG_TEMP_1          ,   // $t1
    REG_TEMP_2          ,   // $t2
    REG_TEMP_3          ,   // $t3
    REG_TEMP_4          ,   // $t4
    REG_TEMP_5          ,   // $t5
    REG_TEMP_6          ,   // $t6
    REG_TEMP_7          ,   // $t7
    REG_SVAL_0          ,   // $s0
    REG_SVAL_1          ,   // $s1
    REG_SVAL_2          ,   // $s2
    REG_SVAL_3          ,   // $s3
    REG_SVAL_4          ,   // $s4
    REG_SVAL_5          ,   // $s5
    REG_SVAL_6          ,   // $s6
    REG_SVAL_7          ,   // $s7
    REG_TEMP_8          ,   // $t8
    REG_TEMP_9          ,   // $t9
    REG_SP          = 29,   // $sp
    REG_FP              ,   // $fp
    REG_RET             ,   // $ra
} REG_CODE;

struct RegDesc {
    uint8_t content_is_imm;
    struct {
        strLinkList lst;
        int imm;
    } content;
} regs[NR_REGS];

struct AddrDesc_ {
    char *var_name;
    uint32_t reg_vec;
    intLinkList addr_list;
};

#endif