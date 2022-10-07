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

typedef struct Node Node;