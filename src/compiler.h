#ifndef COMPILER_H
#define COMPILER_H

#include "common.h"
#include "object.h"
#include "chunk.h"

ObjFunction* compile(const char* source);

void mark_compiler_roots();

#endif
