%locations
%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>
    #include <string.h>
    #include "lex.yy.c"
    typedef unsigned char uint8_t;
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
    typedef struct Node{
        char *name;
        SymbolType type;
        int line;
        union {
            int         type_int;
            float       type_float;
            char        type_str[40];
        } val;
        Node *sons;
        Node *next;
    };

    Node *newNode(char*, SymbolType, ...);
    void print(Node*, int);

    int yylex();

    Node *root;
%}

%right <node> ASSIGNOP
%left  <node> OR
%left  <node> AND
%left  <node> RELOP
%left  <node> PLUS MINUS
%left  <node> STAR DIV
%right <node> NOT
%left  <node> LP RP LB RB DOT
%token <node> INT FLOAT ID 
%token <node> STRUCT RETURN IF ELSE WHILE
%token <node> TYPE
%token <node> SEMI COMMA
%token <node> LC RC

%type  <node> Program ExtDefList ExtDef ExtDecList
%type  <node> Specifier StructSpecifier OptTag Tag
%type  <node> VarDec FunDec VarList ParamDec
%type  <node> CompSt StmtList Stmt
%type  <node> DefList Def DecList Dec
%type  <node> Exp Args

%%
/* High-level Definitions */
Program             : ExtDefList { 
                        root = new_node("Program", T_NTERMINAL, @1.first_line, 1, $1);
                        // printf("Program -> ExtDefList\n"); 
                    }
ExtDefList          : ExtDef ExtDefList { 
                        $$ = new_node("ExtDefList", T_NTERMINAL, @1.first_line, 2, $1, $2);
                        // printf("ExtDefList -> ExtDef ExtDefList\n"); 
                    }
                    | /* Empty */ { 
                        $$ = new_node("ExtDefList", T_NULL);
                        // printf("ExtDefList -> Empty\n"); 
                    }
