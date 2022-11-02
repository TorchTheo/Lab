%locations
%define parse.error verbose
%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>
    #include <string.h>
    #include "include/type.h"
    extern int error;
    

    Node *new_node(char*, SymbolType, ...);
    void print(Node*, int);

    int yylex();
    int yyerror(const char *msg);

    Node *root;
%}

%union {
    Node *node;
}

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
%type  <node> Exp Term Args

%%
/* High-level Definitions */
Program         : ExtDefList {
                    // root = new_node("Program", T_NTERMINAL, @1.first_line, 1, $1);
                    root = $1;
                }
                ;
ExtDefList      : ExtDef ExtDefList {
                    // $$ = new_node("ExtDefList", T_NTERMINAL, @1.first_line, 2, $1, $2);
                    $$ = $1; $1->next = $2;
                }
                | /* Empty */ {
                    // $$ = new_node("ExtDefList", T_NULL);
                    $$ = NULL;
                }
                ;
ExtDef          : Specifier ExtDecList SEMI {
                    // Global variable
                    // $$ = new_node("ExtDef", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = new_node("ExtDef", T_NTERMINAL, @1.first_line, 2, $1, $2);
                }
                | Specifier SEMI {
                    // Structure
                    // $$ = new_node("ExtDef", T_NTERMINAL, @1.first_line, 2, $1, $2);
                    $$ = new_node("ExtDef", T_NTERMINAL, @1.first_line, 1, $1);
                }
                | Specifier FunDec SEMI {
                    // Function declaration
                    $$ = new_node("ExtDef", T_NTERMINAL, @1.first_line, 2, $1, $2);
                }
                | Specifier FunDec CompSt {
                    // Function definition
                    $$ = new_node("ExtDef", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                }
                | Specifier ExtDecList error {
                    yyerror("Probably missing ';' at this line or last line");
                    yyerrok;
                }
                | Specifier error {
                    yyerror("Invalid definition");
                    yyerrok;
                }
                ;
ExtDecList      : VarDec {
                    // $$ = new_node("ExtDecList", T_NTERMINAL, @1.first_line, 1, $1);
                    $$ = $1;
                }
                | VarDec COMMA ExtDecList {
                    // $$ = new_node("ExtDecList", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $1; $1->next = $3;
                }
                | VarDec error ExtDecList {
                    yyerror("Missing ',' between variables");
                    yyerrok;
                }
                | VarDec error {
                    yyerror("Probably missing ';' at this line or last line");
                    yyerrok;
                }
                ;
/* Specifier */
Specifier       : TYPE {
                    // $$ = new_node("Specifier", T_NTERMINAL, @1.first_line, 1, $1);
                    $$ = $1;
                }
                | StructSpecifier {
                    // $$ = new_node("Specifier", T_NTERMINAL, @1.first_line, 1, $1);
                    $$ = $1;
                }
                ;
StructSpecifier : STRUCT OptTag LC DefList RC {
                    // $$ = new_node("StructSpecifier", T_NTERMINAL, @1.first_line, 5, $1, $2, $3, $4, $5);
                    $$ = new_node("StructSpecifier", T_NTERMINAL, @1.first_line, 2, new_node("DefList", T_NTERMINAL, @4.first_line, 1, $4), $2);
                }
                | STRUCT Tag {
                    // $$ = new_node("StructSpecifier", T_NTERMINAL, @1.first_line, 2, $1, $2);
                    $$ = new_node("StructSpecifier", T_NTERMINAL, @1.first_line, 1, $2);
                }
                | STRUCT OptTag LC error RC {
                    yyerror("Invalid struct definition");
                    yyerrok;
                }
                ;
OptTag          : ID {
                    // $$ = new_node("OptTag", T_NTERMINAL, @1.first_line, 1, $1);
                    $$ = $1;
                }
                | /* Empty */ {
                    // $$ = new_node("OptTag", T_NULL);
                    $$ = NULL;
                }
                ;
Tag             : ID {
                    // $$ = new_node("Tag", T_NTERMINAL, @1.first_line, 1, $1);
                    $$ = $1;
                }
                ;
/* Declarations */
VarDec          : ID {
                    $$ = new_node("VarDec", T_NTERMINAL, @1.first_line, 1, $1);
                }
                | VarDec LB INT RB {
                    // $$ = new_node("VarDec", T_NTERMINAL, @1.first_line, 4, $1, $2, $3, $4);
                    $$ = new_node("VarDec", T_NTERMINAL, @1.first_line, 2, $1, $3);
                }
                | VarDec LB error RB {
                    yyerror("Invalid size of array");
                    yyerrok;
                }
                | VarDec LB error {
                    yyerror("Missing ']' after a '['");
                    yyerrok;
                }
                ;
FunDec          : ID LP VarList RP {
                    // $$ = new_node("FunDec", T_NTERMINAL, @1.first_line, 4, $1, $2, $3, $4);
                    // $$ = new_node("FunDec", T_NTERMINAL, @1.first_line, 2, $1, $3);
                    $$ = new_node("FunDec", T_NTERMINAL, @1.first_line, 2, $1, new_node("ParamDecList", T_NTERMINAL, @3.first_line, 1, $3));
                }
                | ID LP RP {
                    // $$ = new_node("FunDec", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = new_node("FunDec", T_NTERMINAL, @1.first_line, 1, $1);
                }
                | ID LP error RP {
                    yyerror("Invalid parameters of function");
                    yyerrok;
                }
                | ID LP error {
                    yyerror("Missing ')' after parameters");
                    yyerrok;
                }
                ;
VarList         : ParamDec COMMA VarList {
                    // $$ = new_node("VarList", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $1; $1->next = $3;
                }
                | ParamDec {
                    // $$ = new_node("VarList", T_NTERMINAL, @1.first_line, 1, $1);
                    $$ = $1;
                }
                | ParamDec error VarList {
                    yyerror("Missing ',' between parameters");
                    yyerrok;
                }
                | ParamDec error {
                    yyerror("Invalid parameters");
                    yyerrok;
                }
                ;
ParamDec        : Specifier VarDec {
                    $$ = new_node("ParamDec", T_NTERMINAL, @1.first_line, 2, $1, $2);
                }
                ;
/* Statements */
CompSt          : LC DefList StmtList RC {
                    // $$ = new_node("CompSt", T_NTERMINAL, @1.first_line, 4, $1, $2, $3, $4);
                    $$ = new_node("CompSt", T_NTERMINAL, @1.first_line, 2,
                                new_node("DefList", T_NTERMINAL, @1.first_line, 1, $2),
                                new_node("StmtList", T_NTERMINAL, @1.first_line, 1, $3));
                }
                | error RC {
                    yyerror("Invalid statements");
                    yyerrok;
                }
                ;
StmtList        : Stmt StmtList {
                    // $$ = new_node("StmtList", T_NTERMINAL, @1.first_line, 2, $1, $2);
                    $$ = $1; $1->next = $2;
                }
                | /* Empty */ {
                    // $$ = new_node("StmtList", T_NULL);
                    $$ = NULL;
                }
                ;
Stmt            : Exp SEMI {
                    // $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 2, $1, $2);
                    $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 1, $1);
                }
                | CompSt {
                    $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 1, $1);
                }
                | RETURN Exp SEMI {
                    // $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 1, $1);
                    $1->sons = $2;
                }
                | IF LP Exp RP Stmt {
                    // $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 5, $1, $2, $3, $4, $5);
                    $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 1, $1);
                    $1->sons = new_node("Cond", T_NTERMINAL, @3.first_line, 1, $3);
                    $1->sons->next = $5;
                }
                | IF LP Exp RP Stmt ELSE Stmt {
                    // $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 7, $1, $2, $3, $4, $5, $6, $7);
                    $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 2, $1, $6);
                    $1->sons = new_node("Cond", T_NTERMINAL, @3.first_line, 1, $3);
                    $1->sons->next = $5;
                    $6->sons = $7;
                }
                | WHILE LP Exp RP Stmt {
                    // $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 5, $1, $2, $3, $4, $5);
                    $$ = new_node("Stmt", T_NTERMINAL, @1.first_line, 1, $1);
                    $1->sons = new_node("Cond", T_NTERMINAL, @3.first_line, 1, $3);
                    $1->sons->next = $5;
                }
                | Exp error {
                    yyerror("Probably missing ';' at this line or last line");
                    yyerrok;
                }
                | error SEMI {
                    yyerror("Invalid expression");
                    yyerrok;
                }
                | RETURN error SEMI {
                    yyerror("Invalid return value");
                    yyerrok;
                }
                | RETURN Exp error {
                    yyerror("Probably missing ';' at this line or last line");
                    yyerrok;
                }
                | IF LP error RP Stmt {
                    yyerror("Illegal condition");
                    yyerrok;
                }
                | IF LP error RP Stmt ELSE Stmt {
                    yyerror("Illegal condition");
                    yyerrok;
                }
                | WHILE LP error RP Stmt {
                    yyerror("Illegal condition");
                    yyerrok;
                }
                | IF LP Exp error Stmt {
                    yyerror("Missing ')' after a '('");
                    yyerrok;
                }
                | IF LP Exp error Stmt ELSE Stmt {
                    yyerror("Missing ')' after a '('");
                    yyerrok;
                }
                | WHILE LP Exp error Stmt {
                    yyerror("Missing ')' after a '('");
                    yyerrok;
                }
                ;
