#include "code_gen.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "lex.h"
#include "memory.h"

// return const value; if not const, return INT_MIN
int evalValue(Node *root) {
    int lv, rv;
    switch (root->tok) {
        case INT: return atoi(root->lexeme);

        case ADD_SUB:
        case MUL_DIV:
        case AND:
        case OR:
        case XOR:
            lv = evalValue(root->lc);
            rv = evalValue(root->rc);
            if (lv == INT_MIN || rv == INT_MIN) return INT_MIN;

            switch (root->lexeme[0]) {
                case '+': return lv + rv;
                case '-': return lv - rv;
                case '*': return lv * rv;
                case '/':
                    if (rv == 0) err("division by zero\n");
                    return lv / rv;
                case '&': return lv & rv;
                case '|': return lv | rv;
                case '^': return lv ^ rv;
                default:  return INT_MIN;  //! unreachable
            }

        default: return INT_MIN;
    }
}

int evaluateTree(Node *root) {
    // const expression
    int x = evalValue(root);
    if (x != INT_MIN) return getInt(x);
    if (root->tok == INT)
        return getInt(atoi(root->lexeme));  // work when INT's value is INT_MIN

    // variable
    if (root->tok == ID) return getSym(root->lexeme);

    int lv, rv;
    if (root->tok == ASSIGN) {
        lv = evaluateTree(root->rc);
        setSym(root->lc->lexeme, lv);
        return lv;
    }

    if (root->tok == ADD_SUB_ASSIGN) {
        addTmp(evaluateTree(root->rc));
        lv = getSym(root->lc->lexeme);
        rv = getTmp(lv);
        printf("%s r%d r%d\n", root->lexeme[0] == '+' ? "ADD" : "SUB", lv, rv);
        setSym(root->lc->lexeme, lv);
        return lv;
    }

    if (root->tok == INC_DEC) {
        addTmp(getSym(root->rc->lexeme));
        rv = getInt(1);
        lv = getTmp(rv);
        printf("%s r%d r%d\n", root->lexeme[0] == '+' ? "ADD" : "SUB", lv, rv);
        setSym(root->rc->lexeme, lv);
        return lv;
    }

    if (root->rc->tok == ID || root->rc->tok == INT) {
        addTmp(evaluateTree(root->lc));
        rv = evaluateTree(root->rc);
        lv = getTmp(rv);
    } else {
        addTmp(evaluateTree(root->rc));
        lv = evaluateTree(root->lc);
        rv = getTmp(lv);
    }

    switch (root->lexeme[0]) {
        case '+': modReg("ADD", lv, rv); break;
        case '-': modReg("SUB", lv, rv); break;
        case '*': modReg("MUL", lv, rv); break;
        case '/':
            if (isZero(rv)) err("division by zero\n");
            modReg("DIV", lv, rv);
            break;
        case '&': modReg("AND", lv, rv); break;
        case '|': modReg("OR", lv, rv); break;
        case '^': modReg("XOR", lv, rv); break;
        default:  break;  //! undefined
    }
    return lv;
}
