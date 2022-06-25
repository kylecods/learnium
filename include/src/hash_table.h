#ifndef file_hash_table_h
#define file_hash_table_h

#include <stdbool.h>

#include "value.h"

typedef struct {
    ObjString* key;
    Value value;
    uint32_t ps1;
}Entry;

typedef struct {
    int count;
    int capacity_mask;
    Entry* entries;
}HashTable;

void init_table(HashTable* table);

void free_table(LnVM* vm, HashTable* table);

bool table_get(HashTable* table, ObjString* key, Value* value);

bool table_set(LnVM* vm, HashTable* table, ObjString* key, Value value);

bool table_delete(HashTable* table, ObjString* key);

void table_add_all(LnVM* vm, HashTable* from, HashTable* to);

ObjString* table_find_string(HashTable * table, const char* chars, int length, uint32_t hash);

void table_remove_whites(LnVM* vm, HashTable* table);

void gray_table(LnVM* vm,HashTable* table);

#endif