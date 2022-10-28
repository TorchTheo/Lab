#include <stdio.h>
#include <stdlib.h>
#include "include/type.h"
extern FILE* yyin;
extern int error;
extern Node *root;
int yyparse();
int yyrestart(FILE *);
void print_AST(Node *, int);
void start_semantics(Node *);
int main(int argc, char **argv) {
    if (argc <= 1) return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    if(!error) {
        print_AST(root, 0);
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        start_semantics(root);
    }
    return 0;
}