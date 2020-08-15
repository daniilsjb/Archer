#ifndef COMPILER_H
#define COMPILER_H

#include "object.h"

struct VM;
struct Compiler;
struct ClassCompiler;

ObjFunction* compile(struct VM* vm, const char* source);

void mark_compiler_roots(struct VM* vm);

#endif
