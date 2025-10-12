#include "ln.h"

MapEntry* map_find_entry(ObjMap* map, int capacity, Value key){
	uint32_t index = hash_value(key) & capacity;
	MapEntry* tombstone = NULL;
	for(;;){
		MapEntry* entry = &map->entries[index];
		if(IS_EMPTY(entry->key)){
			if(IS_EMPTY(entry->value)){
				return tombstone != NULL ? tombstone : entry;
			} else{
				if(tombstone == NULL) tombstone = entry;
			}
		} else if (entry->key == key){
			return entry;
		}
		index = (index + 1) & capacity;
	}
}

void adjust_map_capacity(LnVM* vm, ObjMap* map, int capacity_mask)
{
	//In the future we explore using a memory allocator concept
	MapEntry* entries = ALLOCATE(vm,MapEntry,capacity_mask);

	for (int i = 0; i < capacity_mask; i++) {
		entries[i].key = EMPTY_VAL;
		entries[i].value = NIL_VAL;
	}

	map->count = 0;
	for (int i = 0; i < map->capacity_mask; i++) {
		MapEntry* entry = &map->entries[i];

		if(IS_EMPTY(entry->key)) continue;
		MapEntry* dest = map_find_entry(map, capacity_mask, entry->key);
		dest->key = entry->key;
		dest->value = entry->value;
		map->count++;
	}
	FREE_ARRAY(vm,MapEntry,map->entries,map->capacity_mask);

	map->entries = entries;
	map->capacity_mask = capacity_mask;
}