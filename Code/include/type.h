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
    enum {BASIC, ARRAY, STRUCTURE, FUNCTION} kind;
    union {
        enum {TYPE_INT, TYPE_FLOAT} basic; // 基本类型
        struct {Type base; Type elem; int size;} array; // 数组
        FieldList structure; // 结构体
        Func func;
    } u;
};

struct FieldList_ {
    char *name; // 作用域名
    Type type; // 类型
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