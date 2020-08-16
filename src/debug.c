#include <stdio.h>

#include "debug.h"
#include "object.h"

static uint32_t simple_instruction(const char* name, uint32_t offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static uint32_t constant_instruction(const char* name, Chunk* chunk, uint32_t offset)
{
    uint8_t location = chunk->code[offset + 1];
    printf("%-16s %4d '", name, location);
    print_value(chunk->constants.data[location]);
    printf("'\n");
    return offset + 2;
}

static uint32_t byte_instruction(const char* name, Chunk* chunk, uint32_t offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static uint32_t invoke_instruction(const char* name, Chunk* chunk, uint32_t offset)
{
    uint8_t constant = chunk->code[offset + 1];
    uint8_t argCount = chunk->code[offset + 2];
    printf("%-16s (%d args) %4d '", name, argCount, constant);
    print_value(chunk->constants.data[constant]);
    printf("'\n");
    return offset + 3;
}

static uint32_t jump_instruction(const char* name, int sign, Chunk* chunk, uint32_t offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 0) | (uint16_t)(chunk->code[offset + 2] << 8);
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static uint32_t closure_instruction(Chunk* chunk, uint32_t offset)
{
    uint32_t currentOffset = offset;
    currentOffset++;

    uint8_t constant = chunk->code[currentOffset++];
    printf("%-16s %4d ", "OP_CLOSURE", constant);
    print_value(chunk->constants.data[constant]);
    printf("\n");

    ObjFunction* function = AS_FUNCTION(chunk->constants.data[constant]);
    for (size_t j = 0; j < function->upvalueCount; j++) {
        uint8_t isLocal = chunk->code[currentOffset++];
        uint8_t index = chunk->code[currentOffset++];
        printf("%04d      |                     %s %d\n", currentOffset - 2, isLocal ? "local" : "upvalue", index);
    }

    return currentOffset;
}

static uint32_t unknown_instruction(uint8_t instruction, uint32_t offset)
{
    printf("Unknown OpCode: %d\n", instruction);
    return offset + 1;
}

void disassemble_chunk(Chunk* chunk, const char* name)
{
    printf("Chunk: %s\n", name);

    uint32_t offset = 0;
    while (offset < (uint32_t)chunk->count) {
        offset = disassemble_instruction(chunk, offset);
    }
}

uint32_t disassemble_instruction(Chunk* chunk, uint32_t offset)
{
    printf("%04d ", offset);

    uint32_t previousLine = chunk_get_line(chunk, offset - 1);
    uint32_t currentLine = chunk_get_line(chunk, offset);

    if (offset > 0 && currentLine == previousLine) {
        printf("   | ");
    } else {
        printf("%4d ", currentLine);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constant_instruction("OP_CONSTANT", chunk, offset);
        case OP_TRUE:
            return simple_instruction("OP_TRUE", offset);
        case OP_FALSE:
            return simple_instruction("OP_FALSE", offset);
        case OP_NIL:
            return simple_instruction("OP_NIL", offset);
        case OP_NOT_EQUAL:
            return simple_instruction("OP_NOT_EQUAL", offset);
        case OP_EQUAL:
            return simple_instruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simple_instruction("OP_GREATER", offset);
        case OP_GREATER_EQUAL:
            return simple_instruction("OP_GREATER_EQUAL", offset);
        case OP_LESS:
            return simple_instruction("OP_LESS", offset);
        case OP_LESS_EQUAL:
            return simple_instruction("OP_LESS_EQUAL", offset);
        case OP_NOT:
            return simple_instruction("OP_NOT", offset);
        case OP_NEGATE:
            return simple_instruction("OP_NEGATE", offset);
        case OP_ADD:
            return simple_instruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simple_instruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simple_instruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simple_instruction("OP_DIVIDE", offset);
        case OP_MODULO:
            return simple_instruction("OP_MODULO", offset);
        case OP_BITWISE_NOT:
            return simple_instruction("OP_BITWISE_NOT", offset);
        case OP_BITWISE_AND:
            return simple_instruction("OP_BITWISE_AND", offset);
        case OP_BITWISE_OR:
            return simple_instruction("OP_BITWISE_OR", offset);
        case OP_BITWISE_XOR:
            return simple_instruction("OP_BITWISE_XOR", offset);
        case OP_BITWISE_LEFT_SHIFT:
            return simple_instruction("OP_BITWISE_LEFT_SHIFT", offset);
        case OP_BITWISE_RIGHT_SHIFT:
            return simple_instruction("OP_BITWISE_RIGHT_SHIFT", offset);
        case OP_PRINT:
            return simple_instruction("OP_PRINT", offset);
        case OP_LOOP:
            return jump_instruction("OP_LOOP", -1, chunk, offset);
        case OP_JUMP:
            return jump_instruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_JUMP_IF_NOT_EQUAL:
            return jump_instruction("OP_JUMP_IF_NOT_EQUAL", 1, chunk, offset);
        case OP_POP:
            return simple_instruction("OP_POP", offset);
        case OP_DEFINE_GLOBAL:
            return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constant_instruction("OP_SET_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constant_instruction("OP_GET_GLOBAL", chunk, offset);
        case OP_SET_LOCAL:
            return byte_instruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_LOCAL:
            return byte_instruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_UPVALUE:
            return byte_instruction("OP_SET_UPVALUE", chunk, offset);
        case OP_GET_UPVALUE:
            return byte_instruction("OP_GET_UPVALUE", chunk, offset);
        case OP_SET_PROPERTY:
            return constant_instruction("OP_SET_PROPERTY", chunk, offset);
        case OP_GET_PROPERTY:
            return constant_instruction("OP_GET_PROPERTY", chunk, offset);
        case OP_CLOSURE:
            return closure_instruction(chunk, offset);
        case OP_CLOSE_UPVALUE:
            return simple_instruction("OP_CLOSE_UPVALUE", offset);
        case OP_CALL:
            return byte_instruction("OP_CALL", chunk, offset);
        case OP_INVOKE:
            return invoke_instruction("OP_INVOKE", chunk, offset);
        case OP_RETURN:
            return simple_instruction("OP_RETURN", offset);
        case OP_CLASS:
            return constant_instruction("OP_CLASS", chunk, offset);
        case OP_METHOD:
            return constant_instruction("OP_METHOD", chunk, offset);
        case OP_INHERIT:
            return simple_instruction("OP_INHERIT", offset);
        case OP_GET_SUPER:
            return constant_instruction("OP_GET_SUPER", chunk, offset);
        case OP_SUPER_INVOKE:
            return invoke_instruction("OP_SUPER_INVOKE", chunk, offset);
        default:
            return unknown_instruction(instruction, offset);
    }
}