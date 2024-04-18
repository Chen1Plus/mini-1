#include "code_gen.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "lex.h"
#include "memory.h"

// int evaluateTree(Node *root) {
//     if (!root) return 0;
//     if (root->tok == INT || root->val) return root->val;

//     switch (root->tok) {
//         case ID: return getSym(root->lexeme);

//         case ASSIGN: return setSym(root->lc->lexeme, evaluateTree(root->rc));

//         case ADD_SUB_ASSIGN:
//             return setSym(
//                 root->lc->lexeme,
//                 root->lexeme[0] == '+'
//                     ? getSym(root->lc->lexeme) + evaluateTree(root->rc)
//                     : getSym(root->lc->lexeme) - evaluateTree(root->rc));

//         case INC_DEC:
//             return setSym(
//                 root->rc->lexeme,
//                 getSym(root->rc->lexeme) + (root->lexeme[0] == '+' ? 1 :
//                 -1));

//         case ADD_SUB:
//         case MUL_DIV:
//         case AND:
//         case OR:
//         case XOR:
//             int lv = evaluateTree(root->lc);
//             int rv = evaluateTree(root->rc);

//             switch (root->lexeme[0]) {
//                 case '+': return lv + rv;
//                 case '-': return lv - rv;
//                 case '*': return lv * rv;
//                 case '/':
//                     if (rv == 0) err("division by zero\n");
//                     return lv / rv;
//                 case '&': return lv & rv;
//                 case '|': return lv | rv;
//                 case '^': return lv ^ rv;
//                 default:  return 0;  //! unreachable
//             }
//         default: return 0;  //! unreachable
//     }
// }

void printPrefix(Node *root) {
    if (!root) return;
    printf("%s ", root->lexeme);
    printPrefix(root->lc);
    printPrefix(root->rc);
}

int evaluateTree(Node *root) {
    if (root->tok == INT) return getInt(atoi(root->lexeme));
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
        case '/': modReg("DIV", lv, rv); break;
        case '&': modReg("AND", lv, rv); break;
        case '|': modReg("OR", lv, rv); break;
        case '^': modReg("XOR", lv, rv); break;
        default:  break;  //! undefined
    }
    return lv;
}
