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
    ObjectType* moduleType;
    ObjectType* iteratorType;
    ObjectType* rangeType;
    ObjectType* tupleType;

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

void Vm_Init(VM* vm);
void Vm_Free(VM* vm);

void Vm_Push(VM* vm, Value value);
Value Vm_Pop(VM* vm);
Value Vm_Peek(VM* vm, int distance);

void Vm_PushTemporary(VM* vm, Value value);
Value Vm_PopTemporary(VM* vm);
Value Vm_PeekTemporary(VM* vm, int distance);

InterpretStatus Vm_Interpret(VM* vm, const char* source, const char* path);

bool Vm_Call(VM* vm, ObjectClosure* closure, uint8_t argCount);

InterpretStatus Vm_RuntimeError(VM* vm, const char* format, ...);

#endif
