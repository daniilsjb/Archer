#ifndef LIBRARY_H
#define LIBRARY_H

#include <stdbool.h>

#include "value.h"
#include "obj_native.h"

typedef struct VM VM;

bool Library_Error(VM* vm, const char* message, Value* args);
void Library_Init(VM* vm);

void Library_DefineTypeMethod(ObjectType* type, VM* vm, const char* name, NativeFn function, int arity);

#endif
