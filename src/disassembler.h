#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "chunk.h"

void Disassembler_DisChunk(Chunk* chunk, const char* name);
uint32_t Disassembler_DisInstruction(Chunk* chunk, uint32_t offset);

#endif
