#ifndef LEX_H
#define LEX_H

#include <stdbool.h>

#include "error.h"

// Maximum lexeme's length
#define MAX_LEX 256

typedef enum {
    UNKNOWN,
    // Parentheses
    L_PAREN,
    R_PAREN,
    // Factor
    INT,
    ID,
    // Operator
    ADD_SUB,
    MUL_DIV,
    INC_DEC,
    // Bitwise operator
    AND,
    OR,
    XOR,
    // Assignment
    ASSIGN,
    ADD_SUB_ASSIGN,
    // End
    END,
    END_FILE,
} TokenType;

extern bool match(TokenType);
extern void regret();
extern void next();
extern char *getLex();

typedef struct Node {
    TokenType tok;
    int val;
    char lexeme[MAX_LEX];
    struct Node *lc;  // left child
    struct Node *rc;  // right child
} Node;

Node *newNode(TokenType, const char *lexeme);
void freeTree(Node *root);

Node *try_(TokenType);
Node *expect(TokenType);
void expect_n(TokenType);  // expect without returning

#endif  // LEX_H
