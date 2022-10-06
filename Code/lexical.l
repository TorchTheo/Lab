%option yylineno
%{
    #include "syntax.tab.h"
%}

digit [0-9]
letter [_a-zA-Z]
u_int {digit}+
float ({digit}*\.?{digit}+|{digit}+\.)([eE][+-]?{u_int})?
id {letter}({letter}|{digit})*
%%
"+" { printf("\'%s\' is PLUS", yytext); return PLUS; }
"-" { return MINUS; }
"int"|"float" { return TYPE; }
";" { return SEMI; }
"," { return COMMA; }
"=" { return ASSIGNOP; }
">"|"<"|">="|"<="|"=="|"!=" { return RELOP; }
"*" { return STAR; }
"/" { return DIV; }
"&&" { return AND; }
"||" { return OR; }
"." { return DOT; }
"!" { return NOT; }
"(" { return LP; }
")" { return RP; }
"[" { return LB; }
"]" { return RB; }
"{" { return LC; }
"}" { return RC; }
"struct" { return STRUCT; }
"return" { return RETURN; }
"if" { return IF; }
"else" { return ELSE; }
"while" { return WHILE; }
{u_int} {
    printf("\'%s\' is INT", yytext);
    yylval = atoi(yytext); 
    return INT; 
}
{float} { return FLOAT; }
{id} { return ID; }
. { printf("\nError type A at Line %d: Unknown character \'%s\'", yylineno, yytext); }

%%
