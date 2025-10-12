#include "ln.h"

uint32_t hash_string(const char* key, int length){
	uint32_t hash = 2166136261u;
	for(int i = 0; i< length; i++){
		hash ^= (uint8_t)key[i];
		hash *= 16777619;
	}
	return hash;
}

uint32_t hash_object(Obj* object){
	switch (object->type){
		case OBJ_STRING: return ((ObjString*) object)->hash;
		default: {
			uint64_t hash = (uint64_t)(&object);

			return hash_bits(hash);
		}
	}
}

uint32_t hash_value(Value value){
	if(IS_OBJ(value)) return hash_object(AS_OBJ(value));
	return hash_bits(value);
}