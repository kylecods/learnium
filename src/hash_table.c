#include "ln.h"

#define TABLE_MAX_LOAD 0.75

void init_table(HashTable* table){
    table->count = 0;
    table->capacity_mask = -1;
    table->entries = NULL;
}

void free_table(LnVM* vm, HashTable* table){
    FREE_ARRAY(vm,Entry,table->entries,table->capacity_mask + 1);
    init_table(table);
}

bool table_get(HashTable* table, ObjString* key, Value* value){
    if(table->count == 0) return false;

    Entry* entry;
    uint32_t index = key->hash & table->capacity_mask;
    uint32_t ps1 = 0;

    for (;;) {
        entry = &table->entries[index];
        if (entry->key == NULL || ps1 > entry->ps1) return false;
        if(entry->key == key) break;

        index = (index + 1) & table->capacity_mask;
        ps1++;
    }
    *value = entry->value;
    return true;
}

static void adjust_capacity(LnVM* vm,HashTable* table, int capacity_mask){
    Entry* entries = ALLOCATE(vm,Entry,capacity_mask + 1);
    for (int i = 0; i <= capacity_mask; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
        entries[i].ps1 = 0;
    }

    Entry* old_entries = table->entries;

    int old_mask = table->capacity_mask;

    table->count = 0;
    table->entries = entries;
    table->capacity_mask = capacity_mask;

    for (int i = 0; i <= old_mask; i++) {
        Entry* entry = &old_entries[i];
        if(entry->key == NULL) continue;

        table_set(vm,table,entry->key,entry->value);
    }
    FREE_ARRAY(vm,Entry,old_entries,old_mask +1);
}

bool table_set(LnVM* vm,HashTable* table, ObjString* key,Value value){
    if(table->count + 1 > (table->capacity_mask + 1) * TABLE_MAX_LOAD){
        int capacity_mask = GROW_CAPACITY(table->capacity_mask + 1) - 1;
        adjust_capacity(vm,table,capacity_mask);
    }
    uint32_t index = key->hash & table->capacity_mask;
    Entry* bucket;
    bool is_new_key = false;

    Entry entry;
    entry.key = key;
    entry.value =value;
    entry.ps1 = 0;

    for (;;){
        bucket = &table->entries[index];

        if(bucket->key == NULL){
            is_new_key = true;
            break;
        } else{
            if (bucket->key == key)break;

            if(entry.ps1 > bucket->ps1){
                is_new_key = true;
                Entry tmp = entry;
                entry = *bucket;
                *bucket = tmp;
            }
        }
        index = (index + 1 ) &table->capacity_mask;
        entry.ps1++;
    }
    *bucket =entry;
    if(is_new_key) table->count++;

    return is_new_key;
}

bool table_delete(HashTable* table, ObjString* key){
    if(table->count == 0) return false;

    int capacity_mask = table->capacity_mask;
    uint32_t index = key->hash & table->capacity_mask;
    uint32_t ps1 = 0;
    Entry* entry;

    for (;;) {
        entry = &table->entries[index];
        if(entry->key == NULL || ps1 > entry->ps1) return false;
        if(entry->key == key) break;

        index = (index + 1) & capacity_mask;
        ps1++;
    }
    table->count--;
    while(true){
        Entry* next_entry;
        entry->key = NULL;
        entry->value = NIL_VAL;
        entry->ps1 = 0;

        index = (index + 1) & capacity_mask;
        next_entry = &table->entries[index];

        if(next_entry->key == NULL || next_entry->ps1 == 0) break;

        next_entry->ps1--;
        *entry = *next_entry;
        entry = next_entry;
    }
    return true;
}

void table_add_all(LnVM* vm, HashTable* from, HashTable* to){
    for (int i = 0; i <= from->capacity_mask; ++i) {

        Entry* entry = &from->entries[i];

        if(entry->key != NULL) table_set(vm,to,entry->key,entry->value);
    }
}
ObjString* table_find_string(HashTable* table,const char* chars, int length, uint32_t hash){
    if(table->count == 0) return NULL;

    uint32_t index = hash & table->capacity_mask;
    uint32_t ps1 = 0;

    for (;;){
        Entry* entry = &table->entries[index];
        if (entry->key == NULL || ps1 > entry->ps1) return NULL;

        if(entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars,length) == 0){
            return entry->key;
        }

        index = (index+1) & table->capacity_mask;
        ps1++;
    }
}

void table_remove_whites(LnVM* vm, HashTable* table){
    for (int i = 0; i <= table->capacity_mask; ++i) {
        Entry* entry = &table->entries[i];
        if(entry->key != NULL && !entry->key->obj.is_marked){
            table_delete(table,entry->key);
        }
    }
}

void gray_table(LnVM* vm, HashTable* table){
    for (int i = 0; i <= table->capacity_mask; ++i) {
        Entry* entry = &table->entries[i];
        gray_object(vm,(Obj*)entry->key);
        gray_value(vm,entry->value);
    }
}