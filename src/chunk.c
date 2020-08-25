#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "vm.h"

void chunk_init(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

    VECTOR_INIT(LineArray, &chunk->lines);
    VECTOR_INIT(ValueArray, &chunk->constants);
}

void chunk_free(VM* vm, Chunk* chunk)
{
    FREE_ARRAY(vm, uint8_t, chunk->code, chunk->capacity);
    VECTOR_FREE(vm, LineArray, &chunk->lines, Line);
    VECTOR_FREE(vm, ValueArray, &chunk->constants, Value);
    chunk_init(chunk);
}

static void append_line(VM* vm, LineArray* array, int line)
{
    if (array->count > 0) {
        Line* currentLine = &array->data[array->count - 1];
        if (currentLine->number == line) {
            currentLine->count++;
            return;
        }
    }

    VECTOR_PUSH(vm, LineArray, array, Line, ((Line) { .number = line, .count = 1 }));
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

    append_line(vm, &chunk->lines, line);
}

uint8_t chunk_add_constant(VM* vm, Chunk* chunk, Value constant)
{
    vm_push(vm, constant);
    VECTOR_PUSH(vm, ValueArray, &chunk->constants, Value, constant);
    vm_pop(vm);
    return (uint8_t)(chunk->constants.count - 1);
}

int chunk_get_line(Chunk* chunk, size_t offset)
{
    size_t index = 0;
    size_t span = 0;

    while (true) {
        span += chunk->lines.data[index].count;

        if (span > offset) {
            return chunk->lines.data[index].number;
        }

        index++;
    }
}
