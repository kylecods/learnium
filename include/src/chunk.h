#ifndef file_chunk_h
#define file_chunk_h

#include "value.h"

typedef struct{
  int count;
  int capacity;
  uint8_t *code;
  int *lines;
  ValueArray constants;
}Chunk;

void init_chunk(Chunk* chunk);

void free_chunk(LnVM* vm,Chunk* chunk);

void write_chunk(LnVM* vm,Chunk* chunk, uint8_t byte, int line);

int add_constant(LnVM* vm,Chunk* chunk,Value value);


#endif // file_chunk_h
