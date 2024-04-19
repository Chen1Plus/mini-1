#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =======================================
// ===              ERROR              ===
// =======================================

// #define PRINT_ERR  // Print error on/off

#define COMMA ,

#ifdef PRINT_ERR
#define err(msg)                                                          \
    {                                                                     \
        fprintf(stderr, "\nerr() called at %s:%d: ", __FILE__, __LINE__); \
        fprintf(stderr, msg);                                             \
        fprintf(stderr, "EXIT 1\n");                                      \
        exit(0);                                                          \
    }
#else
#define err(msg)            \
    {                       \
        printf("EXIT 1\n"); \
        exit(0);            \
    }
#endif

// =======================================
// ===               LEX               ===
// =======================================

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

typedef struct Node {
    TokenType tok;
    int val;
    char lexeme[MAX_LEX];
    struct Node* lc;  // left child
    struct Node* rc;  // right child
} Node;

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
    char c = (char)fgetc(stdin);
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
    if ((lex_regret ? lex_last.token : lex_cur.token) == UNKNOWN) next();
    return (lex_regret ? lex_last.token : lex_cur.token) == token;
}

char* getLex() { return lex_regret ? lex_last.lexeme : lex_cur.lexeme; }

Node* newNode(TokenType tok, const char* lexeme) {
    Node* node = calloc(1, sizeof(Node));
    node->tok  = tok;
    strcpy(node->lexeme, lexeme);
    return node;
}

void freeTree(Node* root) {
    if (!root) return;
    freeTree(root->lc);
    freeTree(root->rc);
    free(root);
}

