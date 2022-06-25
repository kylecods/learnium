#ifndef file_memory_h
#define file_memory_h

#include "vm.h"

#define ALLOCATE(vm, type,count) \
        (type*)reallocate(vm,NULL,0,sizeof(type)* (count))
#define FREE(vm,type,pointer) \
        reallocate(vm,pointer,sizeof(type),0)
#define GROW_CAPACITY(capacity) \
        ((capacity) < 8 ? 8 : (capacity) * 2)
#define SHRINK_CAPACITY(capacity) \
        ((capacity) < 16 ? 8 : (capacity) / 2)

#define GROW_ARRAY(vm,previous, type, old_count, count) \
        (type*)reallocate(vm,previous,sizeof(type)* (old_count), sizeof(type) * count)

#define FREE_ARRAY(vm,type,pointer,old_count) \
        reallocate(vm,pointer,sizeof(type)*(old_count), 0)

void* reallocate(LnVM* vm, void* previous, size_t old_size, size_t new_size);

void gray_object(LnVM* vm, Obj* object);

void gray_value(LnVM* vm, Value value);

void collect_garbage(LnVM* vm);

void free_object(LnVM* vm, Obj* object);

void free_objects(LnVM* vm);

#endif