#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "vm.h"

void chunk_init(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

    line_array_init(&chunk->lines);
    value_array_init(&chunk->constants);
}

void chunk_write(Chunk* chunk, uint8_t byte, int line)
{
    if (chunk->count >= chunk->capacity) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;

    line_array_write(&chunk->lines, line);
}

uint8_t chunk_add_constant(Chunk* chunk, Value constant)
{
    vm_push(constant);
    value_array_write(&chunk->constants, constant);
    vm_pop();
    return chunk->constants.count - 1;
}

void chunk_write_constant(Chunk* chunk, Value value, int line)
{
    uint8_t location = chunk_add_constant(chunk, value);
    chunk_write(chunk, OP_CONSTANT, line);
    chunk_write(chunk, location, line);
}

void chunk_free(Chunk* chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    line_array_free(&chunk->lines);
    value_array_free(&chunk->constants);
    chunk_init(chunk);
}

int chunk_get_line(Chunk* chunk, int offset)
{
    LineArray* array = &chunk->lines;

    int index = 0;
    int span = 0;

    while (true) {
        span += array->lines[index].count;

        if (span > offset) {
            return array->lines[index].number;
        }

        index++;
    }
}
