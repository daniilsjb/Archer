#pragma once

#include "common.h"
#include "value.h"
#include "line.h"

typedef enum {
    OP_CONSTANT,
    OP_TRUE,
    OP_FALSE,
    OP_NIL,
    OP_NOT_EQUAL,
    OP_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_NOT,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,
    OP_BITWISE_NOT,
    OP_BITWISE_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,
    OP_BITWISE_LEFT_SHIFT,
    OP_BITWISE_RIGHT_SHIFT,
    OP_PRINT,
    OP_LOOP,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_NOT_EQUAL,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_LOCAL,
    OP_GET_LOCAL,
    OP_SET_UPVALUE,
    OP_GET_UPVALUE,
    OP_SET_PROPERTY,
    OP_GET_PROPERTY,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_CALL,
    OP_INVOKE,
    OP_RETURN,
    OP_CLASS,
    OP_METHOD,
    OP_INHERIT,
    OP_GET_SUPER,
    OP_SUPER_INVOKE
} OpCode;

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