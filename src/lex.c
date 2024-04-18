#include "lex.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

typedef struct {
    TokenType token;
    char lexeme[MAX_LEX];
} Lex;

static bool lex_regret = false;
static Lex lex_last    = {.token = UNKNOWN, .lexeme = ""};
static Lex lex_cur     = {.token = UNKNOWN, .lexeme = ""};

#define lex_write_c()             \
    {                             \
        lex_cur.lexeme[0] = c;    \
        lex_cur.lexeme[1] = '\0'; \
    }

#define lex_multi_input(condition)                           \
    {                                                        \
        int i = 0;                                           \
        for (; (condition) && i < MAX_LEX; c = fgetc(stdin)) \
            lex_cur.lexeme[i++] = c;                         \
        ungetc(c, stdin);                                    \
        lex_cur.lexeme[i] = '\0';                            \
    }

static TokenType lex_getToken() {
    char c = (char) fgetc(stdin);
    while (c == ' ' || c == '\t') c = (char)fgetc(stdin);

    // ID: [_[:alpha:]][_[:alnum:]]*
    if (c == '_' || isalpha(c)) {
        lex_multi_input(c == '_' || isalnum(c));
        return ID;
    }

    // INT: [0-9]+
    if (isdigit(c)) {
        lex_multi_input(isdigit(c));
        return INT;
    }

    switch (c) {
        case '+':
        case '-':
            lex_write_c();
            lex_cur.lexeme[1] = (char)fgetc(stdin);
            lex_cur.lexeme[2] = '\0';

            if (lex_cur.lexeme[1] == c) return INC_DEC;
            if (lex_cur.lexeme[1] == '=') return ADD_SUB_ASSIGN;

            ungetc(lex_cur.lexeme[1], stdin);
            lex_cur.lexeme[1] = '\0';
            return ADD_SUB;

        case '*':
        case '/': lex_write_c(); return MUL_DIV;
        case '=': lex_write_c(); return ASSIGN;
        case '(': lex_write_c(); return L_PAREN;
        case ')': lex_write_c(); return R_PAREN;
        case '&': lex_write_c(); return AND;
        case '|': lex_write_c(); return OR;
        case '^': lex_write_c(); return XOR;

        case '\n': lex_cur.lexeme[0] = '\0'; return END;
        case EOF:  return END_FILE;
        default:   return UNKNOWN;
    }
}

void regret() { lex_regret = true; }

void next() {
    if (lex_regret) {
        lex_regret = false;
    } else {
        lex_last      = lex_cur;
        lex_cur.token = lex_getToken();
    }
}

bool match(TokenType token) {
    if ((lex_regret ? lex_last : lex_cur).token == UNKNOWN) next();
    return (lex_regret ? lex_last : lex_cur).token == token;
}

char *getLex() { return (lex_regret ? lex_last : lex_cur).lexeme; }

Node *newNode(TokenType tok, const char *lexeme) {
    Node *node = calloc(1, sizeof(Node));
    node->tok  = tok;
    strcpy(node->lexeme, lexeme);
    return node;
}

void freeTree(Node *root) {
    if (!root) return;
    freeTree(root->lc);
    freeTree(root->rc);
    free(root);
}

Node *try_(TokenType tok) {
    Node *retp = NULL;
    if (match(tok)) {
        retp = newNode(tok, getLex());
        next();
    }
    return retp;
}

#define expect_err(tok)                                    \
    switch (tok) {                                         \
        case ID:             err("expect identifier\n");   \
        case INT:            err("expect integer\n");      \
        case L_PAREN:        err("expect '('\n");          \
        case R_PAREN:        err("expect ')'\n");          \
        case ADD_SUB:        err("expect '+' or '-'\n");   \
        case MUL_DIV:        err("expect '*' or '/'\n");   \
        case INC_DEC:        err("expect '++' or '--'\n"); \
        case AND:            err("expect '&'\n");          \
        case OR:             err("expect '|'\n");          \
        case XOR:            err("expect '^'\n");          \
        case ASSIGN:         err("expect '='\n");          \
        case ADD_SUB_ASSIGN: err("expect '+=' or '-='\n"); \
        case END:            err("expect '\\n'\n");        \
        default:             err("undefined expect\n");                \
    }

Node *expect(TokenType tok) {
    if (match(tok)) {
        Node *retp = newNode(tok, getLex());
        next();
        return retp;
    }
    expect_err(tok);
}

void expect_n(TokenType tok) {
    if (match(tok)) {
        next();
        return;
    }
    expect_err(tok);
}
