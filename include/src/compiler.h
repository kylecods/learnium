#ifndef file_compiler_h
#define file_compiler_h

#include <stdbool.h>

#include "scanner.h"

typedef struct
{
    Scanner scanner;
    Token current;
    Token previous;
    bool hasError;
    bool panicMode;
}Parser;


#endif