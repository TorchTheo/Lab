%{
    #include <stdio.h>
    #include "lex.yy.c"
%}

%token INT FLOAT
%token TYPE ID
%token SEMI COMMA ASSIGNOP RELOP
%token PLUS MINUS STAR DIV AND OR
%token DOT
%token NOT
%token LP RP LB RB LC RC
%token STRUCT RETURN IF ELSE WHILE

%%
Calc : /* empty */
    | Exp { printf("= %d\n", $1); }
    ;
Exp : Factor
    | Exp PLUS Factor { $$ = $1 + $3; }
    | Exp MINUS Factor { $$ = $1 - $3; }
    ;
Factor : Term
    | Factor STAR Term { $$ = $1 * $3; }
    | Factor DIV Term { $$ = $1 / $3; }
    ;
Term : INT
    ;
%%