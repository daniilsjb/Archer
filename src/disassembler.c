#include <stdio.h>

#include "disassembler.h"
#include "object.h"
#include "objfunction.h"

void disassemble_chunk(Chunk* chunk, const char* name)
{
    printf("Chunk: %s\n", name);

    uint32_t offset = 0;
    while (offset < (uint32_t)chunk->count) {
        offset = disassemble_instruction(chunk, offset);
    }
}

static uint32_t simple_instruction(const char* name, uint32_t offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static uint32_t constant_instruction(const char* name, Chunk* chunk, uint32_t offset)
{
    uint8_t location = chunk->code[offset + 1];
    printf("%-18s %4d '", name, location);
    print_value(chunk->constants.data[location]);
    printf("'\n");
    return offset + 2;
}

static uint32_t byte_instruction(const char* name, Chunk* chunk, uint32_t offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-18s %4d\n", name, slot);
    return offset + 2;
}

static uint32_t invoke_instruction(const char* name, Chunk* chunk, uint32_t offset)
{
    uint8_t constant = chunk->code[offset + 1];
    uint8_t argCount = chunk->code[offset + 2];
    printf("%-18s %4d (%d args) '", name, constant, argCount);
    print_value(chunk->constants.data[constant]);
    printf("'\n");
    return offset + 3;
}

static uint32_t jump_instruction(const char* name, char sign, Chunk* chunk, uint32_t offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 0) | (uint16_t)(chunk->code[offset + 2] << 8);
    printf("%-18s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static uint32_t closure_instruction(Chunk* chunk, uint32_t offset)
{
    uint32_t currentOffset = offset;
    currentOffset++;

    uint8_t constant = chunk->code[currentOffset++];
    printf("%-18s %4d ", "CLOSURE", constant);
    print_value(chunk->constants.data[constant]);
    printf("\n");

    ObjFunction* function = AS_FUNCTION(chunk->constants.data[constant]);
    for (size_t j = 0; j < function->upvalueCount; j++) {
        uint8_t isLocal = chunk->code[currentOffset++];
        uint8_t index = chunk->code[currentOffset++];
        printf("%04d    |                     %s %d\n", currentOffset - 2, isLocal ? "local" : "upvalue", index);
    }

    return currentOffset;
}

static uint32_t unknown_instruction(uint8_t instruction, uint32_t offset)
{
    printf("Unknown Code: %d\n", instruction);
    return offset + 1;
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
        case OP_LOAD_CONSTANT:
            return constant_instruction("LOAD_CONSTANT", chunk, offset);
        case OP_LOAD_TRUE:
            return simple_instruction("LOAD_TRUE", offset);
        case OP_LOAD_FALSE:
            return simple_instruction("LOAD_FALSE", offset);
        case OP_LOAD_NIL:
            return simple_instruction("LOAD_NIL", offset);
        case OP_NOT_EQUAL:
            return simple_instruction("NOT_EQUAL", offset);
        case OP_EQUAL:
            return simple_instruction("EQUAL", offset);
        case OP_GREATER:
            return simple_instruction("GREATER", offset);
        case OP_GREATER_EQUAL:
            return simple_instruction("GREATER_EQUAL", offset);
        case OP_LESS:
            return simple_instruction("LESS", offset);
        case OP_LESS_EQUAL:
            return simple_instruction("LESS_EQUAL", offset);
        case OP_NOT:
            return simple_instruction("NOT", offset);
        case OP_NEGATE:
            return simple_instruction("NEGATE", offset);
        case OP_INC:
            return simple_instruction("INC", offset);
        case OP_DEC:
            return simple_instruction("DEC", offset);
        case OP_ADD:
            return simple_instruction("ADD", offset);
        case OP_SUBTRACT:
            return simple_instruction("SUBTRACT", offset);
        case OP_MULTIPLY:
            return simple_instruction("MULTIPLY", offset);
        case OP_DIVIDE:
            return simple_instruction("DIVIDE", offset);
        case OP_MODULO:
            return simple_instruction("MODULO", offset);
        case OP_POWER:
            return simple_instruction("POWER", offset);
        case OP_BITWISE_NOT:
            return simple_instruction("BITWISE_NOT", offset);
        case OP_BITWISE_AND:
            return simple_instruction("BITWISE_AND", offset);
        case OP_BITWISE_OR:
            return simple_instruction("BITWISE_OR", offset);
        case OP_BITWISE_XOR:
            return simple_instruction("BITWISE_XOR", offset);
        case OP_BITWISE_LEFT_SHIFT:
            return simple_instruction("BITWISE_LEFT_SHIFT", offset);
        case OP_BITWISE_RIGHT_SHIFT:
            return simple_instruction("BITWISE_RIGHT_SHIFT", offset);
        case OP_PRINT:
            return simple_instruction("PRINT", offset);
        case OP_LOOP:
            return jump_instruction("LOOP", -1, chunk, offset);
        case OP_JUMP:
            return jump_instruction("JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jump_instruction("JUMP_IF_FALSE", 1, chunk, offset);
        case OP_POP_JUMP_IF_FALSE:
            return jump_instruction("POP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_POP_JUMP_IF_EQUAL:
            return jump_instruction("POP_JUMP_IF_EQUAL", 1, chunk, offset);
        case OP_JUMP_IF_NOT_NIL:
            return jump_instruction("JUMP_IF_NOT_NIL", 1, chunk, offset);
        case OP_POP:
            return simple_instruction("POP", offset);
        case OP_DUP:
            return simple_instruction("DUP", offset);
        case OP_SWAP:
            return simple_instruction("SWAP", offset);
        case OP_SWAP_THREE:
            return simple_instruction("SWAP_THREE", offset);
        case OP_DEFINE_GLOBAL:
            return constant_instruction("DEFINE_GLOBAL", chunk, offset);
        case OP_LOAD_GLOBAL:
            return constant_instruction("LOAD_GLOBAL", chunk, offset);
        case OP_STORE_GLOBAL:
            return constant_instruction("STORE_GLOBAL", chunk, offset);
        case OP_LOAD_LOCAL:
            return byte_instruction("LOAD_LOCAL", chunk, offset);
        case OP_STORE_LOCAL:
            return byte_instruction("STORE_LOCAL", chunk, offset);
        case OP_LOAD_UPVALUE:
            return byte_instruction("LOAD_UPVALUE", chunk, offset);
        case OP_STORE_UPVALUE:
            return byte_instruction("STORE_UPVALUE", chunk, offset);
        case OP_LOAD_PROPERTY:
            return constant_instruction("LOAD_PROPERTY", chunk, offset);
        case OP_LOAD_PROPERTY_SAFE:
            return constant_instruction("LOAD_PROPERTY_SAFE", chunk, offset);
        case OP_STORE_PROPERTY:
            return constant_instruction("STORE_PROPERTY", chunk, offset);
        case OP_STORE_PROPERTY_SAFE:
            return constant_instruction("STORE_PROPERTY_SAFE", chunk, offset);
        case OP_CLOSURE:
            return closure_instruction(chunk, offset);
        case OP_CLOSE_UPVALUE:
            return simple_instruction("CLOSE_UPVALUE", offset);
        case OP_CALL:
            return byte_instruction("CALL", chunk, offset);
        case OP_INVOKE:
            return invoke_instruction("INVOKE", chunk, offset);
        case OP_INVOKE_SAFE:
            return invoke_instruction("INVOKE_SAFE", chunk, offset);
        case OP_RETURN:
            return simple_instruction("RETURN", offset);
        case OP_CLASS:
            return constant_instruction("CLASS", chunk, offset);
        case OP_METHOD:
            return constant_instruction("METHOD", chunk, offset);
        case OP_STATIC_METHOD:
            return constant_instruction("STATIC_METHOD", chunk, offset);
        case OP_INHERIT:
            return simple_instruction("INHERIT", offset);
        case OP_GET_SUPER:
            return constant_instruction("GET_SUPER", chunk, offset);
        case OP_SUPER_INVOKE:
            return invoke_instruction("SUPER_INVOKE", chunk, offset);
        case OP_END_CLASS:
            return simple_instruction("END_CLASS", offset);
        default:
            return unknown_instruction(instruction, offset);
    }
}