#ifndef OBJNATIVE_H
#define OBJNATIVE_H

#include "object.h"

#define AS_NATIVE(object) ((ObjNative*)object)
#define IS_NATIVE(object) (OBJ_TYPE(object) == &NativeType)

#define VAL_AS_NATIVE(value) (AS_NATIVE(AS_OBJ(value)))
#define VAL_IS_NATIVE(value) (value_is_object_of_type(value, &NativeType))

extern ObjectType NativeType;

typedef bool (*NativeFn)(VM* vm, Value* args);

typedef struct ObjNative {
    Object base;
    NativeFn function;
    int arity;
} ObjNative;

ObjNative* new_native(VM* vm, NativeFn function, int arity);

#endif
