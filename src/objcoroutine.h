#ifndef OBJCOROUTINE_H
#define OBJCOROUTINE_H

#include "object.h"

typedef struct ObjectClosure ObjectClosure;
typedef struct ObjectUpvalue ObjectUpvalue;

#define AS_COROUTINE_FUNCTION(object) ((ObjectCoroutineFunction*)object)
#define IS_COROUTINE_FUNCTION(object, vm) (OBJ_TYPE(object) == vm->coroutineFunctionType)

#define VAL_AS_COROUTINE_FUNCTION(value) (AS_COROUTINE_FUNCTION(AS_OBJ(value)))
#define VAL_IS_COROUTINE_FUNCTION(value, vm) (Object_ValueHasType(value, vm->coroutineFunctionType))

#define ALLOCATE_COROUTINE_FUNCTION(vm) (AS_COROUTINE_FUNCTION(ALLOCATE_OBJ(vm, vm->coroutineFunctionType)))

typedef struct ObjectCoroutineFunction {
    Object base;
    ObjectClosure* closure;
} ObjectCoroutineFunction;

ObjectType* CoroutineFunction_NewType(VM* vm);
void CoroutineFunction_PrepareType(ObjectType* type, VM* vm);

ObjectCoroutineFunction* CoroutineFunction_New(VM* vm, ObjectClosure* closure);

#define AS_COROUTINE(object) ((ObjectCoroutine*)object)
#define IS_COROUTINE(object, vm) (OBJ_TYPE(object) == vm->coroutineType)

#define VAL_AS_COROUTINE(value) (AS_COROUTINE(AS_OBJ(value)))
#define VAL_IS_COROUTINE(value, vm) (Object_ValueHasType(value, vm->coroutineType))

#define ALLOCATE_COROUTINE(vm) (AS_COROUTINE(ALLOCATE_OBJ(vm, vm->coroutineType)))

#define STACK_MAX 512
#define FRAMES_MAX 64

typedef struct {
    ObjectClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct ObjectCoroutine {
    Object base;
    ObjectClosure* closure;

    CallFrame frames[FRAMES_MAX];
    size_t frameCount;

    Value stack[STACK_MAX];
    Value* stackTop;

    ObjectUpvalue* openUpvalues;

    struct ObjectCoroutine* transfer;
    bool started;
} ObjectCoroutine;

ObjectType* Coroutine_NewType(VM* vm);
void Coroutine_PrepareType(ObjectType* type, VM* vm);

void _Coroutine_CallMain(VM* vm, ObjectCoroutine* coroutine);

ObjectCoroutine* Coroutine_New(VM* vm, ObjectClosure* closure);
ObjectCoroutine* Coroutine_NewFromStack(VM* vm, ObjectClosure* closure, Value* slot, uint8_t argCount);

void Coroutine_Error(ObjectCoroutine* coroutine);

bool Coroutine_Call(VM* vm, ObjectCoroutine* coroutine, ObjectClosure* callee, uint8_t argCount);
bool Coroutine_CallValue(VM* vm, ObjectCoroutine* coroutine, Value callee, uint8_t argCount);

bool Coroutine_IsDone(ObjectCoroutine* coroutine);

#endif
