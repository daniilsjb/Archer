#ifndef OBJCOROUTINE_H
#define OBJCOROUTINE_H

#include "object.h"

typedef struct ObjectClosure ObjectClosure;

#define AS_COROUTINE(object) ((ObjectCoroutine*)object)
#define IS_COROUTINE(object, vm) (OBJ_TYPE(object) == vm->coroutineType)

#define VAL_AS_COROUTINE(value) (AS_COROUTINE(AS_OBJ(value)))
#define VAL_IS_COROUTINE(value, vm) (Object_ValueHasType(value, vm->coroutineType))

#define ALLOCATE_COROUTINE(vm) (AS_COROUTINE(ALLOCATE_OBJ(vm, vm->coroutineType)))

#define STACK_MAX 512
#define FRAMES_MAX 64

typedef struct ObjectUpvalue ObjectUpvalue;

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
ObjectCoroutine* Coroutine_NewWithArguments(VM* vm, ObjectClosure* closure, Value* args, size_t argCount);

bool Coroutine_IsDone(ObjectCoroutine* coroutine);

#endif
