#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"
#include "opcode.h"
#include "value.h"
#include "vector.h"

typedef struct VM VM;

typedef struct {
    int number;
    int count;
} Line;

typedef VECTOR(Line) LineArray;

typedef struct {
    size_t count;
    size_t capacity;
    uint8_t* code;

    LineArray lines;
    ValueArray constants;
} Chunk;

void Chunk_Init(Chunk* chunk);
void Chunk_Free(GC* gc, Chunk* chunk);

void Chunk_Write(VM* vm, Chunk* chunk, uint8_t byte, int line);
uint8_t Chunk_AddConst(VM* vm, Chunk* chunk, Value constant);

int Chunk_GetLine(Chunk* chunk, size_t offset);

#endif
