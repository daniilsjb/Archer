#ifndef OBJNATIVE_H
#define OBJNATIVE_H

#include "object.h"

#define AS_NATIVE(object) ((ObjectNative*)object)
#define IS_NATIVE(object, vm) (OBJ_TYPE(object) == vm->nativeType)

#define VAL_AS_NATIVE(value) (AS_NATIVE(AS_OBJ(value)))
#define VAL_IS_NATIVE(value, vm) (Object_ValueHasType(value, vm->nativeType))

#define ALLOCATE_NATIVE(vm) (AS_NATIVE(ALLOCATE_OBJ(vm, vm->nativeType)))

typedef bool (*NativeFn)(VM* vm, Value* args);

typedef struct ObjectNative {
    Object base;
    NativeFn function;
    int arity;
} ObjectNative;

ObjectType* Native_NewType(VM* vm);
void Native_PrepareType(ObjectType* type, VM* vm);

ObjectNative* Native_New(VM* vm, NativeFn function, int arity);

#endif
