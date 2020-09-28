#ifndef COMPILER_H
#define COMPILER_H

typedef struct ObjectFunction ObjectFunction;
typedef struct ObjectModule ObjectModule;
typedef struct VM VM;

ObjectFunction* compile(VM* vm, const char* source, ObjectModule* mod);

void mark_compiler_roots(VM* vm);

#endif
