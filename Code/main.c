#include <stdio.h>
#include <stdlib.h>
extern FILE* yyin;
extern int error;
extern struct Node *root;
int main(int argc, char **argv) {
    if (argc <= 1) return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    if(!error)
        print_AST(root, 0);
    return 0;
}