#ifndef VM_H
#define VM_H

#include "compiler.h"
#include "chunk.h"
#include "value.h"
#include "object.h"
#include "table.h"

#define STACK_MAX 512
#define FRAMES_MAX 64

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct VM {
    struct Compiler* compiler;
    struct ClassCompiler* classCompiler;

    CallFrame frames[FRAMES_MAX];
    size_t frameCount;
    ObjUpvalue* openUpvalues;

    Value stack[STACK_MAX];
    Value* stackTop;

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

void vm_init(VM* vm);
void vm_free(VM* vm);

void vm_push(VM* vm, Value value);
Value vm_pop(VM* vm);

InterpretStatus vm_interpret(VM* vm, const char* source);

#endif
