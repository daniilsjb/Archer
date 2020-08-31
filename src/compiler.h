#ifndef COMPILER_H
#define COMPILER_H

typedef struct ObjFunction ObjFunction;
typedef struct VM VM;

ObjFunction* compile(VM* vm, const char* source);

void mark_compiler_roots(VM* vm);

#endif
