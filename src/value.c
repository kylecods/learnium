#include "ln.h"

#define TABLE_MAX_LOAD 0.75
#define TABLE_MIN_LOAD 0.25



bool values_equal(Value a, Value b);

void init_valueArray(ValueArray* array){
    array->value = NULL;
    array->count= 0;
    array->capacity = 0;
}

void write_valueArray(LnVM* vm, ValueArray* array,Value value){
    if(array->capacity < array->count + 1){
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->value = GROW_ARRAY(vm,array->value, Value,old_capacity,array->capacity);
    }

    array->value[array->count] = value;
    array->count++;
}

void free_valueArray(LnVM* vm,ValueArray* array){
    FREE_ARRAY(vm,Value,array->value,array->capacity);
    init_valueArray(array);
}

void gray_map(LnVM* vm, ObjMap* map){
    for (int i = 0; i <= map->capacity_mask; i++) {
        MapEntry* entry = &map->entries[i];
        gray_value(vm,entry->key);
        gray_value(vm,entry->value);
    }
}

static inline uint32_t hash_bits(uint64_t hash){
    hash = ~hash + (hash << 18);
    hash = hash ^ (hash >> 31);
    hash = hash * 21;
    hash = hash ^ (hash >> 11);
    hash = hash + (hash << 6);
    hash = hash ^ (hash >> 22);
    return (uint32_t) (hash & 0x3fffffff);
}

static uint32_t hash_object(Obj* object){
    switch (object->type){
        case OBJ_STRING: return ((ObjString*) object)->hash;
        default: return -1;
    }
}
static uint32_t hash_value(Value value){
    if(IS_OBJ(value)) return hash_object(AS_OBJ(value));
    return hash_bits(value);
}

static void adjust_map_capacity(LnVM* vm,ObjMap* map, int capacity_mask){
    MapEntry* entries = ALLOCATE(vm,MapEntry,capacity_mask + 1);

    for (int i = 0; i <= capacity_mask; i++) {
        entries[i].key = EMPTY_VAL;
        entries[i].value = NIL_VAL;
        entries[i].ps1 = 0;
    }

    MapEntry* old_entries = map->entries;
    int old_mask = map->capacity_mask;

    map->count = 0;
    map->entries = entries;
    map->capacity_mask = capacity_mask;

    for (int i = 0; i <= old_mask; i++) {
        MapEntry* entry = &old_entries[i];
        if(IS_EMPTY(entry->key)) continue;

        map_set(vm,map,entry->key,entry->value);
    }
    FREE_ARRAY(vm,MapEntry,old_entries,old_mask + 1);
}

bool map_set(LnVM* vm, ObjMap* map, Value key, Value value){
    if(map->count +1 > (map->capacity_mask + 1) * TABLE_MAX_LOAD) {
        int capacity_mask = GROW_CAPACITY(map->capacity_mask + 1) - 1;
        adjust_map_capacity(vm,map,capacity_mask);
    }
    uint32_t index = hash_value(key)& map->capacity_mask;
    MapEntry* bucket;
    bool is_new_key = false;


    MapEntry entry;
    entry.key= key;
    entry.value = value;
    entry.ps1 = 0;
    while (true){
        bucket = &map->entries[index];
        if(IS_EMPTY(bucket->key)){
            is_new_key = true;
            break;
        } else{
            if(values_equal(key,bucket->key)) break;

            if(entry.ps1 > bucket->ps1){
                is_new_key = true;
                MapEntry tmp = entry;
                entry = *bucket;
                *bucket = tmp;
            }
        }
        index = (index + 1) & map->capacity_mask;
        entry.ps1++;
    }
    *bucket = entry;
    if(is_new_key) map->count++;

    return is_new_key;
}

bool map_get(ObjMap* map, Value key,Value* value){
    if(map->count == 0) return false;

    MapEntry* entry;
    uint32_t index = hash_value(key)& map->capacity_mask;
    uint32_t ps1 = 0;

    while (true){
        entry = &map->entries[index];
        if(IS_EMPTY(entry->key) || ps1 > entry->ps1) return false;
        if(values_equal(key,entry->key)) break;

        index = (index + 1) & map->capacity_mask;
        ps1++;
    }

    *value = entry->value;

    return true;
}

bool map_delete(LnVM* vm, ObjMap* map, Value key){
    if(map->count == 0) return false;

    int capacity_mask = map->capacity_mask;
    uint32_t index = hash_value(key) & capacity_mask;
    uint32_t ps1 = 0;
    MapEntry* entry;

    while (true){
        entry = &map->entries[index];
        if(IS_EMPTY(entry->key) || ps1 > entry->ps1) return false;

        if(values_equal(key,entry->key)) break;

        index = (index + 1) & capacity_mask;
        ps1++;
    }

    map->count--;

    while (true){
        MapEntry* next_entry;
        entry->key = EMPTY_VAL;
        entry->value = NIL_VAL;

        index = (index + 1) & capacity_mask;
        next_entry = &map->entries[index];

        if(IS_EMPTY(next_entry->key) || next_entry->ps1 == 0)break;

        next_entry->ps1--;
        *entry = *next_entry;
        entry = next_entry;
    }
    if(map->count -1 < map->capacity_mask * TABLE_MIN_LOAD){
        capacity_mask = SHRINK_CAPACITY(map->capacity_mask + 1) - 1;
        adjust_map_capacity(vm,map,capacity_mask);
    }
    return true;
}

static char* value_to_string(Value value){
    //TODO:print values
    return "TODO";
}

void print_value(Value value){
    char *output = value_to_string(value);
    fprintf(stderr, "%s", output);
    free(output);
}