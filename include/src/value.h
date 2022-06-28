#ifndef file_value_h
#define file_value_h

#include <stdint.h>

typedef struct vm LnVM;


typedef struct sObj Obj;
typedef struct sObjString ObjString;
typedef struct sObjList ObjList;
typedef struct sObjMap ObjMap;
typedef struct sObjFile ObjFile;

#define SIGN_BIT ((uint64_t)1 << 63)
#define QNAN ((uint64_t)0x7ffc000000000000)

#define TAG_NIL   1 
#define TAG_FALSE 2 
#define TAG_TRUE  3
#define TAG_EMPTY 4

typedef uint64_t Value;

#define IS_BOOL(value)      (((value) | 1) == TRUE_VAL)
#define IS_NIL(value)      ((value) == NIL_VAL)
#define IS_EMPTY(value)      ((value) == EMPTY_VAL)
#define IS_NUMBER(value)  (((value) & QNAN) != QNAN)
#define IS_OBJ(value) \
        (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)  ((value) == TRUE_VAL)
#define AS_NUMBER(value) value_to_num(value)
#define AS_OBJ(value) \
        ((Obj*) (uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define BOOL_VAL(b)    ((b) ? TRUE_VAL: FALSE_VAL)
#define FALSE_VAL     ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL     ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL     ((Value)(uint64_t)(QNAN | TAG_NIL))
#define EMPTY_VAL  ((Value)(uint64_t)(QNAN | TAG_EMPTY))
#define NUMBER_VAL(num) num_to_value(num)
#define OBJ_VAL(obj) \
        (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))


typedef union{
    uint64_t bits64;
    uint32_t bits32[2];
    double num;
}DoubleUnion;

static inline double value_to_num(Value value){
    DoubleUnion data;
    data.bits64 = value;
    return data.num;
}

static inline Value num_to_value(double num){
    DoubleUnion data;
    data.num = num;
    return data.bits64;
}

typedef struct
{
    int capacity;
    int count;
    Value* value;
} ValueArray;

bool values_equal(Value a, Value b);

void init_valueArray(ValueArray* array);

void write_valueArray(LnVM* vm, ValueArray* array,Value value);

void free_valueArray(LnVM* vm,ValueArray* array);

void gray_map(LnVM* vm, ObjMap* map);

bool map_set(LnVM* vm, ObjMap* map, Value key, Value value);

bool map_get(ObjMap* map, Value key,Value* value);

bool map_delete(LnVM* vm, ObjMap* map, Value key);

char* value_to_string(Value value);

char* value_type_to_string(LnVM* vm, Value value,int* length);

void print_value(Value value);

#endif