#ifndef file_vm_h
#define file_vm_h

#include <stdint.h>

#include "object.h"
#include "value.h"
#include "compiler.h"


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