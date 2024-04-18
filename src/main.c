#include <stdbool.h>
#include <stdio.h>

#include "code_gen.h"
#include "lex.h"
#include "memory.h"
#include "parser.h"

int main() {
    Node* root;
    while ((root = statement())) {
        if (root->tok == END) goto next_loop;

        // printPrefix(root);
        // printf("\n");
        evaluateTree(root);

        // printf("Result: r%d\nPrefix: ", evaluateTree(root));
        printf("\n");

    next_loop:
        freeTree(root);
    }
    // getXYZ();
    for (int i = 0; i < 8; i++) ownReg(i);
    printf("x: r%d\ny: r%d\nz: r%d\n", getSym("x"), getSym("y"), getSym("z"));
    printf("EXIT 0\n");
}
