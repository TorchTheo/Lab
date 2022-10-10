%option yylineno
%{
    #include "syntax.tab.h"
    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;
    int error = 0;
%}

digit [0-9]
letter [_a-zA-Z]
u_int 0|([1-9]{digit}*)
float ({digit}*\.?{digit}+|{digit}+\.)([eE][+-]?{u_int})?
id {letter}({letter}|{digit})*
%%
[ \t] {}
[\n\r] {
            yycolumn = 1;
            // printf("%d ", yylineno);
        }
"+" {
    // printf("+ ");
    yylval.node = new_node("PLUS", T_TERMINAL, yytext);
    return PLUS;
}
"-" {
    // printf("- ");
    yylval.node = new_node("MINUS", T_TERMINAL, yytext);
    return MINUS;
}
"int"|"float" {
    // printf("TYPE ");
    yylval.node = new_node("TYPE", T_TYPE, yytext);
    return TYPE;
}
";" {
    // printf("SEMI ");
    yylval.node = new_node("SEMI", T_TERMINAL, yytext);
    return SEMI;
}
"," {
    // printf("COMMA ");
    yylval.node = new_node("COMMA", T_TERMINAL, yytext);
    return COMMA;
}
"=" {
    // printf("ASSIGNOP ");
    yylval.node = new_node("ASSIGNOP", T_TERMINAL, yytext);
    return ASSIGNOP;
}
">"|"<"|">="|"<="|"=="|"!=" {
    // printf("RELOP ");
    yylval.node = new_node("RELOP", T_TERMINAL, yytext);
    return RELOP;
}
"*" {
    // printf("STAR ");
    yylval.node = new_node("STAR", T_TERMINAL, yytext);
    return STAR;
}
"/" {
    // printf("DIV ");
    yylval.node = new_node("DIV", T_TERMINAL, yytext);
    return DIV;
}
"&&" {
    // printf("AND ");
    yylval.node = new_node("AND", T_TERMINAL, yytext);
    return AND;
}
"||" {
    // printf("OR ");
    yylval.node = new_node("OR", T_TERMINAL, yytext);
    return OR;
}
"." {
    // printf("DOT ");
    yylval.node = new_node("DOT", T_TERMINAL, yytext);
    return DOT;
}
"!" {
    // printf("NOT ");
    yylval.node = new_node("NOT", T_TERMINAL, yytext);
    return NOT;
}
"(" {
    // printf("LP ");
    yylval.node = new_node("LP", T_TERMINAL, yytext);
    return LP;
}
")" {
    // printf("RP ");
    yylval.node = new_node("RP", T_TERMINAL, yytext);
    return RP;
}
"[" {
    // printf("LB ");
    yylval.node = new_node("LP", T_TERMINAL, yytext);
    return LB;
}
"]" {
    // printf("RB ");
    yylval.node = new_node("RB", T_TERMINAL, yytext);
    return RB;
}
"{" {
    // printf("LC ");
    yylval.node = new_node("LC", T_TERMINAL, yytext);
    return LC;
}
"}" {
    // printf("RC ");
    yylval.node = new_node("RC", T_TERMINAL, yytext);
    return RC;
}
"struct" {
    // printf("STRUCT ");
    yylval.node = new_node("STRUCT", T_TERMINAL, yytext);
    return STRUCT;
}
"return" {
    // printf("RETURN ");
    yylval.node = new_node("RETURN", T_TERMINAL, yytext);
    return RETURN;
}
"if" {
    // printf("IF ");
    yylval.node = new_node("IF", T_TERMINAL, yytext);
    return IF;
}
"else" {
    // printf("ELSE ");
    yylval.node = new_node("ELSE", T_TERMINAL, yytext);
    return ELSE;
}
"while" {
    // printf("WHILE ");
    yylval.node = new_node("WHILE", T_TERMINAL, yytext);
    return WHILE;
}
{u_int} { // Decimal
    // printf("INT ");
    yylval.node = new_node("DEC", T_INT, yytext);
    return INT;
}
0(0|([1-7][0-7]*)) { // Octal
    yylval.node = new_node("OCT", T_INT, yytext);
    return INT;
}
0{u_int} {
    printf("Error type A at Line %d: Illegal OCT number \"%s\".\n", yylineno, yytext);
    error = 1;
    yylval.node = new_node("OCT", T_INT, yytext);
    return INT;
}
0[xX](0|([1-9a-fA-F][0-9a-fA-F]*)) { // Hexadecimal
    yylval.node = new_node("HEX", T_INT, yytext);
    return INT;
}
0[xX](0[0-9A-Za-z]*|([1-9A-Za-z]({digit}|[A-Za-z])*)) {
    printf("Error type A at Line %d: Illegal HEX number \"%s\".\n", yylineno, yytext);
    error = 1;
    yylval.node = new_node("HEX", T_INT, yytext);
    return INT;
}
{float} {
    // printf("FLOAT ");
    yylval.node = new_node("FLOAT", T_FLOAT, yytext);
    return FLOAT;
}
({digit}*\.?{digit}+|{digit}+\.)[eE] {
    printf("Error type A at line %d: Illegal FLOAT number \"%s\".\n", yylineno, yytext);
    error = 1;
    yylval.node = new_node("FLOAT", T_FLOAT, yytext);
    return FLOAT;
}
{id} {
    // printf("ID ");
    yylval.node = new_node("ID", T_ID, yytext);
    return ID;
}
"//".* {
    /* line comment */
}
"/*"(\n|.)*"/*"(\n|.)*"*/"(\n|.)*"*/" {
    printf("Error type B at Line %d: Illegal nested comment.\n", yylineno);
    error = 1;
}
"/*"(\n|.)*"*/"(\n|.)*"*/" {
    printf("Error type B at Line %d: Illegal comment.\n", yylineno);
    error = 1;
}
"/*"(\n|.)*"*/" {
    /* block comment */
}
. {
    printf("Error type A at Line %d: Unknown character \"%s\"\n", yylineno, yytext);
    error = 1;
}
