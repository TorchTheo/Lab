#include <stdio.h>
#include <stdlib.h>
#include "include/type.h"
extern FILE* yyin;
extern int error;
extern Node *root;
int yyparse();
int yyrestart(FILE *);
void print_AST(Node *, int);

void analyse(Node*);
void init();

FILE* file_out = NULL;

void start_ir(Node*);

int main(int argc, char **argv) {
    if (argc <= 1) return 1;
    FILE *file_in = fopen(argv[1], "r");
    if (!file_in) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(file_in);
    yyparse();

    if(argc > 2)
        file_out = fopen(argv[2], "w+");
    

    if(!error) {
        // print_AST(root, 0);
        // printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        init();
        analyse(root);
    }
    if(!error) {
        // print_ic(translate_deflist(root, NULL));
        start_ir(root);
    }
    if(file_out != NULL)
        fclose(file_out);
    fclose(file_in);
    return 0;
}