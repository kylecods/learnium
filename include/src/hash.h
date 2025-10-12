#ifndef file_hash_h
#define file_hash_h

#include "ln.h"

uint32_t hash_value(Value value);
uint32_t hash_object(Obj* object);
uint32_t hash_string(const char* key, int length);

static inline uint32_t hash_bits(uint64_t hash){
	hash = ~hash + (hash << 18);
	hash = hash ^ (hash >> 31);
	hash = hash * 21;
	hash = hash ^ (hash >> 11);
	hash = hash + (hash << 6);
	hash = hash ^ (hash >> 22);
	return (uint32_t) (hash & 0x3fffffff);
}

#endif