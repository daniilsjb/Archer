#ifndef OBJRANGE_H
#define OBJRANGE_H

#include "object.h"

#define AS_RANGE(object) ((ObjectRange*)object)
#define IS_RANGE(object) (OBJ_TYPE(object) == vm->rangeType)

#define VAL_AS_RANGE(value) (AS_RANGE(AS_OBJ(value)))
#define VAL_IS_RANGE(value) (Object_ValueHasType((value), vm->rangeType))

#define ALLOCATE_RANGE(vm) (AS_RANGE(ALLOCATE_OBJ(vm, vm->rangeType)))

typedef struct ObjectRange {
    Object base;
    double begin;
    double end;
    double step;
} ObjectRange;

ObjectType* Range_NewType(VM* vm);
void Range_PrepareType(ObjectType* type, VM* vm);

ObjectRange* Range_New(VM* vm, double begin, double end, double step);

#endif