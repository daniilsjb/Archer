#ifndef VM_H
#define VM_H

#include "gc.h"
#include "value.h"
#include "table.h"

#define STACK_MAX 512
#define FRAMES_MAX 64

typedef struct ObjectClosure ObjectClosure;
typedef struct ObjectUpvalue ObjectUpvalue;
typedef struct Compiler Compiler;
typedef struct ClassCompiler ClassCompiler;

typedef struct ObjectType ObjectType;

typedef struct {
    ObjectClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct VM {
    GC gc;

    Compiler* compiler;
    ClassCompiler* classCompiler;

    ObjectType* stringType;
    ObjectType* functionType;
    ObjectType* upvalueType;
    ObjectType* closureType;
    ObjectType* nativeType;
    ObjectType* boundMethodType;
    ObjectType* listType;

    CallFrame frames[FRAMES_MAX];
    size_t frameCount;
    ObjectUpvalue* openUpvalues;

    Value stack[STACK_MAX];
    Value* stackTop;

    Table globals;
    Table strings;
    ObjectString* initString;
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

bool call(VM* vm, ObjectClosure* closure, uint8_t argCount);

InterpretStatus runtime_error(VM* vm, const char* format, ...);

#endif
