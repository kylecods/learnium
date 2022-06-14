#ifndef file_object_h
#define file_object_h

#include <stdint.h>

typedef struct sObj Obj;
typedef struct sObjString ObjString;

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
    OBJ_MAP
}ObjType;


struct sObj{
    struct sObj* obj;
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
    int arity;
    int upvalue_count;
    ObjString* name;
}ObjFun;

#endif