ExtDef              : Specifier ExtDecList SEMI { // some global var; e.g.int glo1, glo2;
                        $$ = new_node("ExtDef", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                        // printf("ExtDef -> Specifier ExtDecList SEMI\n"); 
                    }
                    | Specifier SEMI {  // e.g.struct{}; int;
                        $$ = new_node("ExtDef", T_NTERMINAL, @1.first_line, 2, $1, $2);
                        // printf("ExtDef -> Specifier SEMI\n"); 
                    }
                    | Specifier FunDec CompSt { // e.g. int func() {}
                        $$ = new_node("ExtDef", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                        // printf("ExtDef -> Specifier FunDec CompSt\n"); 
                    }
ExtDecList          : VarDec { 
                        $$ = new_node("ExtDecList", T_NTERMINAL, @1.first_line, 1, $1);
                        // printf("ExtDecList -> VarDec\n"); 
                    }
                    | VarDec COMMA ExtDecList { 
                        $$ = new_node("ExtDecList", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                        // printf("ExtDecList -> VarDec COMMA ExtDecList\n"); 
                    }
/* Specifiers */
Specifier           : TYPE { 
                        $$ = new_node("Specifier", T_NTERMINAL, @1.first_line, 1, $1);
                        // printf("Specifier -> TYPE\n"); 
                    }
                    | StructSpecifier { 
                        $$ = new_node("Specifier", T_NTERMINAL, @1.first_line, 1, $1);
                        // printf("Specifier -> StructSpecifier\n"); 
                    }
StructSpecifier     : STRUCT OptTag LC DefList RC { 
                        $$ = new_node("StructSpecifier", T_NTERMINAL, @1.first_line, 5, $1, $2, $3, $4, $5);
                        // printf("StructSpecifier -> STRUCT OptTag LC DefList RC\n"); 
                    }
                    | STRUCT Tag {
                        $$ = new_node("StructSpecifier", T_NTERMINAL, @1.first_line, 2, $1, $2);
                        // printf("StructSpecifier -> STRUCT Tag\n"); 
                    }
OptTag              : ID {
                        $$ = new_node("OptTag", T_NTERMINAL, @1.first_line, 1, $1);
                    }
                    | /* empty */ {
                        $$ = new_node("OptTag", T_NULL);
                        // printf("OptTag -> Empty\n");
                    }
Tag                 : ID {
                        $$ = new_node("Tag", T_NTERMINAL, @1.first_line, 1, $1);
                        // printf("Tag -> ID\n");
                    }
/* Declarators */
VarDec              : ID {
                        $$ = new_node("VarDec", T_NTERMINAL, @1.first_line, 1, $1);
                        // printf("VarDec -> ID\n");
                    }
                    | VarDec LB INT RB {
                        $$ = new_node("VarDec", T_NTERMINAL, @1.first_line, 4, $1, $2, $3, $4);
                        // printf("VarDec -> VarDec? LB INT RB\n");
                    }
FunDec              : ID LP VarList RP {
                        $$ = new_node("FunDec", T_NTERMINAL, @1.first_line, 4, $1, $2, $3, $4);
                        // printf("FunDec -> ID LP VarList RP\n");
                    }
                    | ID LP RP {
                        $$ = new_node("FunDec", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                        // printf("FunDec -> ID LP RP\n");
                    }
VarList             : ParamDec COMMA VarList {
                        $$ = new_node("VarList", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                        // printf("VarList -> ParamDec COMMA VarList\n");
                    }
                    | ParamDec {
                        $$ = new_node("VarList", T_NTERMINAL, @1.first_line, 1, $1);
                        // printf("VarList -> ParamDec\n");
                    }
ParamDec            : Specifier VarDec {
                        $$ = new_node("ParamDec", T_NTERMINAL, @1.first_line, 2, $1, $2);
                        // printf("ParamDec -> Specifier VarDec\n");
                    }
/* Statements */
CompSt              : LC DefList StmtList RC {
                        $$ = new_node("CompSt", T_NTERMINAL, @1.first_line, 4, $1, $2, $3, $4);
                        // printf("CompSt -> LC DefList StmtList RC\n");
                    }
StmtList            : Stmt StmtList {
                        $$ = new_node("StmtList", T_NTERMINAL, @1.first_line, 2, $1, $2);
                        // printf("StmtList -> Stmt StmtList\n");
                    }
                    | /* empty */ {
                        $$ = new_node("StmtList", T_NULL);
                        // printf("StmtList -> Empty\n");
                    }
Stmt                : Exp SEMI {
                        $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 2, $1, $2);
                    }
                    | CompSt {
                        $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 1, $1);
                    }
                    | RETURN Exp SEMI {
                        $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | IF LP Exp RP Stmt {
                        $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 5, $1, $2, $3, $4, $5);
                    }
                    | IF LP Exp RP Stmt ELSE Stmt {
                        $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 7, $1, $2, $3, $4, $5, $6, $7);
                    }
                    | WHILE LP Exp RP Stmt {
                        $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 5, $1, $2, $3, $4, $5);
                    }
/* Local Definitions */
DefList             : Def DefList {
                        $$ = new_node("DefList", T_NTERMINAL, @1.first_line, 2, $1, $2);
                    }
                    | /* empty */ {
                        $$ = new_node("DefList", T_NULL);
                    }
Def                 : Specifier DecList SEMI {
                        $$ = new_node("DefList", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
DecList             : Dec {
                        $$ = new_node("DecList", T_NTERMINAL, @1.first_line, 1, $1);
                    }
                    | Dec COMMA DecList {
                        $$ = new_node("DecList", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
Dec                 : VarDec {
                        $$ = new_node("Dec", T_NTERMINAL, @1.first_line, 1, $1);
                    }
                    | VarDec ASSIGNOP Exp {
                        $$ = new_node("Dec", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
/* Expressions */
Exp                 : Exp ASSIGNOP Exp {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | Exp AND Exp {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | Exp OR Exp {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | Exp RELOP Exp {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | Exp PLUS Exp {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | Exp MINUS Exp {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | Exp STAR Exp {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | Exp DIV Exp {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | LP Exp RP {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | MINUS Exp {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 2, $1, $2);
                    }
                    | NOT Exp {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 2, $1, $2);
                    }
                    | ID LP Args RP {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 4, $1, $2, $3, $4);
                    }
                    | ID LP RP {}
                    | Exp LB Exp RB {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 4, $1, $2, $3, $4);
                    }
                    | Exp DOT ID {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | ID {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 1, $1);
                    }
                    | INT {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 1, $1);
                    }
                    | FLOAT {
                        $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 1, $1);
                    }
Args                : Exp COMMA Args {
                        $$ = new_node("Args", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    }
                    | Exp {
                        $$ = new_node("Args", T_NTERMINAL, @1.first_line, 1, $1);
                    }
%%

Node *new_node(char *_name, SymbolType _type, ...) {
    va_list list;
    Node *node = calloc(1, sizeof(Node));
    node->name = _name;
    node->type = _type;
    node->sons = node->next = NULL;

    va_start(list, _type);
    switch(_type) {
        case T_INT: {
            if ('O' == node->name[0]) // OCT
                sscanf(va_arg(list, char*), "%od", &(node->val.type_int));
            else if ('H' == node->name[0]) // HEX
                sscanf(va_arg(list, char*), "%xd", &(node->val.type_int));
            else // DEC
                sscanf(va_arg(list, char*), "%d", &(node->val.type_int));
            break;
        }

        case T_FLOAT: {
            sscanf(va_arg(list, char*), "%f", &(node->val.type_float));
            break;
        }

        case T_ID: case T_TYPE: case T_RELOP: case T_TERMINAL: {
            strcpy(node->val.type_str, va_arg(list, char*));
            break;
        }

        case T_NTERMINAL: {
            node->line = va_arg(list, int);
            int son_num = va_arg(list, int);
            node->sons = (Node*)va_arg(list, Node*);
            Node *p_son = node->sons;
            for (int i = 1; i < son_num; i++) {
                p_son->next = (Node*)va_arg(list, Node*);
                p_son = p_son->next;
            }
            break;
        }

        case T_NULL: {
            break;
        }
    }
    va_end(list);
    return node;
}