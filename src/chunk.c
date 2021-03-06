#include <stdlib.h>

#include "chunk.h"
#include "memory.h"
#include "vm.h"

void Chunk_Init(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

    VECTOR_INIT(LineArray, &chunk->lines);
    VECTOR_INIT(ValueArray, &chunk->constants);
}

void Chunk_Free(GC* gc, Chunk* chunk)
{
    FREE_ARRAY(gc, uint8_t, chunk->code, chunk->capacity);
    VECTOR_FREE(gc, LineArray, &chunk->lines, Line);
    VECTOR_FREE(gc, ValueArray, &chunk->constants, Value);
    Chunk_Init(chunk);
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

    VECTOR_PUSH(&vm->gc, LineArray, array, Line, ((Line) { .number = line, .count = 1 }));
}

void Chunk_Write(VM* vm, Chunk* chunk, uint8_t byte, int line)
{
    if (chunk->count >= chunk->capacity) {
        size_t oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(&vm->gc, uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;

    append_line(vm, &chunk->lines, line);
}

uint8_t Chunk_AddConst(VM* vm, Chunk* chunk, Value constant)
{
    Vm_PushTemporary(vm, constant);
    VECTOR_PUSH(&vm->gc, ValueArray, &chunk->constants, Value, constant);
    Vm_PopTemporary(vm);
    return (uint8_t)(chunk->constants.count - 1);
}

int Chunk_GetLine(Chunk* chunk, size_t offset)
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
