#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"
#include "opcode.h"
#include "value.h"

struct VM;

typedef struct {
    int number;
    int count;
} Line;

typedef struct {
    size_t count;
    size_t capacity;
    Line* lines;
} LineArray;

typedef struct {
    size_t count;
    size_t capacity;
    uint8_t* code;

    LineArray lines;
    ValueArray constants;
} Chunk;

void chunk_init(Chunk* chunk);
void chunk_free(struct VM* vm, Chunk* chunk);

void chunk_write(struct VM* vm, Chunk* chunk, uint8_t byte, int line);
uint8_t chunk_add_constant(struct VM* vm, Chunk* chunk, Value constant);

int chunk_get_line(Chunk* chunk, size_t offset);

#endif
