#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"
#include "opcode.h"
#include "value.h"
#include "line.h"

typedef struct {
    size_t count;
    size_t capacity;
    uint8_t* code;

    LineArray lines;
    ValueArray constants;
} Chunk;

void chunk_init(Chunk* chunk);
void chunk_free(Chunk* chunk);

void chunk_write(Chunk* chunk, uint8_t byte, int line);
uint8_t chunk_add_constant(Chunk* chunk, Value constant);

int chunk_get_line(Chunk* chunk, size_t offset);

#endif
