#ifndef __CONSTANT_H__
#define __CONSTANT_H__

#define HASH_TABLE_SIZE     0x4000
#define FRAME_STACK_SIZE    0x1000

#define VARDEC              0
#define FUNCDEC             1
#define STRUCTDEF           2
#define FUNCDEF             3
#define PARAMDEC            4
#define FIELDDEC            5

#define TRUE                1
#define FALSE               0

#define FIELD_TYPE_PARAM    0
#define FIELD_TYPE_STRUCT   1

#define LEFT_VALUE          0
#define RIGHT_VALUE         1

#define FILE_OUT            1
#define STD_OUT             0

#define STREAM_STRING       0
#define STREAM_CHAR         1
#define STREAM_INT          2
#define STREAM_UINT32       3
#define NO_PARAM            4

#ifdef __SEMATICS_C__

char *formats[] = {
    /* 00 */ "Error type %d at Line %d: Unknown error.\n",
    /* 01 */ "Error type %d at Line %d: Undefined variable \"%s\".\n",
    /* 02 */ "Error type %d at Line %d: Undefined function \"%s\".\n",
    /* 03 */ "Error type %d at Line %d: Redefined variable \"%s\".\n",
    /* 04 */ "Error type %d at Line %d: Redefined function \"%s\".\n",
    /* 05 */ "Error type %d at Line %d: Type mismatched for assignment.\n",
    /* 06 */ "Error type %d at Line %d: The left-hand side of an assignment must be a variable.\n",
    /* 07 */ "Error type %d at Line %d: Type mismatched for operands.\n",
    /* 08 */ "Error type %d at Line %d: Type mismatched for return.\n",
    /* 09 */ "Error type %d at Line %d: Function \"%s\" is not applicable for these arguments.\n",
    /* 10 */ "Error type %d at Line %d: \"%s\" is not an array.\n",
    /* 11 */ "Error type %d at Line %d: \"%s\" is not a function.\n",
    /* 12 */ "Error type %d at Line %d: Index is not an integer.\n",
    /* 13 */ "Error type %d at Line %d: Illegal use of \".\".\n",
    /* 14 */ "Error type %d at Line %d: Non-existent field \"%s\".\n",
    /* 15 */ "Error type %d at Line %d: Redefined field \"%s\" or illegal initialization.\n",
    /* 16 */ "Error type %d at Line %d: Duplicated name \"%s\".\n",
    /* 17 */ "Error type %d at Line %d: Undefined structure \"%s\".\n",
    /* 18 */ "Error type %d at Line %d: Undefined function \"%s\".\n", // With declaration but not definition
    /* 19 */ "Error type %d at Line %d: Inconsistent declaration of function \"%s\".\n",
};

#endif

#ifdef __IR_C__

static const char *icode_name[] = {
    "", "", "", "", 
    "LABEL",
    "FUNCTION",
    "", 
    "GOTO",
    "", 
    "RETURN",
    "", 
    "ARG",
    "PARAM",
    "READ",
    "WRITE",
};

#endif

#ifdef __ASSEMBLE_C__

static const char *relop_str[] = { "==", "!=", ">", "<", ">=", "<=" };
static const char *instrs[] = {
    "",
    "li", "lw", "sw", "move",
    "add", "addi", "sub", "mul", "div", "mflo",
    "j", "jal", "jr",
    "beq", "bne", "bgt", "blt", "bge", "ble",
};

#endif

#endif