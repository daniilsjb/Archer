#ifndef LIBRARY_H
#define LIBRARY_H

#include <stdbool.h>

#include "value.h"

typedef struct VM VM;

bool Library_Error(VM* vm, const char* message, Value* args);
void Library_Init(VM* vm);

#endif
