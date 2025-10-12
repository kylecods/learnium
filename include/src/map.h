#ifndef file_map_h
#define file_map_h

#include "ln.h"


MapEntry* map_find_entry(ObjMap* map, int capacity, Value key);

void adjust_map_capacity(LnVM* vm, ObjMap* map, int capacity);

bool map_get(ObjMap* map, Value key, Value* value);

bool map_set(LnVM* vm, ObjMap* map, Value key, Value value);

bool map_delete(LnVM* vm, ObjMap* map, Value key);



#endif // !file_map_h
