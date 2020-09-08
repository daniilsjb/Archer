#ifndef OBJARRAY_H
#define OBJARRAY_H

#include "object.h"

#define AS_ARRAY(object) ((ObjectArray*)object)
#define IS_ARRAY(object) (OBJ_TYPE(object) == vm->arrayType)

#define VAL_AS_ARRAY(value) (AS_ARRAY(AS_OBJ(value)))
#define VAL_IS_ARRAY(value) (Object_ValueHasType((value), vm->arrayType))

#define ALLOCATE_ARRAY(vm, length)                                                    \
    (AS_ARRAY(Object_Allocate((vm), sizeof(ObjectArray) + sizeof(Value) * (length)))) \

typedef struct ObjectArray {
    Object base;
    size_t length;
    Value elements[];
} ObjectArray;

ObjectType* Array_NewType(VM* vm);
void Array_PrepareType(ObjectType* type, VM* vm);

ObjectArray* Array_New(VM* vm, size_t length);

#endif
