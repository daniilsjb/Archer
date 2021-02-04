#ifndef COMPILER_H
#define COMPILER_H

typedef struct ObjectFunction ObjectFunction;
typedef struct ObjectModule ObjectModule;
typedef struct VM VM;

ObjectFunction* Compiler_Compile(VM* vm, const char* source, ObjectModule* mod);

void Compiler_MarkRoots(VM* vm);

#endif
