%option yylineno
%{
    #include "syntax.tab.h"
    int yycolumn = 1;
    #define YY_USER_ACTION \ 
        yylloc.first_line = yylloc.last_line = yylineno; \ 
        yylloc.first_column = yycolumn; \ 
        yylloc.last_column = yycolumn + yyleng - 1; \ 
        yycolumn += yyleng;
%}

digit [0-9]
letter [_a-zA-Z]
u_int {digit}+
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
    return PLUS; 
}
"-" { 
    // printf("- ");
    return MINUS; 
}
"int"|"float" { 
    // printf("TYPE ");
    return TYPE; 
}
";" { 
    // printf("SEMI ");
    return SEMI; 
}
"," { 
    // printf("COMMA ");
    return COMMA; 
}
"=" { 
    // printf("ASSIGNOP ");
    return ASSIGNOP; 
}
">"|"<"|">="|"<="|"=="|"!=" { 
    // printf("RELOP ");
    return RELOP; 
}
"*" { 
    // printf("STAR ");
    return STAR; 
}
"/" { 
    // printf("DIV ");
    return DIV; 
}
"&&" { 
    // printf("AND ");
    return AND; 
}
"||" { 
    // printf("OR ");
    return OR; 
}
"." { 
    // printf("DOT ");
    return DOT; 
}
"!" { 
    // printf("NOT ");
    return NOT; 
}
"(" { 
    // printf("LP ");
    return LP; 
}
")" { 
    // printf("RP ");
    return RP; 
}
"[" { 
    // printf("LB ");
    return LB; 
}
"]" { 
    // printf("RB ");
    return RB; 
}
"{" { 
    // printf("LC ");
    return LC; 
}
"}" { 
    // printf("RC ");
    return RC; 
}
"struct" { 
    // printf("STRUCT ");
    return STRUCT; 
}
"return" { 
    // printf("RETURN ");
    return RETURN; 
}
"if" { 
    // printf("IF ");
    return IF; 
}
"else" { 
    // printf("ELSE ");
    return ELSE; 
}
"while" { 
    // printf("WHILE ");
    return WHILE; 
}
{u_int} {
    // printf("INT ");
    yylval.type_int = atoi(yytext); 
    return INT; 
}
{float} { 
    // printf("FLOAT ");
    yylval.type_float = atof(yytext);
    return FLOAT; 
}
{id} { 
    // printf("ID ");
    return ID; 
}
. { printf("Error type A at Line %d: Unknown character \"%s\"", yylineno, yytext); }
