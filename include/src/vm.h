#ifndef file_vm_h
#define file_vm_h

#include <stdint.h>

#include "object.h"
#include "value.h"
#include "compiler.h"

typedef enum{//note: implement different op instr for comparison operators
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_GET_SUPER,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_NEGATE,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_BITWISE_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,
    OP_RIGHT_SHIFT,
    OP_LEFT_SHIFT,
    OP_NOT,
    OP_LOOP,
    OP_CALL,
    OP_INVOKE,
    OP_WIDE,
    OP_SUPER_INVOKE,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_RETURN,
    OP_CLASS,
    OP_INHERIT,
    OP_METHOD,
    OP_EMPTY,
    OP_GET_MODULE,
    OP_DEFINE_MODULE,
    OP_SET_MODULE,
    OP_BREAK,
    OP_IMPORT,
    OP_IMPORT_BUILTIN
}Opcode;

#define STACK_MAX (64 * UINT8_COUNT)

typedef struct{
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
}CallFrame;

struct vm {
    Compiler* compiler;
    Value stack[STACK_MAX];
    Value* stack_top;
    CallFrame* frames;
    int frame_count;
    int frame_capacity;
    ObjModule* last_module;
    HashTable modules;
    HashTable globals;
    HashTable strings;
    HashTable string_methods;
    HashTable list_methods;
    HashTable map_methods;
    HashTable file_methods;
    ObjString* init_string;
    ObjUpvalue* open_upvalues;
    size_t bytes_allocated;
    size_t next_gc;
    Obj* objects;
    int gray_count;
    int gray_capacity;
    Obj **gray_stack;
    int argc;
    char** argv;
};

void push(LnVM* vm, Value value);

Value peek(LnVM* vm, int distance);

void runtime_error(LnVM* vm, const char* format, ...);

Value pop(LnVM* vm);

ObjClosure* compile_module_to_closure(LnVM* vm, char* name, char* source);


#endif