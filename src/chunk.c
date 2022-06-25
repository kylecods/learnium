#include "ln.h"

void init_chunk(Chunk* chunk){
    chunk->count = 0;
    chunk->capacity= 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    init_valueArray(&chunk->constants);
}

void free_chunk(LnVM* vm, Chunk* chunk){
    FREE_ARRAY(vm,uint8_t,chunk->code,chunk->capacity);
    FREE_ARRAY(vm,int,chunk->lines,chunk->capacity);
    free_valueArray(vm,&chunk->constants);
    init_chunk(chunk);
}

void write_chunk(LnVM* vm, Chunk* chunk,uint8_t byte, int line){
    if(chunk->capacity < chunk->count + 1){
        int old_cap = chunk->capacity;
        chunk->capacity - GROW_CAPACITY(old_cap);
        chunk->code = GROW_ARRAY(vm,chunk->code,uint8_t,old_cap,chunk->capacity);

        chunk->lines = GROW_ARRAY(vm,chunk->lines,int,old_cap,chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int add_constant(LnVM* vm, Chunk* chunk,Value value){
    push(vm,value);

    write_valueArray(vm,&chunk->constants,value);
    pop(vm);
    return chunk->constants.count - 1;
}