Node* try_(TokenType tok) {
    Node* retp = NULL;
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

Node* expect(TokenType tok) {
    if (match(tok)) {
        Node* retp = newNode(tok, getLex());
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

// ==========================================
// ===               PARSER               ===
// ==========================================

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

// ==========================================
// ===               MEMORY               ===
// ==========================================

#define TBL_SIZE 64

#define REG_SIZE 8
static struct {
    enum { NONE = 0, NUM, VAR, TMP } type;
    int id;
} m_reg[REG_SIZE];

static struct {
    int cnt;
    struct {
        char key[MAX_LEX];
        bool in_reg;
        int reg_idx;
    } tbl[TBL_SIZE];
} m_sym = {.cnt = 3,
           .tbl = {{"x", false, 0}, {"y", false, 0}, {"z", false, 0}}};

static struct {
    int cnt;
    // int tbl[TBL_SIZE];
    struct {
        bool in_reg;
        int reg_idx;
    } tbl[TBL_SIZE];
} m_tmp;

bool isZero(int reg) { return m_reg[reg].type == NUM && m_reg[reg].id == 0; }

void ownReg(int idx) {
    int id = m_reg[idx].id;
    switch (m_reg[idx].type) {
        case NONE:
        case NUM:  break;

        case VAR:
            printf("MOV [%d] r%d\n", id * 4, m_sym.tbl[id].reg_idx);
            m_sym.tbl[id].in_reg = false;
            break;
        case TMP:
            printf("MOV [%d] r%d\n", (TBL_SIZE - id - 1) * 4,
                   m_tmp.tbl[id].reg_idx);
            m_tmp.tbl[id].in_reg = false;
            break;
    }
    m_reg[idx].type = NONE;
}

int getRegSpace(int except) {
    for (int i = 0; i < REG_SIZE; i++)
        if (i != except && m_reg[i].type == NONE) return i;

    for (int i = 0; i < REG_SIZE; i++)
        if (i != except && m_reg[i].type == NUM) {
            m_reg[i].type = NONE;
            return i;
        }

    for (int i = 0; i < REG_SIZE; i++)
        if (i != except && m_reg[i].type == VAR) {
            ownReg(i);
            return i;
        }

    int mn_idx, mn = TBL_SIZE;
    for (int i = 0; i < REG_SIZE; i++)
        if (i != except && m_reg[i].type == TMP && m_reg[i].id < mn) {
            mn     = m_reg[i].id;
            mn_idx = i;
        }
    if (mn != TBL_SIZE) {
        ownReg(mn_idx);
        return mn_idx;
    }

    err("register overflows\n");
}

void modReg(const char* op, int r1, int r2) {
    ownReg(r1);
    printf("%s r%d r%d\n", op, r1, r2);
}

int getInt(int n) {
    for (int i = 0; i < REG_SIZE; i++)
        if (m_reg[i].type == NUM && m_reg[i].id == n) return i;

    int reg = getRegSpace(-1);
    printf("MOV r%d %d\n", reg, n);
    m_reg[reg].type = NUM;
    m_reg[reg].id   = n;
    return reg;
}

void setSym(const char* key, int reg) {
    int i = 0;
    for (; i < m_sym.cnt; i++)
        if (strcmp(m_sym.tbl[i].key, key) == 0) {
            if (m_sym.tbl[i].in_reg && m_sym.tbl[i].reg_idx == reg) return;
            goto write;
        }

    if (m_sym.cnt >= TBL_SIZE) err("symbol table overflows\n");
    strcpy(m_sym.tbl[m_sym.cnt].key, key);
    i = m_sym.cnt++;

write:
    ownReg(reg);
    m_reg[reg].type      = VAR;
    m_reg[reg].id        = i;
    m_sym.tbl[i].in_reg  = true;
    m_sym.tbl[i].reg_idx = reg;
}

int getSym(const char* key) {
    for (int i = 0; i < m_sym.cnt; i++)
        if (strcmp(m_sym.tbl[i].key, key) == 0) {
            if (m_sym.tbl[i].in_reg) return m_sym.tbl[i].reg_idx;

            int reg = getRegSpace(-1);
            printf("MOV r%d [%d]\n", reg, i * 4);
            return reg;
        }

    err("undefined symbol: %s\n" COMMA key);
}

void getXYZ() {
    ownReg(0);
    if (!m_sym.tbl[0].in_reg) {
        printf("MOV r0 [0]\n");
    } else {
        printf("MOV r0 r%d\n", m_sym.tbl[0].reg_idx);
    }

    ownReg(1);
    if (!m_sym.tbl[1].in_reg) {
        printf("MOV r1 [4]\n");
    } else {
        printf("MOV r1 r%d\n", m_sym.tbl[1].reg_idx);
    }

    ownReg(2);
    if (!m_sym.tbl[2].in_reg) {
        printf("MOV r2 [8]\n");
    } else {
        printf("MOV r2 r%d\n", m_sym.tbl[2].reg_idx);
    }
}

void addTmp(int reg) {
    if (m_tmp.cnt >= TBL_SIZE) err("temporary table overflows\n");
    ownReg(reg);
    m_reg[reg].type              = TMP;
    m_reg[reg].id                = m_tmp.cnt;
    m_tmp.tbl[m_tmp.cnt].in_reg  = true;
    m_tmp.tbl[m_tmp.cnt].reg_idx = reg;
    m_tmp.cnt++;
}

int getTmp(int except) {
    m_tmp.cnt--;
    if (m_tmp.tbl[m_tmp.cnt].in_reg) {
        m_reg[m_tmp.tbl[m_tmp.cnt].reg_idx].type = NONE;
        return m_tmp.tbl[m_tmp.cnt].reg_idx;
    }

    int reg = getRegSpace(except);
    printf("MOV r%d [%d]\n", reg, (TBL_SIZE - m_tmp.cnt - 1) * 4);
    return reg;
}

// ==========================================
// ===              CODE GEN              ===
// ==========================================

void printPrefix(Node* root) {
    if (!root) return;
    printf("%s ", root->lexeme);
    printPrefix(root->lc);
    printPrefix(root->rc);
}

// return the const value; otherwise, return INT_MIN
// int evalValue(Node* root) {
//     int lv, rv;
//     switch (root->tok) {
//         case ADD_SUB:
//         case MUL_DIV:
//         case AND:
//         case OR:
//         case XOR:
//             lv = evalValue(root->lc), rv = evalValue(root->rc);
//             if (lv == INT_MIN || rv == INT_MIN) return INT_MIN;

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
//                 default:  return INT_MIN;  //! unreachable
//             }

//         case INT: return atoi(root->lexeme);
//         default:  return INT_MIN;
//     }
// }

int evaluateTree(Node* root) {
    // if the root is a const value, return it
    // int x = evalValue(root);
    // if (x != INT_MIN) return getInt(x);

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

// ==========================================
// ===                MAIN                ===
// ==========================================

int main() {
    for (Node* root; (root = statement()); freeTree(root)) {
        if (root->tok == END) continue;
        evaluateTree(root);
    }
    getXYZ();
    printf("EXIT 0\n");
}
