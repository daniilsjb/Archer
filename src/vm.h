#pragma once

#include "chunk.h"
#include "stack.h"
#include "value.h"
#include "object.h"
#include "table.h"

#define FRAMES_MAX 64

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    size_t frameCount;
    ObjUpvalue* openUpvalues;

    Stack stack;
    Obj* objects;
    Table globals;
    Table strings;
    ObjString* initString;

    size_t bytesAllocated;
    size_t nextGC;

    size_t grayCount;
    size_t grayCapacity;
    Obj** grayStack;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretStatus;

extern VM vm;

void vm_init();
void vm_free();

void vm_push(Value value);
Value vm_pop();

InterpretStatus vm_interpret(const char* source);
