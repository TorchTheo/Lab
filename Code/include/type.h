#ifndef __TYPE_H__
#define __TYPE_H__

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned char boolean;
#define SIZEOF(x) sizeof(struct x)
typedef enum {
    T_INT,
    T_FLOAT,
    T_ID,
    T_TYPE,
    T_RELOP,
    T_TERMINAL,
    T_NTERMINAL,
    T_NULL
} SymbolType;


typedef struct Node Node;

struct Node {
    char *name;
    SymbolType type;
    int line;
    union {
        int         type_int;
        float       type_float;
        char        type_str[40];
    } val;
    struct Node *sons;
    struct Node *next;
};


typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct Func_* Func;
typedef struct SymbolList_* SymbolList;

struct Type_ {
    enum {
        BASIC, ARRAY, STRUCTURE, FUNCTION, POINTER
    } kind;
    union {
        enum {TYPE_INT, TYPE_FLOAT} basic; // 基本类型
        struct {Type elem; int size;} array; // 数组
        FieldList structure; // 结构体
        Func function;
        Type pointer;
    } u;
    uint32_t size;
};

struct FieldList_ {
    char *name; // 作用域名
    Type type; // 类型
    uint32_t offset;
    FieldList next; // 下一个域
};

struct Func_ {
    char *name; // 函数名
    Type ret; // 返回类型
    FieldList parameters; // 参数列表
};

struct SymbolList_ {
    char *name; // 符号名
    Type type; // 符号类型
    enum {DEF, DEC} func_type; // 函数定义和声明
    SymbolList tb_next; // 符号表中的下一个
    SymbolList tb_prev; // 符号表的上一个
    SymbolList fs_next; // 同一层的下一个
    int line; // 位置
    int dep; // 第几层
    uint32_t key; // 在表中的key
};

// 中间代码表示
typedef struct Operand_* Operand;
struct Operand_ {
    enum {
        OP_VAR,
        OP_CONST,
        OP_ADDR,    // &var
        OP_DEREF,   // *var
        OP_FUNC,    // CALL var
    } kind;
    union {
        char *var_name;
        int val;
    } u;
};

struct InterCode {
    enum {
        IC_ADD,        // x := y + z
        IC_SUB,        // x := y - z
        IC_MUL,        // x := y * z
        IC_DIV,        // x := y / z
        IC_LABEL,      // LABEL x : 定义标号x。
        IC_FUNCTION,   // FUNCTION f : 定义函数f。
        IC_ASSIGN,     // x := y
        IC_GOTO,       // GOTO x 无条件跳转至标号x。
        IC_IFGOTO,     // IF x [relop] y GOTO z
        IC_RETURN,     // RETURN x
        IC_DEC,        // DEC x [size] 内存空间申请，大小为4的倍数。
        IC_ARG,        // ARG x 传实参x。
        IC_PARAM,      // PARAM x 函数参数声明。
        IC_READ,       // READ x 从控制台读取x的值。
        IC_WRITE,      // WRITE x 向控制台打印x的值。
    } kind;
    union {
        struct {Operand right, left;} assign;
        struct {Operand right, left, res;} binop;
        struct {Operand op;} uniop;
        struct {Operand op1, op2, target; char* relop;} cond_goto;
        struct {char *first_byte; uint32_t size;} declr;
    } u;
};

typedef struct ICList_* ICList;

struct ICList_ {
    struct InterCode code;
    ICList prev, next, tail;
};

// MIPS32

typedef struct AsmOp_* AsmOp;
typedef struct AsmList_* AsmList;

struct AsmOp_ {
    enum {
        IMM,
        REG,
        NAME,
    } op_kind;
    union {
        int val; // IMM and REG
        char *name; // NAME
    } u;
};

struct AsmCode_ {
    enum {
        ASM_LABEL,
        ASM_LI,
        ASM_LW,
        ASM_SW,
        ASM_MOVE,
        ASM_ADD,
        ASM_ADDI,
        ASM_SUB,
        ASM_MUL,
        ASM_DIV,
        ASM_MFLO,
        ASM_J,
        ASM_JAL,
        ASM_JR,
        ASM_BEQ,
        ASM_BNE,
        ASM_BGT,
        ASM_BLT,
        ASM_BGE,
        ASM_BLE,
    } asm_kind;
    union {
        struct { AsmOp op; } uniop;
        struct { AsmOp op1, op2; } binop;
        struct { AsmOp op1, op2, op3; } triop;
    } u;
};

struct AsmList_ {
    struct AsmCode_ asm_code;
    AsmList prev, next, tail;
};

#endif