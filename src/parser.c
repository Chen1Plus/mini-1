#include "parser.h"

#include <stdlib.h>

#include "lex.h"

// forward declaration
static Node* assignment();

// factor := INT | INC_DEC? ID | '(' assignment ')'
static Node* factor() {
    Node* node = try_(INT);
    if (node) return node;

    if ((node = try_(ID))) return node;
    if ((node = try_(INC_DEC))) {
        node->rc = expect(ID);
        return node;
    }

    expect_n(L_PAREN);
    node = assignment();
    expect_n(R_PAREN);
    return node;
}

// unary := [\+-]* factor
static Node* unary() {
    Node *cur, *root = try_(ADD_SUB);
    if (!root) return factor();

    root->lc = newNode(INT, "0");
    for (cur = root; (cur->rc = try_(ADD_SUB)); cur = cur->rc)
        cur->rc->lc = newNode(INT, "0");
    cur->rc = factor();
    return root;
}

// mul_div_expr := unary ([\*\/] unary)*
static Node* mul_div_expr() {
    Node* root = unary();
    for (Node* tail; (tail = try_(MUL_DIV)); root = tail) {
        tail->lc = root;
        tail->rc = unary();
    }
    return root;
}

// add_sub_expr := mul_div_expr ([\+-] mul_div_expr)*
static Node* add_sub_expr() {
    Node* root = mul_div_expr();
    for (Node* tail; (tail = try_(ADD_SUB)); root = tail) {
        tail->lc = root;
        tail->rc = mul_div_expr();
    }
    return root;
}

// and_expr := add_sub_expr ('&' add_sub_expr)*
static Node* and_expr() {
    Node* root = add_sub_expr();
    for (Node* tail; (tail = try_(AND)); root = tail) {
        tail->lc = root;
        tail->rc = add_sub_expr();
    }
    return root;
}

// xor_expr := and_expr ('^' and_expr)*
static Node* xor_expr() {
    Node* root = and_expr();
    for (Node* tail; (tail = try_(XOR)); root = tail) {
        tail->lc = root;
        tail->rc = and_expr();
    }
    return root;
}

// or_expr := xor_expr ('|' xor_expr)*
static Node* or_expr() {
    Node* root = xor_expr();
    for (Node* tail; (tail = try_(OR)); root = tail) {
        tail->lc = root;
        tail->rc = xor_expr();
    }
    return root;
}

// assignment := ID [\+-]?= assignment | or_expr
static Node* assignment() {
    Node *root, *left = try_(ID);
    if (!left) return or_expr();

    if ((root = try_(ASSIGN))) {
    } else if ((root = try_(ADD_SUB_ASSIGN))) {
    } else {
        regret();
        free(left);
        return or_expr();
    }

    root->lc = left;
    root->rc = assignment();
    return root;
}

// statement := assignment? '\n' | EOF
Node* statement() {
    Node* root = try_(END);
    return root ? root : match(END_FILE) ? NULL : assignment();
}
