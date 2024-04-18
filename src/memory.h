#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>

#include "error.h"
#include "lex.h"

#define TBL_SIZE 64

bool isInt(int reg);
void ownReg(int idx);
void modReg(const char *op, int r1, int r2);

int getInt(int);

void setSym(const char *key, int reg);
int getSym(const char *key);

void getXYZ();

void addTmp(int reg);
int getTmp(int except);

#endif  // MEMORY_H