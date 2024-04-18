#include "memory.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "lex.h"

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
    struct {
        bool in_reg;
        int reg_idx;
    } tbl[TBL_SIZE];
} m_tmp;

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

void modReg(const char *op, int r1, int r2) {
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

void setSym(const char *key, int reg) {
    int target = -1;
    for (int i = 0; i < m_sym.cnt; i++)
        if (strcmp(m_sym.tbl[i].key, key) == 0) {
            if (m_sym.tbl[i].in_reg && m_sym.tbl[i].reg_idx == reg) return;
            target = i;
            goto write;
        }

    if (m_sym.cnt >= TBL_SIZE) err("symbol table overflows\n");
    strcpy(m_sym.tbl[m_sym.cnt].key, key);
    target = m_sym.cnt++;

write:
    ownReg(reg);
    m_reg[reg].type           = VAR;
    m_reg[reg].id             = target;
    m_sym.tbl[target].in_reg  = true;
    m_sym.tbl[target].reg_idx = reg;
}

int getSym(const char *key) {
    for (int i = 0; i < m_sym.cnt; i++)
        if (strcmp(m_sym.tbl[i].key, key) == 0) {
            if (m_sym.tbl[i].in_reg) return m_sym.tbl[i].reg_idx;

            int reg = getRegSpace(-1);
            printf("MOV r%d [%d]\n", reg, i * 4);
            return reg;
        }

    err("undefined symbol: %s\n" COMMA key);
}

// void getXYZ() {
//     for (int i = 0; i < 3; i++) ownReg(i);

//     for (int i = 0; i < 3; i++) {
//         int reg = getRegSpace(-1);
//         printf("MOV r%d [%d]\n", reg, i * 4);
//         m_sym.tbl[i].in_reg  = true;
//         m_sym.tbl[i].reg_idx = reg;
//     }
// }

void addTmp(int reg) {
    if (m_tmp.cnt >= TBL_SIZE) err("temporary table overflows\n");
    ownReg(reg);
    m_reg[reg].type              = TMP;
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
