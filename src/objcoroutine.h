#ifndef OBJCOROUTINE_H
#define OBJCOROUTINE_H

#include "object.h"

typedef struct ObjectClosure ObjectClosure;

#define AS_COROUTINE(object) ((ObjectCoroutine*)object)
#define IS_COROUTINE(object, vm) (OBJ_TYPE(object) == vm->coroutineType)

#define VAL_AS_COROUTINE(value) (AS_COROUTINE(AS_OBJ(value)))
#define VAL_IS_COROUTINE(value, vm) (Object_ValueHasType(value, vm->coroutineType))

#define ALLOCATE_COROUTINE(vm) (AS_COROUTINE(ALLOCATE_OBJ(vm, vm->coroutineType)))

typedef struct ObjectCoroutine {
    Object base;
    ObjectClosure* closure;
} ObjectCoroutine;

ObjectType* Coroutine_NewType(VM* vm);
void Coroutine_PrepareType(ObjectType* type, VM* vm);

ObjectCoroutine* Coroutine_New(VM* vm, ObjectClosure* closure);

#define AS_COROUTINE_INSTANCE(object) ((ObjectCoroutineInstance*)object)
#define IS_COROUTINE_INSTANCE(object, vm) (OBJ_TYPE(object) == vm->coroutineInstanceType)

#define VAL_AS_COROUTINE_INSTANCE(value) (AS_COROUTINE_INSTANCE(AS_OBJ(value)))
#define VAL_IS_COROUTINE_INSTANCE(value, vm) (Object_ValueHasType(value, vm->coroutineInstanceType))

#define ALLOCATE_COROUTINE_INSTANCE(vm) (AS_COROUTINE_INSTANCE(ALLOCATE_OBJ(vm, vm->coroutineInstanceType)))

typedef struct ObjectCoroutineInstance {
    Object base;

    ObjectClosure* closure;
    uint8_t* ip;

    Value* stackCopy;
    size_t stackSize;

    bool started;
    bool done;
} ObjectCoroutineInstance;

ObjectType* CoroutineInstance_NewType(VM* vm);
void CoroutineInstance_PrepareType(ObjectType* type, VM* vm);

ObjectCoroutineInstance* CoroutineInstance_New(VM* vm, ObjectClosure* closure);

#endif
