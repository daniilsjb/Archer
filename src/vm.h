#ifndef VM_H
#define VM_H

#include "gc.h"
#include "value.h"
#include "table.h"

#define TEMP_MAX 64

typedef struct ObjectClosure ObjectClosure;
typedef struct Compiler Compiler;
typedef struct ClassCompiler ClassCompiler;
typedef struct ObjectCoroutine ObjectCoroutine;
typedef struct ObjectModule ObjectModule;

typedef struct ObjectType ObjectType;

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
    ObjectType* coroutineFunctionType;
    ObjectType* coroutineType;
    ObjectType* listType;
    ObjectType* mapType;
    ObjectType* arrayType;
    ObjectType* moduleType;
    ObjectType* iteratorType;
    ObjectType* rangeType;

    ObjectModule* mainModule;

    ObjectModule* moduleRegister;
    ObjectCoroutine* coroutine;

    Table modules;
    Table builtins;
    Table strings;
    ObjectString* initString;

    Value temporaries[TEMP_MAX];
    size_t temporaryCount;
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
Value vm_peek(VM* vm, int distance);

void vm_push_temporary(VM* vm, Value value);
Value vm_pop_temporary(VM* vm);
Value vm_peek_temporary(VM* vm, int distance);

InterpretStatus vm_interpret(VM* vm, const char* source, const char* path);

bool call(VM* vm, ObjectClosure* closure, uint8_t argCount);

InterpretStatus runtime_error(VM* vm, const char* format, ...);

#endif
