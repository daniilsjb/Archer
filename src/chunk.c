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

void chunk_free(VM* vm, Chunk* chunk)
{
    FREE_ARRAY(vm, uint8_t, chunk->code, chunk->capacity);
    line_array_free(vm, &chunk->lines);
    value_array_free(vm, &chunk->constants);
    chunk_init(chunk);
}

void chunk_write(VM* vm, Chunk* chunk, uint8_t byte, int line)
{
    if (chunk->count >= chunk->capacity) {
        size_t oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(vm, uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;

    line_array_write(vm, &chunk->lines, line);
}

uint8_t chunk_add_constant(VM* vm, Chunk* chunk, Value constant)
{
    vm_push(vm, constant);
    value_array_write(vm, &chunk->constants, constant);
    vm_pop(vm);
    return (uint8_t)(chunk->constants.count - 1);
}

int chunk_get_line(Chunk* chunk, size_t offset)
{
    LineArray* array = &chunk->lines;

    size_t index = 0;
    size_t span = 0;

    while (true) {
        span += array->lines[index].count;

        if (span > offset) {
            return array->lines[index].number;
        }

        index++;
    }
}
