#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>

#define PRINT_ERR  // Print error on/off

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
#define err(msg)                       \
    {                                  \
        fprintf(stderr, "\nEXIT 1\n"); \
        exit(0);                       \
    }
#endif

#endif  // ERROR_H