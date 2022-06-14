#ifndef file_compiler_h
#define file_compiler_h

#include <stdbool.h>

#include "scanner.h"
#include "object.h"

typedef enum{
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR, // or
  PREC_AND, // and
  PREC_EQUALITY, // == !=
  PREC_COMPARISON, //< > <= >=
  PREC_BITWISE_OR, // |
  PREC_BITWISE_XOR, // ^
  PREC_BITWISE_AND, // &
  PREC_LEFT_SHIFT, // <<
  PREC_RIGHT_SHIFT, // >>
  PREC_TERM, // + -
  PREC_FACTOR, // * /
  PREC_UNARY, // ! -, pre ++, pre --
  PREC_CALL, // . () []
  PREC_PRIMARY
} Precedence;

typedef struct
{
    Scanner scanner;
    Token current;
    Token previous;
    bool hasError;
    bool panicMode;
}Parser;

typedef struct
{
    uint8_t index;
    bool is_local;
}UpValue;

typedef struct{
    Token name;
    int depth;
    bool is_captured;
}Local;

typedef struct{
    struct ClassCompiler* enclosing;
    Token name;
    bool has_super_class;
}ClassCompiler;

typedef enum{
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT
}FunctionType;

typedef struct{
    struct Loop* enclosing;
    int start;
    int body;
    int end;
    int scope_depth;
}Loop;


typedef struct {
    struct Compiler* enclosing;    
    Parser* parser;
    FunctionType type;
    Local locals[UINT8_COUNT];
    int local_count;
    UpValue upvalues[UINT8_COUNT];
    int scope_depth;
    ClassCompiler* class;
    Loop* loop;
    ObjFun* function;
}Compiler;


typedef void (*ParsePrefixFn)(Compiler* compiler, bool can_assign);
typedef void (*ParseInfixFn)(Compiler* compiler, bool can_assign);


typedef struct{
    ParsePrefixFn prefix;
    ParseInfixFn infix;
    Precedence precedence;
}ParserRule;




#endif