/* Local Definitions */
DefList         : Def DefList {
                    // $$ = new_node("DefList", T_NTERMINAL, @1.first_line, 2, $1, $2);
                    $$ = $1; $1->next = $2;
                }
                | /* Empty */ {
                    // $$ = new_node("DefList", T_NULL);
                    $$ = NULL;
                }
                ;
Def             : Specifier DecList SEMI {
                    // $$ = new_node("Def", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = new_node("Def", T_NTERMINAL, @1.first_line, 2, $1, $2);
                }
                | Specifier DecList error {
                    yyerror("Probably missing ';' at this line or last line");
                    yyerrok;
                }
                | Specifier error SEMI {
                    yyerror("Invalid definition");
                    yyerrok;
                }
                ;
DecList         : Dec {
                    // $$ = new_node("DecList", T_NTERMINAL, @1.first_line, 1, $1);
                    $$ = $1;
                }
                | Dec COMMA DecList {
                    // $$ = new_node("DecList", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $1;
                    if($1->next != NULL) 
                        $1->next->next = $3;
                    else
                        $1->next = $3;
                }
                | Dec error DecList {
                    yyerror("Missing ',' between variables");
                    yyerrok;
                }
                | Dec error {
                    yyerror("Probably missing ';' at this line or last line");
                    yyerrok;
                }
                ;
