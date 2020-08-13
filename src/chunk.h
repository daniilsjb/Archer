#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"
#include "value.h"
#include "line.h"
#include "opcode.h"

typedef struct {
    uint32_t count;
    uint32_t capacity;
    uint8_t* code;
    LineArray lines;
    ValueArray constants;
} Chunk;

void chunk_init(Chunk* chunk);
void chunk_write(Chunk* chunk, uint8_t byte, int line);
uint8_t chunk_add_constant(Chunk* chunk, Value constant);
void chunk_free(Chunk* chunk);
int chunk_get_line(Chunk* chunk, int offset);

#endif
