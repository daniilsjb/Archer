#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "chunk.h"

void disassemble_chunk(Chunk* chunk, const char* name);
uint32_t disassemble_instruction(Chunk* chunk, uint32_t offset);

#endif