Dec             : VarDec {
                    // $$ = new_node("Dec", T_NTERMINAL, @1.first_line, 1, $1);
                    $$ = $1;
                }
                | VarDec ASSIGNOP Exp {
                    // $$ = new_node("Dec", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $1; $1->next = $3;
                }
                | VarDec ASSIGNOP error {
                    yyerror("Invalid value");
                    yyerrok;
                }
                ;
/* Expressions */
Exp             : Exp ASSIGNOP Exp {
                    // $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $2;
                    $2->sons = $1;
                    $1->next = $3;
                }
                | Exp AND Exp {
                    // $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $2;
                    $2->sons = $1;
                    $1->next = $3;
                }
                | Exp OR Exp {
                    // $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $2;
                    $2->sons = $1;
                    $1->next = $3;
                }
                | Exp RELOP Exp {
                    // $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $2;
                    $2->sons = $1;
                    $1->next = $3;
                }
                | Exp PLUS Exp {
                    // $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $2;
                    $2->sons = $1;
                    $1->next = $3;
                }
                | Exp MINUS Exp {
                    // $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $2;
                    $2->sons = $1;
                    $1->next = $3;
                }
                | Exp STAR Exp {
                    // $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $2;
                    $2->sons = $1;
                    $1->next = $3;
                }
                | Exp DIV Exp {
                    // $$ = new_node("Exp", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $2;
                    $2->sons = $1;
                    $1->next = $3;
                }
                | Term {
                    $$ = $1;
                }
                | Exp ASSIGNOP error {
                    yyerror("Invalid expression");
                    yyerrok;
                }
                | Exp AND error {
                    yyerror("Invalid expression");
                    yyerrok;
                }
                | Exp OR error {
                    yyerror("Invalid expression");
                    yyerrok;
                }
                | Exp RELOP error {
                    yyerror("Invalid expression");
                    yyerrok;
                }
                | Exp PLUS error {
                    yyerror("Invalid expression");
                    yyerrok;
                }
                | Exp MINUS error {
                    yyerror("Invalid expression");
                    yyerrok;
                }
                | Exp STAR error {
                    yyerror("Invalid expression");
                    yyerrok;
                }
                | Exp DIV error {
                    yyerror("Invalid expression");
                    yyerrok;
                }
                | LP error {
                    yyerror("Missing ')' after a '('");
                    yyerrok;
                }
                | error RP {
                    yyerror("Invalid expression");
                    yyerrok;
                }
                | ID LP error RP {
                    yyerror("Invalid arguments of function call");
                    yyerrok;
                }
                | ID LP error SEMI {
                    yyerror("Missing ')' after a '('");
                    yyerrok;
                }
                | Exp LB error RB {
                    yyerror("Invalid index of element");
                    yyerrok;
                }
                | Exp LB error SEMI {
                    yyerror("Missing ']' after a '['");
                    yyerrok;
                }
                ;
