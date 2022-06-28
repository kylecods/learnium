#ifndef file_ln_h
#define file_ln_h

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#define UINT8_COUNT (UINT8_MAX + 1)

#include "src/scanner.h"
#include "src/compiler.h"
#include "src/value.h"
#include "src/memory.h"
#include "src/chunk.h"
#include "src/object.h"
#include "src/hash_table.h"
#include "src/vm.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILER_ERROR,
    INTERPRET_RUNTIME_ERROR
}LnInterpretResult;


#endif