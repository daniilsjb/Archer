#ifndef OBJNATIVE_H
#define OBJNATIVE_H

#include "object.h"

#define AS_NATIVE(object) ((ObjNative*)object)
#define IS_NATIVE(object, vm) (OBJ_TYPE(object) == vm->nativeType)

#define VAL_AS_NATIVE(value) (AS_NATIVE(AS_OBJ(value)))
#define VAL_IS_NATIVE(value, vm) (value_is_object_of_type(value, vm->nativeType))

#define ALLOCATE_NATIVE(vm) (ALLOCATE_OBJ(vm, ObjNative, vm->nativeType))

typedef bool (*NativeFn)(VM* vm, Value* args);

typedef struct ObjNative {
    Object base;
    NativeFn function;
    int arity;
} ObjNative;

ObjectType* new_native_type(VM* vm);
void free_native_type(ObjectType* type, VM* vm);

ObjNative* new_native(VM* vm, NativeFn function, int arity);

#endif