Term            : LP Exp RP {
                    $$ = $2;
                }
                | ID LP Args RP {
                    $$ = new_node("FuncCall", T_NTERMINAL, @1.first_line, 2, $1, $3);
                }
                | ID LP RP {
                    $$ = new_node("FuncCall", T_NTERMINAL, @1.first_line, 1, $1);
                }
                | Term LB Exp RB {
                    $$ = new_node("ArrayEval", T_NTERMINAL, @1.first_line, 2, $1, $3);
                }
                | Term DOT ID {
                    $$ = $2, $2->sons = (struct Node *)$1, $1->next = (struct Node *)$3;
                }
                | MINUS Term {
                    $$ = $1, $1->sons = (struct Node *)$2;
                }
                | NOT Term {
                    $$ = $1, $1->sons = (struct Node *)$2;
                }
                | ID {
                    $$ = $1;
                }
                | INT  {
                    $$ = $1;
                }
                | FLOAT  {
                    $$ = $1;
                }
Args            : Exp COMMA Args {
                    // $$ = new_node("Args", T_NTERMINAL, @1.first_line, 3, $1, $2, $3);
                    $$ = $1;
                    $1->next = $3;
                }
                | Exp {
                    // $$ = new_node("Args", T_NTERMINAL, @1.first_line, 1, $1);
                    $$ = $1;
                }
                | Exp error Args {
                    yyerror("Missing ',' between arguments");
                    yyerrok;
                }
                ;
%%

#include "lex.yy.c"
Node *new_node(char *_name, SymbolType _type, ...) {
    va_list list;
    Node *node = calloc(1, sizeof(Node));
    node->name = _name;
    node->type = _type;
    node->sons = node->next = NULL;

    va_start(list, _type);
    switch(_type) {
        case T_INT: {
            node->line = yylineno;
            if ('O' == node->name[0]) // OCT
                sscanf(va_arg(list, char*), "%od", &(node->val.type_int));
            else if ('H' == node->name[0]) // HEX
                sscanf(va_arg(list, char*), "%xd", &(node->val.type_int));
            else // DEC
                sscanf(va_arg(list, char*), "%d", &(node->val.type_int));
            break;
        }

        case T_FLOAT: {
            node->line = yylineno;
            sscanf(va_arg(list, char*), "%f", &(node->val.type_float));
            break;
        }

        case T_ID: case T_TYPE: case T_RELOP: case T_TERMINAL: {
            node->line = yylineno;
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

void print_AST(Node *node, int depth) {
    if (node->type != T_NULL) {
        printf("%d:\t", node->line);
        for(int i = 0; i < depth; i++)
            printf("  ");
    }
    switch (node->type) {
        case T_INT: {
            printf("INT: %d\n", node->val.type_int);
            break;
        }
        case T_FLOAT: {
            printf("FLOAT: %f\n", node->val.type_float);
            break;
        }
        case T_ID: case T_TYPE: {
            printf("%s: %s\n", node->name, node->val.type_str);
            break;
        }
        case T_RELOP: case T_TERMINAL: {
            printf("%s\n", node->name);
            break;
        }
        case T_NTERMINAL: {
            printf("%s (%d)\n", node->name, node->line);
            break;
        }
        case T_NULL: {
            break;
        }
        default: {
            printf("Error type: %s\n", node->name);
            break;
        }
    }
    if(node->sons != NULL) {
        print_AST(node->sons, depth + 1);
    }
    if(node->next != NULL) {
        print_AST(node->next, depth);
    }
}

int yyerror(const char *msg) {
    error = 1;
    if (msg[0] == 's' && msg[1] == 'y') {
        printf("Error type B at Line %d: %s.", yylineno, msg);
    }
    else
        printf(" %s.\n", msg);
    return 0;
}