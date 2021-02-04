#ifndef OBJTUPLE_H
#define OBJTUPLE_H

#include "object.h"

#define AS_TUPLE(object) ((ObjectTuple*)object)
#define IS_TUPLE(object) (OBJ_TYPE(object) == vm->tupleType)

#define VAL_AS_TUPLE(value) (AS_TUPLE(AS_OBJ(value)))
#define VAL_IS_TUPLE(value) (Object_ValueHasType((value), vm->tupleType))

#define ALLOCATE_TUPLE(vm, length)                                                    \
    (AS_TUPLE(Object_Allocate((vm), sizeof(ObjectTuple) + sizeof(Value) * (length)))) \

typedef struct ObjectTuple {
    Object base;
    size_t length;
    Value elements[];
} ObjectTuple;

ObjectType* Tuple_NewType(VM* vm);
void Tuple_PrepareType(ObjectType* type, VM* vm);

ObjectTuple* Tuple_New(VM* vm, size_t length);
void Tuple_SetElement(ObjectTuple* tuple, size_t index, Value value);

#endif