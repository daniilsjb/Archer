#ifndef COMPILER_H
#define COMPILER_H

typedef struct ObjectFunction ObjectFunction;
typedef struct VM VM;

ObjectFunction* compile(VM* vm, const char* source);

void mark_compiler_roots(VM* vm);

#endif
