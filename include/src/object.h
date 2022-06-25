#ifndef file_object_h
#define file_object_h

#include <stdint.h>

#include "value.h"
#include "chunk.h"
#include "hash_table.h"


#define AS_MODULE(value)  ((ObjModule*)AS_OBJ(value))
#define AS_FUNC(value)  ((ObjFun*)AS_OBJ(value))
#define AS_LIST(value)  ((ObjList*)AS_OBJ(value))
#define AS_MAP(value)  ((ObjMap*)AS_OBJ(value))
#define AS_STRING(value)  ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) ((ObjString*)AS_OBJ(value)->chars)
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))
#define AS_ENUM(value) ((ObjEnum*)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value) ((ObjNative*)AS_OBJ(value))
#define AS_FILE(value) ((ObjFile*)AS_OBJ(value))

#define IS_LIST(value)  is_obj_type(value,OBJ_LIST)
#define IS_BOUND_METHOD(value) is_obj_type(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value) is_obj_type(value,OBJ_CLASS)
#define IS_CLOSURE(value)  is_obj_type(value, OBJ_CLOSURE)
#define IS_FUNC(value) is_obj_type(value, OBJ_FUNTION)
#define IS_INSTANCE(value) is_obj_type(value, OBJ_INSTANCE)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define IS_MAP(value) is_obj_type(value, OBJ_MAP)
#define IS_FILE(value) is_obj_type(value, OBJ_FILE)
#define IS_ENUM(value) is_obj_type(value, OBJ_ENUM)

typedef enum{
    OBJ_LIST,
    OBJ_MODULE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_CLOSURE,
    OBJ_STRING,
    OBJ_NATIVE,
    OBJ_FILE,
    OBJ_UPVALUE,
    OBJ_BOUND_METHOD,
    OBJ_MAP,
    OBJ_CLASS,
    OBJ_ENUM
}ObjType;


struct sObj{
    struct sObj* next;
    bool is_marked;
    ObjType type;
};

struct sObjString
{
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};
typedef struct{
    Obj obj;
    ObjString* name;
    ObjString* path;
    HashTable values;
}ObjModule;

typedef struct{
    Obj obj;
    int arity;
    int upvalue_count;
    ObjString* name;
    ObjModule* module;
    Chunk chunk;
}ObjFun;

typedef struct sUpvalue{
    Obj obj;
    Value* value;
    Value closed;
    struct sUpvalue* next;
}ObjUpvalue;

typedef struct{
    Obj obj;
    ObjFun* function;
    ObjUpvalue **upvalues;
    int upvalue_count;
}ObjClosure;

typedef struct{
    Obj obj;
    Value receiver;
    ObjClosure* method;
}ObjBoundMethod;

typedef struct Klass{
    Obj obj;
    ObjString* name;
    struct Klass* super_class;
    HashTable methods;
    HashTable properties;
}ObjClass;

typedef struct{
    Obj obj;
    ObjClass* klass;
    HashTable fields;
}ObjInstance;

typedef struct {
    Obj obj;
    ObjString* name;
    HashTable values;
}ObjEnum;

struct sObjList{
    Obj obj;
    ValueArray values;
};

typedef struct {
    Value key;
    Value  value;
    uint32_t ps1;
}MapEntry;

struct sObjMap{
    Obj obj;
    int count;
    int capacity_mask;
    MapEntry* entries;
};

typedef Value (*NativeFn)(LnVM* vm,int arg_count, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
}ObjNative;

struct sObjFile{
    Obj obj;
    FILE* file;
    char* path;
    char* open_type;
};

ObjModule* new_module(LnVM* vm,ObjString* name);

ObjBoundMethod* new_boundmethod(LnVM* vm, Value receiver, ObjClosure* method);

ObjClass* new_class(LnVM* vm, ObjString* name, ObjClass* super_class);

ObjEnum* new_enum(LnVM* vm,ObjString* name);

ObjClosure* new_closure(LnVM* vm,ObjFun* function);

ObjFun* new_function(LnVM* vm, ObjModule* module);

ObjInstance* new_instance(LnVM* vm, ObjClass* klass);

ObjNative* new_native(LnVM* vm, NativeFn function);

ObjString* take_string(LnVM* vm,char* chars, int length);

ObjString* copy_string(LnVM* vm,const char* chars, int length);

ObjList* new_list(LnVM* vm);

ObjMap* new_map(LnVM* vm);

ObjFile* new_file(LnVM* vm);

ObjUpvalue* new_upvalue(LnVM* vm, Value* slot);

static inline bool is_obj_type(Value value, ObjType type){
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif