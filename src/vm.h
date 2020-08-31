#ifndef VM_H
#define VM_H

#include "gc.h"
#include "compiler.h"
#include "chunk.h"
#include "value.h"
#include "object.h"
#include "table.h"

#define STACK_MAX 512
#define FRAMES_MAX 64

typedef struct Compiler Compiler;
typedef struct ClassCompiler ClassCompiler;

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct VM {
    GC gc;

    Compiler* compiler;
    ClassCompiler* classCompiler;

    CallFrame frames[FRAMES_MAX];
    size_t frameCount;
    ObjUpvalue* openUpvalues;

    Value stack[STACK_MAX];
    Value* stackTop;

    Table globals;
    Table strings;
    ObjString* initString;
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
