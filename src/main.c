#include <stdbool.h>
#include <stdio.h>

#include "code_gen.h"
#include "lex.h"
#include "memory.h"
#include "parser.h"

int main() {
    for (Node *root; (root = statement()); freeTree(root)) {
        if (root->tok == END) continue;
        evaluateTree(root);
        printf("\n");
    }
    getXYZ();
    printf("EXIT 0\n");
}
