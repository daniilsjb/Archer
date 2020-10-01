#ifndef OBJITERATOR_H
#define OBJITERATOR_H

#include "object.h"

#define AS_ITERATOR(object) ((ObjectIterator*)object)
#define IS_ITERATOR(object) (OBJ_TYPE(object) == vm->iteratorType)

#define VAL_AS_ITERATOR(value) (AS_ITERATOR(AS_OBJ(value)))
#define VAL_IS_ITERATOR(value) (Object_ValueHasType((value), vm->iteratorType))

#define ALLOCATE_ITERATOR(vm) (AS_ITERATOR(ALLOCATE_OBJ(vm, vm->iteratorType)))

typedef struct ObjectIterator ObjectIterator;

typedef bool (*IteratorReachedEndFn)(ObjectIterator* iterator);
typedef void (*IteratorAdvanceFn)(ObjectIterator* iterator);
typedef Value(*ItereratorGetValueFn)(VM* vm, ObjectIterator* iterator);

typedef struct ObjectIterator {
    Object base;
    Object* container;
    void* ptr;

    IteratorReachedEndFn ReachedEnd;
    IteratorAdvanceFn Advance;
    ItereratorGetValueFn GetValue;
} ObjectIterator;

ObjectType* Iterator_NewType(VM* vm);
void Iterator_PrepareType(ObjectType* type, VM* vm);

ObjectIterator* Iterator_New(VM* vm);

bool Iterator_ReachedEnd(ObjectIterator* iterator);
void Iterator_Advance(ObjectIterator* iterator);
Value Iterator_GetValue(VM* vm, ObjectIterator* iterator);

#endif
